
#include <json/json.h>
#include <iostream>
#include <fstream>
#include <Eigen/Dense>

class GaussianMixtureRegression
{
public:
    // Constructor
    GaussianMixtureRegression();
    int load_gmm(const std::string &model_file_path);
    int run_inference(const std::vector<Eigen::VectorXd> &x, std::vector<Eigen::VectorXd> &mean_pred, std::vector<Eigen::MatrixXd> &sigma_pred, int number_of_input_variables);
    int print_parameters();

private:
    std::vector<Eigen::VectorXd> mu;
    std::vector<Eigen::MatrixXd> sigma;
    Eigen::VectorXd pi;
    int number_of_variables;
    int number_of_states;
    double gaussPDF(const Eigen::VectorXd &x, const Eigen::VectorXd &mu, const Eigen::MatrixXd &sigma, const int nbVar);
};
