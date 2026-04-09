# Graph Generation Instructions for FF-AODV Simulation Report
## CSE 322 — Computer Networks Sessional | ns-3.39

---

## Overview

This document provides complete, step-by-step instructions to generate all mandatory and bonus graphs for your simulation report. You will generate graphs for:

- **Mandatory Part A (Phase 1 / Base Paper):** Wireless 802.11 (static nodes)
- **Mandatory Part B (Phase 1 / Base Paper):** Wireless 802.15.4 (static nodes)
- **Bonus (Phase 2 / Velocity-Aware Modification):** Mobile FANET with moving nodes (802.11a ad-hoc)

Total mandatory graphs: **40** (20 per network × 2 networks)
Total bonus graphs: **5** (velocity variation for mobile FANET)
Optional comparison graphs: **20** (802.11 vs 802.15.4 overlays)

> **Important correction (based on your phase design):**
> - For mandatory 802.11 and 802.15.4 static experiments, use **base-paper FF-AODV only**.
> - Do **not** evaluate the velocity-aware term in mandatory static graphs.
> - Use the velocity-aware modification only in the **bonus mobile** scenario.

---

## Prerequisites

### Software Requirements

```bash
# Verify ns-3.39 is built
cd ns-allinone-3.39/ns-3.39
./ns3 --version   # Should print ns-3.39

# Install Python dependencies for plotting
pip install matplotlib numpy pandas
```

### Directory Setup

```bash
# Create output directories
mkdir -p results/802_11_static
mkdir -p results/802_15_4_static
mkdir -p results/fanet_bonus
mkdir -p graphs/802_11
mkdir -p graphs/802_15_4
mkdir -p graphs/fanet
```

---

## Part 1: Simulation Scripts

### 1.1 — 802.11 Static Simulation Script

Save this as `scratch/sim-802-11-static.cc`:

```cpp
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/aodv-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/energy-module.h"

using namespace ns3;

int main(int argc, char *argv[]) {
    // ── Tunable parameters (set via command line) ──────────────────────────
    uint32_t nNodes     = 20;
    uint32_t nFlows     = 10;
    uint32_t pktPerSec  = 100;
    double   txRange    = 100.0;   // base Tx range in meters
    double   areaFactor = 1.0;     // coverage multiplier (1–5)
    double   simTime    = 60.0;
    std::string outputFile = "results/802_11_static/output.csv";

    CommandLine cmd;
    cmd.AddValue("nNodes",     "Number of nodes",            nNodes);
    cmd.AddValue("nFlows",     "Number of UDP flows",        nFlows);
    cmd.AddValue("pktPerSec",  "Packets per second",         pktPerSec);
    cmd.AddValue("areaFactor", "Coverage area multiplier",   areaFactor);
    cmd.AddValue("outputFile", "CSV output file path",       outputFile);
    cmd.Parse(argc, argv);

    double sideLength = areaFactor * txRange;

    // ── Node creation ──────────────────────────────────────────────────────
    NodeContainer nodes;
    nodes.Create(nNodes);

    // ── Wi-Fi 802.11b ad-hoc ───────────────────────────────────────────────
    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211b);
    wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager",
                                 "DataMode", StringValue("DsssRate1Mbps"),
                                 "ControlMode", StringValue("DsssRate1Mbps"));

    YansWifiPhyHelper phy;
    YansWifiChannelHelper channel = YansWifiChannelHelper::Default();
    phy.SetChannel(channel.Create());
    phy.Set("TxPowerStart", DoubleValue(16.0206));
    phy.Set("TxPowerEnd",   DoubleValue(16.0206));

    WifiMacHelper mac;
    mac.SetType("ns3::AdhocWifiMac");

    NetDeviceContainer devices = wifi.Install(phy, mac, nodes);

    // ── Energy model ───────────────────────────────────────────────────────
    BasicEnergySourceHelper energyHelper;
    energyHelper.Set("BasicEnergySourceInitialEnergyJ", DoubleValue(100.0));
    EnergySourceContainer sources = energyHelper.Install(nodes);

    WifiRadioEnergyModelHelper radioEnergyHelper;
    DeviceEnergyModelContainer deviceModels =
        radioEnergyHelper.Install(devices, sources);

    // ── Static grid mobility ───────────────────────────────────────────────
    MobilityHelper mobility;
    mobility.SetPositionAllocator("ns3::RandomRectanglePositionAllocator",
                                  "X", StringValue("ns3::UniformRandomVariable[Min=0|Max=" +
                                                   std::to_string(sideLength) + "]"),
                                  "Y", StringValue("ns3::UniformRandomVariable[Min=0|Max=" +
                                                   std::to_string(sideLength) + "]"));
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(nodes);

    // ── Routing: FF-AODV Phase 1 (base paper only) ───────────────────────
    AodvHelper aodv;
    aodv.Set("Alpha",         DoubleValue(0.6));
    aodv.Set("Beta",          DoubleValue(0.4));
    aodv.Set("Gamma",         DoubleValue(0.0)); // disable velocity-aware term
    aodv.Set("InitialEnergy", DoubleValue(100.0));
    aodv.Set("MaxVelocity",   DoubleValue(50.0));
    InternetStackHelper internet;
    internet.SetRoutingHelper(aodv);
    internet.Install(nodes);

    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces = address.Assign(devices);

    // ── UDP traffic flows ──────────────────────────────────────────────────
    uint16_t port = 9;
    uint32_t pktSize = 512;
    double dataRate = pktPerSec * pktSize * 8;  // bps

    ApplicationContainer serverApps, clientApps;
    uint32_t actualFlows = std::min(nFlows, nNodes / 2);

    for (uint32_t i = 0; i < actualFlows; i++) {
        uint32_t src  = i;
        uint32_t dest = nNodes - 1 - i;

        // Server (sink)
        PacketSinkHelper sink("ns3::UdpSocketFactory",
                              InetSocketAddress(Ipv4Address::GetAny(), port + i));
        serverApps.Add(sink.Install(nodes.Get(dest)));

        // Client (source)
        OnOffHelper onoff("ns3::UdpSocketFactory",
                          InetSocketAddress(interfaces.GetAddress(dest), port + i));
        onoff.SetAttribute("DataRate",   DataRateValue(DataRate(dataRate)));
        onoff.SetAttribute("PacketSize", UintegerValue(pktSize));
        onoff.SetAttribute("OnTime",     StringValue("ns3::ConstantRandomVariable[Constant=1]"));
        onoff.SetAttribute("OffTime",    StringValue("ns3::ConstantRandomVariable[Constant=0]"));
        clientApps.Add(onoff.Install(nodes.Get(src)));
    }

    serverApps.Start(Seconds(1.0));
    serverApps.Stop(Seconds(simTime));
    clientApps.Start(Seconds(2.0));
    clientApps.Stop(Seconds(simTime - 1));

    // ── Flow Monitor ───────────────────────────────────────────────────────
    FlowMonitorHelper flowmonHelper;
    Ptr<FlowMonitor> monitor = flowmonHelper.InstallAll();

    Simulator::Stop(Seconds(simTime));
    Simulator::Run();

    // ── Collect metrics ────────────────────────────────────────────────────
    monitor->CheckForLostPackets();
    Ptr<Ipv4FlowClassifier> classifier =
        DynamicCast<Ipv4FlowClassifier>(flowmonHelper.GetClassifier());
    FlowMonitor::FlowStatsContainer stats = monitor->GetFlowStats();

    double totalThroughput = 0, totalDelay = 0, totalTx = 0, totalRx = 0, totalDrop = 0;
    uint32_t flowCount = 0;

    for (auto &flow : stats) {
        if (flow.second.txPackets == 0) continue;
        totalThroughput += flow.second.rxBytes * 8.0 / simTime / 1e6; // Mbps
        if (flow.second.rxPackets > 0)
            totalDelay += flow.second.delaySum.GetSeconds() / flow.second.rxPackets;
        totalTx   += flow.second.txPackets;
        totalRx   += flow.second.rxPackets;
        totalDrop += flow.second.lostPackets;
        flowCount++;
    }

    double pdr = (totalTx > 0) ? (totalRx / totalTx) * 100.0 : 0;
    double pdrOp = (totalTx > 0) ? (totalDrop / totalTx) * 100.0 : 0;
    double avgDelay = (flowCount > 0) ? totalDelay / flowCount * 1000.0 : 0; // ms

    // ── Energy consumption ─────────────────────────────────────────────────
    double totalEnergy = 0;
    for (uint32_t i = 0; i < nNodes; i++) {
        Ptr<BasicEnergySource> src =
            DynamicCast<BasicEnergySource>(sources.Get(i));
        totalEnergy += 100.0 - src->GetRemainingEnergy();
    }

    // ── Write CSV row ──────────────────────────────────────────────────────
    std::ofstream out(outputFile, std::ios::app);
    if (out.is_open()) {
        out << nNodes << "," << nFlows << "," << pktPerSec << ","
            << areaFactor << "," << totalThroughput << "," << avgDelay << ","
            << pdr << "," << pdrOp << "," << totalEnergy << "\n";
        out.close();
    }

    std::cout << "Nodes="    << nNodes
              << " Flows="   << nFlows
              << " PPS="     << pktPerSec
              << " Area="    << areaFactor
              << " Tput="    << totalThroughput << " Mbps"
              << " Delay="   << avgDelay << " ms"
              << " PDR="     << pdr << "%"
              << " PDRop="   << pdrOp << "%"
              << " Energy="  << totalEnergy << " J"
              << std::endl;

    Simulator::Destroy();
    return 0;
}
```

---

### 1.2 — 802.15.4 Static Simulation Script

Save this as `scratch/sim-802-15-4-static.cc`:

```cpp
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/lr-wpan-module.h"
#include "ns3/sixlowpan-module.h"
#include "ns3/mobility-module.h"
#include "ns3/aodv-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/energy-module.h"
#include "ns3/propagation-module.h"

using namespace ns3;

int main(int argc, char *argv[]) {
    uint32_t nNodes     = 20;
    uint32_t nFlows     = 10;
    uint32_t pktPerSec  = 100;
    double   txRange    = 50.0;    // 802.15.4 has shorter range ~50m
    double   areaFactor = 1.0;
    double   simTime    = 60.0;
    std::string outputFile = "results/802_15_4_static/output.csv";

    CommandLine cmd;
    cmd.AddValue("nNodes",     "Number of nodes",            nNodes);
    cmd.AddValue("nFlows",     "Number of UDP flows",        nFlows);
    cmd.AddValue("pktPerSec",  "Packets per second",         pktPerSec);
    cmd.AddValue("areaFactor", "Coverage area multiplier",   areaFactor);
    cmd.AddValue("outputFile", "CSV output file path",       outputFile);
    cmd.Parse(argc, argv);

    double sideLength = areaFactor * txRange;

    // ── Node creation ──────────────────────────────────────────────────────
    NodeContainer nodes;
    nodes.Create(nNodes);

    // ── 802.15.4 LR-WPAN ──────────────────────────────────────────────────
    LrWpanHelper lrWpan;
    NetDeviceContainer lrDevices = lrWpan.Install(nodes);
    lrWpan.AssociateToPan(lrDevices, 0);

    // ── Energy model ───────────────────────────────────────────────────────
    BasicEnergySourceHelper energyHelper;
    energyHelper.Set("BasicEnergySourceInitialEnergyJ", DoubleValue(100.0));
    EnergySourceContainer sources = energyHelper.Install(nodes);

    // ── Static random mobility ─────────────────────────────────────────────
    MobilityHelper mobility;
    mobility.SetPositionAllocator("ns3::RandomRectanglePositionAllocator",
                                  "X", StringValue("ns3::UniformRandomVariable[Min=0|Max=" +
                                                   std::to_string(sideLength) + "]"),
                                  "Y", StringValue("ns3::UniformRandomVariable[Min=0|Max=" +
                                                   std::to_string(sideLength) + "]"));
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(nodes);

    // ── 6LoWPAN + Internet stack ───────────────────────────────────────────
    SixLowPanHelper sixlowpan;
    NetDeviceContainer sixDevices = sixlowpan.Install(lrDevices);

    InternetStackHelper internet;
    AodvHelper aodv;
    aodv.Set("Alpha",         DoubleValue(0.6));
    aodv.Set("Beta",          DoubleValue(0.4));
    aodv.Set("Gamma",         DoubleValue(0.0)); // disable velocity-aware term
    aodv.Set("InitialEnergy", DoubleValue(100.0));
    aodv.Set("MaxVelocity",   DoubleValue(50.0));
    internet.SetRoutingHelper(aodv);
    internet.Install(nodes);

    Ipv4AddressHelper address;
    address.SetBase("10.1.2.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces = address.Assign(sixDevices);

    // ── UDP traffic flows ──────────────────────────────────────────────────
    uint16_t port = 9;
    uint32_t pktSize = 80;   // 802.15.4 max payload is ~102 bytes
    double dataRate = pktPerSec * pktSize * 8;

    ApplicationContainer serverApps, clientApps;
    uint32_t actualFlows = std::min(nFlows, nNodes / 2);

    for (uint32_t i = 0; i < actualFlows; i++) {
        uint32_t src  = i;
        uint32_t dest = nNodes - 1 - i;

        PacketSinkHelper sink("ns3::UdpSocketFactory",
                              InetSocketAddress(Ipv4Address::GetAny(), port + i));
        serverApps.Add(sink.Install(nodes.Get(dest)));

        OnOffHelper onoff("ns3::UdpSocketFactory",
                          InetSocketAddress(interfaces.GetAddress(dest), port + i));
        onoff.SetAttribute("DataRate",   DataRateValue(DataRate(dataRate)));
        onoff.SetAttribute("PacketSize", UintegerValue(pktSize));
        onoff.SetAttribute("OnTime",     StringValue("ns3::ConstantRandomVariable[Constant=1]"));
        onoff.SetAttribute("OffTime",    StringValue("ns3::ConstantRandomVariable[Constant=0]"));
        clientApps.Add(onoff.Install(nodes.Get(src)));
    }

    serverApps.Start(Seconds(1.0));
    serverApps.Stop(Seconds(simTime));
    clientApps.Start(Seconds(2.0));
    clientApps.Stop(Seconds(simTime - 1));

    // ── Flow Monitor ───────────────────────────────────────────────────────
    FlowMonitorHelper flowmonHelper;
    Ptr<FlowMonitor> monitor = flowmonHelper.InstallAll();

    Simulator::Stop(Seconds(simTime));
    Simulator::Run();

    monitor->CheckForLostPackets();
    Ptr<Ipv4FlowClassifier> classifier =
        DynamicCast<Ipv4FlowClassifier>(flowmonHelper.GetClassifier());
    FlowMonitor::FlowStatsContainer stats = monitor->GetFlowStats();

    double totalThroughput = 0, totalDelay = 0, totalTx = 0, totalRx = 0, totalDrop = 0;
    uint32_t flowCount = 0;

    for (auto &flow : stats) {
        if (flow.second.txPackets == 0) continue;
        totalThroughput += flow.second.rxBytes * 8.0 / simTime / 1e6;
        if (flow.second.rxPackets > 0)
            totalDelay += flow.second.delaySum.GetSeconds() / flow.second.rxPackets;
        totalTx   += flow.second.txPackets;
        totalRx   += flow.second.rxPackets;
        totalDrop += flow.second.lostPackets;
        flowCount++;
    }

    double pdr    = (totalTx > 0) ? (totalRx / totalTx) * 100.0 : 0;
    double pdrOp  = (totalTx > 0) ? (totalDrop / totalTx) * 100.0 : 0;
    double avgDelay = (flowCount > 0) ? totalDelay / flowCount * 1000.0 : 0;

    double totalEnergy = 0;
    for (uint32_t i = 0; i < nNodes; i++) {
        Ptr<BasicEnergySource> src =
            DynamicCast<BasicEnergySource>(sources.Get(i));
        totalEnergy += 100.0 - src->GetRemainingEnergy();
    }

    std::ofstream out(outputFile, std::ios::app);
    if (out.is_open()) {
        out << nNodes << "," << nFlows << "," << pktPerSec << ","
            << areaFactor << "," << totalThroughput << "," << avgDelay << ","
            << pdr << "," << pdrOp << "," << totalEnergy << "\n";
        out.close();
    }

    std::cout << "Nodes="   << nNodes
              << " Flows="  << nFlows
              << " PPS="    << pktPerSec
              << " Area="   << areaFactor
              << " Tput="   << totalThroughput << " Mbps"
              << " Delay="  << avgDelay << " ms"
              << " PDR="    << pdr << "%"
              << " PDRop="  << pdrOp << "%"
              << " Energy=" << totalEnergy << " J"
              << std::endl;

    Simulator::Destroy();
    return 0;
}
```

---

### 1.3 — BONUS: FF-AODV FANET Simulation Script (Mobile Drones)

Save this as `scratch/sim-fanet-bonus.cc`. This uses your **Phase 2 velocity-aware FF-AODV** implementation:

```cpp
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/aodv-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/energy-module.h"

using namespace ns3;

int main(int argc, char *argv[]) {
    uint32_t nNodes    = 20;
    uint32_t nFlows    = 10;
    uint32_t pktPerSec = 100;
    double   speed     = 10.0;   // m/s — this is the varied parameter for bonus
    double   simTime   = 60.0;
    std::string outputFile = "results/fanet_bonus/output.csv";

    CommandLine cmd;
    cmd.AddValue("nNodes",     "Number of nodes",     nNodes);
    cmd.AddValue("nFlows",     "Number of flows",     nFlows);
    cmd.AddValue("pktPerSec",  "Packets per second",  pktPerSec);
    cmd.AddValue("speed",      "Node speed (m/s)",    speed);
    cmd.AddValue("outputFile", "Output CSV path",     outputFile);
    cmd.Parse(argc, argv);

    NodeContainer nodes;
    nodes.Create(nNodes);

    // ── Wi-Fi 802.11a ad-hoc ───────────────────────────────────────────────
    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211a);
    wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager",
                                 "DataMode", StringValue("OfdmRate6Mbps"),
                                 "ControlMode", StringValue("OfdmRate6Mbps"));

    YansWifiPhyHelper phy;
    YansWifiChannelHelper channel = YansWifiChannelHelper::Default();
    phy.SetChannel(channel.Create());

    WifiMacHelper mac;
    mac.SetType("ns3::AdhocWifiMac");

    NetDeviceContainer devices = wifi.Install(phy, mac, nodes);

    // ── Energy ─────────────────────────────────────────────────────────────
    BasicEnergySourceHelper energyHelper;
    energyHelper.Set("BasicEnergySourceInitialEnergyJ", DoubleValue(100.0));
    EnergySourceContainer sources = energyHelper.Install(nodes);
    WifiRadioEnergyModelHelper radioEnergyHelper;
    radioEnergyHelper.Install(devices, sources);

    // ── Gauss-Markov mobility (simulates drone movement) ──────────────────
    MobilityHelper mobility;
    mobility.SetPositionAllocator("ns3::RandomRectanglePositionAllocator",
                                  "X", StringValue("ns3::UniformRandomVariable[Min=0|Max=500]"),
                                  "Y", StringValue("ns3::UniformRandomVariable[Min=0|Max=500]"));
    mobility.SetMobilityModel(
        "ns3::GaussMarkovMobilityModel",
        "Bounds",      BoxValue(Box(0, 500, 0, 500, 0, 100)),
        "TimeStep",    TimeValue(Seconds(0.5)),
        "Alpha",       DoubleValue(0.85),
        "MeanVelocity",StringValue("ns3::ConstantRandomVariable[Constant=" +
                                   std::to_string(speed) + "]"),
        "MeanDirection", StringValue("ns3::UniformRandomVariable[Min=0|Max=6.283185]"),
        "MeanPitch",   StringValue("ns3::ConstantRandomVariable[Constant=0.0]")
    );
    mobility.Install(nodes);

    // ── FF-AODV routing (Phase 2: velocity-aware) ─────────────────────────
    AodvHelper aodv;
    aodv.Set("Alpha",         DoubleValue(0.5));
    aodv.Set("Beta",          DoubleValue(0.3));
    aodv.Set("Gamma",         DoubleValue(0.2));
    aodv.Set("InitialEnergy", DoubleValue(100.0));
    aodv.Set("MaxVelocity",   DoubleValue(50.0));

    InternetStackHelper internet;
    internet.SetRoutingHelper(aodv);
    internet.Install(nodes);

    Ipv4AddressHelper address;
    address.SetBase("10.1.3.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces = address.Assign(devices);

    // ── Traffic ────────────────────────────────────────────────────────────
    uint16_t port    = 9;
    uint32_t pktSize = 512;
    double   dataRate = pktPerSec * pktSize * 8;

    ApplicationContainer serverApps, clientApps;
    uint32_t actualFlows = std::min(nFlows, nNodes / 2);

    for (uint32_t i = 0; i < actualFlows; i++) {
        uint32_t src  = i;
        uint32_t dest = nNodes - 1 - i;

        PacketSinkHelper sink("ns3::UdpSocketFactory",
                              InetSocketAddress(Ipv4Address::GetAny(), port + i));
        serverApps.Add(sink.Install(nodes.Get(dest)));

        OnOffHelper onoff("ns3::UdpSocketFactory",
                          InetSocketAddress(interfaces.GetAddress(dest), port + i));
        onoff.SetAttribute("DataRate",   DataRateValue(DataRate(dataRate)));
        onoff.SetAttribute("PacketSize", UintegerValue(pktSize));
        onoff.SetAttribute("OnTime",     StringValue("ns3::ConstantRandomVariable[Constant=1]"));
        onoff.SetAttribute("OffTime",    StringValue("ns3::ConstantRandomVariable[Constant=0]"));
        clientApps.Add(onoff.Install(nodes.Get(src)));
    }

    serverApps.Start(Seconds(1.0));
    serverApps.Stop(Seconds(simTime));
    clientApps.Start(Seconds(2.0));
    clientApps.Stop(Seconds(simTime - 1));

    FlowMonitorHelper flowmonHelper;
    Ptr<FlowMonitor> monitor = flowmonHelper.InstallAll();

    Simulator::Stop(Seconds(simTime));
    Simulator::Run();

    monitor->CheckForLostPackets();
    FlowMonitor::FlowStatsContainer stats = monitor->GetFlowStats();

    double totalThroughput = 0, totalDelay = 0, totalTx = 0, totalRx = 0, totalDrop = 0;
    uint32_t flowCount = 0;

    for (auto &flow : stats) {
        if (flow.second.txPackets == 0) continue;
        totalThroughput += flow.second.rxBytes * 8.0 / simTime / 1e6;
        if (flow.second.rxPackets > 0)
            totalDelay += flow.second.delaySum.GetSeconds() / flow.second.rxPackets;
        totalTx   += flow.second.txPackets;
        totalRx   += flow.second.rxPackets;
        totalDrop += flow.second.lostPackets;
        flowCount++;
    }

    double pdr    = (totalTx > 0) ? (totalRx / totalTx) * 100.0 : 0;
    double pdrOp  = (totalTx > 0) ? (totalDrop / totalTx) * 100.0 : 0;
    double avgDelay = (flowCount > 0) ? totalDelay / flowCount * 1000.0 : 0;

    double totalEnergy = 0;
    for (uint32_t i = 0; i < nNodes; i++) {
        Ptr<BasicEnergySource> src =
            DynamicCast<BasicEnergySource>(sources.Get(i));
        totalEnergy += 100.0 - src->GetRemainingEnergy();
    }

    std::ofstream out(outputFile, std::ios::app);
    if (out.is_open()) {
        out << nNodes << "," << nFlows << "," << pktPerSec << ","
            << speed << "," << totalThroughput << "," << avgDelay << ","
            << pdr << "," << pdrOp << "," << totalEnergy << "\n";
        out.close();
    }

    std::cout << "Speed="  << speed
              << " Tput="  << totalThroughput
              << " Delay=" << avgDelay
              << " PDR="   << pdr
              << " PDRop=" << pdrOp
              << " Energy=" << totalEnergy << std::endl;

    Simulator::Destroy();
    return 0;
}
```

---

## Part 2: Build All Simulation Scripts

```bash
cd ns-allinone-3.39/ns-3.39

# Build all three scripts
./ns3 build scratch/sim-802-11-static
./ns3 build scratch/sim-802-15-4-static
./ns3 build scratch/sim-fanet-bonus
```

---

## Part 3: Run All Simulations

### 3.1 — Initialize CSV Files with Headers

```bash
# 802.11 static CSVs used below
for f in vary_nodes vary_flows vary_pps vary_area; do
    echo "nNodes,nFlows,pktPerSec,areaFactor,throughput_mbps,delay_ms,pdr_pct,pdrop_pct,energy_j" \
        > "results/802_11_static/${f}.csv"
done

# 802.15.4 static CSVs used below
for f in vary_nodes vary_flows vary_pps vary_area; do
    echo "nNodes,nFlows,pktPerSec,areaFactor,throughput_mbps,delay_ms,pdr_pct,pdrop_pct,energy_j" \
        > "results/802_15_4_static/${f}.csv"
done

# BONUS mobile FANET CSV (speed variation)
echo "nNodes,nFlows,pktPerSec,speed_mps,throughput_mbps,delay_ms,pdr_pct,pdrop_pct,energy_j" \
    > results/fanet_bonus/vary_speed.csv
```

---

### 3.2 — Run 802.11 Static Simulations

Each block fixes all parameters at their default values while varying one at a time.

**Default values:** nNodes=20, nFlows=10, pktPerSec=100, areaFactor=1.0

```bash
# --- Vary NUMBER OF NODES (fix flows=10, pps=100, area=1) ---
for n in 20 40 60 80 100; do
  ./ns3 run "scratch/sim-802-11-static \
    --nNodes=$n --nFlows=10 --pktPerSec=100 --areaFactor=1.0 \
    --outputFile=results/802_11_static/vary_nodes.csv"
done

# --- Vary NUMBER OF FLOWS (fix nodes=40, pps=100, area=1) ---
for f in 10 20 30 40 50; do
  ./ns3 run "scratch/sim-802-11-static \
    --nNodes=40 --nFlows=$f --pktPerSec=100 --areaFactor=1.0 \
    --outputFile=results/802_11_static/vary_flows.csv"
done

# --- Vary PACKETS PER SECOND (fix nodes=40, flows=10, area=1) ---
for p in 100 200 300 400 500; do
  ./ns3 run "scratch/sim-802-11-static \
    --nNodes=40 --nFlows=10 --pktPerSec=$p --areaFactor=1.0 \
    --outputFile=results/802_11_static/vary_pps.csv"
done

# --- Vary COVERAGE AREA (fix nodes=40, flows=10, pps=100) ---
for a in 1 2 3 4 5; do
  ./ns3 run "scratch/sim-802-11-static \
    --nNodes=40 --nFlows=10 --pktPerSec=100 --areaFactor=$a \
    --outputFile=results/802_11_static/vary_area.csv"
done
```

---

### 3.3 — Run 802.15.4 Static Simulations

```bash
# --- Vary NUMBER OF NODES ---
for n in 20 40 60 80 100; do
  ./ns3 run "scratch/sim-802-15-4-static \
    --nNodes=$n --nFlows=10 --pktPerSec=100 --areaFactor=1.0 \
    --outputFile=results/802_15_4_static/vary_nodes.csv"
done

# --- Vary NUMBER OF FLOWS ---
for f in 10 20 30 40 50; do
  ./ns3 run "scratch/sim-802-15-4-static \
    --nNodes=40 --nFlows=$f --pktPerSec=100 --areaFactor=1.0 \
    --outputFile=results/802_15_4_static/vary_flows.csv"
done

# --- Vary PACKETS PER SECOND ---
for p in 100 200 300 400 500; do
  ./ns3 run "scratch/sim-802-15-4-static \
    --nNodes=40 --nFlows=10 --pktPerSec=$p --areaFactor=1.0 \
    --outputFile=results/802_15_4_static/vary_pps.csv"
done

# --- Vary COVERAGE AREA ---
for a in 1 2 3 4 5; do
  ./ns3 run "scratch/sim-802-15-4-static \
    --nNodes=40 --nFlows=10 --pktPerSec=100 --areaFactor=$a \
    --outputFile=results/802_15_4_static/vary_area.csv"
done
```

---

### 3.4 — Run BONUS: FF-AODV FANET Simulations

```bash
# --- Vary SPEED (mobile drone scenario) ---
for s in 5 10 15 20 25; do
  ./ns3 run "scratch/sim-fanet-bonus \
    --nNodes=40 --nFlows=10 --pktPerSec=100 --speed=$s \
    --outputFile=results/fanet_bonus/vary_speed.csv"
done
```

---

## Part 4: Python Graph Generation Script

Save this as `plot_graphs.py` in your `ns-3.39/` directory:

```python
import pandas as pd
import matplotlib.pyplot as plt
import matplotlib.ticker as ticker
import os

# ── Style settings ──────────────────────────────────────────────────────────
plt.rcParams.update({
    'font.size': 12,
    'axes.titlesize': 13,
    'axes.labelsize': 12,
    'lines.linewidth': 2,
    'lines.markersize': 7,
    'grid.alpha': 0.3,
    'figure.dpi': 150,
})

# ── Colors and markers ───────────────────────────────────────────────────────
COLOR_80211  = '#1f77b4'   # blue
COLOR_802154 = '#ff7f0e'   # orange
COLOR_FANET  = '#2ca02c'   # green
MARKER_80211  = 'o'
MARKER_802154 = 's'
MARKER_FANET  = '^'

# ── Output directories ───────────────────────────────────────────────────────
os.makedirs("graphs/802_11",  exist_ok=True)
os.makedirs("graphs/802_15_4", exist_ok=True)
os.makedirs("graphs/fanet",   exist_ok=True)
os.makedirs("graphs/combined", exist_ok=True)

# ── Metrics config ───────────────────────────────────────────────────────────
METRICS = [
    ("throughput_mbps", "Network Throughput (Mbps)",       "throughput"),
    ("delay_ms",        "Average End-to-End Delay (ms)",   "delay"),
    ("pdr_pct",         "Packet Delivery Ratio (%)",       "pdr"),
    ("pdrop_pct",       "Packet Drop Ratio (%)",           "pdrop"),
    ("energy_j",        "Total Energy Consumption (J)",    "energy"),
]

# ── Column names for X-axis per variation ───────────────────────────────────
VARY_CONFIG = {
    "vary_nodes": ("nNodes",     "Number of Nodes",          [20, 40, 60, 80, 100]),
    "vary_flows": ("nFlows",     "Number of Flows",          [10, 20, 30, 40, 50]),
    "vary_pps":   ("pktPerSec",  "Packets per Second",       [100, 200, 300, 400, 500]),
    "vary_area":  ("areaFactor", "Coverage Area (×Tx range)",[1, 2, 3, 4, 5]),
    "vary_speed": ("speed_mps",  "Node Speed (m/s)",         [5, 10, 15, 20, 25]),
}


def load_csv(path, xcol):
    """Load CSV, sort by xcol, return DataFrame or None."""
    if not os.path.exists(path):
        print(f"  [SKIP] File not found: {path}")
        return None
    df = pd.read_csv(path)
    df = df.sort_values(xcol).reset_index(drop=True)
    return df


def plot_single(df, xcol, xlabel, xvals, metric_col, ylabel, title,
                color, marker, out_path):
    """Plot one line for one network type."""
    fig, ax = plt.subplots(figsize=(7, 5))
    yvals = [df[df[xcol] == x][metric_col].mean() for x in xvals]
    ax.plot(xvals, yvals, color=color, marker=marker, label=title)
    ax.set_xlabel(xlabel)
    ax.set_ylabel(ylabel)
    ax.set_title(f"{ylabel}\nvs {xlabel}")
    ax.set_xticks(xvals)
    ax.grid(True)
    ax.legend()
    plt.tight_layout()
    plt.savefig(out_path)
    plt.close()
    print(f"  Saved: {out_path}")


def plot_comparison(df1, df2, xcol, xlabel, xvals, metric_col, ylabel,
                    label1, label2, color1, color2, marker1, marker2,
                    out_path, df3=None, label3=None, color3=None, marker3=None):
    """Plot two (or three) lines on the same graph for comparison."""
    fig, ax = plt.subplots(figsize=(8, 5))

    if df1 is not None:
        y1 = [df1[df1[xcol] == x][metric_col].mean() for x in xvals]
        ax.plot(xvals, y1, color=color1, marker=marker1, label=label1)
    if df2 is not None:
        y2 = [df2[df2[xcol] == x][metric_col].mean() for x in xvals]
        ax.plot(xvals, y2, color=color2, marker=marker2, label=label2)
    if df3 is not None:
        y3 = [df3[df3[xcol] == x][metric_col].mean() for x in xvals]
        ax.plot(xvals, y3, color=color3, marker=marker3, label=label3)

    ax.set_xlabel(xlabel)
    ax.set_ylabel(ylabel)
    ax.set_title(f"{ylabel} vs {xlabel}\n(802.11 vs 802.15.4)")
    ax.set_xticks(xvals)
    ax.grid(True)
    ax.legend()
    plt.tight_layout()
    plt.savefig(out_path)
    plt.close()
    print(f"  Saved: {out_path}")


# ═══════════════════════════════════════════════════════════════════════════
# SECTION A — 802.11 Static (individual graphs)
# ═══════════════════════════════════════════════════════════════════════════
print("\n── Generating 802.11 Static Graphs ──")
for vary_key, (xcol, xlabel, xvals) in VARY_CONFIG.items():
    if vary_key == "vary_speed":
        continue   # speed only for FANET
    df = load_csv(f"results/802_11_static/{vary_key}.csv", xcol)
    if df is None:
        continue
    for metric_col, ylabel, metric_key in METRICS:
        out = f"graphs/802_11/{vary_key}_{metric_key}.png"
        plot_single(df, xcol, xlabel, xvals, metric_col, ylabel,
                    "802.11 (Static)", COLOR_80211, MARKER_80211, out)

# ═══════════════════════════════════════════════════════════════════════════
# SECTION B — 802.15.4 Static (individual graphs)
# ═══════════════════════════════════════════════════════════════════════════
print("\n── Generating 802.15.4 Static Graphs ──")
for vary_key, (xcol, xlabel, xvals) in VARY_CONFIG.items():
    if vary_key == "vary_speed":
        continue
    df = load_csv(f"results/802_15_4_static/{vary_key}.csv", xcol)
    if df is None:
        continue
    for metric_col, ylabel, metric_key in METRICS:
        out = f"graphs/802_15_4/{vary_key}_{metric_key}.png"
        plot_single(df, xcol, xlabel, xvals, metric_col, ylabel,
                    "802.15.4 (Static)", COLOR_802154, MARKER_802154, out)

# ═══════════════════════════════════════════════════════════════════════════
# SECTION C — Combined Comparison Graphs (802.11 vs 802.15.4)
# ═══════════════════════════════════════════════════════════════════════════
print("\n── Generating Comparison Graphs (802.11 vs 802.15.4) ──")
for vary_key, (xcol, xlabel, xvals) in VARY_CONFIG.items():
    if vary_key == "vary_speed":
        continue
    df1 = load_csv(f"results/802_11_static/{vary_key}.csv",  xcol)
    df2 = load_csv(f"results/802_15_4_static/{vary_key}.csv", xcol)
    for metric_col, ylabel, metric_key in METRICS:
        out = f"graphs/combined/{vary_key}_{metric_key}_compare.png"
        plot_comparison(df1, df2, xcol, xlabel, xvals, metric_col, ylabel,
                        "802.11 (Static)", "802.15.4 (Static)",
                        COLOR_80211, COLOR_802154,
                        MARKER_80211, MARKER_802154, out)

# ═══════════════════════════════════════════════════════════════════════════
# SECTION D — BONUS: FF-AODV FANET Graphs
# ═══════════════════════════════════════════════════════════════════════════
print("\n── Generating BONUS: FF-AODV FANET Graphs ──")

# Speed variation (main bonus)
for vary_key in ["vary_speed"]:
    xcol, xlabel, xvals = VARY_CONFIG[vary_key]
    df = load_csv(f"results/fanet_bonus/{vary_key}.csv", xcol)
    if df is None:
        continue
    for metric_col, ylabel, metric_key in METRICS:
        out = f"graphs/fanet/{vary_key}_{metric_key}.png"
        plot_single(df, xcol, xlabel, xvals, metric_col, ylabel,
                    "FF-AODV FANET (Mobile)", COLOR_FANET, MARKER_FANET, out)

print("\n✅ All graphs generated successfully!")
print("   Check the 'graphs/' directory for output.")
```

---

## Part 5: Run the Graph Generator

```bash
# From ns-3.39 directory
python3 plot_graphs.py
```

---

## Part 6: Verify Output — Expected Files

After running everything, you should have:

```
graphs/
├── 802_11/
│   ├── vary_nodes_throughput.png
│   ├── vary_nodes_delay.png
│   ├── vary_nodes_pdr.png
│   ├── vary_nodes_pdrop.png
│   ├── vary_nodes_energy.png
│   ├── vary_flows_*.png         (5 files)
│   ├── vary_pps_*.png           (5 files)
│   └── vary_area_*.png          (5 files)     → 20 graphs total
│
├── 802_15_4/
│   └── (same 20 files)
│
├── combined/
│   └── (20 comparison graphs: 802.11 vs 802.15.4 on same plot)
│
└── fanet/
    └── vary_speed_*.png         (5 files — bonus mobility graphs)
```

**Required graphs: 45** (20 for 802.11 + 20 for 802.15.4 + 5 bonus speed graphs)

**Optional extra graphs: 20** (`graphs/combined/` comparison plots)

---

## Part 7: Troubleshooting Common Issues

### Issue: 802.15.4 module not found

```bash
# Check if lr-wpan is available
./ns3 configure --enable-examples --enable-tests
grep -r "lr-wpan" build/lib/
# If not found, use olsr instead of aodv for 802.15.4 and switch to
# a simpler UdpEchoClient setup
```

### Issue: CSV file has wrong columns

```bash
# Check what was written
head -5 results/802_11_static/vary_nodes.csv
# If no header, regenerate with:
echo "nNodes,nFlows,pktPerSec,areaFactor,throughput_mbps,delay_ms,pdr_pct,pdrop_pct,energy_j" > results/802_11_static/vary_nodes.csv
```

### Issue: FF-AODV attributes (Alpha/Beta/Gamma) not found

This means the FF-AODV modifications from your Phase 1/2 are not compiled in. Run:
```bash
./ns3 configure --enable-examples
./ns3 build
# Then check that aodv-routing-protocol.cc has GetTypeId() registering Alpha, Beta, Gamma
```

### Issue: All metric values are 0

This usually means flows never connected. Common fixes:
- Increase `simTime` to 120 seconds
- Decrease node count or increase area so topology isn't too dense/sparse
- Check that `port + i` doesn't exceed 65535 for high flow counts

### Issue: Segmentation fault on 802.15.4

The 6LoWPAN + AODV combination can be unstable. Replace AODV with OLSR:
```cpp
OlsrHelper olsr;
internet.SetRoutingHelper(olsr);
```

---

## Part 8: Report Integration Checklist

For each graph you include in your report, make sure you:

- [ ] Label both X and Y axes with units
- [ ] Add a descriptive title mentioning which parameter is varied
- [ ] Include a legend distinguishing 802.11 vs 802.15.4 (or FF-AODV for bonus)
- [ ] Write 3–5 sentences of discussion below each graph explaining the trend
- [ ] For bonus graphs, explain *why* FF-AODV performs differently from standard AODV

---

*Generated for: Mosharaf Hossain Apurbo | CSE 322 | BUET | ns-3.39*