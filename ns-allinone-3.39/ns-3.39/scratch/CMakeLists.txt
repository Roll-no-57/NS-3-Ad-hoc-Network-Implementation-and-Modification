build_example(
  NAME test-ff-aodv-p2
























































































































ok "Phase 2 packet-size pipeline complete"info "Graph: ${SCRIPT_DIR}/graphs/overview/fanet_phase2_vary_packet_size_all_metrics.png"info "CSV: ${CSV_OUT}"section "DONE"python3 "${SCRIPT_DIR}/plot_fanet_phase2_packet_size.py"fi    die "plot_fanet_phase2_packet_size.py not found"if [[ ! -f "${SCRIPT_DIR}/plot_fanet_phase2_packet_size.py" ]]; thensection "STEP 2 - Generate packet-size graph"ok "CSV rows: ${rows} (expected: ${expected})"fi    die "Packet-size CSV is empty: ${CSV_OUT}"if [[ "${rows}" -eq 0 ]]; thenexpected="$(( ${#RUNS[@]} * ${#PKT_SIZES[@]} ))"rows="$(count_rows "${CSV_OUT}")"done    done                --outputFile="${CSV_OUT}"                --speed="${SPEED}" --speedSpread="${SPEED_SPREAD}" \                --pktPerSec="${PPS}" --packetSize="${psize}" \                --nNodes="${NODES}" --nFlows="${FLOWS}" \                --RngRun="${r}" --runId="${r}" \            "${BIN_PHASE2_PKT}" \        run_case "Phase2 run=${r} packetSize=${psize} bytes" \    for psize in "${PKT_SIZES[@]}"; dofor r in "${RUNS[@]}"; doecho "${HEADER}" > "${CSV_OUT}"mkdir -p "${RES_DIR}"section "STEP 1 - Run packet-size sweep (Phase 2, multi-seed)"ok "Binary ready"[[ -x "${BIN_PHASE2_PKT}" ]] || die "Binary not found after build: ${BIN_PHASE2_PKT}"fi    cmake --build cmake-cache --target scratch_sim-fanet-phase2-pktsize -j"$(nproc)"    info "Building packet-size FANET binary"    warn "Binary missing: ${BIN_PHASE2_PKT}"if [[ ! -x "${BIN_PHASE2_PKT}" ]]; thencd "${SCRIPT_DIR}" || die "Cannot cd to ${SCRIPT_DIR}"section "STEP 0 - Build checks"}    ok "Done: ${label}"    "$@"    echo -e "\n${CYAN}[RUN]${RESET} ${label}"    shift    local label="$1"run_case() {}    tail -n +2 "${csv_path}" | wc -l | tr -d ' '    fi        return        echo 0    if [[ ! -f "${csv_path}" ]]; then    local csv_path="$1"count_rows() {read -r -a RUNS <<< "${RUNS_STR}"read -r -a PKT_SIZES <<< "${PKT_SIZES_STR}"RUNS_STR="${RUNS_STR:-1 2 3 4 5}"PKT_SIZES_STR="${PKT_SIZES_STR:-128 256 512 768 1024}"SPEED_SPREAD="${SPEED_SPREAD:-0.6}"SPEED="${SPEED:-15}"PPS="${PPS:-20}"FLOWS="${FLOWS:-15}"NODES="${NODES:-60}"# Defaults (override via env if needed)HEADER="nNodes,nFlows,pktPerSec,packet_size_bytes,speed_mps,throughput_mbps,delay_ms,pdr_pct,pdrop_pct,energy_j,run_id"CSV_OUT="${RES_DIR}/vary_packet_size.csv"RES_DIR="${SCRIPT_DIR}/results/fanet_bonus"export LD_LIBRARY_PATH="${LIBDIR}${LD_LIBRARY_PATH:+:${LD_LIBRARY_PATH}}"LIBDIR="${SCRIPT_DIR}/build/lib"BIN_PHASE2_PKT="${BIN_DIR}/ns3.39-sim-fanet-phase2-pktsize-default"BIN_DIR="${SCRIPT_DIR}/build/examples"SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"}    echo -e "${BOLD}==========================================${RESET}"    echo -e "${BOLD}$*${RESET}"    echo -e "\n${BOLD}==========================================${RESET}"section() {die()     { echo -e "${RED}[ERROR]${RESET} $*" >&2; exit 1; }warn()    { echo -e "${YELLOW}[WARN]${RESET}  $*"; }ok()      { echo -e "${GREEN}[OK]${RESET}    $*"; }info()    { echo -e "${CYAN}[INFO]${RESET}  $*"; }RESET='\033[0m'BOLD='\033[1m'CYAN='\033[0;36m'YELLOW='\033[1;33m'GREEN='\033[0;32m'RED='\033[0;31m'set -euo pipefail# =============================================================================#   graphs/overview/fanet_phase2_vary_packet_size_all_metrics.png#   results/fanet_bonus/vary_packet_size.csv# Outputs:## This script is intentionally separate from speed-sweep scripts.# Standalone runner for FF-AODV Phase 2 packet-size variation in FANET.## run_fanet_phase2_packet_size.sh  SOURCE_FILES test-ff-aodv-p2.cc
  LIBRARIES_TO_LINK
    ${libcore}
    ${libnetwork}
    ${libinternet}
    ${libwifi}
    ${libmobility}
    ${libapplications}
    ${libenergy}
    ${libaodv}
    ${libflow-monitor}
    ${libnetanim}
)

build_example(
  NAME test-ff-aodv-energy-route-demo
  SOURCE_FILES test-ff-aodv-energy-route-demo.cc
  LIBRARIES_TO_LINK
    ${libcore}
    ${libnetwork}
    ${libinternet}
    ${libwifi}
    ${libmobility}
    ${libapplications}
    ${libenergy}
    ${libaodv}
    ${libnetanim}
)

build_example(
  NAME test-ff-aodv
  SOURCE_FILES test-ff-aodv.cc
  LIBRARIES_TO_LINK
    ${libcore}
    ${libnetwork}
    ${libinternet}
    ${libwifi}
    ${libmobility}
    ${libapplications}
    ${libenergy}
    ${libaodv}
    ${libflow-monitor}
)

build_example(
  NAME test-ff-aodv-v2
  SOURCE_FILES test-ff-aodv-v2.cc
  LIBRARIES_TO_LINK
    ${libcore}
    ${libnetwork}
    ${libinternet}
    ${libwifi}
    ${libmobility}
    ${libapplications}
    ${libenergy}
    ${libaodv}
    ${libflow-monitor}
)

build_example(
  NAME experiment-ff-aodv
  SOURCE_FILES experiment-ff-aodv.cc
  LIBRARIES_TO_LINK
    ${libcore}
    ${libnetwork}
    ${libinternet}
    ${libwifi}
    ${libmobility}
    ${libapplications}
    ${libenergy}
    ${libaodv}
    ${libflow-monitor}
)

build_example(
  NAME sim-802-11-static
  SOURCE_FILES sim-802-11-static.cc
  LIBRARIES_TO_LINK
    ${libcore}
    ${libnetwork}
    ${libinternet}
    ${libwifi}
    ${libmobility}
    ${libapplications}
    ${libenergy}
    ${libaodv}
    ${libflow-monitor}
)

build_example(
  NAME sim-802-15-4-static
  SOURCE_FILES sim-802-15-4-static.cc
  LIBRARIES_TO_LINK
    ${libcore}
    ${libnetwork}
    ${libinternet}
    ${libwifi}
    ${libmobility}
    ${libapplications}
    ${libenergy}
    ${libaodv}
    ${libflow-monitor}
)

build_example(
  NAME sim-fanet-bonus
  SOURCE_FILES sim-fanet-bonus.cc
  LIBRARIES_TO_LINK
    ${libcore}
    ${libnetwork}
    ${libinternet}
    ${libwifi}
    ${libmobility}
    ${libapplications}
    ${libenergy}
    ${libaodv}
    ${libflow-monitor}
)

build_example(
  NAME sim-fanet-phase1-bonus
  SOURCE_FILES sim-fanet-phase1-bonus.cc
  LIBRARIES_TO_LINK
    ${libcore}
    ${libnetwork}
    ${libinternet}
    ${libwifi}
    ${libmobility}
    ${libapplications}
    ${libenergy}
    ${libaodv}
    ${libflow-monitor}
)

build_example(
  NAME sim-fanet-phase2-pktsize
  SOURCE_FILES sim-fanet-phase2-pktsize.cc
  LIBRARIES_TO_LINK
    ${libcore}
    ${libnetwork}
    ${libinternet}
    ${libwifi}
    ${libmobility}
    ${libapplications}
    ${libenergy}
    ${libaodv}
    ${libflow-monitor}
)

build_example(
  NAME scratch-simulator
  SOURCE_FILES scratch-simulator.cc
  LIBRARIES_TO_LINK ${libcore}
)
