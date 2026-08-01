#include <json/json.h>
#include <iostream>
#include <fstream>
#include "gmr.hpp"
#include "lc-ns-kmp.hpp"
#include <Eigen/Dense>
#include <Eigen/Eigen>
#include <Eigen/Core>
#include <chrono>



static void dump_mu_and_sigma(const std::vector<Eigen::VectorXd> mu, const std::vector<Eigen::MatrixXd> sigma, std::string file_name)
{
    const static Eigen::IOFormat CSVFormat(Eigen::StreamPrecision, Eigen::DontAlignCols, ", ", "\n");
    std::string mu_csv_data;
    std::string mu_file_name = file_name + "_mu.csv";
    std::ofstream mu_file(mu_file_name.c_str());
    for (auto m : mu)
    {
        mu_file << m.transpose().format(CSVFormat) << std::endl;
    }

    std::string sigma_csv_data;
    std::string sigma_file_name = file_name + "_sigma.csv";
    std::ofstream sigma_file(sigma_file_name.c_str());
    for (auto s : sigma)
    {
        sigma_file << s.transpose().format(CSVFormat) << std::endl;
    }
}


int generate_trajectory()
{

    const static Eigen::IOFormat CSVFormat(Eigen::StreamPrecision, Eigen::DontAlignCols, ", ", "\n");


    // nbVar/dim must be output_dim + 1 (time). This example uses 2D outputs.
    std::string model_file_path = "../assets/models/model_bottle_neck.json";

    std::cout << "Loading GMM from file: " << model_file_path << std::endl;

    LC_NS_KMP kmp_obj(2, 500);

    GaussianMixtureRegression gmr; // Create an object of GaussianMixtureRegression
    int status = gmr.load_gmm(model_file_path);

    if (status != 0)
    {
        std::cerr << "Error loading GMM." << std::endl;
        return 1;
    }
    // float upper_lim = 0.1250;
    // Time normalized to [0, 1] (same as demo CSVs / GMM training).
    float upper_lim = 1.0f;

    gmr.print_parameters();
    Eigen::VectorXd time = Eigen::VectorXd::LinSpaced(500, 0.0, upper_lim);
    std::vector<Eigen::VectorXd> pred;
    std::vector<Eigen::VectorXd> kmp_pred;
    std::vector<Eigen::MatrixXd> pred_sigma;

    std::cout << "Time: \n"
              << time.format(CSVFormat) << "\n";
    std::vector<Eigen::VectorXd> obs;
    for (auto t : time)
    {
        Eigen::VectorXd o = Eigen::VectorXd::Zero(1);
        o[0] = t;
        obs.push_back(o);
    }
    // for (auto o : obs)
    // {
    //     std::cout << o << "\n";
    // }
    gmr.print_parameters();
    auto start = std::chrono::high_resolution_clock::now();
    gmr.run_inference(obs, pred, pred_sigma, 1);
    std::cout << "Calculated GMR" << std::endl;
    // for (auto p : pred)
    // {
    //     std::cout << p << "\n";
    // }
    auto stop = std::chrono::high_resolution_clock::now();
    std::cout << "Time taken: \n"
              << std::chrono::duration<double, std::milli>(stop - start).count() / 1000 << "\n";
    std::cout << "Calculated GMR" << std::endl;

    std::vector<Eigen::VectorXd> p;
    std::vector<Eigen::MatrixXd> sp;

    std::vector<Eigen::VectorXd> gs;
    Eigen::VectorXd cs(4);
    std::cout << "None created" << std::endl;

    Eigen::VectorXd g1(2);
    g1 << 1, 0;
    std::cout << "G1 created" << std::endl;

    Eigen::VectorXd g2(2);
    g2 << -1, 0;
    std::cout << "G2 created" << std::endl;

    Eigen::VectorXd g3(2);
    g3 << 0, 1;

    std::cout << "G3 created" << std::endl;
    Eigen::VectorXd g4(2);
    g4 << 0, -1;
    std::cout << "G4 created" << std::endl;

    gs.push_back(g1);
    gs.push_back(g2);
    gs.push_back(g3);
    gs.push_back(g4);
    std::cout << "Gs created" << std::endl;
    // Loose box |x|,|y| <= 6 so NSA can push past the GMR mean (demos ~[0.2,0.8]x[0.1,1]).
    // Tighten (e.g. cs << -0.6, ...) to see the QP clamp the excursion.
    cs << -6, -6, -6, -6;
    std::cout << "Cs created" << std::endl;
    kmp_obj.add_constriants(gs, cs);
    std::cout << "Constraints added" << std::endl;
    // Non-zero null-space action: this is what moves KMP beyond the demonstrated mean.
    Eigen::VectorXd nsa(2);
    nsa << 0, 0;
    std::cout << "Nsa created" << std::endl;
    kmp_obj.predict_LCNS(obs, nsa, obs[10], obs, pred, pred_sigma, p, sp);
    kmp_pred = p;
    std::cout << "KMP Prediction done" << std::endl;
    stop = std::chrono::high_resolution_clock::now();
    std::cout << "Time taken: \n"
              << std::chrono::duration<double, std::milli>(stop - start).count() / 1000 << "\n";
    std::string file_name = "../examples/gmm";

    dump_mu_and_sigma(pred, pred_sigma, file_name);
    file_name = "../examples/kmp";

    dump_mu_and_sigma(kmp_pred, pred_sigma, file_name);
    return 0;
}


int main(int argc, char **argv)
{
    generate_trajectory();
}