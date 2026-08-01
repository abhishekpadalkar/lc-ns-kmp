
#include <json/json.h>
#include <iostream>
#include <fstream>
#include "lc-ns-kmp.hpp"
#include <limits>
#include <cfloat>
#include <Eigen/Eigen>
#include "piqp/piqp.hpp"
#include <cmath>
#include <chrono>

LC_NS_KMP::LC_NS_KMP(int dim, int hor) : dimmension(dim), horizon(hor)
{
}

LC_NS_KMP::~LC_NS_KMP()
{
}

Eigen::VectorXd LC_NS_KMP::solve_dual_qp(const Eigen::MatrixXd &P, const Eigen::VectorXd &c)
{
    // PIQP >= 0.5 API: setup(P, c, A, b, G, h_l, h_u, x_l, x_u)
    // Older code passed (P, c, A=0, b=0, G=-I, h=0, lb=-inf, ub=+inf), which
    // maps incorrectly onto the new signature and inflates the KKT system with
    // a full square of fake equalities.
    Eigen::VectorXd x_l = Eigen::VectorXd::Zero(c.size());
    solver.setup(P, c,
                 piqp::nullopt, piqp::nullopt,
                 piqp::nullopt, piqp::nullopt, piqp::nullopt,
                 x_l, piqp::nullopt);
    solver.solve();
    return solver.result().x;
}

Eigen::MatrixXd LC_NS_KMP::invert_block_diagonal_sigma() const
{
    Eigen::MatrixXd Sigma_inv = Eigen::MatrixXd::Zero(Sigma.rows(), Sigma.cols());
    for (int i = 0; i < horizon; ++i)
    {
        const int offset = i * dimmension;
        Sigma_inv.block(offset, offset, dimmension, dimmension) =
            Sigma.block(offset, offset, dimmension, dimmension).inverse();
    }
    return Sigma_inv;
}

Eigen::MatrixXd LC_NS_KMP::kernel_function(Eigen::VectorXd s1, Eigen::VectorXd s2)
{
    const static Eigen::IOFormat CSVFormat(Eigen::StreamPrecision, 0, ", ", "\n");

    auto diff = s1 - s2;
    double k = exp(-h * diff.transpose() * diff);

    Eigen::VectorXd v = Eigen::VectorXd::Ones(dimmension) * k;
    Eigen::MatrixXd m = v.matrix().asDiagonal();
    // std::cout << m.format(CSVFormat) << "  " << s1.format(CSVFormat) << " " << s2.format(CSVFormat) << diff.format(CSVFormat) << std::endl << std::endl;
    return m;
}

double LC_NS_KMP::time_kernel_value(double t_i, double t_j)
{
    return exp(-h * std::pow((t_i - t_j), 2));
}

Eigen::MatrixXd LC_NS_KMP::extended_kernel_function(Eigen::VectorXd s1, Eigen::VectorXd s2)
{
    const static Eigen::IOFormat CSVFormat(Eigen::StreamPrecision, 0, ", ", "\n");
    double del = 0.0001;
    auto t_i = s1[0];
    auto t_j = s2[0];
    double k_tt = time_kernel_value(t_i, t_j);
    double k_td = (time_kernel_value(t_i, t_j + del) - k_tt) / del;
    double k_dt = (time_kernel_value(t_i + del, t_j) - k_tt) / del;
    double k_dd = (time_kernel_value(t_i + del, t_j + del) - time_kernel_value(t_i + del, t_j) - time_kernel_value(t_i, t_j + del) + k_tt) / (del * del);

    Eigen::MatrixXd K_tt = Eigen::MatrixXd::Identity(dimmension / 2, dimmension / 2) * k_tt;
    Eigen::MatrixXd K_td = Eigen::MatrixXd::Identity(dimmension / 2, dimmension / 2) * k_td;
    Eigen::MatrixXd K_dt = Eigen::MatrixXd::Identity(dimmension / 2, dimmension / 2) * k_dt;
    Eigen::MatrixXd K_dd = Eigen::MatrixXd::Identity(dimmension / 2, dimmension / 2) * k_dd;
    Eigen::MatrixXd m = Eigen::MatrixXd::Zero(dimmension, dimmension);
    m.block(0, 0, dimmension / 2, dimmension / 2) = K_tt;
    m.block(0, dimmension / 2, dimmension / 2, dimmension / 2) = K_td;
    m.block(dimmension / 2, 0, dimmension / 2, dimmension / 2) = K_dt;
    m.block(dimmension / 2, dimmension / 2, dimmension / 2, dimmension / 2) = K_dd;
    // std::cout << m.format(CSVFormat) << "\n k_tt" << k_tt << " k_td" << k_td << " k_dt" << k_dt << " k_dd" << k_dd << std::endl;

    return m;
}

int LC_NS_KMP::calculate_K(std::vector<Eigen::VectorXd> state_in, Eigen::VectorXd current_state)
{
    const static Eigen::IOFormat CSVFormat(Eigen::StreamPrecision, 0, ", ", "\n");

    for (size_t i = 0; i < horizon; i++)
    {
        for (size_t j = 0; j < horizon; j++)
        {
            K.block(dimmension * i, dimmension * j, dimmension, dimmension) = kernel_function(state_in[i], state_in[j]);
        }
        k.block(0, dimmension * i, dimmension, dimmension) = kernel_function(state_in[i], current_state);
    }

    std::cout << "K : " << std::endl;
    std::cout << K.format(CSVFormat) << std::endl;
    return 0;
}

int LC_NS_KMP::construct_matrices(std::vector<Eigen::VectorXd> mu, std::vector<Eigen::MatrixXd> sigma)
{
    for (size_t i = 0; i < horizon; i++)
    {
        Sigma.block(i * dimmension, i * dimmension, dimmension, dimmension) = sigma[i];

        Mu.block(i * dimmension, 0, dimmension, 1) = mu[i];
    }
    return 0;
}

int LC_NS_KMP::predict(std::vector<Eigen::VectorXd> current_state, std::vector<Eigen::VectorXd> state_in, std::vector<Eigen::VectorXd> mu_in, std::vector<Eigen::MatrixXd> sigma_in,
                 std::vector<Eigen::VectorXd> &mu_out, std::vector<Eigen::MatrixXd> &sigma_out)
{
    // const static Eigen::IOFormat CSVFormat(Eigen::StreamPrecision, 0, ", ", "\n");

    // for (auto s:state_in)
    // {
    //     std::cout << "state is : " << s.format(CSVFormat) << std::endl;
    // }

    construct_matrices(mu_in, sigma_in);

    for (size_t i = 0; i < horizon; i++)
    {
        for (size_t j = 0; j < horizon; j++)
        {
            K.block(dimmension * i, dimmension * j, dimmension, dimmension) = kernel_function(state_in[i], state_in[j]);
        }
    }

    Eigen::MatrixXd K_sigma = K + lambda * Sigma;

    // std::cout << "Sigma : " << std::endl;
    // std::cout << Sigma.format(CSVFormat) << std::endl;

    // Eigen::MatrixXd K_sigma_inv = K_sigma.inverse();
    Eigen::MatrixXd temp = K_sigma.inverse() * Mu;

    // std::cout << "K_Sigma : " << std::endl;
    // std::cout << K_sigma_inv.format(CSVFormat) << std::endl;

    for (size_t i = 0; i < horizon; i++)
    {

        for (size_t j = 0; j < horizon; j++)
        {
            k.block(0, dimmension * j, dimmension, dimmension) = kernel_function(current_state[i], state_in[j]);
        }

        Eigen::VectorXd m = k * temp;
        mu_out.push_back(m);
        // std::cout << Mu.format(CSVFormat);
    }
    std::cout << "Current state " << std::endl;
    return 0;
}

int LC_NS_KMP::predict_LC(std::vector<Eigen::VectorXd> current_state, std::vector<Eigen::VectorXd> state_in, std::vector<Eigen::VectorXd> mu_in, std::vector<Eigen::MatrixXd> sigma_in,
                    std::vector<Eigen::VectorXd> &mu_out, std::vector<Eigen::MatrixXd> &sigma_out)
{
    const static Eigen::IOFormat CSVFormat(Eigen::StreamPrecision, 0, ", ", "\n");

    // for (auto s:state_in)
    // {
    //     std::cout << "state is : " << s.format(CSVFormat) << std::endl;
    // }

    construct_matrices(mu_in, sigma_in);

    for (size_t i = 0; i < horizon; i++)
    {
        for (size_t j = 0; j < horizon; j++)
        {
            K.block(dimmension * i, dimmension * j, dimmension, dimmension) = kernel_function(state_in[i], state_in[j]);
        }
    }

    Eigen::MatrixXd K_sigma = K + lambda * Sigma;

    Eigen::MatrixXd Sigma_inv = invert_block_diagonal_sigma();
    Eigen::MatrixXd K_sigma_inv = K_sigma.inverse();

    Eigen::MatrixXd A = -0.5 * K_sigma_inv * (K * Sigma_inv * K + lambda * K) * K_sigma_inv;

    Eigen::MatrixXd G_bar;
    Eigen::VectorXd C_bar;

    build_constraint_matrices(G_bar, C_bar);

    Eigen::MatrixXd B1 = G_bar.transpose() * Sigma * A * Sigma * G_bar;
    Eigen::MatrixXd B2 = 2 * Mu.transpose() * A * Sigma * G_bar + C_bar.transpose();

    Eigen::MatrixXd _P = -B1 - B1.transpose();
    Eigen::VectorXd _c = -B2.transpose();
    Eigen::VectorXd dual = solve_dual_qp(_P, _c);

    Eigen::MatrixXd temp = K_sigma_inv * (Mu + Sigma * G_bar * dual);

    // std::cout << "K_Sigma : " << std::endl;
    // std::cout << K_sigma_inv.format(CSVFormat) << std::endl;

    for (size_t i = 0; i < horizon; i++)
    {

        for (size_t j = 0; j < horizon; j++)
        {
            k.block(0, dimmension * j, dimmension, dimmension) = kernel_function(current_state[i], state_in[j]);
        }

        Eigen::VectorXd m = k * temp;
        mu_out.push_back(m);
        // std::cout << Mu.format(CSVFormat);
    }
    std::cout << "Current state " << std::endl;
    return 0;
}

int LC_NS_KMP::predict_LCNS(std::vector<Eigen::VectorXd> current_state, Eigen::VectorXd nsa, Eigen::VectorXd nss, std::vector<Eigen::VectorXd> state_in, std::vector<Eigen::VectorXd> mu_in, std::vector<Eigen::MatrixXd> sigma_in,
                      std::vector<Eigen::VectorXd> &mu_out, std::vector<Eigen::MatrixXd> &sigma_out)
{
    const static Eigen::IOFormat CSVFormat(Eigen::StreamPrecision, 0, ", ", "\n");

    // for (auto s:current_state)
    // {
    //     std::cout << "state is : " << s.format(CSVFormat) << std::endl;
    // }

    auto start = std::chrono::high_resolution_clock::now();

    construct_matrices(mu_in, sigma_in);

    std::cout << "Time taken to construct matrices: \n"
              << std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - start).count() << " ms\n";

    start = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < horizon; i++)
    {
        for (size_t j = 0; j < horizon; j++)
        {
            K.block(dimmension * i, dimmension * j, dimmension, dimmension) = kernel_function(state_in[i], state_in[j]);
        }
        K_cap.block(0, i * dimmension, dimmension, dimmension) = kernel_function(nss, state_in[i]);
    }

    std::cout << "Time taken to compute K matrix: \n"
              << std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - start).count() << " ms\n";

    Eigen::MatrixXd K_sigma = K + hbar * Sigma;

    start = std::chrono::high_resolution_clock::now();

    Eigen::MatrixXd Sigma_inv = invert_block_diagonal_sigma();
    Eigen::MatrixXd K_sigma_inv = K_sigma.inverse();
    std::cout << "Time taken to compute inverses: \n"
              << std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - start).count() << " ms\n";

    Eigen::MatrixXd A = -0.5 * K_sigma_inv * (K * Sigma_inv * K + hbar * K) * K_sigma_inv;

    Eigen::MatrixXd G_bar;
    Eigen::VectorXd C_bar;

    build_constraint_matrices(G_bar, C_bar);

    Eigen::MatrixXd B1 = G_bar.transpose() * Sigma * A * Sigma * G_bar;
    Eigen::MatrixXd B2 = 2 * Mu.transpose() * A * Sigma * G_bar + C_bar.transpose();
    Eigen::MatrixXd B3 = -beta * nsa.transpose() * K_cap * K_sigma_inv * Sigma * G_bar;

    Eigen::MatrixXd _P = -B1 - B1.transpose();
    Eigen::VectorXd _c = -(B2.transpose() + B3.transpose());

    start = std::chrono::high_resolution_clock::now();
    Eigen::VectorXd dual = solve_dual_qp(_P, _c);
    std::cout << "Time taken to solve QP: \n"
              << std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - start).count() << " ms\n";

    start = std::chrono::high_resolution_clock::now();
    Eigen::MatrixXd temp = K_sigma_inv * (Mu + Sigma * G_bar * dual);
    // std::cout << "K_Sigma : " << std::endl;
    // std::cout << K_sigma_inv.format(CSVFormat) << std::endl;
    for (size_t i = 0; i < current_state.size(); i++)
    {
        Eigen::MatrixXd k_cap = kernel_function(current_state[i], nss);

        for (size_t j = 0; j < horizon; j++)
        {
            k.block(0, dimmension * j, dimmension, dimmension) = kernel_function(current_state[i], state_in[j]);
        }
        Eigen::MatrixXd pred_nsa = (beta / hbar) * (k_cap - k * K_sigma_inv * K_cap.transpose()) * nsa; 

        Eigen::VectorXd m = k * temp + pred_nsa;
        mu_out.push_back(m);
        // std::cout << Mu.format(CSVFormat);
    }
    std::cout << "Time taken to compute predictions: \n"
              << std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - start).count() << " ms\n";
    // std::cout << "Current state " << std::endl;
    return 0;
}

int LC_NS_KMP::predict_LCNS_extended(std::vector<Eigen::VectorXd> current_state, Eigen::VectorXd nsa, Eigen::VectorXd nss, std::vector<Eigen::VectorXd> state_in, std::vector<Eigen::VectorXd> mu_in, std::vector<Eigen::MatrixXd> sigma_in,
                               std::vector<Eigen::VectorXd> &mu_out, std::vector<Eigen::MatrixXd> &sigma_out)
{
    const static Eigen::IOFormat CSVFormat(Eigen::StreamPrecision, 0, ", ", "\n");

    construct_matrices(mu_in, sigma_in);

    for (size_t i = 0; i < horizon; i++)
    {
        for (size_t j = 0; j < horizon; j++)
        {
            K.block(dimmension * i, dimmension * j, dimmension, dimmension) = extended_kernel_function(state_in[i], state_in[j]);
        }
        K_cap.block(0, i * dimmension, dimmension, dimmension) = extended_kernel_function(nss, state_in[i]);
    }

    Eigen::MatrixXd K_sigma = K + hbar * Sigma;

    Eigen::MatrixXd Sigma_inv = invert_block_diagonal_sigma();
    Eigen::MatrixXd K_sigma_inv = K_sigma.inverse();

    Eigen::MatrixXd A = -0.5 * K_sigma_inv * (K * Sigma_inv * K + hbar * K) * K_sigma_inv;

    Eigen::MatrixXd G_bar;
    Eigen::VectorXd C_bar;

    build_constraint_matrices(G_bar, C_bar);

    Eigen::MatrixXd B1 = G_bar.transpose() * Sigma * A * Sigma * G_bar;
    Eigen::MatrixXd B2 = 2 * Mu.transpose() * A * Sigma * G_bar + C_bar.transpose();
    Eigen::MatrixXd B3 = -beta * nsa.transpose() * K_cap * K_sigma_inv * Sigma * G_bar;

    Eigen::MatrixXd _P = -B1 - B1.transpose();
    Eigen::VectorXd _c = -(B2.transpose() + B3.transpose());
    Eigen::VectorXd dual = solve_dual_qp(_P, _c);

    Eigen::MatrixXd temp = K_sigma_inv * (Mu + Sigma * G_bar * dual);
    // std::cout << "K_Sigma : " << std::endl;
    // std::cout << K_sigma_inv.format(CSVFormat) << std::endl;
    for (size_t i = 0; i < current_state.size(); i++)
    {
        Eigen::MatrixXd k_cap = extended_kernel_function(current_state[i], nss);

        for (size_t j = 0; j < horizon; j++)
        {
            k.block(0, dimmension * j, dimmension, dimmension) = extended_kernel_function(current_state[i], state_in[j]);
        }
        Eigen::MatrixXd pred_nsa = (beta / hbar) * (k_cap - k * K_sigma_inv * K_cap.transpose()) * nsa; // Doubtful implementation here

        Eigen::VectorXd m = k * temp + pred_nsa;
        mu_out.push_back(m);
        // std::cout << Mu.format(CSVFormat);
    }
    return 0;
    // std::cout << "Current state " << std::endl;
}


int LC_NS_KMP::add_constriants(std::vector<Eigen::VectorXd> g, Eigen::VectorXd c)
{
    const static Eigen::IOFormat CSVFormat(Eigen::StreamPrecision, 0, ", ", "\n");
    std::cout << c.format(CSVFormat) << std::endl;
    Eigen::MatrixXd gs_temp = Eigen::MatrixXd::Zero(dimmension, g.size());
    for (int i = 0; i < g.size(); i++)
    {
        gs_temp.col(i) = g[i];
        // std::cout << g[i].format(CSVFormat) << std::endl;

        // std::cout << "gs_temp: " << gs_temp.format(CSVFormat) << std::endl;
    }
    gs = gs_temp;
    // std::cout << "Here 1" << std::endl;
    Eigen::VectorXd cs_temp(c.size());
    // cs_temp << cs, c;
    cs_temp << c;
    cs = cs_temp;
    // std::cout << "Here 2" << std::endl;

    // std::cout << gs.format(CSVFormat) << std::endl;
    // std::cout << cs.format(CSVFormat) << std::endl;

    number_of_constraints = gs.cols();

    // std::cout << "Number of constraints" << number_of_constraints << std::endl;
    return 0;
}

int LC_NS_KMP::build_constraint_matrices(Eigen::MatrixXd &G_bar, Eigen::VectorXd &C_bar)
{
    const static Eigen::IOFormat CSVFormat(Eigen::StreamPrecision, 0, ", ", "\n");
    // std::cout << "gs \n " << gs.format(CSVFormat) << "gs end \n"
    //           << std::endl;
    // std::cout << "cs \n " << cs.format(CSVFormat) << "cs end \n"
    //           << std::endl;
    G_bar = Eigen::MatrixXd::Zero(dimmension * horizon, number_of_constraints * horizon);
    C_bar = Eigen::VectorXd::Zero(number_of_constraints * horizon);
    for (size_t i = 0; i < horizon; i++)
    {
        G_bar.block(i * dimmension, i * number_of_constraints, dimmension, number_of_constraints) = gs;
        C_bar.segment(i * number_of_constraints, number_of_constraints) = cs;
    }

    // std::cout << G_bar.format(CSVFormat) << std::endl;
    // std::cout << C_bar.format(CSVFormat) << std::endl;
    return 0;
}
