#include <json/json.h>
#include <iostream>
#include <fstream>
#include "gmr.hpp"
#include "lc-ns-kmp.hpp"
#include <Eigen/Dense>
#include <Eigen/Eigen>
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


// int generate_synthetic_trajectory()
// {

//     const static Eigen::IOFormat CSVFormat(Eigen::StreamPrecision, Eigen::DontAlignCols, ", ", "\n");

//     // const char *cStr = argv[0];
//     // std::cout << cStr << std::endl;
//     // std::cout << argv[0] << std::endl;

//     std::string model_file_path = "/home/pada_ab/rl_workspace/rl_lfd/assets/models/model_synthetic_vel.json";
//     // std::string model_file_path = "/home/pada_ab/rl_workspace/rl_lfd/assets/model_bottle_neck_new.json";
//     // std::string model_file_path = "/home/pada_ab/rl_workspace/rl_lfd/assets/";

//     int horizon = 500;

//     kmp kmp_obj(6, horizon);

//     GaussianMixtureRegression gmr; 
//     int status = gmr.load_gmm(model_file_path);

//     if (status != 0)
//     {
//         std::cerr << "Error loading GMM." << std::endl;
//         return 1;
//     }
//     // float upper_lim = 0.1250;
//     float upper_lim = 0.01 * horizon;

//     gmr.print_parameters();
//     Eigen::VectorXd time = Eigen::VectorXd::LinSpaced(horizon, 0.01, upper_lim);
//     std::vector<Eigen::VectorXd> pred;
//     std::vector<Eigen::VectorXd> kmp_pred;
//     std::vector<Eigen::MatrixXd> pred_sigma;

//     std::cout << "Time: \n"
//               << time.format(CSVFormat) << "\n";
//     std::vector<Eigen::VectorXd> obs;
//     for (auto t : time)
//     {
//         Eigen::VectorXd o = Eigen::VectorXd::Zero(1);
//         o[0] = t;
//         obs.push_back(o);
//     }

//     auto start = std::chrono::high_resolution_clock::now();
//     gmr.run_inference(obs, pred, pred_sigma, 1);

//     std::cout<< "predicted sigma size : " << pred_sigma[0].rows() << " x " << pred_sigma[0].cols() << "\n";

//     auto stop = std::chrono::high_resolution_clock::now();
//     std::cout << "Time taken: \n"
//               << std::chrono::duration<double, std::milli>(stop - start).count() / 1000 << "\n";
//     std::cout << "Calculated GMR" << std::endl;

//     std::vector<Eigen::VectorXd> p;
//     std::vector<Eigen::MatrixXd> sp;

//     std::vector<Eigen::VectorXd> gs;
//     Eigen::VectorXd cs(4);
//     std::cout << "None created" << std::endl;

//     Eigen::VectorXd g1(6);
//     g1 << 0, 0, 0, 0, 1, 0;
//     std::cout << "G1 created" << std::endl;

//     Eigen::VectorXd g2(6);
//     g2 << 0, 0, 0, 0, -1, 0;
//     std::cout << "G2 created" << std::endl;

//     Eigen::VectorXd g3(6);
//     g3 << 0, 0, 0, 0, 0, 1;

//     std::cout << "G3 created" << std::endl;
//     Eigen::VectorXd g4(6);
//     g4 << 0, 0, 0, 0, 0, -1;
//     std::cout << "G4 created" << std::endl;

//     gs.push_back(g1);
//     gs.push_back(g2);
//     gs.push_back(g3);
//     gs.push_back(g4);

//     cs << -6, -6, -0.8, -0.8;

//     kmp_obj.add_constraints(gs, cs);

//     Eigen::VectorXd xi(6);
//     xi << 0, 0, 0, 0, 0, 0;
//     kmp_obj.predict_LCNS_extended_acc(obs, xi, obs[1], obs, pred, pred_sigma, p, sp);
//     kmp_pred = p;

//     stop = std::chrono::high_resolution_clock::now();
//     std::cout << "Time taken: \n"
//               << std::chrono::duration<double, std::milli>(stop - start).count() / 1000 << "\n";
//     std::string file_name = "/home/pada_ab/rl_workspace/rl_lfd/gmm";

//     dump_mu_and_sigma(pred, pred_sigma, file_name);
//     file_name = "/home/pada_ab/rl_workspace/rl_lfd/kmp";

//     dump_mu_and_sigma(kmp_pred, pred_sigma, file_name);
//     return 0;
// }


// int rl_exploration_trajectory()
// {

//     const static Eigen::IOFormat CSVFormat(Eigen::StreamPrecision, Eigen::DontAlignCols, ", ", "\n");

//     std::string model_file_path = "/home/pada_ab/rl_workspace/rl_lfd/assets/models/model_synthetic_vel.json";
//     // std::string model_file_path = "/home/pada_ab/rl_workspace/rl_lfd/assets/model_bottle_neck_new.json";
//     // std::string model_file_path = "/home/pada_ab/rl_workspace/rl_lfd/assets/";

//     int number_of_points = 450;

//     int horizon = 40;

//     kmp kmp_obj(6, horizon);

//     GaussianMixtureRegression gmr;
//     int status = gmr.load_gmm(model_file_path);

//     gmr.print_parameters();

//     if (status != 0)
//     {
//         std::cerr << "Error loading GMM." << std::endl;
//         return 1;
//     }

//     Eigen::VectorXd cs(4);
//     Eigen::VectorXd g1(6);
//     g1 << 0, 0, 0, 0, 1, 0;
//     Eigen::VectorXd g2(6);
//     g2 << 0, 0, 0, 0, -1, 0;
//     Eigen::VectorXd g3(6);
//     g3 << 0, 0, 0, 0, 0, 1;
//     Eigen::VectorXd g4(6);
//     g4 << 0, 0, 0, 0, 0, -1;

//     std::vector<Eigen::VectorXd> gs;

//     gs.push_back(g1);
//     gs.push_back(g2);
//     gs.push_back(g3);
//     gs.push_back(g4);

//     cs << -6, -6, -0.8, -0.8;

//     kmp_obj.add_constraints(gs, cs);
//     std::vector<Eigen::VectorXd> kmp_pred;

//     auto start = std::chrono::high_resolution_clock::now();

//     std::srand(42);

//     for (int _n = 0; _n < number_of_points; _n++)
//     {
//         float lower_lim = 0.01 * _n;
//         float upper_lim = lower_lim + 0.01 * horizon;
//         Eigen::VectorXd time = Eigen::VectorXd::LinSpaced(horizon, lower_lim, upper_lim);

//         // std::cout << "Time: \n" << time.format(CSVFormat) << "\n";

//         std::vector<Eigen::VectorXd> pred;
//         std::vector<Eigen::MatrixXd> pred_sigma;
//         std::vector<Eigen::VectorXd> obs;
//         for (auto t : time)
//         {
//             Eigen::VectorXd o = Eigen::VectorXd::Zero(1);
//             o[0] = t;
//             obs.push_back(o);
//         }

//         gmr.run_inference(obs, pred, pred_sigma, 1);

//         // std::cout << "predicted sigma size : " << pred_sigma[0].rows() << " x " << pred_sigma[0].cols() << "\n";

//         // std::cout << "Calculated GMR" << std::endl;

//         std::vector<Eigen::VectorXd> p;
//         std::vector<Eigen::MatrixXd> sp;


//         // Eigen::VectorXd xi(6);
//         // xi << 0, 0, 0, 0, 0, 0;

//         Eigen::VectorXd xi = Eigen::VectorXd::Random(6)*300;
//         xi[0] = 0;
//         xi[1] = 0;
//         xi[2] = 0;
//         xi[3] = 0;
//         xi[4] = 0;
//         xi[5] = 0;
//         std::vector<Eigen::VectorXd> current_state;
//         current_state.push_back(obs[1]);
//         kmp_obj.predict_LCNS_extended_acc(current_state, xi, obs[1], obs, pred, pred_sigma, p, sp);
//         kmp_pred.push_back(p[0]);
//     }

//     auto stop = std::chrono::high_resolution_clock::now();

//     std::cout << "Time taken: \n"
//               << std::chrono::duration<double, std::milli>(stop - start).count() / 1000 << "\n";
//     std::string file_name = "/home/pada_ab/rl_workspace/rl_lfd/gmm";

//     // dump_mu_and_sigma(pred, pred_sigma, file_name);
//     file_name = "/home/pada_ab/rl_workspace/rl_lfd/kmp";
//     std::vector<Eigen::MatrixXd> pred_sigma;

//     dump_mu_and_sigma(kmp_pred, pred_sigma, file_name);
//     return 0;
// }

int bottle_neck_svc_provider(int argc, char **argv)
{

    // ln::client clnt("kmp_client");
    // kmp_action_svc = clnt.get_service_provider("rl.kmp_step", "rl/kmp_step",
    //                                            rl_kmp_step_signature);

    // area_circle_svc->set_handler(call_circle_area, NULL);

    // area_circle_svc->do_register("default group");

    const static Eigen::IOFormat CSVFormat(Eigen::StreamPrecision, Eigen::DontAlignCols, ", ", "\n");

    const char *cStr = argv[0];
    std::cout << cStr << std::endl;
    std::cout << argv[0] << std::endl;

    // std::string model_file_path = "../assets/models/model.json";
    std::string model_file_path = "../assets/models/model_bottle_neck_new.json";
    int dim = 2;
    int horizon = 400;
    LC_NS_KMP kmp_obj(dim, horizon);

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
    Eigen::VectorXd time = Eigen::VectorXd::LinSpaced(horizon, 0.0, upper_lim);
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
    auto start = std::chrono::high_resolution_clock::now();
    gmr.run_inference(obs, pred, pred_sigma, 1);
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
    Eigen::VectorXd cs(1);
    std::cout << "None created" << std::endl;

    Eigen::VectorXd g1(2);
    g1 << 1, 0;
    std::cout << "G1 created" << std::endl;

    // Eigen::VectorXd g2(6);
    // g2 << -1, 0, 0, 0, 0, 0;
    // std::cout << "G2 created" << std::endl;

    // Eigen::VectorXd g3(6);
    // g3 << 0, 0, 0, 1, 0, 0;

    // std::cout << "G3 created" << std::endl;
    // Eigen::VectorXd g4(6);
    // g4 << 0, 0, 0, -1, 0, 0;
    // std::cout << "G4 created" << std::endl;

    gs.push_back(g1);
    // gs.push_back(g2);
    // gs.push_back(g3);
    // gs.push_back(g4);

    // cs << -0.002, -0.002, -2, -2;
    cs << -0.6;

    kmp_obj.add_constraints(gs, cs);

    Eigen::VectorXd xi(2);
    xi << 0, 0;
    std::vector<Eigen::VectorXd> current_state;
    for (size_t i = 0; i < 200; i++)
    {
        current_state.push_back(obs[i]);
    }

    // current_state.push_back(obs[0]);
    kmp_obj.predict_LCNS(current_state, xi, obs[10], obs, pred, pred_sigma, p, sp);
    kmp_pred = p;

    stop = std::chrono::high_resolution_clock::now();
    std::cout << "Time taken: \n"
              << std::chrono::duration<double, std::milli>(stop - start).count() / 1000 << "\n";
    std::string file_name = "../examples/gmm_bt";

    dump_mu_and_sigma(pred, pred_sigma, file_name);
    file_name = "../examples/kmp_bt";

    dump_mu_and_sigma(kmp_pred, pred_sigma, file_name);
    return 0;
}

int main(int argc, char **argv)
{

    // // // bottle_neck_svc_provider(argc, argv);
    // KMPAction kmp_action(2, 40, false, 10);
    // kmp_action.run();
    // generate_synthetic_trajectory();
}