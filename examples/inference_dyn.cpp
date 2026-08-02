// WIP: lab LN middleware demo; not built on main. Finish on a feature branch.
#include <json/json.h>
#include <iostream>
#include <fstream>
#include "gmr.hpp"
#include "kmp.hpp"
#include <Eigen/Dense>
#include <Eigen/Eigen>
#include <chrono>
#include "ln_messages.h"
#include <ln/ln.h>


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

class KMPAction : public rl::kmp_step_base
{
    ln::client clnt;
    // std::string model_file_path = "../assets/models/model.json";
    std::string model_file_path = "../assets/models/model_bottle_neck_new.json";
    int horizon;
    int dim;
    kmp kmp_obj;
    double upper_lim = 1.0;
    GaussianRegression gmr;

    bool use_history;
    int history_horizon;

    std::vector<Eigen::MatrixXd> history_sigma;
    std::vector<Eigen::VectorXd> history_msr_state;
    std::vector<Eigen::VectorXd> history_obs;
    
public:
    KMPAction(int _dim, int _horizon, bool _use_history=false, int _history_horizon=0)
        : kmp_step_base(),
          clnt("kmp_action"),
          kmp_obj(_dim, _horizon),
          dim(_dim),
          horizon(_horizon),
          use_history(_use_history),
          history_horizon(_history_horizon)
    {
        register_kmp_step(&clnt, "rl.kmp_step");
        int status = gmr.load_gmm(model_file_path);
        
        gmr.print_parameters();

    }

    int on_kmp_step(ln::service_request &req, rl::kmp_step_t &svc) override
    {
        printf("handle service request!\n");
        
 
        std::vector<Eigen::VectorXd> pred;
        std::vector<Eigen::VectorXd> kmp_pred;
        std::vector<Eigen::MatrixXd> pred_sigma;
        std::vector<Eigen::VectorXd> obs;

        

        Eigen::VectorXd time = Eigen::VectorXd::LinSpaced(horizon, svc.req.t, svc.req.t + horizon * 0.0125);
        for (auto t : time)
        {
            Eigen::VectorXd o = Eigen::VectorXd::Zero(1);
            o[0] = t;
            obs.push_back(o);
        }
        gmr.run_inference(obs, pred, pred_sigma, 1);
        if (svc.req.clear_history == 1)
        {
            history_sigma.clear();
            history_msr_state.clear();
            history_obs.clear();
        }
        if (use_history)
        {
            std::cout << "Using History" << std::endl;
            Eigen::MatrixXd sigma = Eigen::MatrixXd::Identity(dim, dim) * 0.000001;
            Eigen::VectorXd robot_state = Eigen::VectorXd::Zero(dim);
            for (int i = 0; i < dim; i++)
            {
                robot_state[i] = svc.req.robot_state[i];
            }
            append_to_history(obs[0], robot_state, sigma);
            std::vector<Eigen::VectorXd> _pred;
            std::vector<Eigen::VectorXd> _obs;
            std::vector<Eigen::MatrixXd> _pred_sigma;
            
            _obs.insert(_obs.end(), history_obs.begin(), history_obs.end());
            _obs.insert(_obs.end(), obs.begin(), obs.end());
            _pred.insert(_pred.end(), history_msr_state.begin(), history_msr_state.end());
            _pred.insert(_pred.end(), pred.begin(), pred.end());
            _pred_sigma.insert(_pred_sigma.end(), history_sigma.begin(), history_sigma.end());
            _pred_sigma.insert(_pred_sigma.end(), pred_sigma.begin(), pred_sigma.end());
            
            obs.clear();
            pred.clear();
            pred_sigma.clear();

            for (int i = 0; i < horizon; i++)
            {
                obs.push_back(_obs[i]);
                pred.push_back(_pred[i]);
                pred_sigma.push_back(_pred_sigma[i]);
            }
        }

        std::vector<Eigen::VectorXd> p;
        std::vector<Eigen::MatrixXd> sp;

        std::vector<Eigen::VectorXd> gs;
        Eigen::VectorXd cs(1);
        Eigen::VectorXd g1(2);
        g1 << 1, 0;
        gs.clear();
        
        gs.push_back(g1);
        cs << -0.6;
        std::cout << "******************* " << cs.size() << std::endl;
        kmp_obj.add_constraints(gs, cs);
        cs.resize(0);
        Eigen::VectorXd xi(2);
        xi << svc.req.rl_action[0], svc.req.rl_action[1];

        int current_index = history_obs.size();

        std::vector<Eigen::VectorXd> current_state;
        
        current_state.push_back(obs[current_index]);
        
        kmp_obj.predict_LCNS(current_state, xi, obs[current_index], obs, pred, pred_sigma, p, sp);
        // std::cout << gs.size() << '\n';
        // std::cout << cs.size() << '\n';
        std::cout << "***************** History size "<< current_index << std::endl;
        std::cout << "*****************" << p[0][0] << p[0][1] << std::endl;
        double kmp_action[2];
        kmp_action[0] = p[0][0];
        kmp_action[1] = p[0][1];
        svc.resp.kmp_action = kmp_action;
        svc.resp.kmp_action_len = 2;

        req.respond();
        return 0;
    }

    int run()
    {
        while (true)
        {
            clnt.wait_and_handle_service_group_requests(NULL, 0.05);
        }
        return 0;
    }

    int append_to_history(Eigen::VectorXd _history_obs,
                              Eigen::VectorXd _history_msr_state,
                              Eigen::MatrixXd _history_sigma)
    {
        history_obs.push_back(_history_obs);
        history_msr_state.push_back(_history_msr_state);
        history_sigma.push_back(_history_sigma);
        if (history_obs.size() > history_horizon)
        {
            history_obs.erase(history_obs.begin());
            history_msr_state.erase(history_msr_state.begin());
            history_sigma.erase(history_sigma.begin());
        }
    }

};


int main(int argc, char **argv)
{

    KMPAction kmp_action(2, 40, true, 10);
    kmp_action.run();
    // generate_trajectory();
}