C++ API
=======

Public headers live in ``include/`` under namespace ``lc_ns_kmp``.
Link CMake targets ``lc-ns-kmp::gmr`` and ``lc-ns-kmp::lc-ns-kmp``.

When ``doxygen`` is available, run ``doxygen Doxyfile`` for full XML/HTML under
``docs/doxygen/`` (Sphinx will then enable Breathe automatically if XML exists).

GaussianMixtureRegression
-------------------------

Declared in ``gmr.hpp``. Load a GMM JSON and run Gaussian Mixture Regression
along query inputs (typically time).

.. code-block:: cpp

   #include "gmr.hpp"

   lc_ns_kmp::GaussianMixtureRegression gmr;
   gmr.load_gmm("assets/models/model_bottle_neck.json");
   gmr.run_inference(x, mean, sigma, /*number_of_input_variables=*/1);

**Methods**

* ``int load_gmm(const std::string &path)`` — load model; accepts ``C``/``dim`` or ``nbStates``/``nbVar``. Returns 0 on success.
* ``int run_inference(x, mean, sigma, n_input)`` — fill mean / covariance per query.
* ``void set_verbose(bool)`` / ``bool verbose() const`` — library logging (default off).
* ``int print_parameters() const`` — debug dump of GMM parameters.

LC_NS_KMP
---------

Declared in ``lc-ns-kmp.hpp``. Kernelized movement primitive with linear
constraints and optional null-space action.

.. code-block:: cpp

   #include "lc-ns-kmp.hpp"

   lc_ns_kmp::LC_NS_KMP kmp(/*O=*/2, /*N=*/500, /*lambda=*/6, /*beta=*/6, /*l=*/2);
   kmp.add_constraints(g_list, c);
   kmp.predict_LCNS(s_star, xi, s_hat, s, mu, Sigma, eta, sigma_out);

**Constructor**

``LC_NS_KMP(int output_dim, int horizon, double lambda = 6, double beta = 6, double l = 2)``

**Prediction** (``sigma_out`` currently unused)

* ``predict`` — unconstrained KMP
* ``predict_LC`` — linearly constrained KMP
* ``predict_LCNS`` — LC + null-space action ``xi`` at ``s_hat``
* ``predict_LCNS_extended`` — same with position–velocity extended kernel

**Constraints**

``add_constraints(g, c)`` — ``g`` is a list of normals (length ``O`` each).
Examples use ``gᵀη + c ≤ 0``. See :doc:`symbols`.

**Kernels**

* ``kernel_function(s1, s2)`` — diagonal SE kernel block
* ``extended_kernel_function(s1, s2)`` — position–velocity extended kernel
* ``time_kernel_value(t_i, t_j)`` — scalar SE on time
