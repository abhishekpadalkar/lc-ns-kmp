#pragma once

#include <Eigen/Dense>
#include <string>
#include <vector>

namespace lc_ns_kmp
{

/// Gaussian Mixture Regression: load a GMM JSON and regress along query inputs.
class GaussianMixtureRegression
{
public:
    GaussianMixtureRegression();

    /// Load GMM parameters from JSON (`C`/`dim` or `nbStates`/`nbVar`).
    /// \return 0 on success, non-zero on failure.
    int load_gmm(const std::string &model_file_path);

    /// Run GMR for each query in \p x.
    /// \param x query inputs (e.g. time), one Eigen vector per sample
    /// \param mean_pred filled with output means
    /// \param sigma_pred filled with output covariances
    /// \param number_of_input_variables input dimension of the GMM (usually 1)
    int run_inference(const std::vector<Eigen::VectorXd> &x,
                      std::vector<Eigen::VectorXd> &mean_pred,
                      std::vector<Eigen::MatrixXd> &sigma_pred,
                      int number_of_input_variables);

    /// Print loaded GMM parameters to stdout.
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
