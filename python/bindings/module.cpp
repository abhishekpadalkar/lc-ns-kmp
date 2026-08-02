#include <pybind11/eigen.h>
#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <stdexcept>
#include <string>
#include <vector>

#include <Eigen/Dense>

#include "gmr.hpp"
#include "lc-ns-kmp.hpp"

namespace py = pybind11;

namespace
{

std::vector<Eigen::VectorXd> matrix_rows_to_vectors(const Eigen::MatrixXd &m)
{
    std::vector<Eigen::VectorXd> out;
    out.reserve(static_cast<size_t>(m.rows()));
    for (Eigen::Index i = 0; i < m.rows(); ++i)
    {
        out.emplace_back(m.row(i).transpose());
    }
    return out;
}

Eigen::MatrixXd vectors_to_matrix_rows(const std::vector<Eigen::VectorXd> &vecs)
{
    if (vecs.empty())
    {
        return Eigen::MatrixXd(0, 0);
    }
    const Eigen::Index cols = vecs.front().size();
    Eigen::MatrixXd out(static_cast<Eigen::Index>(vecs.size()), cols);
    for (size_t i = 0; i < vecs.size(); ++i)
    {
        if (vecs[i].size() != cols)
        {
            throw std::invalid_argument("All vectors must have the same length");
        }
        out.row(static_cast<Eigen::Index>(i)) = vecs[i].transpose();
    }
    return out;
}

std::vector<Eigen::MatrixXd> ndarray_to_matrices(
    const py::array_t<double, py::array::c_style | py::array::forcecast> &arr)
{
    if (arr.ndim() != 3)
    {
        throw std::invalid_argument("Sigma stack must have shape (N, d, d)");
    }
    const auto n = arr.shape(0);
    const auto r = arr.shape(1);
    const auto c = arr.shape(2);
    if (r != c)
    {
        throw std::invalid_argument("Each Sigma block must be square");
    }
    auto buf = arr.unchecked<3>();
    std::vector<Eigen::MatrixXd> out;
    out.reserve(static_cast<size_t>(n));
    for (py::ssize_t i = 0; i < n; ++i)
    {
        Eigen::MatrixXd m(r, c);
        for (py::ssize_t a = 0; a < r; ++a)
        {
            for (py::ssize_t b = 0; b < c; ++b)
            {
                m(a, b) = buf(i, a, b);
            }
        }
        out.push_back(std::move(m));
    }
    return out;
}

py::array_t<double> matrices_to_ndarray(const std::vector<Eigen::MatrixXd> &mats)
{
    if (mats.empty())
    {
        return py::array_t<double>(std::vector<py::ssize_t>{0, 0, 0});
    }
    const auto r = mats.front().rows();
    const auto c = mats.front().cols();
    py::array_t<double> out({static_cast<py::ssize_t>(mats.size()), r, c});
    auto buf = out.mutable_unchecked<3>();
    for (size_t i = 0; i < mats.size(); ++i)
    {
        if (mats[i].rows() != r || mats[i].cols() != c)
        {
            throw std::invalid_argument("All Sigma blocks must have the same shape");
        }
        for (Eigen::Index a = 0; a < r; ++a)
        {
            for (Eigen::Index b = 0; b < c; ++b)
            {
                buf(static_cast<py::ssize_t>(i), a, b) = mats[i](a, b);
            }
        }
    }
    return out;
}

std::vector<Eigen::VectorXd> constraint_rows_to_vectors(const Eigen::MatrixXd &g_rows)
{
    // Each row is one g vector of length output_dim → store as column vectors in add_constraints
    return matrix_rows_to_vectors(g_rows);
}

} // namespace

PYBIND11_MODULE(_core, m)
{
    m.doc() = "Python bindings for lc-ns-kmp (GMR + LC-NS-KMP)";

    py::class_<lc_ns_kmp::GaussianMixtureRegression>(m, "GaussianMixtureRegression")
        .def(py::init<>())
        .def("set_verbose", &lc_ns_kmp::GaussianMixtureRegression::set_verbose)
        .def("verbose", &lc_ns_kmp::GaussianMixtureRegression::verbose)
        .def("load_gmm", &lc_ns_kmp::GaussianMixtureRegression::load_gmm, py::arg("model_file_path"))
        .def(
            "run_inference",
            [](lc_ns_kmp::GaussianMixtureRegression &self, const Eigen::MatrixXd &x_rows,
               int number_of_input_variables) {
                auto x = matrix_rows_to_vectors(x_rows);
                std::vector<Eigen::VectorXd> mean;
                std::vector<Eigen::MatrixXd> sigma;
                const int rc = self.run_inference(x, mean, sigma, number_of_input_variables);
                if (rc != 0)
                {
                    throw std::runtime_error("GMR run_inference failed");
                }
                return py::make_tuple(vectors_to_matrix_rows(mean), matrices_to_ndarray(sigma));
            },
            py::arg("x"), py::arg("number_of_input_variables") = 1,
            R"pbdoc(
              Run GMR on query inputs.

              Parameters
              ----------
              x : ndarray, shape (N, n_input)
                  One query input per row (typically time).
              number_of_input_variables : int
                  Number of input dimensions in the GMM (usually 1 for time).

              Returns
              -------
              mean : ndarray, shape (N, O)
              sigma : ndarray, shape (N, O, O)
            )pbdoc");

    py::class_<lc_ns_kmp::LC_NS_KMP>(m, "LC_NS_KMP")
        .def(py::init<int, int, double, double, double>(),
             py::arg("output_dim"), py::arg("horizon"), py::arg("lambda_") = 6.0,
             py::arg("beta") = 6.0, py::arg("l") = 2.0)
        .def("set_verbose", &lc_ns_kmp::LC_NS_KMP::set_verbose)
        .def("verbose", &lc_ns_kmp::LC_NS_KMP::verbose)
        .def(
            "add_constraints",
            [](lc_ns_kmp::LC_NS_KMP &self, const Eigen::MatrixXd &g_rows, const Eigen::VectorXd &c) {
                return self.add_constraints(constraint_rows_to_vectors(g_rows), c);
            },
            py::arg("g"), py::arg("c"),
            R"pbdoc(
              Set linear constraints. ``g`` has shape (F, O) — one constraint normal per row.
              Examples use gᵀη + c ≤ 0.
            )pbdoc")
        .def(
            "predict",
            [](lc_ns_kmp::LC_NS_KMP &self, const Eigen::MatrixXd &s_star, const Eigen::MatrixXd &s,
               const Eigen::MatrixXd &mu, const py::array_t<double> &sigma) {
                std::vector<Eigen::VectorXd> eta;
                std::vector<Eigen::MatrixXd> sigma_out;
                const int rc = self.predict(matrix_rows_to_vectors(s_star), matrix_rows_to_vectors(s),
                                            matrix_rows_to_vectors(mu), ndarray_to_matrices(sigma), eta,
                                            sigma_out);
                if (rc != 0)
                {
                    throw std::runtime_error("predict failed");
                }
                return vectors_to_matrix_rows(eta);
            },
            py::arg("s_star"), py::arg("s"), py::arg("mu"), py::arg("sigma"))
        .def(
            "predict_LC",
            [](lc_ns_kmp::LC_NS_KMP &self, const Eigen::MatrixXd &s_star, const Eigen::MatrixXd &s,
               const Eigen::MatrixXd &mu, const py::array_t<double> &sigma) {
                std::vector<Eigen::VectorXd> eta;
                std::vector<Eigen::MatrixXd> sigma_out;
                const int rc =
                    self.predict_LC(matrix_rows_to_vectors(s_star), matrix_rows_to_vectors(s),
                                    matrix_rows_to_vectors(mu), ndarray_to_matrices(sigma), eta, sigma_out);
                if (rc != 0)
                {
                    throw std::runtime_error("predict_LC failed");
                }
                return vectors_to_matrix_rows(eta);
            },
            py::arg("s_star"), py::arg("s"), py::arg("mu"), py::arg("sigma"))
        .def(
            "predict_LCNS",
            [](lc_ns_kmp::LC_NS_KMP &self, const Eigen::MatrixXd &s_star, const Eigen::VectorXd &xi,
               const Eigen::VectorXd &s_hat, const Eigen::MatrixXd &s, const Eigen::MatrixXd &mu,
               const py::array_t<double> &sigma) {
                std::vector<Eigen::VectorXd> eta;
                std::vector<Eigen::MatrixXd> sigma_out;
                const int rc = self.predict_LCNS(matrix_rows_to_vectors(s_star), xi, s_hat,
                                                 matrix_rows_to_vectors(s), matrix_rows_to_vectors(mu),
                                                 ndarray_to_matrices(sigma), eta, sigma_out);
                if (rc != 0)
                {
                    throw std::runtime_error("predict_LCNS failed");
                }
                return vectors_to_matrix_rows(eta);
            },
            py::arg("s_star"), py::arg("xi"), py::arg("s_hat"), py::arg("s"), py::arg("mu"),
            py::arg("sigma"))
        .def(
            "predict_LCNS_extended",
            [](lc_ns_kmp::LC_NS_KMP &self, const Eigen::MatrixXd &s_star, const Eigen::VectorXd &xi,
               const Eigen::VectorXd &s_hat, const Eigen::MatrixXd &s, const Eigen::MatrixXd &mu,
               const py::array_t<double> &sigma) {
                std::vector<Eigen::VectorXd> eta;
                std::vector<Eigen::MatrixXd> sigma_out;
                const int rc = self.predict_LCNS_extended(
                    matrix_rows_to_vectors(s_star), xi, s_hat, matrix_rows_to_vectors(s),
                    matrix_rows_to_vectors(mu), ndarray_to_matrices(sigma), eta, sigma_out);
                if (rc != 0)
                {
                    throw std::runtime_error("predict_LCNS_extended failed");
                }
                return vectors_to_matrix_rows(eta);
            },
            py::arg("s_star"), py::arg("xi"), py::arg("s_hat"), py::arg("s"), py::arg("mu"),
            py::arg("sigma"));
}
