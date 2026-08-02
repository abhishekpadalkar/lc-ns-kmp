#!/usr/bin/env python3
"""Bottle-neck LC-NS-KMP demo (mirrors examples/inference.cpp)."""

from __future__ import annotations

import argparse
from pathlib import Path

import numpy as np

from lc_ns_kmp import GaussianMixtureRegression, LC_NS_KMP

REPO_ROOT = Path(__file__).resolve().parents[1]


def make_box_constraints(dim: int, bound: float) -> tuple[np.ndarray, np.ndarray]:
    """Axis-aligned |η_i| ≤ bound as gᵀη + c ≤ 0 with c = -bound."""
    g_rows = []
    c = np.full(2 * dim, -bound, dtype=float)
    for i in range(dim):
        g_pos = np.zeros(dim)
        g_neg = np.zeros(dim)
        g_pos[i] = 1.0
        g_neg[i] = -1.0
        g_rows.append(g_pos)
        g_rows.append(g_neg)
    return np.asarray(g_rows, dtype=float), c


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--model",
        type=Path,
        default=REPO_ROOT / "assets/models/model_bottle_neck.json",
    )
    parser.add_argument("--dim", type=int, default=2)
    parser.add_argument("--N", type=int, default=500)
    parser.add_argument("--t-max", type=float, default=1.0)
    parser.add_argument("--lambda_", type=float, default=6.0, dest="lambda_")
    parser.add_argument("--beta", type=float, default=6.0)
    parser.add_argument("--l", type=float, default=2.0)
    parser.add_argument("--bound", type=float, default=6.0)
    parser.add_argument("--s-hat-idx", type=int, default=100)
    parser.add_argument(
        "--gmm-out",
        type=Path,
        default=REPO_ROOT / "examples/gmm",
    )
    parser.add_argument(
        "--kmp-out",
        type=Path,
        default=REPO_ROOT / "examples/kmp",
    )
    parser.add_argument("--xi", type=float, nargs=2, default=[0.0, 0.0])
    args = parser.parse_args()

    if not (0 <= args.s_hat_idx < args.N):
        raise SystemExit("s_hat_idx out of range [0, N)")

    print(f"Loading GMM from file: {args.model}")
    gmr = GaussianMixtureRegression()
    if gmr.load_gmm(str(args.model)) != 0:
        raise SystemExit("Error loading GMM.")

    t = np.linspace(0.0, args.t_max, args.N)
    s = t.reshape(-1, 1)

    mu_gmr, sigma_gmr = gmr.run_inference(s, number_of_input_variables=1)

    kmp = LC_NS_KMP(args.dim, args.N, args.lambda_, args.beta, args.l)
    g_rows, c = make_box_constraints(args.dim, args.bound)
    kmp.add_constraints(g_rows, c)

    xi = np.asarray(args.xi, dtype=float)
    s_hat = s[args.s_hat_idx]
    eta = kmp.predict_LCNS(s, xi, s_hat, s, mu_gmr, sigma_gmr)

    args.gmm_out.parent.mkdir(parents=True, exist_ok=True)
    np.savetxt(f"{args.gmm_out}_mu.csv", mu_gmr, delimiter=",")
    # Flatten each (O,O) block row-major to match C++ Eigen transpose dump loosely;
    # store as N lines of O*O values for plotting means is enough for kmp.
    with open(f"{args.gmm_out}_sigma.csv", "w", encoding="utf-8") as fp:
        for block in sigma_gmr:
            fp.write(", ".join(f"{v:.16g}" for v in block.T.ravel()) + "\n")
    np.savetxt(f"{args.kmp_out}_mu.csv", eta, delimiter=",")
    print(f"Wrote {args.gmm_out}_mu.csv, {args.gmm_out}_sigma.csv, {args.kmp_out}_mu.csv")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
