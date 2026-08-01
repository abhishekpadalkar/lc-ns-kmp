
#include <json/json.h>
#include <iostream>
#include <fstream>
#include "gmr.hpp"
#include <limits>
#include <cfloat>
#include <Eigen/Eigen>

GaussianMixtureRegression::GaussianMixtureRegression()
{
}

int GaussianMixtureRegression::load_gmm(const std::string &model_file_path)
{
    std::ifstream jsonFile(model_file_path);
    if (!jsonFile.is_open())
    {
        std::cerr << "Failed to open model file: " << model_file_path << std::endl;
        return 1;
    }

    Json::Value jsonData;
    jsonFile >> jsonData;

    if (!jsonData.isObject())
    {
        std::cerr << "JSON is not an object." << std::endl;
        return 1;
    }

    // Accept both export schemas: C/dim (legacy) and nbStates/nbVar (Python notebooks).
    if (jsonData.isMember("C") && jsonData.isMember("dim"))
    {
        number_of_states = jsonData["C"].asInt();
        number_of_variables = jsonData["dim"].asInt();
    }
    else if (jsonData.isMember("nbStates") && jsonData.isMember("nbVar"))
    {
        number_of_states = jsonData["nbStates"].asInt();
        number_of_variables = jsonData["nbVar"].asInt();
    }
    else
    {
        std::cerr << "Model must define either C/dim or nbStates/nbVar." << std::endl;
        return 1;
    }

    const Json::Value &mus = jsonData["Mu"];
    const Json::Value &sigmas = jsonData["Sigma"];
    const Json::Value &priors = jsonData["Priors"];

    if (!mus.isArray() || !priors.isArray() || !sigmas.isArray())
    {
        std::cerr << "Mu, Priors, and Sigma must be arrays." << std::endl;
        return 1;
    }
    if (number_of_states <= 0 || number_of_variables <= 0)
    {
        std::cerr << "Invalid model dimensions: states=" << number_of_states
                  << ", variables=" << number_of_variables << std::endl;
        return 1;
    }
    if (static_cast<int>(mus.size()) != number_of_states ||
        static_cast<int>(priors.size()) != number_of_states ||
        static_cast<int>(sigmas.size()) != number_of_states)
    {
        std::cerr << "Mu/Priors/Sigma size does not match number of states." << std::endl;
        return 1;
    }

    mu.clear();
    sigma.clear();

    std::cout << "Loading model: " << model_file_path << std::endl;
    std::cout << "Number of states : " << number_of_states << std::endl;
    std::cout << "Number of variables : " << number_of_variables << std::endl;

    for (const auto &mu_row : mus)
    {
        if (!mu_row.isArray() || static_cast<int>(mu_row.size()) != number_of_variables)
        {
            std::cerr << "Mu row has unexpected size." << std::endl;
            return 1;
        }
        Eigen::VectorXd m(number_of_variables);
        for (int i = 0; i < number_of_variables; ++i)
        {
            m(i) = mu_row[i].asDouble();
        }
        mu.push_back(m);
    }

    Eigen::VectorXd p(number_of_states);
    for (int i = 0; i < number_of_states; ++i)
    {
        p(i) = priors[i].asDouble();
    }
    pi = p;

    for (const auto &sigma_mat : sigmas)
    {
        if (!sigma_mat.isArray() || static_cast<int>(sigma_mat.size()) != number_of_variables)
        {
            std::cerr << "Sigma matrix has unexpected size." << std::endl;
            return 1;
        }
        Eigen::MatrixXd s(number_of_variables, number_of_variables);
        for (int i = 0; i < number_of_variables; ++i)
        {
            const auto &row = sigma_mat[i];
            if (!row.isArray() || static_cast<int>(row.size()) != number_of_variables)
            {
                std::cerr << "Sigma row has unexpected size." << std::endl;
                return 1;
            }
            for (int j = 0; j < number_of_variables; ++j)
            {
                s(i, j) = row[j].asDouble();
            }
        }
        sigma.push_back(s);
    }

    return 0;
}

int GaussianMixtureRegression::print_parameters()
{
    const static Eigen::IOFormat CSVFormat(Eigen::StreamPrecision, 0, ", ", "\n");

    std::cout << "Printing mu: " << std::endl;
    for (auto m : mu)
    {
        std::cout << m.transpose().format(CSVFormat) << std::endl;
    }
    std::cout << std::endl;

    std::cout << "Printing pi: " << std::endl;
    std::cout << pi.transpose().format(CSVFormat) << std::endl;
    std::cout << std::endl;

    std::cout << "Printing Sigma: " << std::endl;
    for (auto s : sigma)
    {
        std::cout << s.transpose().format(CSVFormat) << std::endl;
        std::cout << std::endl;
    }
    std::cout << std::endl;

    return 0;
}

int GaussianMixtureRegression::run_inference(const std::vector<Eigen::VectorXd> &_x, std::vector<Eigen::VectorXd> &_mean, std::vector<Eigen::MatrixXd> &_sigma, int number_of_input_variables)
{
    int number_of_output_variables = number_of_variables - number_of_input_variables;
    int input_start_index = 0;
    int input_end_index = number_of_input_variables - 1;
    int output_start_index = number_of_input_variables;
    int output_end_index = number_of_variables;

    std::cout << "Number of input variables: " << number_of_input_variables << std::endl;
    std::cout << "Number of output variables: " << number_of_output_variables << std::endl;

    for (auto x : _x)
    {
        std::cout << "Input x: " << x.transpose().format(Eigen::IOFormat(Eigen::StreamPrecision, 0, ", ", "\n")) << std::endl;
        Eigen::VectorXd posterior(number_of_states);
        for (int i = 0; i < number_of_states; ++i)
        {
            Eigen::VectorXd m = mu[i].head(number_of_input_variables);
            Eigen::MatrixXd s = sigma[i].block(input_start_index, input_start_index, number_of_input_variables, number_of_input_variables);
            posterior(i) = pi(i) * gaussPDF(x, m, s, number_of_variables);
        }
        posterior = posterior / (posterior.sum() + DBL_MIN);


        std::vector<Eigen::VectorXd> pred_temps;
        Eigen::VectorXd pred = Eigen::VectorXd::Zero(number_of_output_variables);
        for (size_t i = 0; i < number_of_states; i++)
        {
            Eigen::VectorXd diff = x.head(number_of_input_variables) - mu[i].head(number_of_input_variables);
            auto sigma_temp = sigma[i].block(output_start_index, input_start_index, number_of_output_variables, number_of_input_variables) * 
                        sigma[i].block(input_start_index, input_start_index, number_of_input_variables, number_of_input_variables).inverse();
            Eigen::VectorXd pred_temp = mu[i].tail(number_of_output_variables) + sigma_temp * diff;
            pred += posterior(i) * pred_temp;
            pred_temps.push_back(pred_temp);
        }

        _mean.push_back(pred);
        std::vector<Eigen::MatrixXd> sigma_temps;
        Eigen::MatrixXd sigma_pred = Eigen::MatrixXd::Zero(number_of_output_variables, number_of_output_variables);
        for (size_t i = 0; i < number_of_states; i++)
        {
            auto s = sigma[i].block(output_start_index, output_start_index, number_of_output_variables, number_of_output_variables) -
                     sigma[i].block(output_start_index, input_start_index, number_of_output_variables, number_of_input_variables) * 
                     sigma[i].block(input_start_index, input_start_index, number_of_input_variables, number_of_input_variables).inverse() * 
                     sigma[i].block(input_start_index, output_start_index, number_of_input_variables, number_of_output_variables);
            sigma_pred += posterior(i) * (s + pred_temps[i] * pred_temps[i].transpose());
        }
        sigma_pred -= pred * pred.transpose();
        _sigma.push_back(sigma_pred);
    }
    std::cout << "Inference completed." << std::endl;
    return 0;
}

double GaussianMixtureRegression::gaussPDF(const Eigen::VectorXd &x, const Eigen::VectorXd &mu, const Eigen::MatrixXd &sigma, const int nbVar)
{
    Eigen::VectorXd diff = x - mu;
    Eigen::VectorXd e = -diff.transpose() * sigma.inverse() * diff / 2;
    double res = exp(e(0)) / std::sqrt(std::pow(M_PI * 2, nbVar) * sigma.determinant());
    return res;
}