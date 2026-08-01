#include <iostream>
#include <fstream>
// #include <Eigen/Dense>
#include <Eigen/Eigen>
#include <limits>
#include <cfloat>
#include "piqp/piqp.hpp"

class LC_NS_KMP
{
private:
    const int dimmension;
    const int horizon;
    double h = 2.0;
    const double lambda = 6;
    const double beta = 6;
    const double hbar = lambda + beta;
    int number_of_constraints = 4;

    Eigen::MatrixXd K = Eigen::MatrixXd::Zero(dimmension * horizon, dimmension *horizon);
    Eigen::MatrixXd k = Eigen::MatrixXd::Zero(dimmension, dimmension *horizon);
    Eigen::MatrixXd Sigma = Eigen::MatrixXd::Zero(dimmension * horizon, dimmension *horizon);
    Eigen::MatrixXd Mu = Eigen::VectorXd::Zero(dimmension * horizon);
    Eigen::MatrixXd K_cap = Eigen::MatrixXd::Zero(dimmension, dimmension *horizon);

    Eigen::MatrixXd gs;
    Eigen::VectorXd cs;
    piqp::DenseSolver<double> solver;

    // Dual multipliers λ >= 0 via PIQP box bounds (not dense G=-I).
    // Do not pass a square zero A: that creates fake equality constraints.
    Eigen::VectorXd solve_dual_qp(const Eigen::MatrixXd &P, const Eigen::VectorXd &c);
    Eigen::MatrixXd invert_block_diagonal_sigma() const;

public:
    Eigen::MatrixXd kernel_function(Eigen::VectorXd s1, Eigen::VectorXd s2);
    Eigen::MatrixXd extended_kernel_function(Eigen::VectorXd s1, Eigen::VectorXd s2);
    Eigen::MatrixXd extended_kernel_function_acc(Eigen::VectorXd s1, Eigen::VectorXd s2);
    double time_kernel_value(double t_i, double t_j);

    LC_NS_KMP(int dim, int hor);
    int predict(std::vector<Eigen::VectorXd> current_state, std::vector<Eigen::VectorXd> state_in, std::vector<Eigen::VectorXd> mu_in, std::vector<Eigen::MatrixXd> sigma_in,
                     std::vector<Eigen::VectorXd> &mu_out, std::vector<Eigen::MatrixXd> &sigma_out);
    int predict_LCNS(std::vector<Eigen::VectorXd> current_state, Eigen::VectorXd nsa, Eigen::VectorXd nss, std::vector<Eigen::VectorXd> state_in, std::vector<Eigen::VectorXd> mu_in, std::vector<Eigen::MatrixXd> sigma_in,
                     std::vector<Eigen::VectorXd> &mu_out, std::vector<Eigen::MatrixXd> &sigma_out);
    int predict_LC(std::vector<Eigen::VectorXd> current_state, std::vector<Eigen::VectorXd> state_in, std::vector<Eigen::VectorXd> mu_in, std::vector<Eigen::MatrixXd> sigma_in,
                     std::vector<Eigen::VectorXd> &mu_out, std::vector<Eigen::MatrixXd> &sigma_out);
    int calculate_K(std::vector<Eigen::VectorXd> state_in, Eigen::VectorXd current_state);
    int construct_matrices(std::vector<Eigen::VectorXd> mu, std::vector<Eigen::MatrixXd> sigma);
    int add_constriants(std::vector<Eigen::VectorXd> g, Eigen::VectorXd c);
    int build_constraint_matrices(Eigen::MatrixXd &G_bar, Eigen::VectorXd &C_bar);
    int predict_LCNS_extended(std::vector<Eigen::VectorXd> current_state, Eigen::VectorXd nsa, Eigen::VectorXd nss, std::vector<Eigen::VectorXd> state_in, std::vector<Eigen::VectorXd> mu_in, std::vector<Eigen::MatrixXd> sigma_in,
                              std::vector<Eigen::VectorXd> &mu_out, std::vector<Eigen::MatrixXd> &sigma_out);
    int predict_LCNS_extended_acc(std::vector<Eigen::VectorXd> current_state, Eigen::VectorXd nsa, Eigen::VectorXd nss, std::vector<Eigen::VectorXd> state_in, std::vector<Eigen::VectorXd> mu_in, std::vector<Eigen::MatrixXd> sigma_in,
                              std::vector<Eigen::VectorXd> &mu_out, std::vector<Eigen::MatrixXd> &sigma_out);
    ~LC_NS_KMP();
};



