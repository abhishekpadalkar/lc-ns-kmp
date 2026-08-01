"""Shared Python utilities for demos, paths, and CSV I/O."""

from __future__ import annotations

from pathlib import Path
from typing import Any

import numpy as np

REPO_ROOT = Path(__file__).resolve().parents[1]
DEMO_DIR = REPO_ROOT / "assets" / "demonstrations"
MODEL_DIR = REPO_ROOT / "assets" / "models"

DEMO_NAMES = ("bottle_neck", "synthetic", "time_traj")

# Canonical time interval for demos and inference.
T_MIN = 0.0
T_MAX = 1.0


def demo_csv_path(name: str) -> Path:
    """Map logical demo name to CSV path, e.g. 'synthetic' → demo_synthetic.csv."""
    if name not in DEMO_NAMES:
        raise ValueError(f"Unknown demo name {name!r}. Choose from {DEMO_NAMES}.")
    return DEMO_DIR / f"demo_{name}.csv"


def normalize_time(t: np.ndarray) -> np.ndarray:
    """Map a trajectory time vector onto [T_MIN, T_MAX] (= [0, 1]).

    Each demo may have a different length / raw duration; normalization is
    per-trajectory using that trajectory's own min/max time.
    """
    t = np.asarray(t, dtype=float).reshape(-1)
    t0 = float(np.min(t))
    t1 = float(np.max(t))
    if not np.isfinite(t0) or not np.isfinite(t1) or t1 <= t0:
        raise ValueError(f"Invalid time range [{t0}, {t1}] for normalization.")
    return T_MIN + (T_MAX - T_MIN) * (t - t0) / (t1 - t0)


def load_demo_csv(path_or_name: str | Path, *, normalize: bool = True) -> list[dict[str, Any]]:
    """Load a demo CSV with columns demo_id,t,<outputs...>.

    Returns a list of dicts:
        {"t": (N,), "y": (N, D)}
    where D is the number of columns after t (x,y today; any D later).

    If normalize=True (default), each demo's time is mapped to [0, 1].
    """
    path = Path(path_or_name)
    if not path.suffix:
        path = demo_csv_path(str(path_or_name))
    elif not path.is_file() and path.name in DEMO_NAMES:
        path = demo_csv_path(path.name)

    if not path.is_file():
        raise FileNotFoundError(f"Missing demo CSV: {path}")

    data = np.loadtxt(path, delimiter=",", skiprows=1)
    if data.ndim != 2 or data.shape[1] < 3:
        raise ValueError(f"Expected columns demo_id,t,<y...>; got shape {data.shape} in {path}")

    demo_ids = data[:, 0].astype(int)
    demos: list[dict[str, Any]] = []
    dims: set[int] = set()
    for demo_id in np.unique(demo_ids):
        rows = data[demo_ids == demo_id]
        t = rows[:, 1].astype(float)
        y = rows[:, 2:].astype(float)
        if y.ndim == 1:
            y = y.reshape(-1, 1)
        if normalize:
            t = normalize_time(t)
        dims.add(y.shape[1])
        demos.append({"t": t, "y": y})

    if len(dims) != 1:
        raise ValueError(f"Inconsistent output dims across demos in {path}: {dims}")
    return demos


def write_demo_csv(path: Path, demos: list[dict[str, Any]]) -> None:
    """Write demos as demo_id,t,<y...> (assumes times already desired)."""
    rows = []
    for demo_id, d in enumerate(demos):
        t = np.asarray(d["t"], dtype=float).reshape(-1)
        y = np.asarray(d["y"], dtype=float)
        if y.ndim == 1:
            y = y.reshape(-1, 1)
        n = len(t)
        block = np.column_stack([np.full(n, demo_id, dtype=float), t, y])
        rows.append(block)
    table = np.vstack(rows)
    n_out = table.shape[1] - 2
    if n_out == 2:
        header = "demo_id,t,x,y"
    else:
        header = "demo_id,t," + ",".join(f"y{i}" for i in range(n_out))
    path.parent.mkdir(parents=True, exist_ok=True)
    np.savetxt(path, table, delimiter=",", header=header, comments="", fmt="%.10g")


def normalize_all_demo_csvs() -> None:
    """Rewrite every demo_*.csv with per-trajectory time in [0, 1]."""
    for name in DEMO_NAMES:
        path = demo_csv_path(name)
        demos = load_demo_csv(path, normalize=True)
        write_demo_csv(path, demos)
        tmax = max(float(d["t"].max()) for d in demos)
        tmin = min(float(d["t"].min()) for d in demos)
        lengths = [len(d["t"]) for d in demos]
        print(f"{path.name}: {len(demos)} demos, lengths={lengths}, t∈[{tmin:g}, {tmax:g}]")


def output_dim(demos: list[dict[str, Any]]) -> int:
    return int(demos[0]["y"].shape[1])


if __name__ == "__main__":
    normalize_all_demo_csvs()
