Python API
==========

Install the package (``pip install .``) or set ``PYTHONPATH=python`` after
building with ``-DLC_NS_KMP_BUILD_PYTHON=ON``.

.. automodule:: lc_ns_kmp
   :members:
   :undoc-members:
   :imported-members:
   :show-inheritance:

NumPy conventions
-----------------

* Query / reference times ``s``, ``s_star``: shape ``(N, n_input)`` (usually ``n_input=1``).
* GMR / KMP means: shape ``(N, O)``.
* Covariances: shape ``(N, O, O)``.
* Constraint matrix ``g``: shape ``(F, O)`` — one normal per row.
* ``xi``, ``s_hat``, ``c``: 1-D arrays.

Example
-------

See ``examples/inference_python.py``.
