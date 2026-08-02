Quickstart
==========

Bottle-neck demo (C++)
----------------------

.. code-block:: bash

   cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
     -DCMAKE_PREFIX_PATH=$HOME/piqp-install
   cmake --build build -j"$(nproc)"
   cd build && ./inference

Outputs (relative to the repo): ``examples/gmm_mu.csv``, ``examples/gmm_sigma.csv``,
``examples/kmp_mu.csv``.

Bottle-neck demo (Python)
-------------------------

.. code-block:: bash

   python3 -m venv .venv && source .venv/bin/activate
   export CMAKE_PREFIX_PATH=$HOME/piqp-install
   pip install -e .
   python examples/inference_python.py

Plot with ``python/plot.ipynb`` (``DEMO_NAME = "bottle_neck"``).

Minimal C++ usage
-----------------

.. code-block:: cpp

   #include "gmr.hpp"
   #include "lc-ns-kmp.hpp"

   lc_ns_kmp::GaussianMixtureRegression gmr;
   gmr.load_gmm("model.json");

   lc_ns_kmp::LC_NS_KMP kmp(/*output_dim=*/2, /*horizon=*/500);
   // add_constraints(...);
   // predict_LCNS(...);

Minimal Python usage
--------------------

.. code-block:: python

   import numpy as np
   from lc_ns_kmp import GaussianMixtureRegression, LC_NS_KMP

   gmr = GaussianMixtureRegression()
   gmr.load_gmm("model.json")
   s = np.linspace(0, 1, 500).reshape(-1, 1)
   mu, sigma = gmr.run_inference(s, number_of_input_variables=1)

   kmp = LC_NS_KMP(2, 500)
   # kmp.add_constraints(g_rows, c)
   # eta = kmp.predict_LCNS(s, xi, s_hat, s, mu, sigma)
