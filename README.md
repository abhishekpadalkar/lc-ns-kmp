# lc-ns-kmp

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

**Linearly Constrained Null-Space Kernelized Movement Primitives** — a C++ library (with Python GMM tooling) for learning constrained trajectories from demonstrations, with optional null-space actions for guided exploration.

Paper (please cite this primarily):

> Padalkar, A., Stulp, F., Neumann, G., & Silverio, J. (2025).
> *Towards Safe and Efficient Learning in the Wild: Guiding RL With Constrained Uncertainty-Aware Movement Primitives.*
> IEEE Robotics and Automation Letters, 10(7), 6880–6887.
> [doi:10.1109/LRA.2025.3566599](https://doi.org/10.1109/LRA.2025.3566599) · [elib](https://elib.dlr.de/216474)

See [`CITATION.cff`](CITATION.cff) (paper = preferred citation; this repo is secondary).

## Quickstart (bottle-neck demo)

Reproduce the end-to-end path: **build → run inference → plot**.

### 1. Dependencies

| Dependency | Notes |
|---|---|
| CMake ≥ 3.16, C++17 | |
| Eigen3 ≥ 3.3 | e.g. `libeigen3-dev` |
| jsoncpp | e.g. `libjsoncpp-dev` |
| OpenMP | usually with the compiler |
| [PIQP](https://github.com/PREDICT-EPFL/piqp) | build/install; pass its prefix to CMake |

Ubuntu-style packages (PIQP still needs a separate install):

```bash
sudo apt install build-essential cmake libeigen3-dev libjsoncpp-dev libomp-dev
```

Install PIQP to a prefix (example):

```bash
git clone https://github.com/PREDICT-EPFL/piqp.git
cmake -S piqp -B piqp/build -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=$HOME/piqp-install
cmake --build piqp/build -j"$(nproc)"
cmake --install piqp/build
```

### 2. Build and run

From the repository root:

```bash
rm -rf build
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_PREFIX_PATH=$HOME/piqp-install   # adjust to your PIQP prefix
cmake --build build -j"$(nproc)"

cd build
./inference
```

`./inference` loads [`assets/models/model_bottle_neck.json`](assets/models/model_bottle_neck.json), runs GMR + `predict_LCNS`, and writes:

- `examples/gmm_mu.csv`, `examples/gmm_sigma.csv`
- `examples/kmp_mu.csv` (KMP predictive covariance is not exported yet)

Run `./inference` from `build/` so the relative asset paths resolve.

### 3. Plot

```bash
cd python
python -m venv .venv && source .venv/bin/activate
pip install -r requirements.txt
jupyter notebook plot.ipynb
```

In the notebook, set `DEMO_NAME = "bottle_neck"` and run the cells that load `examples/gmm_mu.csv` / `examples/kmp_mu.csv`.

## Install as a CMake package

```bash
cmake --install build --prefix /path/to/prefix
```

Downstream:

```cmake
find_package(lc-ns-kmp 0.1 REQUIRED)
target_link_libraries(my_app PRIVATE lc-ns-kmp::gmr lc-ns-kmp::lc-ns-kmp)
```

Point `CMAKE_PREFIX_PATH` at both this install prefix and PIQP’s prefix. A minimal consumer lives in [`examples/cmake_consumer/`](examples/cmake_consumer/).

## Pipeline

```text
demos (CSV) → Python GMM fit → GMM JSON → C++ GMR → LC-NS-KMP predict_* → trajectory CSV
```

- **GMR** ([`include/gmr.hpp`](include/gmr.hpp)): load GMM JSON, regress means/covariances along a time grid.
- **LC-NS-KMP** ([`include/lc-ns-kmp.hpp`](include/lc-ns-kmp.hpp)): kernelized MP with linear constraints and optional null-space action ξ at ŝ.

Namespace: `lc_ns_kmp`.

### Hyperparameters

| Symbol | Code | Role |
|---|---|---|
| λ | `lambda` | KMP regularization (with Σ) |
| β | `beta` | null-space / via-point weight; γ = λ + β |
| l | `l` | SE kernel lengthscale: k = exp(−l ‖sᵢ − sⱼ‖²) |
| N | `horizon` | number of reference / query times |
| O | `output_dim` | output dimension |
| F | set by `add_constraints` | constraints per timestep |

Defaults in the bottle-neck example: λ = β = 6, l = 2, N = 500, O = 2.

### Constraint convention

- **Paper:** gᵀη ≥ c  
- **This repo’s examples:** gᵀη + c ≤ 0 (e.g. g = [1, 0], c = −2 ⇒ x ≤ 2)

Flip the sign of g (and adjust c) to map between the two forms. See comments in [`include/lc-ns-kmp.hpp`](include/lc-ns-kmp.hpp).

### Time convention

Demo times are normalized **per trajectory** to `[0, 1]`. Inference uses the same interval. See [`python/util.py`](python/util.py).

## GMM JSON schema

C++ GMR accepts either naming style:

```json
{
  "C": 7,
  "dim": 3,
  "nbStates": 7,
  "nbVar": 3,
  "Priors": [ /* length C */ ],
  "Mu": [ /* C vectors of length dim */ ],
  "Sigma": [ /* C matrices dim×dim */ ]
}
```

For position-only models, `dim = 1 + O` (time + outputs). Extra metadata (`mode`, `dt`, …) from the Python exporter is ignored by C++.

Learn a model:

```bash
cd python
pip install -r requirements.txt
python -c "from gmm_learning import learn_and_export; learn_and_export('bottle_neck', 'model_bottle_neck.json', mode='pos')"
```

Interactive flow: [`python/learn_gmm.ipynb`](python/learn_gmm.ipynb).

## Layout

```text
├── CMakeLists.txt
├── cmake/                 # package Config template
├── include/               # gmr.hpp, lc-ns-kmp.hpp
├── src/
├── examples/              # inference (+ cmake_consumer); WIP: *_extended, *_dyn
├── python/                # GMM learning / plotting
└── assets/                # demonstrations and GMM models
```

## Documentation / bindings

### Python bindings (pybind11)

Build the extension in-tree:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_PREFIX_PATH=$HOME/piqp-install \
  -DLC_NS_KMP_BUILD_PYTHON=ON
cmake --build build -j"$(nproc)"
```

Or install the package (editable):

```bash
python3 -m venv .venv && source .venv/bin/activate
export CMAKE_PREFIX_PATH=$HOME/piqp-install   # PIQP (+ other custom deps)
pip install -e .
python examples/inference_python.py
```

If `import lc_ns_kmp` fails with `libpiqp.so: cannot open shared object file`, ensure PIQP was on `CMAKE_PREFIX_PATH` at build time (RPATH is embedded from that install), or set `LD_LIBRARY_PATH` to PIQP’s `lib/` directory.

API: `from lc_ns_kmp import GaussianMixtureRegression, LC_NS_KMP`.

C++ / Sphinx docs — coming in a later release.

## License

MIT — see [`LICENSE`](LICENSE).
