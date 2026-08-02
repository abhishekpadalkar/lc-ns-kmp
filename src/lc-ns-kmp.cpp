#include "lc-ns-kmp.hpp"

#include <chrono>
#include <cmath>
#include <iostream>

#include <Eigen/Eigen>
#include "piqp/piqp.hpp"

namespace lc_ns_kmp
{

LC_NS_KMP::LC_NS_KMP(int output_dim_, int horizon, double lambda_, double beta_, double l_)
    : output_dim(output_dim_),
      N(horizon),
      lambda(lambda_),
      beta(beta_),
      l(l_),
      gamma(lambda_ + beta_)
{
}

LC_NS_KMP::~LC_NS_KMP()
{
}

Eigen::VectorXd LC_NS_KMP::solve_dual_qp(const Eigen::MatrixXd &P, const Eigen::VectorXd &q)
{
    // PIQP >= 0.5 API: setup(P, q, A, b, G, h_l, h_u, x_l, x_u)
    // Older code passed (P, c, A=0, b=0, G=-I, h=0, lb=-inf, ub=+inf), which
    // maps incorrectly onto the new signature and inflates the KKT system with
    // a full square of fake equalities.
    Eigen::VectorXd x_l = Eigen::VectorXd::Zero(q.size());
    solver.setup(P, q,
                 piqp::nullopt, piqp::nullopt,
                 piqp::nullopt, piqp::nullopt, piqp::nullopt,
                 x_l, piqp::nullopt);
    solver.solve();
    return solver.result().x;
}

Eigen::MatrixXd LC_NS_KMP::invert_block_diagonal_sigma() const
{
    Eigen::MatrixXd Sigma_inv = Eigen::MatrixXd::Zero(Sigma.rows(), Sigma.cols());
    for (int i = 0; i < N; ++i)
    {
        const int offset = i * output_dim;
        Sigma_inv.block(offset, offset, output_dim, output_dim) =
            Sigma.block(offset, offset, output_dim, output_dim).inverse();
    }
    return Sigma_inv;
}

Eigen::MatrixXd LC_NS_KMP::kernel_function(const Eigen::VectorXd &s1, const Eigen::VectorXd &s2) const
{
    const auto diff = s1 - s2;
    const double k_val = std::exp(-l * diff.transpose() * diff);

    Eigen::VectorXd v = Eigen::VectorXd::Ones(output_dim) * k_val;
    return v.matrix().asDiagonal();
}

double LC_NS_KMP::time_kernel_value(double t_i, double t_j) const
{
    return std::exp(-l * std::pow((t_i - t_j), 2));
}

Eigen::MatrixXd LC_NS_KMP::extended_kernel_function(const Eigen::VectorXd &s1,
                                                    const Eigen::VectorXd &s2) const
{
    const double del = 0.0001;
    const auto t_i = s1[0];
    const auto t_j = s2[0];
    const double k_tt = time_kernel_value(t_i, t_j);
    const double k_td = (time_kernel_value(t_i, t_j + del) - k_tt) / del;
    const double k_dt = (time_kernel_value(t_i + del, t_j) - k_tt) / del;
    const double k_dd = (time_kernel_value(t_i + del, t_j + del) - time_kernel_value(t_i + del, t_j) -
                         time_kernel_value(t_i, t_j + del) + k_tt) /
                        (del * del);

    const int half = output_dim / 2;
    Eigen::MatrixXd K_tt = Eigen::MatrixXd::Identity(half, half) * k_tt;
    Eigen::MatrixXd K_td = Eigen::MatrixXd::Identity(half, half) * k_td;
    Eigen::MatrixXd K_dt = Eigen::MatrixXd::Identity(half, half) * k_dt;
    Eigen::MatrixXd K_dd = Eigen::MatrixXd::Identity(half, half) * k_dd;
    Eigen::MatrixXd m = Eigen::MatrixXd::Zero(output_dim, output_dim);
    m.block(0, 0, half, half) = K_tt;
    m.block(0, half, half, half) = K_td;
    m.block(half, 0, half, half) = K_dt;
    m.block(half, half, half, half) = K_dd;
    return m;
}

int LC_NS_KMP::calculate_K(const std::vector<Eigen::VectorXd> &s, const Eigen::VectorXd &s_query)
{
    const static Eigen::IOFormat CSVFormat(Eigen::StreamPrecision, 0, ", ", "\n");

    for (int i = 0; i < N; i++)
    {
        for (int j = 0; j < N; j++)
        {
            K.block(output_dim * i, output_dim * j, output_dim, output_dim) = kernel_function(s[i], s[j]);
        }
        k_star.block(0, output_dim * i, output_dim, output_dim) = kernel_function(s[i], s_query);
    }

    if (verbose_)
    {
        std::cout << "K : " << std::endl;
        std::cout << K.format(CSVFormat) << std::endl;
    }
    return 0;
}

int LC_NS_KMP::construct_matrices(const std::vector<Eigen::VectorXd> &mu_in,
                                  const std::vector<Eigen::MatrixXd> &Sigma_in)
{
    for (int i = 0; i < N; i++)
    {
        Sigma.block(i * output_dim, i * output_dim, output_dim, output_dim) = Sigma_in[i];
        mu.block(i * output_dim, 0, output_dim, 1) = mu_in[i];
    }
    return 0;
}

int LC_NS_KMP::predict(const std::vector<Eigen::VectorXd> &s_star,
                       const std::vector<Eigen::VectorXd> &s,
                       const std::vector<Eigen::VectorXd> &mu_in,
                       const std::vector<Eigen::MatrixXd> &Sigma_in,
                       std::vector<Eigen::VectorXd> &eta_out,
                       std::vector<Eigen::MatrixXd> & /*sigma_out*/)
{
    construct_matrices(mu_in, Sigma_in);

    for (int i = 0; i < N; i++)
    {
        for (int j = 0; j < N; j++)
        {
            K.block(output_dim * i, output_dim * j, output_dim, output_dim) = kernel_function(s[i], s[j]);
        }
    }

    Eigen::MatrixXd K_lambda_Sigma = K + lambda * Sigma;
    Eigen::MatrixXd temp = K_lambda_Sigma.inverse() * mu;

    for (int i = 0; i < N; i++)
    {
        for (int j = 0; j < N; j++)
        {
            k_star.block(0, output_dim * j, output_dim, output_dim) = kernel_function(s_star[i], s[j]);
        }
        Eigen::VectorXd eta = k_star * temp;
        eta_out.push_back(eta);
    }
    return 0;
}

int LC_NS_KMP::predict_LC(const std::vector<Eigen::VectorXd> &s_star,
                          const std::vector<Eigen::VectorXd> &s,
                          const std::vector<Eigen::VectorXd> &mu_in,
                          const std::vector<Eigen::MatrixXd> &Sigma_in,
                          std::vector<Eigen::VectorXd> &eta_out,
                          std::vector<Eigen::MatrixXd> & /*sigma_out*/)
{
    construct_matrices(mu_in, Sigma_in);

    for (int i = 0; i < N; i++)
    {
        for (int j = 0; j < N; j++)
        {
            K.block(output_dim * i, output_dim * j, output_dim, output_dim) = kernel_function(s[i], s[j]);
        }
    }

    Eigen::MatrixXd K_lambda_Sigma = K + lambda * Sigma;
    Eigen::MatrixXd Sigma_inv = invert_block_diagonal_sigma();
    Eigen::MatrixXd A = K_lambda_Sigma.inverse(); // (K + λΣ)⁻¹

    Eigen::MatrixXd A_quad = -0.5 * A * (K * Sigma_inv * K + lambda * K) * A;

    Eigen::MatrixXd G_bar;
    Eigen::VectorXd C_bar;
    build_constraint_matrices(G_bar, C_bar);

    Eigen::MatrixXd B1 = G_bar.transpose() * Sigma * A_quad * Sigma * G_bar;
    Eigen::MatrixXd B2 = 2 * mu.transpose() * A_quad * Sigma * G_bar + C_bar.transpose();

    Eigen::MatrixXd P = -B1 - B1.transpose();
    Eigen::VectorXd q = -B2.transpose();
    Eigen::VectorXd alpha = solve_dual_qp(P, q);

    Eigen::MatrixXd temp = A * (mu + Sigma * G_bar * alpha);

    for (int i = 0; i < N; i++)
    {
        for (int j = 0; j < N; j++)
        {
            k_star.block(0, output_dim * j, output_dim, output_dim) = kernel_function(s_star[i], s[j]);
        }
        Eigen::VectorXd eta = k_star * temp;
        eta_out.push_back(eta);
    }
    return 0;
}

int LC_NS_KMP::predict_LCNS(const std::vector<Eigen::VectorXd> &s_star,
                            const Eigen::VectorXd &xi,
                            const Eigen::VectorXd &s_hat,
                            const std::vector<Eigen::VectorXd> &s,
                            const std::vector<Eigen::VectorXd> &mu_in,
                            const std::vector<Eigen::MatrixXd> &Sigma_in,
                            std::vector<Eigen::VectorXd> &eta_out,
                            std::vector<Eigen::MatrixXd> & /*sigma_out*/)
{
    auto start = std::chrono::high_resolution_clock::now();

    construct_matrices(mu_in, Sigma_in);

    if (verbose_)
    {
        std::cout << "Time taken to construct matrices: \n"
                  << std::chrono::duration<double, std::milli>(
                         std::chrono::high_resolution_clock::now() - start)
                         .count()
                  << " ms\n";
    }

    start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < N; i++)
    {
        for (int j = 0; j < N; j++)
        {
            K.block(output_dim * i, output_dim * j, output_dim, output_dim) = kernel_function(s[i], s[j]);
        }
        K_hat.block(0, i * output_dim, output_dim, output_dim) = kernel_function(s_hat, s[i]);
    }

    if (verbose_)
    {
        std::cout << "Time taken to compute K matrix: \n"
                  << std::chrono::duration<double, std::milli>(
                         std::chrono::high_resolution_clock::now() - start)
                         .count()
                  << " ms\n";
    }

    Eigen::MatrixXd K_gamma_Sigma = K + gamma * Sigma;

    start = std::chrono::high_resolution_clock::now();

    Eigen::MatrixXd Sigma_inv = invert_block_diagonal_sigma();
    Eigen::MatrixXd A = K_gamma_Sigma.inverse(); // (K + γΣ)⁻¹
    if (verbose_)
    {
        std::cout << "Time taken to compute inverses: \n"
                  << std::chrono::duration<double, std::milli>(
                         std::chrono::high_resolution_clock::now() - start)
                         .count()
                  << " ms\n";
    }

    Eigen::MatrixXd A_quad = -0.5 * A * (K * Sigma_inv * K + gamma * K) * A;

    Eigen::MatrixXd G_bar;
    Eigen::VectorXd C_bar;
    build_constraint_matrices(G_bar, C_bar);

    Eigen::MatrixXd B1 = G_bar.transpose() * Sigma * A_quad * Sigma * G_bar;
    Eigen::MatrixXd B2 = 2 * mu.transpose() * A_quad * Sigma * G_bar + C_bar.transpose();
    Eigen::MatrixXd B3 = -beta * xi.transpose() * K_hat * A * Sigma * G_bar;

    Eigen::MatrixXd P = -B1 - B1.transpose();
    Eigen::VectorXd q = -(B2.transpose() + B3.transpose());

    start = std::chrono::high_resolution_clock::now();
    Eigen::VectorXd alpha = solve_dual_qp(P, q);
    if (verbose_)
    {
        std::cout << "Time taken to solve QP: \n"
                  << std::chrono::duration<double, std::milli>(
                         std::chrono::high_resolution_clock::now() - start)
                         .count()
                  << " ms\n";
    }

    start = std::chrono::high_resolution_clock::now();
    Eigen::MatrixXd temp = A * (mu + Sigma * G_bar * alpha);
    for (size_t i = 0; i < s_star.size(); i++)
    {
        Eigen::MatrixXd k_hat = kernel_function(s_star[i], s_hat);

        for (int j = 0; j < N; j++)
        {
            k_star.block(0, output_dim * j, output_dim, output_dim) = kernel_function(s_star[i], s[j]);
        }
        // (β/γ)(k̂* − k* A K̂ᵀ) ξ
        Eigen::MatrixXd eta_ns = (beta / gamma) * (k_hat - k_star * A * K_hat.transpose()) * xi;

        Eigen::VectorXd eta = k_star * temp + eta_ns;
        eta_out.push_back(eta);
    }
    if (verbose_)
    {
        std::cout << "Time taken to compute predictions: \n"
                  << std::chrono::duration<double, std::milli>(
                         std::chrono::high_resolution_clock::now() - start)
                         .count()
                  << " ms\n";
    }
    return 0;
}

int LC_NS_KMP::predict_LCNS_extended(const std::vector<Eigen::VectorXd> &s_star,
                                     const Eigen::VectorXd &xi,
                                     const Eigen::VectorXd &s_hat,
                                     const std::vector<Eigen::VectorXd> &s,
                                     const std::vector<Eigen::VectorXd> &mu_in,
                                     const std::vector<Eigen::MatrixXd> &Sigma_in,
                                     std::vector<Eigen::VectorXd> &eta_out,
                                     std::vector<Eigen::MatrixXd> & /*sigma_out*/)
{
    construct_matrices(mu_in, Sigma_in);

    for (int i = 0; i < N; i++)
    {
        for (int j = 0; j < N; j++)
        {
            K.block(output_dim * i, output_dim * j, output_dim, output_dim) =
                extended_kernel_function(s[i], s[j]);
        }
        K_hat.block(0, i * output_dim, output_dim, output_dim) =
            extended_kernel_function(s_hat, s[i]);
    }

    Eigen::MatrixXd K_gamma_Sigma = K + gamma * Sigma;
    Eigen::MatrixXd Sigma_inv = invert_block_diagonal_sigma();
    Eigen::MatrixXd A = K_gamma_Sigma.inverse(); // (K + γΣ)⁻¹

    Eigen::MatrixXd A_quad = -0.5 * A * (K * Sigma_inv * K + gamma * K) * A;

    Eigen::MatrixXd G_bar;
    Eigen::VectorXd C_bar;
    build_constraint_matrices(G_bar, C_bar);

    Eigen::MatrixXd B1 = G_bar.transpose() * Sigma * A_quad * Sigma * G_bar;
    Eigen::MatrixXd B2 = 2 * mu.transpose() * A_quad * Sigma * G_bar + C_bar.transpose();
    Eigen::MatrixXd B3 = -beta * xi.transpose() * K_hat * A * Sigma * G_bar;

    Eigen::MatrixXd P = -B1 - B1.transpose();
    Eigen::VectorXd q = -(B2.transpose() + B3.transpose());
    Eigen::VectorXd alpha = solve_dual_qp(P, q);

    Eigen::MatrixXd temp = A * (mu + Sigma * G_bar * alpha);
    for (size_t i = 0; i < s_star.size(); i++)
    {
        Eigen::MatrixXd k_hat = extended_kernel_function(s_star[i], s_hat);

        for (int j = 0; j < N; j++)
        {
            k_star.block(0, output_dim * j, output_dim, output_dim) =
                extended_kernel_function(s_star[i], s[j]);
        }
        Eigen::MatrixXd eta_ns = (beta / gamma) * (k_hat - k_star * A * K_hat.transpose()) * xi;

        Eigen::VectorXd eta = k_star * temp + eta_ns;
        eta_out.push_back(eta);
    }
    return 0;
}

int LC_NS_KMP::add_constraints(const std::vector<Eigen::VectorXd> &g, const Eigen::VectorXd &c_in)
{
    if (verbose_)
    {
        const static Eigen::IOFormat CSVFormat(Eigen::StreamPrecision, 0, ", ", "\n");
        std::cout << c_in.format(CSVFormat) << std::endl;
    }

    Eigen::MatrixXd G_temp = Eigen::MatrixXd::Zero(output_dim, static_cast<int>(g.size()));
    for (int i = 0; i < static_cast<int>(g.size()); i++)
    {
        G_temp.col(i) = g[i];
    }
    G = G_temp;
    c = c_in;
    F = static_cast<int>(G.cols());
    return 0;
}

int LC_NS_KMP::build_constraint_matrices(Eigen::MatrixXd &G_bar, Eigen::VectorXd &C_bar)
{
    G_bar = Eigen::MatrixXd::Zero(output_dim * N, F * N);
    C_bar = Eigen::VectorXd::Zero(F * N);
    for (int i = 0; i < N; i++)
    {
        G_bar.block(i * output_dim, i * F, output_dim, F) = G;
        C_bar.segment(i * F, F) = c;
    }
    return 0;
}

} // namespace lc_ns_kmp
