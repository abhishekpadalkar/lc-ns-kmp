#include <chrono>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include <Eigen/Core>
#include <Eigen/Dense>

#include "gmr.hpp"
#include "lc-ns-kmp.hpp"

static void dump_mu(const std::vector<Eigen::VectorXd> &mu, const std::string &file_name)
{
    const static Eigen::IOFormat CSVFormat(Eigen::StreamPrecision, Eigen::DontAlignCols, ", ", "\n");
    std::ofstream mu_file((file_name + "_mu.csv").c_str());
    for (const auto &m : mu)
    {
        mu_file << m.transpose().format(CSVFormat) << std::endl;
    }
}

static void dump_mu_and_sigma(const std::vector<Eigen::VectorXd> &mu,
                              const std::vector<Eigen::MatrixXd> &sigma,
                              const std::string &file_name)
{
    dump_mu(mu, file_name);

    const static Eigen::IOFormat CSVFormat(Eigen::StreamPrecision, Eigen::DontAlignCols, ", ", "\n");
    std::ofstream sigma_file((file_name + "_sigma.csv").c_str());
    for (const auto &s : sigma)
    {
        sigma_file << s.transpose().format(CSVFormat) << std::endl;
    }
}

// Axis-aligned box |η_i| ≤ bound encoded as gᵀη + c ≤ 0 with c = -bound.
static void make_box_constraints(int dim, double bound,
                                 std::vector<Eigen::VectorXd> &g_list,
                                 Eigen::VectorXd &c)
{
    g_list.clear();
    c = Eigen::VectorXd::Constant(2 * dim, -bound);
    for (int i = 0; i < dim; ++i)
    {
        Eigen::VectorXd g_pos = Eigen::VectorXd::Zero(dim);
        Eigen::VectorXd g_neg = Eigen::VectorXd::Zero(dim);
        g_pos[i] = 1.0;
        g_neg[i] = -1.0;
        g_list.push_back(g_pos);
        g_list.push_back(g_neg);
    }
}

int generate_trajectory()
{
    // Knobs for this run (edit here; no CLI).
    struct
    {
        std::string model = "../assets/models/model_bottle_neck.json";
        int dim = 2; // O
        int N = 500; // horizon
        double t_max = 1.0;
        double lambda = 6.0;
        double beta = 6.0;
        double l = 2.0;
        double bound = 6.0; // |η_i| ≤ bound
        int s_hat_idx = 100;
        std::string gmm_out = "../examples/gmm";
        std::string kmp_out = "../examples/kmp";
    } cfg;

    Eigen::VectorXd xi = Eigen::VectorXd::Zero(cfg.dim);
    xi << 0.0, 0.0;

    if (cfg.s_hat_idx < 0 || cfg.s_hat_idx >= cfg.N)
    {
        std::cerr << "s_hat_idx out of range [0, N)." << std::endl;
        return 1;
    }

    std::cout << "Loading GMM from file: " << cfg.model << std::endl;

    lc_ns_kmp::LC_NS_KMP kmp(cfg.dim, cfg.N, cfg.lambda, cfg.beta, cfg.l);

    lc_ns_kmp::GaussianMixtureRegression gmr;
    if (gmr.load_gmm(cfg.model) != 0)
    {
        std::cerr << "Error loading GMM." << std::endl;
        return 1;
    }

    Eigen::VectorXd time = Eigen::VectorXd::LinSpaced(cfg.N, 0.0, cfg.t_max);
    std::vector<Eigen::VectorXd> s;
    s.reserve(cfg.N);
    for (int i = 0; i < cfg.N; ++i)
    {
        Eigen::VectorXd o(1);
        o[0] = time[i];
        s.push_back(o);
    }

    std::vector<Eigen::VectorXd> mu_gmr;
    std::vector<Eigen::MatrixXd> Sigma_gmr;
    auto start = std::chrono::high_resolution_clock::now();
    gmr.run_inference(s, mu_gmr, Sigma_gmr, 1);
    auto stop = std::chrono::high_resolution_clock::now();
    std::cout << "GMR done in "
              << std::chrono::duration<double, std::milli>(stop - start).count() / 1000.0
              << " s\n";

    std::vector<Eigen::VectorXd> g_list;
    Eigen::VectorXd c;
    make_box_constraints(cfg.dim, cfg.bound, g_list, c);
    kmp.add_constraints(g_list, c);

    std::vector<Eigen::VectorXd> eta_kmp;
    std::vector<Eigen::MatrixXd> Sigma_kmp; // unused: predict_LCNS does not fill KMP covariance
    start = std::chrono::high_resolution_clock::now();
    kmp.predict_LCNS(s, xi, s[cfg.s_hat_idx], s, mu_gmr, Sigma_gmr, eta_kmp, Sigma_kmp);
    stop = std::chrono::high_resolution_clock::now();
    std::cout << "KMP done in "
              << std::chrono::duration<double, std::milli>(stop - start).count() / 1000.0
              << " s\n";

    dump_mu_and_sigma(mu_gmr, Sigma_gmr, cfg.gmm_out);
    // KMP predictive covariance is not computed yet; write means only.
    dump_mu(eta_kmp, cfg.kmp_out);
    return 0;
}

int main()
{
    return generate_trajectory();
}
