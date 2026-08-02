"""Python bindings for Linearly Constrained Null-Space KMPs.

Classes
-------
GaussianMixtureRegression
    Load a GMM JSON and run GMR (NumPy in/out).
LC_NS_KMP
    Constrained KMP with optional null-space action.

See ``examples/inference_python.py`` for an end-to-end bottle-neck demo.
"""

from lc_ns_kmp._core import GaussianMixtureRegression, LC_NS_KMP

__all__ = ["GaussianMixtureRegression", "LC_NS_KMP"]
