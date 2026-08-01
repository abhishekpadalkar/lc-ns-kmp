"""GMM learning from demonstration CSVs (D-agnostic features)."""

from __future__ import annotations

import json
from pathlib import Path
from typing import Any, Literal

import numpy as np
from sklearn.mixture import GaussianMixture

from util import MODEL_DIR, load_demo_csv, output_dim

FeatureMode = Literal["pos", "pos_vel", "pos_vel_acc"]


def _finite_diff(y: np.ndarray, t: np.ndarray) -> np.ndarray:
    """Forward difference; last sample repeated. y: (N,D), t: (N,)."""
    dy = np.zeros_like(y)
    dt = np.diff(t)
    if np.any(dt <= 0):
        raise ValueError("Time must be strictly increasing within each demo.")
    dy[:-1] = np.diff(y, axis=0) / dt[:, None]
    dy[-1] = dy[-2] if len(y) > 1 else 0.0
    return dy


def build_feature_matrix(
    demos: list[dict[str, Any]],
    mode: FeatureMode = "pos",
) -> tuple[np.ndarray, dict[str, Any]]:
    """Stack demos into feature matrix X of shape (N_total, dim)."""
    blocks: list[np.ndarray] = []
    for d in demos:
        t = np.asarray(d["t"], dtype=float)
        y = np.asarray(d["y"], dtype=float)
        feats = [t[:, None], y]
        if mode in ("pos_vel", "pos_vel_acc"):
            v = _finite_diff(y, t)
            feats.append(v)
            if mode == "pos_vel_acc":
                feats.append(_finite_diff(v, t))
        blocks.append(np.hstack(feats))

    X = np.vstack(blocks)
    dts = [np.diff(np.asarray(d["t"], dtype=float)) for d in demos]
    dt = float(np.median(np.concatenate(dts))) if dts else 0.01
    meta = {
        "mode": mode,
        "n_demos": len(demos),
        "output_dim": output_dim(demos),
        "nbData": int(max(len(d["t"]) for d in demos)),
        "dt": dt,
        "dim": int(X.shape[1]),
    }
    return X, meta


def fit_gmm(
    X: np.ndarray,
    n_states: int,
    *,
    random_state: int = 0,
    reg_covar: float = 1e-6,
) -> GaussianMixture:
    gmm = GaussianMixture(
        n_components=n_states,
        covariance_type="full",
        random_state=random_state,
        reg_covar=reg_covar,
    )
    gmm.fit(X)
    return gmm


def export_model(
    gmm: GaussianMixture,
    path: str | Path,
    meta: dict[str, Any],
    *,
    cov_jitter: float = 1e-9,
) -> Path:
    """Export GMM JSON compatible with C++ GMR (C/dim and nbStates/nbVar)."""
    path = Path(path)
    if not path.is_absolute():
        path = MODEL_DIR / path

    means = np.asarray(gmm.means_, dtype=float)
    covs = np.asarray(gmm.covariances_, dtype=float).copy()
    priors = np.asarray(gmm.weights_, dtype=float)
    dim = int(means.shape[1])
    n_states = int(means.shape[0])

    eye = np.eye(dim)
    for k in range(n_states):
        covs[k] = covs[k] + cov_jitter * eye

    model = {
        "C": n_states,
        "dim": dim,
        "nbStates": n_states,
        "nbVar": dim,
        "nbData": int(meta.get("nbData", 0)),
        "dt": float(meta.get("dt", 0.01)),
        "mode": meta.get("mode", "pos"),
        "Mu": means.tolist(),
        "Sigma": covs.tolist(),
        "Priors": priors.tolist(),
    }
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w") as fp:
        json.dump(model, fp, indent=2)
    return path


def learn_and_export(
    demo_name: str,
    out_path: str | Path,
    *,
    n_states: int = 7,
    mode: FeatureMode = "pos",
    random_state: int = 0,
) -> tuple[GaussianMixture, dict[str, Any], Path]:
    demos = load_demo_csv(demo_name)
    X, meta = build_feature_matrix(demos, mode=mode)
    gmm = fit_gmm(X, n_states, random_state=random_state)
    path = export_model(gmm, out_path, meta)
    return gmm, meta, path
