#pragma once

#include <Eigen/Eigen>
#include <cfloat>
#include <limits>
#include <vector>

#include "piqp/piqp.hpp"

namespace lc_ns_kmp
{

// LC-NS-KMP symbols follow the paper (RA-L 2025 / elib.dlr.de/216474):
//
//   s, s*     input state (time) and query state (time)
//   η(s)      predicted output
//   μ, Σ      stacked GMR means / block-diag covariances
//   N         number of reference points (horizon)
//   O         output dimension  → member output_dim
//   λ, β      hyperparameters;  γ = λ + β
//   l         squared exponential kernel lengthscale  k = exp(-l ‖s_i - s_j‖²)
//   K, k*     Kernel matrix / cross-kernel at s*
//   K̂, k̂*   kernels involving NSA via-point ŝ
//   ξ, ŝ      null-space action and its input location
//   g, c      Constraint hyperplane parameters (paper: gᵀη ≥ c)
//   F         Number of constraints per timestep
//   Ḡ, C̄     Block-diagonal constraint parameters / stacked offsets
//   α ≥ 0     Dual multipliers (solved by PIQP)
//
// Examples historically pass (g, c) such that gᵀη + c ≤ 0
// (e.g. g=[1,0], c=-2 ⇒ x ≤ 2), equivalent to the paper ≥ form after flipping the sign of g.

class LC_NS_KMP
{
private:
    const int output_dim; // paper: O
    const int N;          // paper: N (reference / prediction length)
    const double lambda;  // paper: λ
    const double beta;    // paper: β
    const double l;       // SE lengthscale (paper: l)
    const double gamma;   // paper: γ = λ + β
    int F = 4;            // constraints per timestep
    bool verbose_ = false;

    Eigen::MatrixXd K = Eigen::MatrixXd::Zero(output_dim * N, output_dim * N);
    Eigen::MatrixXd k_star = Eigen::MatrixXd::Zero(output_dim, output_dim * N);
    Eigen::MatrixXd Sigma = Eigen::MatrixXd::Zero(output_dim * N, output_dim * N);
    Eigen::MatrixXd mu = Eigen::VectorXd::Zero(output_dim * N);
    Eigen::MatrixXd K_hat = Eigen::MatrixXd::Zero(output_dim, output_dim * N);

    Eigen::MatrixXd G; // [g_1 … g_F]  (output_dim × F)
    Eigen::VectorXd c; // [c_1 … c_F]
    piqp::DenseSolver<double> solver;

    // Dual α ≥ 0 via PIQP variable bounds (not dense G=-I).
    // Do not pass a square zero equality matrix A.
    Eigen::VectorXd solve_dual_qp(const Eigen::MatrixXd &P, const Eigen::VectorXd &q);
    Eigen::MatrixXd invert_block_diagonal_sigma() const;

public:
    Eigen::MatrixXd kernel_function(const Eigen::VectorXd &s1, const Eigen::VectorXd &s2) const;
    Eigen::MatrixXd extended_kernel_function(const Eigen::VectorXd &s1, const Eigen::VectorXd &s2) const;
    double time_kernel_value(double t_i, double t_j) const;

    LC_NS_KMP(int output_dim, int horizon,
              double lambda = 6.0, double beta = 6.0, double l = 2.0);
    ~LC_NS_KMP();

    void set_verbose(bool verbose) { verbose_ = verbose; }
    bool verbose() const { return verbose_; }

    int predict(const std::vector<Eigen::VectorXd> &s_star,
                const std::vector<Eigen::VectorXd> &s,
                const std::vector<Eigen::VectorXd> &mu_in,
                const std::vector<Eigen::MatrixXd> &Sigma_in,
                std::vector<Eigen::VectorXd> &eta_out,
                std::vector<Eigen::MatrixXd> &sigma_out);

    int predict_LC(const std::vector<Eigen::VectorXd> &s_star,
                   const std::vector<Eigen::VectorXd> &s,
                   const std::vector<Eigen::VectorXd> &mu_in,
                   const std::vector<Eigen::MatrixXd> &Sigma_in,
                   std::vector<Eigen::VectorXd> &eta_out,
                   std::vector<Eigen::MatrixXd> &sigma_out);

    // ξ: null-space action, s_hat (ŝ): input where ξ is applied.
    // sigma_out is currently unused (KMP predictive covariance not returned).
    int predict_LCNS(const std::vector<Eigen::VectorXd> &s_star,
                     const Eigen::VectorXd &xi,
                     const Eigen::VectorXd &s_hat,
                     const std::vector<Eigen::VectorXd> &s,
                     const std::vector<Eigen::VectorXd> &mu_in,
                     const std::vector<Eigen::MatrixXd> &Sigma_in,
                     std::vector<Eigen::VectorXd> &eta_out,
                     std::vector<Eigen::MatrixXd> &sigma_out);

    int predict_LCNS_extended(const std::vector<Eigen::VectorXd> &s_star,
                              const Eigen::VectorXd &xi,
                              const Eigen::VectorXd &s_hat,
                              const std::vector<Eigen::VectorXd> &s,
                              const std::vector<Eigen::VectorXd> &mu_in,
                              const std::vector<Eigen::MatrixXd> &Sigma_in,
                              std::vector<Eigen::VectorXd> &eta_out,
                              std::vector<Eigen::MatrixXd> &sigma_out);

    int calculate_K(const std::vector<Eigen::VectorXd> &s, const Eigen::VectorXd &s_query);
    int construct_matrices(const std::vector<Eigen::VectorXd> &mu_in,
                           const std::vector<Eigen::MatrixXd> &Sigma_in);
    int add_constraints(const std::vector<Eigen::VectorXd> &g, const Eigen::VectorXd &c_in);
    int build_constraint_matrices(Eigen::MatrixXd &G_bar, Eigen::VectorXd &C_bar);
};

} // namespace lc_ns_kmp
