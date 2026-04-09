#!/usr/bin/env python3
"""
plot_graphs.py — FF-AODV Simulation Report Graph Generator
===========================================================
Run from the ns-3.39/ directory after all simulations are complete:

    python3 plot_graphs.py

Generates:
  graphs/802_11/        — 20 graphs  (4 variations × 5 metrics, 802.11b only)
  graphs/802_15_4/      — 20 graphs  (4 variations × 5 metrics, 802.15.4 only)
  graphs/combined/      — 20 graphs  (802.11 vs 802.15.4 overlay, 4 × 5)
  graphs/overview/       — 8 figures  (all-5-metrics dashboard per variation × 2 nets)
  graphs/fanet/         —  5 graphs  (speed variation, FF-AODV Phase 2)

Total: 73 PNG files.

ROOT CAUSE FIX (vs previous version):
  Old code used `pd.read_csv(path, header=None, names=columns)` which treated
  the CSV header row as a *data row*, making every column object/string dtype.
  Filters like df[df["nNodes"] == 20] then always returned empty (str "20" != int 20),
  so every yval was 0 and all curves looked identical.

  Fix: use `pd.read_csv(path)` (header=0 by default) so pandas reads column names
  from the file's own header row, then force-convert to numeric.
"""

import os
import sys

import matplotlib

matplotlib.use("Agg")  # non-interactive backend — safe on headless machines
import matplotlib.pyplot as plt
import matplotlib.ticker as ticker
import numpy as np
import pandas as pd

# ═══════════════════════════════════════════════════════════════════════════
# 1. Style / colour / marker constants
# ═══════════════════════════════════════════════════════════════════════════
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

COLOR_80211 = "#1f77b4"  # blue
COLOR_802154 = "#e07b00"  # amber/orange
COLOR_FANET = "#2ca02c"  # green

MARKER_80211 = "o"
MARKER_802154 = "s"
MARKER_FANET = "^"

LABEL_80211 = "802.11b (Static, FF-AODV)"
LABEL_802154 = "802.15.4 emul. (Static, FF-AODV)"
LABEL_FANET = "FF-AODV FANET Phase 2 (Mobile)"

# ═══════════════════════════════════════════════════════════════════════════
# 2. Output directories
# ═══════════════════════════════════════════════════════════════════════════
for d in (
    "graphs/802_11",
    "graphs/802_15_4",
    "graphs/combined",
    "graphs/overview",
    "graphs/fanet",
):
    os.makedirs(d, exist_ok=True)

# ═══════════════════════════════════════════════════════════════════════════
# 3. Metric definitions
# ═══════════════════════════════════════════════════════════════════════════
# Each entry: (csv_column, y-axis label, short_key_for_filename)
METRICS = [
    ("throughput_mbps", "Network Throughput (Mbps)", "throughput"),
    ("delay_ms", "Avg. End-to-End Delay (ms)", "delay"),
    ("pdr_pct", "Packet Delivery Ratio (%)", "pdr"),
    ("pdrop_pct", "Packet Drop Ratio (%)", "pdrop"),
    ("energy_j", "Total Energy Consumption (J)", "energy"),
]

# ═══════════════════════════════════════════════════════════════════════════
# 4. Variation (X-axis) definitions
# ═══════════════════════════════════════════════════════════════════════════
# Each entry: vary_key -> (csv_x_column, x-axis label, expected x values)
VARY_CONFIG = {
    "vary_nodes": ("nNodes", "Number of Nodes", [20, 40, 60, 80, 100]),
    "vary_flows": ("nFlows", "Number of Flows", [10, 20, 30, 40, 50]),
    "vary_pps": ("pktPerSec", "Packets per Second", [100, 200, 300, 400, 500]),
    "vary_area": ("areaFactor", "Coverage Area (× Tx range)", [1, 2, 3, 4, 5]),
}
VARY_SPEED = ("speed_mps", "Node Speed (m/s)", [5, 10, 15, 20, 25])


# ═══════════════════════════════════════════════════════════════════════════
# 5. Helper: load CSV
# ═══════════════════════════════════════════════════════════════════════════
def load_csv(path: str, xcol: str) -> "pd.DataFrame | None":
    """
    Read a simulation CSV file and return a sorted DataFrame.

    Key fix: use default header=0 so pandas reads column names from the
    CSV's own first row.  Then force all columns to numeric so comparisons
    like df[df["nNodes"] == 20] work correctly even if pandas inferred
    a wider dtype.
    """
    if not os.path.exists(path):
        print(f"  [SKIP ] Not found : {path}")
        return None
    try:
        df = pd.read_csv(path)  # header=0 by default ← THE FIX
        # Force every column to numeric; non-convertible cells become NaN
        for col in df.columns:
            df[col] = pd.to_numeric(df[col], errors="coerce")
        df = df.dropna(subset=[xcol])  # drop rows where x is NaN
        df = df.sort_values(xcol).reset_index(drop=True)
        print(
            f"  [LOAD ] {path}  ({len(df)} rows)  "
            f"x=[{', '.join(str(v) for v in df[xcol].tolist())}]"
        )
        return df
    except Exception as exc:
        print(f"  [ERROR] {path}: {exc}")
        return None


# ═══════════════════════════════════════════════════════════════════════════
# 6. Helper: extract y-values for plotting
# ═══════════════════════════════════════════════════════════════════════════
def get_yvals(
    df: "pd.DataFrame | None", xcol: str, xvals: list, metric_col: str
) -> list:
    """
    For each x in xvals, return the mean of metric_col where xcol ≈ x.
    Returns NaN for any x that has no matching rows (shown as a gap in plot).
    """
    if df is None:
        return [float("nan")] * len(xvals)
    result = []
    for x in xvals:
        # Use near-equality to handle int/float mismatch (e.g. 1 vs 1.0)
        mask = (df[xcol] - float(x)).abs() < 1e-6
        subset = df.loc[mask, metric_col]
        result.append(float(subset.mean()) if not subset.empty else float("nan"))
    return result


# ═══════════════════════════════════════════════════════════════════════════
# 7. Core plot functions
# ═══════════════════════════════════════════════════════════════════════════
def _apply_common_formatting(
    ax, xlabel: str, ylabel: str, title: str, xvals: list, legend: bool = True
):
    ax.set_xlabel(xlabel)
    ax.set_ylabel(ylabel)
    ax.set_title(title, pad=8)
    ax.set_xticks(xvals)
    ax.set_xticklabels([str(v) for v in xvals])
    ax.grid(True)
    if legend:
        ax.legend(loc="best", fontsize=10)
    ax.yaxis.set_major_formatter(ticker.FormatStrFormatter("%.3g"))


def plot_single(
    df,
    xcol: str,
    xlabel: str,
    xvals: list,
    metric_col: str,
    ylabel: str,
    net_label: str,
    color: str,
    marker: str,
    out_path: str,
):
    """One curve, one metric, one network type."""
    yvals = get_yvals(df, xcol, xvals, metric_col)
    fig, ax = plt.subplots(figsize=(7, 4.5))
    ax.plot(xvals, yvals, color=color, marker=marker, label=net_label, zorder=3)
    # Mark NaN positions clearly
    nan_xs = [xvals[i] for i, v in enumerate(yvals) if np.isnan(v)]
    if nan_xs:
        ax.scatter(
            nan_xs,
            [0] * len(nan_xs),
            color="red",
            marker="x",
            s=80,
            zorder=4,
            label="No data",
        )
    _apply_common_formatting(ax, xlabel, ylabel, f"{ylabel}\nvs {xlabel}", xvals)
    plt.tight_layout()
    plt.savefig(out_path)
    plt.close()
    print(f"  [SAVE ] {out_path}")


def plot_comparison(
    df1,
    df2,
    xcol: str,
    xlabel: str,
    xvals: list,
    metric_col: str,
    ylabel: str,
    label1: str,
    label2: str,
    color1: str,
    color2: str,
    marker1: str,
    marker2: str,
    out_path: str,
):
    """Two curves on the same axes — 802.11 vs 802.15.4."""
    y1 = get_yvals(df1, xcol, xvals, metric_col)
    y2 = get_yvals(df2, xcol, xvals, metric_col)
    fig, ax = plt.subplots(figsize=(8, 4.8))
    ax.plot(xvals, y1, color=color1, marker=marker1, label=label1, zorder=3)
    ax.plot(
        xvals, y2, color=color2, marker=marker2, label=label2, linestyle="--", zorder=3
    )
    _apply_common_formatting(ax, xlabel, ylabel, f"{ylabel}\nvs {xlabel}", xvals)
    plt.tight_layout()
    plt.savefig(out_path)
    plt.close()
    print(f"  [SAVE ] {out_path}")


def plot_overview_dashboard(
    df,
    xcol: str,
    xlabel: str,
    xvals: list,
    net_label: str,
    color: str,
    marker: str,
    out_path: str,
):
    """
    5-subplot figure: all metrics for one (network, variation) combination.
    Useful for a quick visual overview in the report.
    """
    fig, axes = plt.subplots(1, 5, figsize=(22, 4))
    fig.suptitle(f"{net_label}\n(x-axis: {xlabel})", fontsize=13, fontweight="bold")
    for ax, (metric_col, ylabel, _) in zip(axes, METRICS):
        yvals = get_yvals(df, xcol, xvals, metric_col)
        ax.plot(xvals, yvals, color=color, marker=marker, linewidth=2)
        ax.set_xlabel(xlabel, fontsize=9)
        ax.set_ylabel(ylabel, fontsize=9)
        ax.set_title(ylabel, fontsize=9, fontweight="bold")
        ax.set_xticks(xvals)
        ax.set_xticklabels([str(v) for v in xvals], fontsize=8)
        ax.grid(True)
        ax.yaxis.set_major_formatter(ticker.FormatStrFormatter("%.3g"))
    plt.tight_layout()
    plt.savefig(out_path)
    plt.close()
    print(f"  [SAVE ] {out_path}")


def plot_comparison_dashboard(
    df1,
    df2,
    xcol: str,
    xlabel: str,
    xvals: list,
    label1: str,
    label2: str,
    color1: str,
    color2: str,
    marker1: str,
    marker2: str,
    out_path: str,
):
    """
    5-subplot comparison dashboard: both networks, all metrics,
    one variation parameter on the x-axis.
    """
    fig, axes = plt.subplots(1, 5, figsize=(22, 4))
    fig.suptitle(
        f"{label1}  vs  {label2}\n(x-axis: {xlabel})", fontsize=12, fontweight="bold"
    )
    for ax, (metric_col, ylabel, _) in zip(axes, METRICS):
        y1 = get_yvals(df1, xcol, xvals, metric_col)
        y2 = get_yvals(df2, xcol, xvals, metric_col)
        ax.plot(xvals, y1, color=color1, marker=marker1, linewidth=2, label=label1)
        ax.plot(
            xvals,
            y2,
            color=color2,
            marker=marker2,
            linewidth=2,
            linestyle="--",
            label=label2,
        )
        ax.set_xlabel(xlabel, fontsize=9)
        ax.set_ylabel(ylabel, fontsize=9)
        ax.set_title(ylabel, fontsize=9, fontweight="bold")
        ax.set_xticks(xvals)
        ax.set_xticklabels([str(v) for v in xvals], fontsize=8)
        ax.grid(True)
        ax.yaxis.set_major_formatter(ticker.FormatStrFormatter("%.3g"))
        ax.legend(fontsize=7, loc="best")
    plt.tight_layout()
    plt.savefig(out_path)
    plt.close()
    print(f"  [SAVE ] {out_path}")


# ═══════════════════════════════════════════════════════════════════════════
# 8. SECTION A — 802.11b Static  (individual: 4 × 5 = 20 graphs)
# ═══════════════════════════════════════════════════════════════════════════
print("\n" + "=" * 60)
print("  SECTION A — 802.11b Static Individual Graphs")
print("=" * 60)

for vary_key, (xcol, xlabel, xvals) in VARY_CONFIG.items():
    csv_path = f"results/802_11_static/{vary_key}.csv"
    df = load_csv(csv_path, xcol)
    if df is None:
        continue
    # 5 individual metric graphs
    for metric_col, ylabel, metric_key in METRICS:
        out = f"graphs/802_11/{vary_key}_{metric_key}.png"
        plot_single(
            df,
            xcol,
            xlabel,
            xvals,
            metric_col,
            ylabel,
            LABEL_80211,
            COLOR_80211,
            MARKER_80211,
            out,
        )
    # 1 overview dashboard (all 5 metrics in one figure)
    out_dash = f"graphs/overview/802_11_{vary_key}_all_metrics.png"
    plot_overview_dashboard(
        df, xcol, xlabel, xvals, LABEL_80211, COLOR_80211, MARKER_80211, out_dash
    )

# ═══════════════════════════════════════════════════════════════════════════
# 9. SECTION B — 802.15.4 Static  (individual: 4 × 5 = 20 graphs)
# ═══════════════════════════════════════════════════════════════════════════
print("\n" + "=" * 60)
print("  SECTION B — 802.15.4 Static Individual Graphs")
print("=" * 60)

for vary_key, (xcol, xlabel, xvals) in VARY_CONFIG.items():
    csv_path = f"results/802_15_4_static/{vary_key}.csv"
    df = load_csv(csv_path, xcol)
    if df is None:
        continue
    for metric_col, ylabel, metric_key in METRICS:
        out = f"graphs/802_15_4/{vary_key}_{metric_key}.png"
        plot_single(
            df,
            xcol,
            xlabel,
            xvals,
            metric_col,
            ylabel,
            LABEL_802154,
            COLOR_802154,
            MARKER_802154,
            out,
        )
    out_dash = f"graphs/overview/802_15_4_{vary_key}_all_metrics.png"
    plot_overview_dashboard(
        df, xcol, xlabel, xvals, LABEL_802154, COLOR_802154, MARKER_802154, out_dash
    )

# ═══════════════════════════════════════════════════════════════════════════
# 10. SECTION C — Combined 802.11 vs 802.15.4  (4 × 5 = 20 comparison graphs
#                                                + 4 comparison dashboards)
# ═══════════════════════════════════════════════════════════════════════════
print("\n" + "=" * 60)
print("  SECTION C — Combined 802.11 vs 802.15.4 Comparison Graphs")
print("=" * 60)

for vary_key, (xcol, xlabel, xvals) in VARY_CONFIG.items():
    df1 = load_csv(f"results/802_11_static/{vary_key}.csv", xcol)
    df2 = load_csv(f"results/802_15_4_static/{vary_key}.csv", xcol)
    # Individual comparison graphs (one per metric)
    for metric_col, ylabel, metric_key in METRICS:
        out = f"graphs/combined/{vary_key}_{metric_key}_compare.png"
        plot_comparison(
            df1,
            df2,
            xcol,
            xlabel,
            xvals,
            metric_col,
            ylabel,
            LABEL_80211,
            LABEL_802154,
            COLOR_80211,
            COLOR_802154,
            MARKER_80211,
            MARKER_802154,
            out,
        )
    # Comparison dashboard (all 5 metrics side-by-side)
    out_dash = f"graphs/overview/compare_{vary_key}_all_metrics.png"
    plot_comparison_dashboard(
        df1,
        df2,
        xcol,
        xlabel,
        xvals,
        LABEL_80211,
        LABEL_802154,
        COLOR_80211,
        COLOR_802154,
        MARKER_80211,
        MARKER_802154,
        out_dash,
    )

# ═══════════════════════════════════════════════════════════════════════════
# 11. SECTION D — BONUS: FF-AODV FANET Mobile  (5 graphs)
# ═══════════════════════════════════════════════════════════════════════════
print("\n" + "=" * 60)
print("  SECTION D — BONUS: FF-AODV FANET (Mobile, Phase 2)")
print("=" * 60)

xcol_f, xlabel_f, xvals_f = VARY_SPEED
df_fanet = load_csv("results/fanet_bonus/vary_speed.csv", xcol_f)

if df_fanet is not None:
    for metric_col, ylabel, metric_key in METRICS:
        out = f"graphs/fanet/vary_speed_{metric_key}.png"
        plot_single(
            df_fanet,
            xcol_f,
            xlabel_f,
            xvals_f,
            metric_col,
            ylabel,
            LABEL_FANET,
            COLOR_FANET,
            MARKER_FANET,
            out,
        )
    # Dashboard
    out_dash = "graphs/overview/fanet_vary_speed_all_metrics.png"
    plot_overview_dashboard(
        df_fanet,
        xcol_f,
        xlabel_f,
        xvals_f,
        LABEL_FANET,
        COLOR_FANET,
        MARKER_FANET,
        out_dash,
    )

# ═══════════════════════════════════════════════════════════════════════════
# 12. Summary
# ═══════════════════════════════════════════════════════════════════════════
print("\n" + "=" * 60)
all_pngs = []
for root, dirs, files in os.walk("graphs"):
    for f in files:
        if f.endswith(".png"):
            all_pngs.append(os.path.join(root, f))

counts = {}
for p in all_pngs:
    subdir = p.split(os.sep)[1]
    counts[subdir] = counts.get(subdir, 0) + 1

print("  Graph generation complete.\n")
for subdir, cnt in sorted(counts.items()):
    print(f"    graphs/{subdir}/  →  {cnt} PNG files")
print(f"\n  Total: {len(all_pngs)} graphs saved to graphs/")
print("=" * 60)
