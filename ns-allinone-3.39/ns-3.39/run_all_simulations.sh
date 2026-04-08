#!/usr/bin/env bash
# =============================================================================
#  run_all_simulations.sh
#
#  Full automated runner for the FF-AODV graph-generation experiments.
#
#  Run this script from the ns-3.39/ directory:
#      chmod +x run_all_simulations.sh
#      ./run_all_simulations.sh
#
#  What it does:
#    1. Verifies the three simulation binaries exist (rebuilds if missing).
#    2. Reinitialises all CSV files with the correct header row.
#    3. Runs all 802.11 static simulations  (4 parameter sweeps × 5 points).
#    4. Runs all 802.15.4 static simulations (4 parameter sweeps × 5 points).
#    5. Runs FANET bonus simulations         (1 parameter sweep  × 5 points).
#    6. Generates all PNG graphs with plot_graphs.py.
#
#  Estimated total wall-clock time: ~30–60 minutes depending on hardware.
# =============================================================================

set -euo pipefail

# ── Colour helpers ────────────────────────────────────────────────────────────
RED='\033[0;31m'; GREEN='\033[0;32m'; YELLOW='\033[1;33m'
CYAN='\033[0;36m'; BOLD='\033[1m'; RESET='\033[0m'

info()    { echo -e "${CYAN}[INFO]${RESET}  $*"; }
ok()      { echo -e "${GREEN}[OK]${RESET}    $*"; }
warn()    { echo -e "${YELLOW}[WARN]${RESET}  $*"; }
die()     { echo -e "${RED}[ERROR]${RESET} $*" >&2; exit 1; }
section() { echo -e "\n${BOLD}══════════════════════════════════════════${RESET}"; \
            echo -e "${BOLD}  $*${RESET}"; \
            echo -e "${BOLD}══════════════════════════════════════════${RESET}"; }

# ── Paths ─────────────────────────────────────────────────────────────────────
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BIN_DIR="${SCRIPT_DIR}/build/examples"
BIN_80211="${BIN_DIR}/ns3.39-sim-802-11-static-default"
BIN_802154="${BIN_DIR}/ns3.39-sim-802-15-4-static-default"
BIN_FANET="${BIN_DIR}/ns3.39-sim-fanet-bonus-default"
LIBDIR="${SCRIPT_DIR}/build/lib"

export LD_LIBRARY_PATH="${LIBDIR}${LD_LIBRARY_PATH:+:${LD_LIBRARY_PATH}}"

RES_80211="${SCRIPT_DIR}/results/802_11_static"
RES_802154="${SCRIPT_DIR}/results/802_15_4_static"
RES_FANET="${SCRIPT_DIR}/results/fanet_bonus"

HEADER_STATIC="nNodes,nFlows,pktPerSec,areaFactor,throughput_mbps,delay_ms,pdr_pct,pdrop_pct,energy_j"
HEADER_FANET="nNodes,nFlows,pktPerSec,speed_mps,throughput_mbps,delay_ms,pdr_pct,pdrop_pct,energy_j"

# ── Run counter ───────────────────────────────────────────────────────────────
TOTAL_RUNS=45   # 20 + 20 + 5
RUN_NUM=0

run() {
    # run <label> <binary> [args...]
    local label="$1"; shift
    RUN_NUM=$((RUN_NUM + 1))
    echo -e "\n${CYAN}[${RUN_NUM}/${TOTAL_RUNS}]${RESET} ${label}"
    "$@"
    ok "Done."
}

# =============================================================================
# STEP 0 — Sanity checks
# =============================================================================
section "STEP 0 — Sanity checks"

cd "${SCRIPT_DIR}" || die "Cannot cd to ${SCRIPT_DIR}"

for bin in "${BIN_80211}" "${BIN_802154}" "${BIN_FANET}"; do
    if [[ ! -x "${bin}" ]]; then
        warn "Binary not found: ${bin}"
        info "Attempting to build missing binaries …"
        cd "${SCRIPT_DIR}" && cmake --build cmake-cache \
            --target scratch_sim-802-11-static \
                    scratch_sim-802-15-4-static \
                    scratch_sim-fanet-bonus \
            -j"$(nproc)" 2>&1 | tail -6
        break
    fi
done

for bin in "${BIN_80211}" "${BIN_802154}" "${BIN_FANET}"; do
    [[ -x "${bin}" ]] || die "Binary still missing after build attempt: ${bin}"
done
ok "All binaries present."

# Check Python + matplotlib
python3 -c "import matplotlib, pandas" 2>/dev/null \
    || { warn "matplotlib/pandas not found — installing …"; pip3 install -q matplotlib pandas; }
ok "Python dependencies OK."

# =============================================================================
# STEP 1 — (Re-)initialise CSV files
# =============================================================================
section "STEP 1 — Initialising CSV files"

mkdir -p "${RES_80211}" "${RES_802154}" "${RES_FANET}"
mkdir -p "${SCRIPT_DIR}/graphs/802_11" \
         "${SCRIPT_DIR}/graphs/802_15_4" \
         "${SCRIPT_DIR}/graphs/fanet" \
         "${SCRIPT_DIR}/graphs/combined"

for f in vary_nodes vary_flows vary_pps vary_area; do
    echo "${HEADER_STATIC}" > "${RES_80211}/${f}.csv"
    echo "${HEADER_STATIC}" > "${RES_802154}/${f}.csv"
    info "Initialised ${RES_80211}/${f}.csv"
    info "Initialised ${RES_802154}/${f}.csv"
done

echo "${HEADER_FANET}" > "${RES_FANET}/vary_speed.csv"
info "Initialised ${RES_FANET}/vary_speed.csv"
ok "All CSV files initialised."

# =============================================================================
# STEP 2 — 802.11 Static simulations
# =============================================================================
section "STEP 2 — 802.11b Static simulations (20 runs)"

# Default fixed values: nNodes=40, nFlows=10, pktPerSec=100, areaFactor=1
# ── Vary NUMBER OF NODES (flows=10, pps=100, area=1) ───────────────────────
for n in 20 40 60 80 100; do
    run "802.11 | vary_nodes | nNodes=${n}" \
        "${BIN_80211}" \
            --nNodes="${n}" --nFlows=10 --pktPerSec=100 --areaFactor=1 \
            --outputFile="${RES_80211}/vary_nodes.csv"
done

# ── Vary NUMBER OF FLOWS (nodes=40, pps=100, area=1) ──────────────────────
for f in 10 20 30 40 50; do
    run "802.11 | vary_flows | nFlows=${f}" \
        "${BIN_80211}" \
            --nNodes=40 --nFlows="${f}" --pktPerSec=100 --areaFactor=1 \
            --outputFile="${RES_80211}/vary_flows.csv"
done

# ── Vary PACKETS PER SECOND (nodes=40, flows=10, area=1) ─────────────────
for p in 100 200 300 400 500; do
    run "802.11 | vary_pps | pktPerSec=${p}" \
        "${BIN_80211}" \
            --nNodes=40 --nFlows=10 --pktPerSec="${p}" --areaFactor=1 \
            --outputFile="${RES_80211}/vary_pps.csv"
done

# ── Vary COVERAGE AREA (nodes=40, flows=10, pps=100) ─────────────────────
for a in 1 2 3 4 5; do
    run "802.11 | vary_area | areaFactor=${a}" \
        "${BIN_80211}" \
            --nNodes=40 --nFlows=10 --pktPerSec=100 --areaFactor="${a}" \
            --outputFile="${RES_80211}/vary_area.csv"
done

ok "802.11 static simulations complete."

# =============================================================================
# STEP 3 — 802.15.4 Static simulations
# =============================================================================
section "STEP 3 — 802.15.4 Static simulations (20 runs)"

# ── Vary NUMBER OF NODES ───────────────────────────────────────────────────
for n in 20 40 60 80 100; do
    run "802.15.4 | vary_nodes | nNodes=${n}" \
        "${BIN_802154}" \
            --nNodes="${n}" --nFlows=10 --pktPerSec=100 --areaFactor=1 \
            --outputFile="${RES_802154}/vary_nodes.csv"
done

# ── Vary NUMBER OF FLOWS ──────────────────────────────────────────────────
for f in 10 20 30 40 50; do
    run "802.15.4 | vary_flows | nFlows=${f}" \
        "${BIN_802154}" \
            --nNodes=40 --nFlows="${f}" --pktPerSec=100 --areaFactor=1 \
            --outputFile="${RES_802154}/vary_flows.csv"
done

# ── Vary PACKETS PER SECOND ───────────────────────────────────────────────
for p in 100 200 300 400 500; do
    run "802.15.4 | vary_pps | pktPerSec=${p}" \
        "${BIN_802154}" \
            --nNodes=40 --nFlows=10 --pktPerSec="${p}" --areaFactor=1 \
            --outputFile="${RES_802154}/vary_pps.csv"
done

# ── Vary COVERAGE AREA ────────────────────────────────────────────────────
for a in 1 2 3 4 5; do
    run "802.15.4 | vary_area | areaFactor=${a}" \
        "${BIN_802154}" \
            --nNodes=40 --nFlows=10 --pktPerSec=100 --areaFactor="${a}" \
            --outputFile="${RES_802154}/vary_area.csv"
done

ok "802.15.4 static simulations complete."

# =============================================================================
# STEP 4 — FANET Bonus simulations (speed variation)
# =============================================================================
section "STEP 4 — BONUS: FF-AODV FANET simulations (5 runs)"

# ── Vary NODE SPEED (nNodes=40, nFlows=10, pktPerSec=100) ────────────────
for s in 5 10 15 20 25; do
    run "FANET | vary_speed | speed=${s} m/s" \
        "${BIN_FANET}" \
            --nNodes=40 --nFlows=10 --pktPerSec=100 --speed="${s}" \
            --outputFile="${RES_FANET}/vary_speed.csv"
done

ok "FANET bonus simulations complete."

# =============================================================================
# STEP 5 — Verify data was written
# =============================================================================
section "STEP 5 — Verifying CSV output"

all_ok=true
for csv in \
    "${RES_80211}/vary_nodes.csv" \
    "${RES_80211}/vary_flows.csv" \
    "${RES_80211}/vary_pps.csv" \
    "${RES_80211}/vary_area.csv" \
    "${RES_802154}/vary_nodes.csv" \
    "${RES_802154}/vary_flows.csv" \
    "${RES_802154}/vary_pps.csv" \
    "${RES_802154}/vary_area.csv" \
    "${RES_FANET}/vary_speed.csv"
do
    rows=$(tail -n +2 "${csv}" | wc -l)
    if [[ "${rows}" -eq 0 ]]; then
        warn "${csv} — 0 data rows (header only)!"
        all_ok=false
    else
        ok "${csv} — ${rows} data row(s)"
    fi
done

if ! "${all_ok}"; then
    warn "Some CSV files are empty. Simulations may not have run correctly."
    warn "Check the output above for error messages."
fi

# =============================================================================
# STEP 6 — Generate graphs
# =============================================================================
section "STEP 6 — Generating graphs with plot_graphs.py"

if [[ ! -f "${SCRIPT_DIR}/plot_graphs.py" ]]; then
    die "plot_graphs.py not found in ${SCRIPT_DIR}"
fi

cd "${SCRIPT_DIR}"
python3 plot_graphs.py

# =============================================================================
# STEP 7 — Summary
# =============================================================================
section "STEP 7 — Summary"

echo ""
info "Simulation results:"
echo "  ${RES_80211}/"
echo "  ${RES_802154}/"
echo "  ${RES_FANET}/"
echo ""
info "Generated graphs:"
echo "  ${SCRIPT_DIR}/graphs/802_11/     (20 graphs — 802.11b static)"
echo "  ${SCRIPT_DIR}/graphs/802_15_4/   (20 graphs — 802.15.4 emulation static)"
echo "  ${SCRIPT_DIR}/graphs/combined/   (20 graphs — 802.11 vs 802.15.4 overlays)"
echo "  ${SCRIPT_DIR}/graphs/fanet/      ( 5 graphs — FF-AODV FANET bonus)"
echo ""
echo -e "${GREEN}${BOLD}All done! Total graphs: 65 (45 mandatory + 20 combined comparison)${RESET}"
