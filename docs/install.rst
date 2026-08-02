Install
=======

Dependencies
------------

* CMake ≥ 3.16, C++17
* Eigen3, jsoncpp, OpenMP
* `PIQP <https://github.com/PREDICT-EPFL/piqp>`_
* (Python) Python ≥ 3.9, NumPy; build needs pybind11 (fetched by CMake/pip)

Build C++ library
-----------------

.. code-block:: bash

   cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
     -DCMAKE_PREFIX_PATH=/path/to/piqp-install
   cmake --build build -j"$(nproc)"
   cmake --install build --prefix /path/to/prefix

Downstream CMake
----------------

.. code-block:: cmake

   find_package(lc-ns-kmp 0.1 REQUIRED)
   target_link_libraries(my_app PRIVATE lc-ns-kmp::gmr lc-ns-kmp::lc-ns-kmp)

Python package
--------------

.. code-block:: bash

   export CMAKE_PREFIX_PATH=/path/to/piqp-install
   pip install .

Or enable the extension from CMake:

.. code-block:: bash

   cmake -S . -B build -DLC_NS_KMP_BUILD_PYTHON=ON \
     -DCMAKE_PREFIX_PATH=/path/to/piqp-install
   cmake --build build -j"$(nproc)"
   PYTHONPATH=python python -c "from lc_ns_kmp import LC_NS_KMP"

Building this documentation
---------------------------

.. code-block:: bash

   pip install -r docs/requirements.txt
   # optional, if doxygen is installed:
   doxygen Doxyfile
   sphinx-build -b html docs docs/_build/html

Or::

   cmake --build build --target docs
