#pragma once

#include <Eigen/Dense>
#include <string>
#include <vector>

namespace lc_ns_kmp
{

class GaussianMixtureRegression
{
public:
    GaussianMixtureRegression();
    int load_gmm(const std::string &model_file_path);
    int run_inference(const std::vector<Eigen::VectorXd> &x,
                      std::vector<Eigen::VectorXd> &mean_pred,
                      std::vector<Eigen::MatrixXd> &sigma_pred,
                      int number_of_input_variables);
    int print_parameters() const;

    void set_verbose(bool verbose) { verbose_ = verbose; }
    bool verbose() const { return verbose_; }

private:
    std::vector<Eigen::VectorXd> mu;
    std::vector<Eigen::MatrixXd> sigma;
    Eigen::VectorXd pi;
    int number_of_variables = 0;
    int number_of_states = 0;
    bool verbose_ = false;

    double gaussPDF(const Eigen::VectorXd &x, const Eigen::VectorXd &mu,
                    const Eigen::MatrixXd &sigma, int nbVar) const;
};

} // namespace lc_ns_kmp
