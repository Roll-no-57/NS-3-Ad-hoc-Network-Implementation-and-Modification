# ns-allinone-3.39 — Complete Codebase Guide

> **Version**: ns-3.39  
> **Language**: C++ (with Python bindings)  
> **Build System**: CMake (wrapped by the `ns3` Python script)  
> **Purpose**: Discrete-event network simulator for research, education, and protocol development

---

## Table of Contents

1. [Top-Level Directory Structure](#1-top-level-directory-structure)
2. [How to Build & Run](#2-how-to-build--run)
3. [ns-3.39 Internal Layout](#3-ns-339-internal-layout)
4. [Module Architecture & Standard Layout](#4-module-architecture--standard-layout)
5. [All Modules — Grouped by Category](#5-all-modules--grouped-by-category)
   - [Core & Foundation](#51-core--foundation)
   - [Wired Link Models](#52-wired-link-models)
   - [Wireless Link Models](#53-wireless-link-models)
   - [Internet Protocols (TCP/IP)](#54-internet-protocols-tcpip)
   - [Routing Protocols](#55-routing-protocols)
   - [Applications & Traffic Generators](#56-applications--traffic-generators)
   - [Traffic Control & Queue Management](#57-traffic-control--queue-management)
   - [Propagation & Spectrum](#58-propagation--spectrum)
   - [Antenna & Buildings](#59-antenna--buildings)
   - [Mobility Models](#510-mobility-models)
   - [Energy Framework](#511-energy-framework)
   - [Statistics & Flow Monitoring](#512-statistics--flow-monitoring)
   - [Visualization & Animation](#513-visualization--animation)
   - [Emulation & Real-World Integration](#514-emulation--real-world-integration)
   - [Topology & Layout Helpers](#515-topology--layout-helpers)
   - [Configuration & Utilities](#516-configuration--utilities)
6. [Writing Your First Simulation](#6-writing-your-first-simulation)
7. [Scratch Directory — Where You Code](#7-scratch-directory--where-you-code)
8. [Tutorial Examples](#8-tutorial-examples)
9. [Key Concepts to Know](#9-key-concepts-to-know)
10. [Common Workflow](#10-common-workflow)
11. [Quick Reference — Useful Commands](#11-quick-reference--useful-commands)

---

## 1. Top-Level Directory Structure

```
ns-allinone-3.39/
├── build.py            # Top-level build orchestrator (builds ns-3 + NetAnim)
├── constants.py        # Build constants
├── util.py             # Build utilities
├── README.md           # Original minimal README
├── bake/               # Bake — optional package manager for ns-3 extensions
├── netanim-3.109/      # NetAnim — Qt-based GUI animator for simulation traces
└── ns-3.39/            # ★ The main ns-3 simulator (this is where everything happens)
```

| Component | What It Does |
|-----------|-------------|
| `build.py` | Orchestrates building the whole distribution: runs CMake for ns-3 and optionally qmake for NetAnim |
| `bake/` | A package manager that can fetch/build optional ns-3 modules (BRITE, Click, Openflow, etc.) |
| `netanim-3.109/` | Qt5-based GUI that reads XML animation files produced during simulation to visualize topology, packet flow, and node mobility |
| **`ns-3.39/`** | **The simulator itself — all modules, examples, build system, and your simulation code live here** |

---

## 2. How to Build & Run

### First-Time Build (from `ns-allinone-3.39/`)

```bash
# Option A: Use top-level build script
python3 build.py

# Option B: Use ns3 wrapper directly (recommended for daily use)
cd ns-3.39
./ns3 configure --enable-examples --enable-tests
./ns3 build
```

### Configure Options

```bash
./ns3 configure --enable-examples          # Build example programs
./ns3 configure --enable-tests             # Build test suites
./ns3 configure --enable-mpi               # Enable MPI for distributed simulation
./ns3 configure --enable-python-bindings   # Build Python bindings
./ns3 configure -d optimized               # Optimized build (faster execution)
./ns3 configure -d debug                   # Debug build (for development)
```

### Run a Simulation

```bash
cd ns-3.39

# Run a built-in example
./ns3 run "first"                          # Runs examples/tutorial/first.cc
./ns3 run "first --verbose"                # With command-line args

# Run your scratch simulation
./ns3 run "scratch-simulator"              # Runs scratch/scratch-simulator.cc
./ns3 run "scratch/my-simulation"          # Runs scratch/my-simulation.cc
```

### Run Tests

```bash
./ns3 run "test-runner --suite=aodv"       # Run a specific test suite
python3 test.py                            # Run the full test framework
```

---

## 3. ns-3.39 Internal Layout

```
ns-3.39/
├── ns3                  # ★ The main CLI tool (Python script wrapping CMake)
├── CMakeLists.txt       # Root CMake configuration
├── VERSION              # Contains "3.39"
│
├── src/                 # ★ All ns-3 modules (44 modules — see Section 5)
│   ├── core/            #   Simulation engine, object system, logging, tracing
│   ├── network/         #   Node, Packet, NetDevice, Channel abstractions
│   ├── internet/        #   Full TCP/IP stack (IPv4, IPv6, TCP, UDP, ARP, routing)
│   ├── wifi/            #   IEEE 802.11 (Wi-Fi 4/5/6/7)
│   ├── lte/             #   3GPP LTE (4G) with EPC
│   ├── point-to-point/  #   Simple dedicated wired links
│   ├── csma/            #   Ethernet-like shared medium
│   ├── applications/    #   Traffic generators and sinks
│   ├── flow-monitor/    #   Per-flow statistics collection
│   ├── mobility/        #   Node movement models
│   └── ...              #   (34 more modules — see full list below)
│
├── scratch/             # ★ YOUR simulation scripts go here
│   ├── CMakeLists.txt   #   Automatically discovers and builds .cc files
│   └── scratch-simulator.cc  # Minimal example scratch file
│
├── examples/            # Official example simulations organized by topic
│   ├── tutorial/        #   first.cc, second.cc, third.cc, etc.
│   ├── wireless/        #   Wi-Fi/mesh examples
│   ├── routing/         #   Routing protocol examples
│   ├── tcp/             #   TCP variant examples
│   └── ...              #   Many more categories
│
├── contrib/             # Your custom modules go here (like src/ but user-managed)
├── build/               # Build output (binaries, libraries)
├── bindings/            # Python binding definitions
├── doc/                 # Documentation sources
├── build-support/       # CMake helper scripts and macros
├── cmake-cache/        # CMake cached configuration
├── utils/               # Utility scripts
└── testpy-output/       # Test output directory
```

---

## 4. Module Architecture & Standard Layout

Every ns-3 module under `src/` follows a consistent structure:

```
src/module-name/
├── CMakeLists.txt       # Declares sources, headers, dependencies, tests, examples
├── model/               # ★ Core implementation (the actual protocol/model code)
│   ├── some-class.h     #   Header files
│   └── some-class.cc    #   Implementation files
├── helper/              # Convenience classes that simplify setting up simulations
│   ├── some-helper.h
│   └── some-helper.cc
├── doc/                 # RST documentation for the module
├── examples/            # Example simulations demonstrating the module
│   └── example.cc
└── test/                # Unit and integration tests
    └── some-test-suite.cc
```

**Key design pattern**: The `model/` directory contains the real implementation. The `helper/` directory provides user-friendly wrappers so you don't have to manually configure every low-level detail.

---

## 5. All Modules — Grouped by Category

### 5.1 Core & Foundation

#### `core/` — Simulation Engine & Object System
The bedrock of ns-3. Provides everything the simulator needs to function.

| Component | Key Files | Purpose |
|-----------|-----------|---------|
| Simulator | `simulator.h/cc`, `default-simulator-impl.cc` | Discrete-event scheduling engine |
| Object System | `object.h/cc`, `type-id.h/cc`, `ptr.h` | Smart pointers, object aggregation, runtime type info |
| Attributes | `attribute.h`, `config.h/cc` | Configurable parameters on all ns-3 objects |
| Tracing | `traced-callback.h`, `traced-value.h` | Output hooks to capture simulation data |
| Logging | `log.h/cc` | `NS_LOG_*` macros for debug output |
| Random Variables | `random-variable-stream.h/cc` | Uniform, Exponential, Normal, Pareto, etc. |
| Time | `nstime.h` | Nanosecond-precision simulation time |
| Command Line | `command-line.h/cc` | Parse CLI arguments in simulations |
| Callbacks | `callback.h` | Type-safe function pointer system |
| Scheduler | `scheduler.h/cc` | Event queue (heap, list, map, calendar) |

#### `network/` — Network Abstractions
Fundamental building blocks for all network models.

| Component | Key Files | Purpose |
|-----------|-----------|---------|
| Node | `node.h/cc` | A simulated computer/device |
| Packet | `packet.h/cc` | Simulated network packet with headers/trailers |
| NetDevice | `net-device.h/cc` | Network interface card abstraction |
| Channel | `channel.h/cc` | Communication medium connecting NetDevices |
| Socket | `socket.h/cc` | BSD-like socket API |
| Application | `application.h/cc` | Base class for traffic generation |
| Address | `address.h/cc` | Generic network address |
| Containers | `node-container.h`, `net-device-container.h` | Manage groups of objects |
| Helpers | `trace-helper.h/cc`, `packet-socket-helper.h/cc` | PCAP/ASCII trace output, socket setup |

---

### 5.2 Wired Link Models

#### `point-to-point/` — Dedicated Wired Links
The simplest link model — a direct connection between two nodes.
- **Key classes**: `PointToPointNetDevice`, `PointToPointChannel`, `PointToPointHelper`
- **Configurable**: `DataRate` (e.g., "5Mbps"), `Delay` (e.g., "2ms")
- **Use when**: You need a basic wired link with fixed bandwidth and delay

#### `csma/` — CSMA/CD (Ethernet-like)
Shared bus network model similar to classic Ethernet.
- **Key classes**: `CsmaNetDevice`, `CsmaChannel`, `CsmaHelper`
- **Configurable**: `DataRate`, `Delay`, backoff parameters
- **Use when**: You need a LAN/Ethernet segment with multiple nodes sharing a bus

#### `bridge/` — Learning Bridge (L2 Switch)
IEEE 802.1D-like Ethernet learning bridge.
- **Key classes**: `BridgeNetDevice`, `BridgeChannel`
- **Use when**: You need L2 switching behavior between CSMA segments

---

### 5.3 Wireless Link Models

#### `wifi/` — IEEE 802.11 (Wi-Fi)
Full-featured Wi-Fi implementation spanning multiple generations.

| Standard | ns-3 Name | Band |
|----------|-----------|------|
| 802.11a/b/g | Legacy | 2.4/5 GHz |
| 802.11n | HT (High Throughput) | 2.4/5 GHz |
| 802.11ac | VHT (Very High Throughput) | 5 GHz |
| 802.11ax | HE (High Efficiency / Wi-Fi 6) | 2.4/5/6 GHz |
| 802.11be | EHT (Extremely High Throughput / Wi-Fi 7) | 2.4/5/6 GHz |

- **Key classes**: `WifiNetDevice`, `WifiPhy`, `WifiMac`, `WifiHelper`, `YansWifiPhyHelper`, `SpectrumWifiPhyHelper`
- **PHY models**: YANS (simple) or Spectrum-based (frequency-selective)
- **MAC modes**: Ad-hoc, Infrastructure (AP + STA), Mesh
- **Rate adaptation**: AARF, ARF, Ideal, Minstrel, MinstrelHT, etc.
- **Features**: A-MPDU/A-MSDU aggregation, Block ACK, EDCA QoS, OFDMA, MU-MIMO

#### `lte/` — 3GPP LTE (4G)
Complete LTE system with EPC (core network).

| Layer | Key Classes |
|-------|-------------|
| PHY | `LteEnbPhy`, `LteUePhy`, `LteSpectrumPhy` |
| MAC | `LteEnbMac` + 11 schedulers (PF, RR, CQA, etc.) |
| RLC | `LteRlcAm`, `LteRlcUm`, `LteRlcTm` |
| PDCP | `LtePdcp` |
| RRC | `LteEnbRrc`, `LteUeRrc` |
| EPC | `EpcMmeApplication`, `EpcPgwApplication`, `EpcSgwApplication` |
| Helpers | `LteHelper`, `PointToPointEpcHelper`, `NoBackhaulEpcHelper` |

- **MAC Schedulers**: Proportional Fair (PF), Round Robin (RR), Channel-Aware (CQA), FDBET, TDBET, PSS, and more
- **Features**: Carrier aggregation, X2 handover, frequency reuse, REM (Radio Environment Map)

#### `wimax/` — IEEE 802.16 (WiMAX)
Broadband wireless access modeling with base station and subscriber stations.
- **Key classes**: `WimaxNetDevice`, `BsNetDevice`, `SsNetDevice`, `SimpleOfdmWimaxPhy`
- **Features**: Service flows, bandwidth management, scheduling

#### `lr-wpan/` — IEEE 802.15.4 (ZigBee/IoT)
Low-rate wireless PAN for IoT simulations.
- **Key classes**: `LrWpanMac`, `LrWpanPhy`, `LrWpanNetDevice`
- **Features**: CSMA/CA, GTS, beacon mode

#### `mesh/` — IEEE 802.11s Wireless Mesh
Mesh networking with HWMP and FLAME routing.
- **Key classes**: `MeshPointDevice`, `MeshWifiInterfaceMac`
- **Protocols**: HWMP (Hybrid Wireless Mesh Protocol), FLAME

#### `uan/` — Underwater Acoustic Networks
Models for underwater communication simulations.
- **Key classes**: `UanChannel`, `UanPhyGen`, `UanMacAloha`, `UanMacCw`, `UanMacRc`
- **Features**: Acoustic propagation, noise models, energy-aware modem models

---

### 5.4 Internet Protocols (TCP/IP)

#### `internet/` — Full TCP/IP Stack
Complete IPv4 and IPv6 protocol stack with extensive TCP congestion control.

**Protocol layers:**

| Layer | Protocols |
|-------|-----------|
| Layer 3 | IPv4, IPv6, ARP, ICMPv4, ICMPv6, NDP (Neighbor Discovery) |
| Layer 4 | TCP (15+ CC variants), UDP |
| Routing | Global routing, Static routing, RIP, RIPng |

**TCP Congestion Control Algorithms:**

| Algorithm | Class | Type |
|-----------|-------|------|
| NewReno | `TcpNewReno` | Loss-based (default) |
| Cubic | `TcpCubic` | Loss-based |
| BBR | `TcpBbr` | Model-based |
| Vegas | `TcpVegas` | Delay-based |
| Westwood+ | `TcpWestwoodPlus` | Bandwidth-estimated |
| DCTCP | `TcpDctcp` | ECN-based (data center) |
| Hybla | `TcpHybla` | Satellite/high-delay |
| HTCP | `TcpHtcp` | High-speed |
| Illinois | `TcpIllinois` | Hybrid |
| LEDBAT | `TcpLedbat` | Low-priority |
| LP | `TcpLp` | Low-priority |
| Scalable | `TcpScalable` | High-speed |
| BIC | `TcpBic` | Loss-based |
| Veno | `TcpVeno` | Wireless-optimized |
| Yeah | `TcpYeah` | Hybrid |

**Key helpers**: `InternetStackHelper`, `Ipv4AddressHelper`, `Ipv6AddressHelper`, `Ipv4GlobalRoutingHelper`

---

### 5.5 Routing Protocols

#### Ad-Hoc / MANET Routing

| Module | Protocol | Type | RFC |
|--------|----------|------|-----|
| `aodv/` | Ad hoc On-Demand Distance Vector | Reactive | RFC 3561 |
| `dsdv/` | Destination-Sequenced Distance Vector | Proactive | — |
| `dsr/` | Dynamic Source Routing | Reactive (source-routed) | RFC 4728 |
| `olsr/` | Optimized Link State Routing | Proactive (MPR-based) | RFC 3626 |

#### Infrastructure Routing

| Module | Protocol | Description |
|--------|----------|-------------|
| `internet/` (RIP) | RIP / RIPng | Distance-vector for IPv4/IPv6 |
| `internet/` (Global) | Global Routing | Dijkstra SPF computed from global topology knowledge |
| `internet/` (Static) | Static Routing | Manually configured routes |
| `nix-vector-routing/` | Nix-Vector | Efficient on-demand routing via bit-vector encoded paths |

---

### 5.6 Applications & Traffic Generators

#### `applications/` — Standard Applications

| Application | Class | Description |
|-------------|-------|-------------|
| UDP Echo | `UdpEchoClient/Server` | Simple request-response over UDP |
| On/Off | `OnOffApplication` | Generates traffic in on/off bursts (configurable distribution) |
| Bulk Send | `BulkSendApplication` | TCP bulk data transfer (saturates the link) |
| Packet Sink | `PacketSink` | Receives packets (pairs with On/Off or Bulk Send) |
| UDP Client/Server | `UdpClient/Server` | Unidirectional UDP traffic with sequence numbers |
| HTTP | `ThreeGppHttpClient/Server` | Realistic web browsing traffic model (3GPP) |

---

### 5.7 Traffic Control & Queue Management

#### `traffic-control/` — AQM & Queueing Disciplines

| Queue Disc | Class | Description |
|------------|-------|-------------|
| FIFO | `FifoQueueDisc` | Simple first-in-first-out |
| pfifo_fast | `PfifoFastQueueDisc` | Priority-based FIFO (Linux default) |
| RED | `RedQueueDisc` | Random Early Detection |
| CoDel | `CoDelQueueDisc` | Controlled Delay |
| FQ-CoDel | `FqCoDelQueueDisc` | Fair Queueing + CoDel (modern default) |
| PIE | `PieQueueDisc` | Proportional Integral controller Enhanced |
| COBALT | `CobaltQueueDisc` | Codel + BLUE |
| FQ-COBALT | `FqCobaltQueueDisc` | FQ + COBALT |
| FQ-PIE | `FqPieQueueDisc` | FQ + PIE |
| TBF | `TbfQueueDisc` | Token Bucket Filter (rate limiting) |
| Priority | `PrioQueueDisc` | Multi-band priority queueing |
| MQ | `MqQueueDisc` | Multi-queue (per interface queue) |

---

### 5.8 Propagation & Spectrum

#### `propagation/` — Radio Propagation Models

| Model | Description |
|-------|-------------|
| `FriisPropagationLossModel` | Free-space path loss |
| `LogDistancePropagationLossModel` | Log-distance (configurable exponent) |
| `ThreeLogDistancePropagationLossModel` | Three-segment log-distance |
| `NakagamiPropagationLossModel` | Nakagami-m fading |
| `Cost231PropagationLossModel` | COST 231 Hata (urban) |
| `OkumuraHataPropagationLossModel` | Okumura-Hata (outdoor large cells) |
| `ItuR1411PropagationLossModel` | ITU-R M.1411 (short-range outdoor) |
| `ThreeGppPropagationLossModel` | 3GPP TR 38.901 (UMa, UMi, RMa, InH) |
| `JakesPropagationLossModel` | Jakes Doppler fading |
| `ChannelConditionModel` | LOS/NLOS/NLOSv decision |

#### `spectrum/` — Spectrum-Aware Channel

Provides frequency-selective channel modeling for multi-technology coexistence.
- **Key classes**: `SpectrumChannel`, `MultiModelSpectrumChannel`, `SpectrumValue`, `SpectrumModel`
- **3GPP channel**: `ThreeGppChannelModel`, `ThreeGppSpectrumPropagationLossModel`
- **Use when**: You need accurate frequency-dependent propagation or multi-RAT coexistence

---

### 5.9 Antenna & Buildings

#### `antenna/` — Antenna Models

| Model | Description |
|-------|-------------|
| `IsotropicAntennaModel` | Omnidirectional (uniform in all directions) |
| `CosineAntennaModel` | Cosine-shaped radiation pattern |
| `ParabolicAntennaModel` | Parabolic radiation pattern |
| `ThreeGppAntennaModel` | 3GPP-standardized antenna element |
| `UniformPlanarArray` | Phased array for beamforming (MIMO) |

#### `buildings/` — Building Models

Physical building structures for realistic urban propagation.
- **Key classes**: `Building`, `BuildingList`, `MobilityBuildingInfo`
- **Propagation**: `HybridBuildingsPropagationLossModel`, `OhBuildingsPropagationLossModel`
- **Features**: Indoor/outdoor classification, floor/room assignment, building-aware channel conditions, V2V channel conditions

---

### 5.10 Mobility Models

#### `mobility/` — Node Movement

| Model | Class | Description |
|-------|-------|-------------|
| Constant Position | `ConstantPositionMobilityModel` | Node stays at fixed location |
| Constant Velocity | `ConstantVelocityMobilityModel` | Node moves at constant speed/direction |
| Random Walk 2D | `RandomWalk2dMobilityModel` | Brownian-motion-like random walk |
| Random Waypoint | `RandomWaypointMobilityModel` | Random destination, pause, repeat |
| Gauss-Markov | `GaussMarkovMobilityModel` | Temporally correlated movement |
| Hierarchical | `HierarchicalMobilityModel` | Combines reference-point + local mobility |
| Waypoint | `WaypointMobilityModel` | Follow predefined waypoints with timestamps |
| NS2 Trace | `Ns2MobilityHelper` | Import movement from NS-2 trace files |
| Group Mobility | `GroupMobilityHelper` | Coordinated group movement |

**Position Allocators**: `GridPositionAllocator`, `RandomRectanglePositionAllocator`, `RandomDiscPositionAllocator`, `UniformDiscPositionAllocator`

---

### 5.11 Energy Framework

#### `energy/` — Battery & Energy Consumption

| Component | Class | Description |
|-----------|-------|-------------|
| Energy Source | `BasicEnergySource` | Simple battery with linear depletion |
| Li-Ion Battery | `LiIonEnergySource` | Realistic Li-Ion voltage-capacity model |
| RV Battery | `RvBatteryModel` | Recovery-effect battery model |
| Device Energy | `DeviceEnergyModel` | Base class for device power consumption |
| Energy Harvester | `BasicEnergyHarvester` | Models energy harvesting (solar, etc.) |

---

### 5.12 Statistics & Flow Monitoring

#### `stats/` — Data Collection Framework

| Component | Description |
|-----------|-------------|
| Probes | `DoubleProbe`, `BooleanProbe`, etc. — tap into trace sources |
| Collectors | `TimeSeriesAdaptor` — transform data streams |
| Aggregators | `GnuplotAggregator`, `FileAggregator`, `SqliteOutput` — output results |
| Gnuplot | `Gnuplot`, `Gnuplot2dDataset` — generate plots |
| Histogram | `Histogram` — statistical distribution |

#### `flow-monitor/` — Per-Flow Statistics

Classifies packets into flows and measures key metrics:
- **Throughput** (bytes/time per flow)
- **End-to-end delay** (mean, min, max)
- **Jitter** (delay variation)
- **Packet loss ratio**
- Supports both **IPv4** and **IPv6** flow classification
- **Key classes**: `FlowMonitor`, `FlowMonitorHelper`, `Ipv4FlowClassifier`

---

### 5.13 Visualization & Animation

#### `netanim/` (in `src/`)  — Animation Interface
- **Class**: `AnimationInterface`
- Generates XML animation files during simulation
- XML consumed by the standalone NetAnim GUI (`netanim-3.109/`)

#### `netanim-3.109/` (top-level) — NetAnim GUI
- Qt5-based graphical application
- Visualizes node positions, packet flow, link utilization
- Shows mobility traces, counters, flow-monitor stats
- Build separately with qmake

#### `visualizer/` — Live Python Visualizer
- Real-time visualization of running simulations
- Uses PyGObject/GTK
- **Class**: `VisualSimulatorImpl`

---

### 5.14 Emulation & Real-World Integration

#### `tap-bridge/` — TAP Device Bridge
Connects ns-3 to real host network stack via Linux TAP devices.
- **Use when**: Running hybrid simulations where some nodes are real machines

#### `fd-net-device/` — File Descriptor Net Device
High-performance real I/O integration using raw sockets, DPDK, or Netmap.
- **Backends**: Raw socket, DPDK, Netmap

#### `virtual-net-device/` — Virtual TUN Device
For tunneling and VPN simulation (like Linux TUN).

---

### 5.15 Topology & Layout Helpers

#### `point-to-point-layout/` — P2P Topology Helpers

| Helper | Description |
|--------|-------------|
| `PointToPointDumbbellHelper` | Dumbbell topology (two LANs connected by a bottleneck) |
| `PointToPointGridHelper` | NxN grid topology |
| `PointToPointStarHelper` | Star topology with central hub |

#### `csma-layout/` — CSMA Topology Helpers
- `CsmaStarHelper` — Star topology with CSMA links

#### `topology-read/` — Read Topologies from Files
Read real-world ISP/research topologies:
- **Formats**: Inet, Orbis, Rocketfuel

---

### 5.16 Configuration & Utilities

#### `config-store/` — Configuration Serialization
Save/load the complete attribute configuration of a simulation.
- **Formats**: Raw text, XML, GTK GUI browser
- **Use when**: You want reproducible configurations or a GUI to explore attributes

#### `brite/`, `click/`, `openflow/` — Optional External Integrations
- **BRITE**: Generate realistic AS/router-level topologies
- **Click**: Integrate Click modular router
- **Openflow**: Software-defined networking with OpenFlow switch model

> These require fetching external dependencies — use `./ns3 configure --enable-fetch-optional-components`

---

## 6. Writing Your First Simulation

Every ns-3 simulation follows this pattern:

```cpp
#include "ns3/core-module.h"           // Always needed
#include "ns3/network-module.h"        // Nodes, packets, devices
#include "ns3/internet-module.h"       // TCP/IP stack
#include "ns3/point-to-point-module.h" // Point-to-point links
#include "ns3/applications-module.h"   // Traffic generators

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("MySimulation");   // Define a log component

int main(int argc, char* argv[])
{
    // (Optional) Parse command-line arguments
    CommandLine cmd(__FILE__);
    cmd.Parse(argc, argv);

    // 1. CREATE NODES
    NodeContainer nodes;
    nodes.Create(2);

    // 2. CREATE LINKS (channel + devices)
    PointToPointHelper p2p;
    p2p.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
    p2p.SetChannelAttribute("Delay", StringValue("2ms"));
    NetDeviceContainer devices = p2p.Install(nodes);

    // 3. INSTALL INTERNET STACK
    InternetStackHelper stack;
    stack.Install(nodes);

    // 4. ASSIGN IP ADDRESSES
    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces = address.Assign(devices);

    // 5. INSTALL APPLICATIONS
    UdpEchoServerHelper echoServer(9);
    ApplicationContainer serverApps = echoServer.Install(nodes.Get(1));
    serverApps.Start(Seconds(1.0));
    serverApps.Stop(Seconds(10.0));

    UdpEchoClientHelper echoClient(interfaces.GetAddress(1), 9);
    echoClient.SetAttribute("MaxPackets", UintegerValue(1));
    echoClient.SetAttribute("Interval", TimeValue(Seconds(1.0)));
    echoClient.SetAttribute("PacketSize", UintegerValue(1024));
    ApplicationContainer clientApps = echoClient.Install(nodes.Get(0));
    clientApps.Start(Seconds(2.0));
    clientApps.Stop(Seconds(10.0));

    // 6. RUN SIMULATION
    Simulator::Run();
    Simulator::Destroy();
    return 0;
}
```

---

## 7. Scratch Directory — Where You Code

Place your simulation files in `ns-3.39/scratch/`:

```
scratch/
├── CMakeLists.txt         # Auto-discovers .cc files
├── scratch-simulator.cc   # Built-in minimal example
├── my-simulation.cc       # ← Your single-file simulation
└── my-project/            # ← Your multi-file simulation
    ├── main.cc
    ├── my-header.h
    └── my-helper.cc
```

### Single File
Just drop a `.cc` file in `scratch/` — it's automatically discovered and built.

### Multi-File Project
Create a subdirectory, and register it in `scratch/CMakeLists.txt`:

```cmake
build_example(
  NAME my-project
  SOURCE_FILES my-project/main.cc
               my-project/my-helper.cc
  LIBRARIES_TO_LINK
    ${libcore}
    ${libnetwork}
    ${libinternet}
    ${libpoint-to-point}
    ${libapplications}
)
```

### Run Your Code

```bash
./ns3 run "my-simulation"                        # Single file
./ns3 run "my-simulation --arg1=value"            # With arguments
./ns3 run "my-project"                            # Multi-file
```

---

## 8. Tutorial Examples

Located in `ns-3.39/examples/tutorial/`:

| File | Topology | What It Demonstrates |
|------|----------|---------------------|
| `hello-simulator.cc` | None | Bare minimum: just prints and exits |
| `first.cc` | 2 nodes, P2P | UDP echo client/server on a point-to-point link |
| `second.cc` | P2P + CSMA bus | Point-to-point link connected to a CSMA LAN |
| `third.cc` | P2P + CSMA + Wi-Fi | Adds Wi-Fi with mobile nodes |
| `fourth.cc` | None | Low-level object and event API |
| `fifth.cc` | P2P | Custom application class with tracing |
| `sixth.cc` | P2P | Callback and trace connection examples |
| `seventh.cc` | P2P | Advanced tracing to file |

**Recommended learning order**: `hello-simulator` → `first` → `second` → `third` → `fifth`

---

## 9. Key Concepts to Know

### Helpers vs. Model Classes
- **Model classes** (`src/*/model/`): The actual protocol/device implementation. Low-level, complete control.
- **Helper classes** (`src/*/helper/`): Convenience wrappers. Use these in your simulations unless you need fine-grained control.

### Containers
Group objects for bulk operations:
- `NodeContainer` — group of nodes
- `NetDeviceContainer` — group of net devices
- `Ipv4InterfaceContainer` — group of IP interfaces
- `ApplicationContainer` — group of applications

### Attributes
Every ns-3 object exposes configurable parameters as **Attributes** (set via helpers or `Config::Set`):
```cpp
// Via helper
p2p.SetDeviceAttribute("DataRate", StringValue("10Mbps"));

// Via global config path
Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue(1448));
```

### Tracing
Two tracing mechanisms:
1. **PCAP traces**: `helper.EnablePcapAll("prefix")` — generates `.pcap` files (open with Wireshark)
2. **ASCII traces**: `helper.EnableAsciiAll(stream)` — human-readable event logs

### FlowMonitor
Measure per-flow performance:
```cpp
FlowMonitorHelper flowHelper;
Ptr<FlowMonitor> monitor = flowHelper.InstallAll();
// ... run simulation ...
monitor->SerializeToXmlFile("flowmon.xml", true, true);
```

### Logging
Enable debug output:
```cpp
LogComponentEnable("UdpEchoClientApplication", LOG_LEVEL_INFO);
```
Or from the shell:
```bash
NS_LOG="UdpEchoClientApplication=level_info" ./ns3 run "first"
```

---

## 10. Common Workflow

```
1. Write simulation         →  scratch/my-sim.cc
2. Configure build          →  ./ns3 configure --enable-examples
3. Build                    →  ./ns3 build
4. Run                      →  ./ns3 run "my-sim"
5. Collect traces           →  .pcap files, FlowMonitor XML, ASCII traces
6. Analyze results          →  Wireshark, Python/gnuplot scripts
7. Visualize (optional)     →  NetAnim (XML animation) or PyViz (live)
```

---

## 11. Quick Reference — Useful Commands

```bash
# Build
./ns3 configure --enable-examples --enable-tests
./ns3 build

# Run simulations
./ns3 run "first"
./ns3 run "scratch/my-sim"
./ns3 run "scratch/my-sim -- --arg1=val1"

# Run with logging
NS_LOG="*=level_all" ./ns3 run "first"            # All logs (very verbose)
NS_LOG="UdpEchoClientApplication=info" ./ns3 run "first"  # Specific component

# Run tests
./ns3 run "test-runner --suite=wifi"
python3 test.py -s wifi

# List available programs
./ns3 show targets

# Clean build
./ns3 clean

# Generate documentation
./ns3 docs doxygen       # API reference
./ns3 docs sphinx        # Tutorial/manual
```

---

## Module Dependency Quick Map

```
                        ┌──────────┐
                        │   core   │   ← Everything depends on this
                        └────┬─────┘
                             │
                        ┌────▼─────┐
                        │ network  │   ← Node, Packet, NetDevice
                        └────┬─────┘
                             │
              ┌──────────────┼──────────────┐
              │              │              │
        ┌─────▼────┐  ┌─────▼────┐  ┌──────▼──────┐
        │ internet │  │  wifi    │  │point-to-point│
        │ (TCP/IP) │  │ (802.11) │  │   (wired)    │
        └─────┬────┘  └──────────┘  └──────────────┘
              │
     ┌────────┼────────┐
     │        │        │
┌────▼───┐ ┌──▼──┐ ┌───▼────┐
│  apps  │ │ lte │ │traffic-│
│        │ │     │ │control │
└────────┘ └─────┘ └────────┘
```

---

## Contributing Custom Modules

Place custom modules in `contrib/`:
```
contrib/
└── my-module/
    ├── CMakeLists.txt
    ├── model/
    │   ├── my-model.h
    │   └── my-model.cc
    ├── helper/
    │   ├── my-helper.h
    │   └── my-helper.cc
    ├── examples/
    └── test/
```

They follow the same structure as `src/` modules and are automatically discovered by CMake.

---

> **Tip**: Start with the tutorial examples, understand the helper pattern, then build up to custom simulations in `scratch/`. Use `FlowMonitor` to measure performance and PCAP traces for packet-level debugging.
