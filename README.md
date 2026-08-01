# lc-ns-kmp

Linearly Constrained Null-Space Kernelized Movement Primitives

## Layout

```text
├── CMakeLists.txt
├── include/          # GMR, LC-NS-KMP headers
├── src/              # library sources
├── examples/         # inference binaries
├── python/           # GMM learning / plotting (util.py, notebooks)
└── assets/           # demonstrations and GMM models
```

## Dependencies

- CMake ≥ 3.16
- C++17 compiler
- Eigen3
- jsoncpp
- [PIQP](https://github.com/PREDICT-EPFL/piqp)
- OpenMP

## Build

```bash
mkdir -p build && cd build
cmake ..
cmake --build .
```

If a dependency is installed in a custom prefix (e.g. PIQP):

```bash
cmake -DCMAKE_PREFIX_PATH=/path/to/prefix ..
```

Run examples from `build/` (asset paths are relative to that directory):

```bash
./inference
./inference_extended
```

## Time convention

All demo times are normalized **per trajectory** to `[0, 1]` (different demos may have different lengths). Inference uses the same interval (`LinSpaced(..., 0, 1)`). See `python/util.py` (`normalize_time`, `T_MAX`).

## Python

From `python/` (use the project venv if present):

```bash
# rewrite demo CSVs with t ∈ [0, 1]
python util.py

# learn a GMM (see learn_gmm.ipynb for the interactive flow)
python -c "from gmm_learning import learn_and_export; learn_and_export('synthetic', 'model_synthetic.json', mode='pos')"
```
