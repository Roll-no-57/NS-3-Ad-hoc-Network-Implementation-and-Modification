#!/usr/bin/env bash
# =============================================================================
# run_fanet_phase_comparison.sh
#
# Standalone runner for new stats only:
#   1) Regenerates FANET speed-sweep results for FF-AODV Phase 1 baseline.
#   2) Regenerates FANET speed-sweep results for FF-AODV Phase 2 variant.
#   3) Runs multiple RNG seeds for robust mean/std comparison.
#   4) Produces dedicated Phase 1 and Phase 1-vs-Phase 2 comparison plots.
#
# Usage:
#   chmod +x run_fanet_phase_comparison.sh
#   ./run_fanet_phase_comparison.sh
# =============================================================================

set -euo pipefail

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
BOLD='\033[1m'
RESET='\033[0m'

info()    { echo -e "${CYAN}[INFO]${RESET}  $*"; }
ok()      { echo -e "${GREEN}[OK]${RESET}    $*"; }
warn()    { echo -e "${YELLOW}[WARN]${RESET}  $*"; }
die()     { echo -e "${RED}[ERROR]${RESET} $*" >&2; exit 1; }
section() {
    echo -e "\n${BOLD}==========================================${RESET}"
    echo -e "${BOLD}$*${RESET}"
    echo -e "${BOLD}==========================================${RESET}"
}

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BIN_DIR="${SCRIPT_DIR}/build/examples"
BIN_PHASE1="${BIN_DIR}/ns3.39-sim-fanet-phase1-bonus-default"
BIN_PHASE2="${BIN_DIR}/ns3.39-sim-fanet-bonus-default"
LIBDIR="${SCRIPT_DIR}/build/lib"
PYTHON_BIN="${PYTHON_BIN:-/usr/bin/python3}"

if [[ -x "${SCRIPT_DIR}/.venv/bin/python" ]]; then
    PYTHON_BIN="${SCRIPT_DIR}/.venv/bin/python"
fi

export LD_LIBRARY_PATH="${LIBDIR}${LD_LIBRARY_PATH:+:${LD_LIBRARY_PATH}}"

RES_PHASE1="${SCRIPT_DIR}/results/fanet_phase1"
RES_PHASE2="${SCRIPT_DIR}/results/fanet_bonus"
CSV_PHASE1="${RES_PHASE1}/vary_speed.csv"
CSV_PHASE2="${RES_PHASE2}/vary_speed.csv"

HEADER="nNodes,nFlows,pktPerSec,speed_mps,throughput_mbps,delay_ms,pdr_pct,pdrop_pct,energy_j,run_id"

# These defaults are chosen to avoid heavy saturation where both phases collapse
# to nearly identical behavior.
NODES="${NODES:-60}"
FLOWS="${FLOWS:-15}"
PPS="${PPS:-20}"
SPEED_SPREAD="${SPEED_SPREAD:-0.6}"

SPEEDS_STR="${SPEEDS_STR:-5 10 15 20 25}"
RUNS_STR="${RUNS_STR:-1 2 3 4 5}"
read -r -a SPEEDS <<< "${SPEEDS_STR}"
read -r -a RUNS <<< "${RUNS_STR}"

run_case() {
    local label="$1"
    shift
    echo -e "\n${CYAN}[RUN]${RESET} ${label}"
    "$@"
    ok "Done: ${label}"
}

count_rows() {
    local csv_path="$1"
    if [[ ! -f "${csv_path}" ]]; then
        echo 0
        return
    fi
    tail -n +2 "${csv_path}" | wc -l | tr -d ' '
}

section "STEP 0 - Build checks"

cd "${SCRIPT_DIR}" || die "Cannot cd to ${SCRIPT_DIR}"

for bin in "${BIN_PHASE1}" "${BIN_PHASE2}"; do
    if [[ ! -x "${bin}" ]]; then
        warn "Binary missing: ${bin}"
        info "Building FANET binaries"
        cmake --build cmake-cache \
            --target scratch_sim-fanet-phase1-bonus scratch_sim-fanet-bonus \
            -j"$(nproc)"
        break
    fi
done

[[ -x "${BIN_PHASE1}" ]] || die "Phase 1 binary not found after build: ${BIN_PHASE1}"
[[ -x "${BIN_PHASE2}" ]] || die "Phase 2 binary not found after build: ${BIN_PHASE2}"
ok "Required binaries are ready in build/examples"

section "STEP 1 - Run Phase 1 speed sweep (multi-seed)"

mkdir -p "${RES_PHASE1}"
echo "${HEADER}" > "${CSV_PHASE1}"

for r in "${RUNS[@]}"; do
    for s in "${SPEEDS[@]}"; do
        run_case "Phase1 run=${r} speed=${s} m/s" \
            "${BIN_PHASE1}" \
                --RngRun="${r}" --runId="${r}" \
                --nNodes="${NODES}" --nFlows="${FLOWS}" --pktPerSec="${PPS}" \
                --speed="${s}" --speedSpread="${SPEED_SPREAD}" --outputFile="${CSV_PHASE1}"
    done
done

rows_p1="$(count_rows "${CSV_PHASE1}")"
if [[ "${rows_p1}" -eq 0 ]]; then
    die "Phase 1 CSV is empty: ${CSV_PHASE1}"
fi
ok "Phase 1 CSV rows: ${rows_p1} (expected: $((${#RUNS[@]} * ${#SPEEDS[@]})))"

section "STEP 2 - Run Phase 2 speed sweep (multi-seed)"

mkdir -p "${RES_PHASE2}"
echo "${HEADER}" > "${CSV_PHASE2}"

for r in "${RUNS[@]}"; do
    for s in "${SPEEDS[@]}"; do
        run_case "Phase2 run=${r} speed=${s} m/s" \
            "${BIN_PHASE2}" \
                --RngRun="${r}" --runId="${r}" \
                --nNodes="${NODES}" --nFlows="${FLOWS}" --pktPerSec="${PPS}" \
                --speed="${s}" --speedSpread="${SPEED_SPREAD}" --outputFile="${CSV_PHASE2}"
    done
done

rows_p2="$(count_rows "${CSV_PHASE2}")"
[[ "${rows_p2}" -gt 0 ]] || die "Phase 2 CSV is empty after rerun: ${CSV_PHASE2}"
ok "Phase 2 CSV rows: ${rows_p2} (expected: $((${#RUNS[@]} * ${#SPEEDS[@]})))"

section "STEP 3 - Plot new comparison stats"

if ! "${PYTHON_BIN}" -c "import matplotlib, pandas, numpy" >/dev/null 2>&1; then
    die "Missing plotting deps for ${PYTHON_BIN}. Install with: ${PYTHON_BIN} -m pip install matplotlib pandas numpy"
fi

"${PYTHON_BIN}" "${SCRIPT_DIR}/plot_fanet_phase_comparison.py"

section "DONE"

info "Input CSV files:"
echo "  ${CSV_PHASE1}"
echo "  ${CSV_PHASE2}"
info "Run configuration:"
echo "  runs=${RUNS[*]}"
echo "  speeds=${SPEEDS[*]}"
echo "  speedSpread=${SPEED_SPREAD}"

info "Generated graph folders:"
echo "  ${SCRIPT_DIR}/graphs/fanet_phase1"
echo "  ${SCRIPT_DIR}/graphs/fanet_compare"
echo "  ${SCRIPT_DIR}/graphs/overview"

ok "Standalone FANET comparison pipeline complete"
