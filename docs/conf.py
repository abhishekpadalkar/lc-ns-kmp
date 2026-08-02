# Configuration file for the Sphinx documentation builder.

from __future__ import annotations

import os
import sys
from pathlib import Path

DOC_DIR = Path(__file__).resolve().parent
REPO_ROOT = DOC_DIR.parent

# Prefer the in-tree package (with built _core.so) for autodoc.
sys.path.insert(0, str(REPO_ROOT / "python"))

project = "lc-ns-kmp"
copyright = "2025, Abhishek Padalkar"
author = "Abhishek Padalkar"
release = "0.1.0"
version = "0.1.0"

extensions = [
    "sphinx.ext.autodoc",
    "sphinx.ext.napoleon",
    "sphinx.ext.viewcode",
    "sphinx.ext.intersphinx",
]

# Optional Breathe bridge when Doxygen XML is present.
_doxy_xml = DOC_DIR / "doxygen" / "xml"
if _doxy_xml.is_dir() and any(_doxy_xml.glob("*.xml")):
    extensions.append("breathe")
    breathe_projects = {"lc-ns-kmp": str(_doxy_xml)}
    breathe_default_project = "lc-ns-kmp"
    breathe_default_members = ("members", "undoc-members")

templates_path = ["_templates"]
exclude_patterns = ["_build", "Thumbs.db", ".DS_Store", "doxygen"]

html_theme = "sphinx_rtd_theme"
html_static_path = ["_static"]

autodoc_member_order = "bysource"
autodoc_typehints = "description"
napoleon_google_docstring = True
napoleon_numpy_docstring = True

intersphinx_mapping = {
    "python": ("https://docs.python.org/3", None),
    "numpy": ("https://numpy.org/doc/stable/", None),
}

# Avoid crashing the full build if the extension is not built yet.
autodoc_mock_imports = []
if not any((REPO_ROOT / "python" / "lc_ns_kmp").glob("_core*.so")):
    autodoc_mock_imports = ["lc_ns_kmp._core"]
