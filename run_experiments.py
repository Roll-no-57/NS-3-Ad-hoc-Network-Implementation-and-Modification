#!/usr/bin/env python3

import argparse
import csv
import importlib
import itertools
import statistics
import subprocess
import sys
from collections import defaultdict
from pathlib import Path


ROOT = Path(__file__).resolve().parent
NS3_DIR = ROOT / "ns-allinone-3.39" / "ns-3.39"
RESULTS_DIR = ROOT / "results"
PLOTS_DIR = RESULTS_DIR / "plots"

# Default fixed values used when a parameter is NOT being varied
DEFAULTS = {
    "n_nodes":            10,
    "n_flows":            30,
    "packets_per_second": 300,
    "speed_mps":          15,
}

# X-axis parameter definitions  (user number → internal key → display label)
ALL_PARAMS = [
    ("n_flows",            "Number of Flows"),        # 1
    ("n_nodes",            "Number of Nodes"),        # 2
    ("packets_per_second", "Packets per Second"),     # 3
    ("speed_mps",          "Node Speed (m/s)"),       # 4
]

# Full value ranges for each parameter
PARAM_RANGES = {
    "n_nodes":            [20, 40, 60, 80, 100],
    "n_flows":            [10, 20, 30, 40, 50],
    "packets_per_second": [100, 200, 300, 400, 500],
    "speed_mps":          [5, 10, 15, 20, 25],
}

PARAM_RANGES_QUICK = {
    "n_nodes":            [20, 40],
    "n_flows":            [10, 20],
    "packets_per_second": [100, 300],
    "speed_mps":          [5, 20],
}

# Y-axis metric definitions  (user number → csv key → display label)
ALL_METRICS = [
    ("avg_e2e_delay_ms",           "Average End-to-End Delay (ms)"),  # 1
    ("drop_ratio",                 "Packet Drop Ratio"),               # 2
    ("pdr",                        "Packet Delivery Ratio"),           # 3
    ("throughput_mbps",            "Network Throughput (Mbps)"),       # 4
    ("total_energy_consumption_j", "Total Energy Consumption (J)"),    # 5
]

# Order params are passed to the binary
PARAM_ORDER = ["n_nodes", "n_flows", "packets_per_second", "speed_mps"]


def find_experiment_binary():
    candidates = sorted((NS3_DIR / "build" / "examples").glob("ns3.39-experiment-ff-aodv-*"))
    if candidates:
        return candidates[0]
    fallback = NS3_DIR / "build" / "examples" / "ns3.39-experiment-ff-aodv-default"
    if fallback.exists():
        return fallback
    generic = sorted((NS3_DIR / "build" / "examples").glob("*experiment-ff-aodv*"))
    for candidate in generic:
        if candidate.is_file() and candidate.stat().st_mode & 0o111:
            return candidate
    return None


def run_cmd(cmd, cwd):
    print("[cmd]", " ".join(str(x) for x in cmd))
    subprocess.run(cmd, cwd=cwd, check=True)


def parse_selection(raw, total, label):
    """Parse comma-separated 1-based indices into a 0-based index list."""
    indices = []
    for token in raw.split(","):
        token = token.strip()
        if not token:
            continue
        try:
            n = int(token)
        except ValueError:
            raise SystemExit(f"Invalid {label} value '{token}'. Must be an integer.")
        if not 1 <= n <= total:
            raise SystemExit(f"{label} value {n} out of range (1–{total}).")
        indices.append(n - 1)
    return indices


def parse_args():
    parser = argparse.ArgumentParser(
        description="Run FF-AODV ns-3 experiments and generate graphs.",
        formatter_class=argparse.RawTextHelpFormatter,
    )
    parser.add_argument("--configure",   action="store_true", help="Run ./ns3 configure before build")
    parser.add_argument("--skip-build",  action="store_true", help="Skip ./ns3 build step")
    parser.add_argument("--plot-only",   action="store_true",
                        help="Skip all simulations; plot from the existing CSV only")
    parser.add_argument("--max-runs",    type=int,   default=0,     help="Limit simulation runs (0 = all)")
    parser.add_argument("--csv",         default=str(RESULTS_DIR / "ff_aodv_experiments.csv"),
                        help="CSV file path")
    parser.add_argument("--sim-time",    type=float, default=60.0,  help="Simulation time per run (s)")
    parser.add_argument("--packet-size", type=int,   default=256,   help="UDP packet size (bytes)")
    parser.add_argument("--tx-range",    type=float, default=120.0, help="Transmission range (m)")
    parser.add_argument("--coverage-scale", type=float, default=3.0,
                        help="Area side = coverage_scale × tx_range")
    parser.add_argument("--quick", action="store_true",
                        help="Use reduced value ranges for quick validation")
    parser.add_argument(
        "--metrics", type=str, default=None,
        help=(
            "Y-axis metrics to plot (comma-separated):\n"
            "  1 = End-to-end delay\n"
            "  2 = Drop ratio\n"
            "  3 = PDR\n"
            "  4 = Throughput\n"
            "  5 = Energy consumption\n"
            "Example: --metrics 4,3"
        ),
    )
    parser.add_argument(
        "--params", type=str, default=None,
        help=(
            "X-axis parameters to vary (comma-separated):\n"
            "  1 = Flows\n"
            "  2 = Nodes\n"
            "  3 = Packets per second\n"
            "  4 = Speed\n"
            "Only selected params are varied in simulations.\n"
            "Others are fixed at defaults, massively reducing run count.\n"
            "Example: --params 2,1"
        ),
    )
    return parser.parse_args()


def build_combinations(selected_param_keys, quick):
    """
    Build the MINIMAL set of simulation combinations needed.

    Strategy: for each selected param, vary it through its full range while
    holding all other params at their default values. Union all such sets.

    Example: --params 2,1  (nodes + flows)
      - Vary nodes  [20,40,60,80,100] with flows=30, pps=300, speed=15
      - Vary flows  [10,20,30,40,50]  with nodes=60, pps=300, speed=15
      → 10 unique combos instead of 625
    """
    ranges = PARAM_RANGES_QUICK if quick else PARAM_RANGES
    combo_set = set()

    for varied_key in selected_param_keys:
        for val in ranges[varied_key]:
            combo = tuple(
                val if key == varied_key else DEFAULTS[key]
                for key in PARAM_ORDER
            )
            combo_set.add(combo)

    return sorted(combo_set)


def load_rows(csv_path):
    with csv_path.open("r", newline="") as handle:
        reader = csv.DictReader(handle)
        return list(reader)


def aggregate_metric(rows, varied_param, metric_name):
    buckets = defaultdict(lambda: defaultdict(list))
    for row in rows:
        protocol = int(row["protocol_version"])
        x_val = float(row[varied_param])
        y_val = float(row[metric_name])
        buckets[protocol][x_val].append(y_val)

    merged = {}
    for protocol, by_x in buckets.items():
        points = sorted(
            [(x, statistics.mean(vals)) for x, vals in by_x.items()]
        )
        merged[protocol] = points
    return merged


def get_pyplot():
    try:
        return importlib.import_module("matplotlib.pyplot")
    except ImportError as exc:
        raise SystemExit("matplotlib is required: pip install matplotlib") from exc


def make_plots(rows, plot_dir, selected_metric_indices=None, selected_param_indices=None):
    plt = get_pyplot()
    plot_dir.mkdir(parents=True, exist_ok=True)

    varied_params = (
        [ALL_PARAMS[i] for i in selected_param_indices]
        if selected_param_indices is not None else ALL_PARAMS
    )
    metrics = (
        [ALL_METRICS[i] for i in selected_metric_indices]
        if selected_metric_indices is not None else ALL_METRICS
    )

    graphs_made = 0
    for metric_key, metric_label in metrics:
        for param_key, param_label in varied_params:
            series = aggregate_metric(rows, param_key, metric_key)
            if not series:
                continue

            plt.figure(figsize=(8, 5))
            for protocol, points in sorted(series.items()):
                xs = [x for x, _ in points]
                ys = [y for _, y in points]
                legend = "Phase 1 (Energy+Hop)" if protocol == 1 else "Phase 2 (Energy+Hop+Velocity)"
                plt.plot(xs, ys, marker="o", linewidth=1.8, label=legend)

            plt.title(f"{metric_label} vs {param_label}")
            plt.xlabel(param_label)
            plt.ylabel(metric_label)
            plt.grid(True, linestyle="--", alpha=0.35)
            plt.legend()
            plt.tight_layout()

            out_file = plot_dir / f"{metric_key}_vs_{param_key}.png"
            plt.savefig(out_file, dpi=160)
            plt.close()
            print("[plot]", out_file)
            graphs_made += 1

    print(f"[info] {graphs_made} graph(s) saved to {plot_dir}")


def main():
    args = parse_args()

    # Parse selections
    selected_metric_indices = (
        parse_selection(args.metrics, len(ALL_METRICS), "--metrics")
        if args.metrics else None
    )
    selected_param_indices = (
        parse_selection(args.params, len(ALL_PARAMS), "--params")
        if args.params else None
    )

    selected_param_keys = (
        [ALL_PARAMS[i][0] for i in selected_param_indices]
        if selected_param_indices is not None
        else [p[0] for p in ALL_PARAMS]
    )

    # Summary
    chosen_m = [ALL_METRICS[i][1] for i in selected_metric_indices] if selected_metric_indices else [m[1] for m in ALL_METRICS]
    chosen_p = [ALL_PARAMS[i][1]  for i in selected_param_indices]  if selected_param_indices  else [p[1] for p in ALL_PARAMS]
    print("[info] Metrics  :", chosen_m)
    print("[info] X-params :", chosen_p)

    if not NS3_DIR.exists():
        raise SystemExit(f"ns-3 directory not found: {NS3_DIR}")

    RESULTS_DIR.mkdir(parents=True, exist_ok=True)
    csv_path = Path(args.csv).resolve()
    csv_path.parent.mkdir(parents=True, exist_ok=True)

    # ── Simulation phase ──────────────────────────────────────────────── #
    if not args.plot_only:
        if csv_path.exists():
            csv_path.unlink()

        if args.configure:
            run_cmd(["./ns3", "configure"], cwd=NS3_DIR)
        if not args.skip_build:
            run_cmd(["./ns3", "build"], cwd=NS3_DIR)

        binary = find_experiment_binary()
        if binary is None:
            raise SystemExit(
                "Experiment binary not found. Build first and confirm the experiment "
                "target exists in scratch/CMakeLists.txt"
            )

        base_combos = build_combinations(selected_param_keys, args.quick)
        protocol_values = [1, 2]
        combinations = [
            (n_nodes, n_flows, pps, speed, proto)
            for (n_nodes, n_flows, pps, speed) in base_combos
            for proto in protocol_values
        ]

        if args.max_runs > 0:
            combinations = combinations[: args.max_runs]

        total_runs = len(combinations)
        full_grid  = len(PARAM_RANGES["n_nodes"]) * len(PARAM_RANGES["n_flows"]) * \
                     len(PARAM_RANGES["packets_per_second"]) * len(PARAM_RANGES["speed_mps"]) * 2
        print(f"[info] Runs needed : {total_runs}  (full grid = {full_grid})")

        for idx, (n_nodes, n_flows, pps, speed, protocol) in enumerate(combinations, start=1):
            print(
                f"[run {idx}/{total_runs}] protocol={protocol} nodes={n_nodes} "
                f"flows={n_flows} pps={pps} speed={speed}"
            )
            run_cmd(
                [
                    str(binary),
                    f"--protocolVersion={protocol}",
                    f"--nNodes={n_nodes}",
                    f"--nFlows={n_flows}",
                    f"--packetsPerSecond={pps}",
                    f"--speed={speed}",
                    f"--simTime={args.sim_time}",
                    f"--packetSize={args.packet_size}",
                    f"--txRange={args.tx_range}",
                    f"--coverageScale={args.coverage_scale}",
                    f"--runId={idx}",
                    f"--csvFile={csv_path}",
                ],
                cwd=NS3_DIR,
            )
    else:
        if not csv_path.exists():
            raise SystemExit(f"--plot-only specified but CSV not found: {csv_path}")
        print("[info] Skipping simulations (--plot-only). Reading:", csv_path)

    # ── Plotting phase ────────────────────────────────────────────────── #
    rows = load_rows(csv_path)
    if not rows:
        raise SystemExit("No rows found in CSV. Aborting plotting.")

    make_plots(rows, PLOTS_DIR,
               selected_metric_indices=selected_metric_indices,
               selected_param_indices=selected_param_indices)

    print("[done] CSV  :", csv_path)
    print("[done] Plots:", PLOTS_DIR)


if __name__ == "__main__":
    try:
        main()
    except subprocess.CalledProcessError as exc:
        print("[error] command failed:", exc, file=sys.stderr)
        sys.exit(exc.returncode)