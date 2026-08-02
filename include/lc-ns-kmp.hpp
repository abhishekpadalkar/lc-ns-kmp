#pragma once

#include <Eigen/Eigen>
#include <cfloat>
#include <limits>
#include <vector>

#include "piqp/piqp.hpp"

namespace lc_ns_kmp
{

/// \file lc-ns-kmp.hpp
/// Linearly Constrained Null-Space Kernelized Movement Primitives.
///
/// Paper symbol map (RA-L 2025 / https://elib.dlr.de/216474 ):
/// - s, s* — input / query state (time)
/// - η(s) — predicted output
/// - μ, Σ — stacked GMR means / block-diag covariances
/// - N — reference / prediction length (horizon)
/// - O — output dimension (`output_dim`)
/// - λ, β — hyperparameters; γ = λ + β
/// - l — SE lengthscale: k = exp(-l ‖s_i - s_j‖²)
/// - ξ, ŝ — null-space action and its input location
/// - g, c — constraint hyperplane (paper: gᵀη ≥ c)
///
/// Examples historically pass (g, c) such that gᵀη + c ≤ 0
/// (e.g. g=[1,0], c=-2 ⇒ x ≤ 2), equivalent after flipping the sign of g.

/// LC-NS-KMP predictor with optional linear constraints and null-space action.
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

    Eigen::VectorXd solve_dual_qp(const Eigen::MatrixXd &P, const Eigen::VectorXd &q);
    Eigen::MatrixXd invert_block_diagonal_sigma() const;

public:
    /// Diagonal squared-exponential kernel block of size O×O.
    Eigen::MatrixXd kernel_function(const Eigen::VectorXd &s1, const Eigen::VectorXd &s2) const;
    /// Position–velocity extended kernel (finite-difference on time).
    Eigen::MatrixXd extended_kernel_function(const Eigen::VectorXd &s1, const Eigen::VectorXd &s2) const;
    /// Scalar SE kernel on scalar times.
    double time_kernel_value(double t_i, double t_j) const;

    /// \param output_dim output dimension O
    /// \param horizon number of reference points N
    /// \param lambda KMP regularization λ
    /// \param beta null-space weight β
    /// \param l SE lengthscale
    LC_NS_KMP(int output_dim, int horizon,
              double lambda = 6.0, double beta = 6.0, double l = 2.0);
    ~LC_NS_KMP();

    void set_verbose(bool verbose) { verbose_ = verbose; }
    bool verbose() const { return verbose_; }

    /// Unconstrained KMP prediction. \p sigma_out is currently unused.
    int predict(const std::vector<Eigen::VectorXd> &s_star,
                const std::vector<Eigen::VectorXd> &s,
                const std::vector<Eigen::VectorXd> &mu_in,
                const std::vector<Eigen::MatrixXd> &Sigma_in,
                std::vector<Eigen::VectorXd> &eta_out,
                std::vector<Eigen::MatrixXd> &sigma_out);

    /// Linearly constrained KMP. \p sigma_out is currently unused.
    int predict_LC(const std::vector<Eigen::VectorXd> &s_star,
                   const std::vector<Eigen::VectorXd> &s,
                   const std::vector<Eigen::VectorXd> &mu_in,
                   const std::vector<Eigen::MatrixXd> &Sigma_in,
                   std::vector<Eigen::VectorXd> &eta_out,
                   std::vector<Eigen::MatrixXd> &sigma_out);

    /// LC-NS-KMP: constraints + null-space action \p xi at input \p s_hat.
    /// \p sigma_out is currently unused (KMP predictive covariance not returned).
    int predict_LCNS(const std::vector<Eigen::VectorXd> &s_star,
                     const Eigen::VectorXd &xi,
                     const Eigen::VectorXd &s_hat,
                     const std::vector<Eigen::VectorXd> &s,
                     const std::vector<Eigen::VectorXd> &mu_in,
                     const std::vector<Eigen::MatrixXd> &Sigma_in,
                     std::vector<Eigen::VectorXd> &eta_out,
                     std::vector<Eigen::MatrixXd> &sigma_out);

    /// Same as predict_LCNS with the extended position–velocity kernel.
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

    /// Set per-timestep linear constraints (examples: gᵀη + c ≤ 0).
    int add_constraints(const std::vector<Eigen::VectorXd> &g, const Eigen::VectorXd &c_in);
    int build_constraint_matrices(Eigen::MatrixXd &G_bar, Eigen::VectorXd &C_bar);
};

} // namespace lc_ns_kmp
