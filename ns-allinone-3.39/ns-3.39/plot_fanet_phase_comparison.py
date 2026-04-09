#!/usr/bin/env python3
"""
plot_fanet_phase_comparison.py

Standalone plot generator for the new FANET comparison stats:
  - Phase 1 baseline (paper FF-AODV, Gamma=0.0)
  - Phase 2 velocity-aware FF-AODV (Gamma=0.2)

Expected input CSV files:
  results/fanet_phase1/vary_speed.csv
  results/fanet_bonus/vary_speed.csv

Input format supports repeated speed points across multiple runs.
If a 'run_id' column exists, the script reports unique run counts.

Output folders:
  graphs/fanet_phase1/
  graphs/fanet_compare/
  graphs/overview/
"""

import os
import sys

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt
import matplotlib.ticker as ticker
import numpy as np
import pandas as pd

plt.rcParams.update(
    {
        "font.size": 12,
        "axes.titlesize": 13,
        "axes.labelsize": 12,
        "axes.titleweight": "bold",
        "lines.linewidth": 2.2,
        "lines.markersize": 8,
        "grid.alpha": 0.35,
        "grid.linestyle": "--",
        "figure.dpi": 150,
        "savefig.dpi": 150,
        "savefig.bbox": "tight",
    }
)

COLOR_P1 = "#8c564b"
COLOR_P2 = "#2ca02c"
MARKER_P1 = "D"
MARKER_P2 = "^"

LABEL_P1 = "FF-AODV Phase 1 Baseline (Mobile)"
LABEL_P2 = "FF-AODV Phase 2 Velocity-Aware (Mobile)"

METRICS = [
    ("throughput_mbps", "Network Throughput (Mbps)", "throughput"),
    ("delay_ms", "Avg. End-to-End Delay (ms)", "delay"),
    ("pdr_pct", "Packet Delivery Ratio (%)", "pdr"),
    ("pdrop_pct", "Packet Drop Ratio (%)", "pdrop"),
    ("energy_j", "Total Energy Consumption (J)", "energy"),
]

XCOL = "speed_mps"
XLABEL = "Node Speed (m/s)"
XVALS = [5, 10, 15, 20, 25]

CSV_P1 = "results/fanet_phase1/vary_speed.csv"
CSV_P2 = "results/fanet_bonus/vary_speed.csv"

OUT_P1 = "graphs/fanet_phase1"
OUT_CMP = "graphs/fanet_compare"
OUT_OV = "graphs/overview"


def ensure_dirs() -> None:
    for d in (OUT_P1, OUT_CMP, OUT_OV):
        os.makedirs(d, exist_ok=True)


def load_csv(path: str, xcol: str) -> "pd.DataFrame | None":
    if not os.path.exists(path):
        print(f"  [SKIP ] missing: {path}")
        return None
    try:
        df = pd.read_csv(path)
        for col in df.columns:
            df[col] = pd.to_numeric(df[col], errors="coerce")
        if xcol not in df.columns:
            print(f"  [ERROR] {path}: missing x column '{xcol}'")
            return None
        df = df.dropna(subset=[xcol]).sort_values(xcol).reset_index(drop=True)
        print(f"  [LOAD ] {path} ({len(df)} rows)")
        return df
    except Exception as exc:
        print(f"  [ERROR] {path}: {exc}")
        return None


def get_stats(df: "pd.DataFrame | None", xcol: str, xvals: list, metric_col: str):
    if df is None or metric_col not in df.columns:
        n = len(xvals)
        return [float("nan")] * n, [float("nan")] * n

    means = []
    stds = []
    for x in xvals:
        mask = (df[xcol] - float(x)).abs() < 1e-6
        subset = df.loc[mask, metric_col].dropna()
        if subset.empty:
            means.append(float("nan"))
            stds.append(float("nan"))
        else:
            means.append(float(subset.mean()))
            stds.append(float(subset.std(ddof=0)))
    return means, stds


def apply_common(ax, xlabel: str, ylabel: str, title: str, xvals: list, legend: bool = True):
    ax.set_xlabel(xlabel)
    ax.set_ylabel(ylabel)
    ax.set_title(title, pad=8)
    ax.set_xticks(xvals)
    ax.set_xticklabels([str(v) for v in xvals])
    ax.grid(True)
    if legend:
        ax.legend(loc="best", fontsize=10)
    ax.yaxis.set_major_formatter(ticker.FormatStrFormatter("%.3g"))


def plot_single_phase1(df_p1, metric_col: str, ylabel: str, metric_key: str):
    y, yerr = get_stats(df_p1, XCOL, XVALS, metric_col)
    fig, ax = plt.subplots(figsize=(7, 4.5))
    ax.errorbar(
        XVALS,
        y,
        yerr=yerr,
        color=COLOR_P1,
        marker=MARKER_P1,
        capsize=4,
        elinewidth=1.2,
        label=LABEL_P1,
        zorder=3,
    )
    apply_common(ax, XLABEL, ylabel, f"{ylabel} vs {XLABEL} (Phase 1, mean±std)", XVALS)
    out = os.path.join(OUT_P1, f"vary_speed_{metric_key}.png")
    plt.tight_layout()
    plt.savefig(out)
    plt.close()
    print(f"  [SAVE ] {out}")


def plot_compare(df_p1, df_p2, metric_col: str, ylabel: str, metric_key: str):
    y1, y1err = get_stats(df_p1, XCOL, XVALS, metric_col)
    y2, y2err = get_stats(df_p2, XCOL, XVALS, metric_col)
    fig, ax = plt.subplots(figsize=(8, 4.8))
    ax.errorbar(
        XVALS,
        y1,
        yerr=y1err,
        color=COLOR_P1,
        marker=MARKER_P1,
        capsize=4,
        elinewidth=1.1,
        label=LABEL_P1,
        zorder=3,
    )
    ax.errorbar(
        XVALS,
        y2,
        yerr=y2err,
        color=COLOR_P2,
        marker=MARKER_P2,
        linestyle="--",
        capsize=4,
        elinewidth=1.1,
        label=LABEL_P2,
        zorder=3,
    )
    apply_common(
        ax,
        XLABEL,
        ylabel,
        f"{ylabel} vs {XLABEL} (Phase 1 vs Phase 2, mean±std)",
        XVALS,
    )
    out = os.path.join(OUT_CMP, f"vary_speed_{metric_key}_phase1_vs_phase2.png")
    plt.tight_layout()
    plt.savefig(out)
    plt.close()
    print(f"  [SAVE ] {out}")


def plot_compare_dashboard(df_p1, df_p2):
    fig, axes = plt.subplots(1, 5, figsize=(22, 4))
    fig.suptitle("FANET Speed Sweep: Phase 1 vs Phase 2", fontsize=12, fontweight="bold")

    for ax, (metric_col, ylabel, _) in zip(axes, METRICS):
        y1, y1err = get_stats(df_p1, XCOL, XVALS, metric_col)
        y2, y2err = get_stats(df_p2, XCOL, XVALS, metric_col)
        ax.errorbar(
            XVALS,
            y1,
            yerr=y1err,
            color=COLOR_P1,
            marker=MARKER_P1,
            linewidth=2,
            capsize=3,
            elinewidth=1,
            label="Phase 1",
        )
        ax.errorbar(
            XVALS,
            y2,
            yerr=y2err,
            color=COLOR_P2,
            marker=MARKER_P2,
            linewidth=2,
            linestyle="--",
            capsize=3,
            elinewidth=1,
            label="Phase 2",
        )
        ax.set_xlabel(XLABEL, fontsize=9)
        ax.set_ylabel(ylabel, fontsize=9)
        ax.set_title(ylabel, fontsize=9, fontweight="bold")
        ax.set_xticks(XVALS)
        ax.set_xticklabels([str(v) for v in XVALS], fontsize=8)
        ax.grid(True)
        ax.yaxis.set_major_formatter(ticker.FormatStrFormatter("%.3g"))
        ax.legend(fontsize=7, loc="best")

    out = os.path.join(OUT_OV, "fanet_phase1_vs_phase2_all_metrics.png")
    plt.tight_layout()
    plt.savefig(out)
    plt.close()
    print(f"  [SAVE ] {out}")


def summarize_outputs() -> None:
    all_pngs = []
    for root, _, files in os.walk("graphs"):
        for fname in files:
            if fname.endswith(".png") and ("fanet_phase1" in root or "fanet_compare" in root):
                all_pngs.append(os.path.join(root, fname))

    print("\n" + "=" * 60)
    print("  New FANET comparison plots complete")
    print(f"  Total newly generated PNGs (fanet_phase1/fanet_compare): {len(all_pngs)}")
    print("=" * 60)


def main() -> int:
    print("\n" + "=" * 60)
    print("  FANET Phase 1 vs Phase 2 Plot Generator")
    print("=" * 60)

    ensure_dirs()

    df_p1 = load_csv(CSV_P1, XCOL)
    df_p2 = load_csv(CSV_P2, XCOL)

    if df_p1 is None:
        print("  [ERROR] Phase 1 CSV is required for this standalone plot script")
        return 1

    if "run_id" in df_p1.columns:
        print(f"  [INFO ] Phase 1 unique runs: {int(df_p1['run_id'].nunique())}")
    else:
        print("  [INFO ] Phase 1 run_id column missing; treating each speed as a single run")

    if df_p2 is not None and "run_id" in df_p2.columns:
        print(f"  [INFO ] Phase 2 unique runs: {int(df_p2['run_id'].nunique())}")
    elif df_p2 is not None:
        print("  [INFO ] Phase 2 run_id column missing; treating each speed as a single run")

    for metric_col, ylabel, metric_key in METRICS:
        plot_single_phase1(df_p1, metric_col, ylabel, metric_key)

    if df_p2 is not None:
        for metric_col, ylabel, metric_key in METRICS:
            plot_compare(df_p1, df_p2, metric_col, ylabel, metric_key)
        plot_compare_dashboard(df_p1, df_p2)
    else:
        print("  [WARN ] Phase 2 CSV missing, generated only Phase 1 plots")

    summarize_outputs()
    return 0


if __name__ == "__main__":
    sys.exit(main())
