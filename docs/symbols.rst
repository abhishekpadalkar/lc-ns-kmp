Paper symbol map
================

Symbols follow the RA-L 2025 paper
(`elib.dlr.de/216474 <https://elib.dlr.de/216474>`_).

.. list-table::
   :header-rows: 1
   :widths: 20 80

   * - Symbol
     - Meaning
   * - s, s\*
     - input state (time) and query state
   * - η(s)
     - predicted output
   * - μ, Σ
     - stacked GMR means / block-diagonal covariances
   * - N
     - number of reference points (horizon)
   * - O
     - output dimension (``output_dim``)
   * - λ, β
     - hyperparameters; γ = λ + β
   * - l
     - SE lengthscale: k = exp(−l ‖sᵢ − sⱼ‖²)
   * - K, k\*
     - kernel matrix / cross-kernel at s\*
   * - K̂, k̂\*
     - kernels involving null-space via-point ŝ
   * - ξ, ŝ
     - null-space action and its input location
   * - g, c
     - constraint hyperplane (paper: gᵀη ≥ c)
   * - F
     - number of constraints per timestep
   * - Ḡ, C̄
     - block-diagonal constraint parameters / stacked offsets
   * - α ≥ 0
     - dual multipliers (solved by PIQP)

Constraint convention
---------------------

* **Paper:** gᵀη ≥ c
* **Examples in this repo:** gᵀη + c ≤ 0
  (e.g. g = [1, 0], c = −2 ⇒ x ≤ 2)

Flip the sign of g (and adjust c) to map between the forms.
