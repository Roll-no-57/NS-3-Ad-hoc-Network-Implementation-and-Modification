/*
 * sim-802-11-static.cc
 * 802.11b ad-hoc static simulation with FF-AODV (Phase 1 — energy + hop).
 * Nodes are STATIC (ConstantPositionMobilityModel).
 * Gamma=0.0 disables the velocity term.
 */
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

int main(int argc, char *argv[])
{
    // ── Tunable parameters ────────────────────────────────────────────────
    uint32_t    nNodes     = 20;
    uint32_t    nFlows     = 10;
    uint32_t    pktPerSec  = 100;
    double      txRange    = 100.0;   // used only for area side length (m)
    double      areaFactor = 1.0;     // area multiplier (1–5)
    double      simTime    = 60.0;
    std::string outputFile = "results/802_11_static/output.csv";

    CommandLine cmd;
    cmd.AddValue("nNodes",     "Number of nodes",           nNodes);
    cmd.AddValue("nFlows",     "Number of UDP flows",       nFlows);
    cmd.AddValue("pktPerSec",  "Packets per second",        pktPerSec);
    cmd.AddValue("areaFactor", "Coverage area multiplier",  areaFactor);
    cmd.AddValue("outputFile", "CSV output file path",      outputFile);
    cmd.Parse(argc, argv);

    double sideLength = areaFactor * txRange;

    // ── Nodes ─────────────────────────────────────────────────────────────
    NodeContainer nodes;
    nodes.Create(nNodes);

    // ── Wi-Fi 802.11b ad-hoc ──────────────────────────────────────────────
    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211b);
    wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager",
                                 "DataMode",    StringValue("DsssRate1Mbps"),
                                 "ControlMode", StringValue("DsssRate1Mbps"));

    YansWifiPhyHelper phy;
    YansWifiChannelHelper channel = YansWifiChannelHelper::Default();
    phy.SetChannel(channel.Create());
    phy.Set("TxPowerStart", DoubleValue(16.0206));
    phy.Set("TxPowerEnd",   DoubleValue(16.0206));

    WifiMacHelper mac;
    mac.SetType("ns3::AdhocWifiMac");

    NetDeviceContainer devices = wifi.Install(phy, mac, nodes);

    // ── Energy model ──────────────────────────────────────────────────────
    BasicEnergySourceHelper energyHelper;
    energyHelper.Set("BasicEnergySourceInitialEnergyJ", DoubleValue(100.0));
    EnergySourceContainer sources = energyHelper.Install(nodes);

    WifiRadioEnergyModelHelper radioEnergyHelper;
    radioEnergyHelper.Install(devices, sources);

    // ── Static mobility (random placement, no movement) ───────────────────
    MobilityHelper mobility;
    mobility.SetPositionAllocator(
        "ns3::RandomRectanglePositionAllocator",
        "X", StringValue("ns3::UniformRandomVariable[Min=0|Max=" +
                         std::to_string(sideLength) + "]"),
        "Y", StringValue("ns3::UniformRandomVariable[Min=0|Max=" +
                         std::to_string(sideLength) + "]"));
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(nodes);

    // ── Routing: FF-AODV Phase 1 (energy + hop count, no velocity) ───────
    AodvHelper aodv;
    aodv.Set("Alpha",         DoubleValue(0.6));
    aodv.Set("Beta",          DoubleValue(0.4));
    aodv.Set("Gamma",         DoubleValue(0.0));   // velocity term disabled
    aodv.Set("InitialEnergy", DoubleValue(100.0));
    aodv.Set("MaxVelocity",   DoubleValue(50.0));

    InternetStackHelper internet;
    internet.SetRoutingHelper(aodv);
    internet.Install(nodes);

    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces = address.Assign(devices);

    // ── UDP traffic flows ─────────────────────────────────────────────────
    uint16_t port     = 9;
    uint32_t pktSize  = 512;
    double   dataRate = static_cast<double>(pktPerSec) * pktSize * 8; // bps

    ApplicationContainer serverApps, clientApps;
    uint32_t actualFlows = std::min(nFlows, nNodes / 2);

    for (uint32_t i = 0; i < actualFlows; i++)
    {
        uint32_t src  = i;
        uint32_t dest = nNodes - 1 - i;

        PacketSinkHelper sink("ns3::UdpSocketFactory",
                              InetSocketAddress(Ipv4Address::GetAny(), port + i));
        serverApps.Add(sink.Install(nodes.Get(dest)));

        OnOffHelper onoff("ns3::UdpSocketFactory",
                          InetSocketAddress(interfaces.GetAddress(dest), port + i));
        onoff.SetAttribute("DataRate",   DataRateValue(DataRate(static_cast<uint64_t>(dataRate))));
        onoff.SetAttribute("PacketSize", UintegerValue(pktSize));
        onoff.SetAttribute("OnTime",     StringValue("ns3::ConstantRandomVariable[Constant=1]"));
        onoff.SetAttribute("OffTime",    StringValue("ns3::ConstantRandomVariable[Constant=0]"));
        clientApps.Add(onoff.Install(nodes.Get(src)));
    }

    serverApps.Start(Seconds(1.0));
    serverApps.Stop(Seconds(simTime));
    clientApps.Start(Seconds(2.0));
    clientApps.Stop(Seconds(simTime - 1.0));

    // ── Flow Monitor ──────────────────────────────────────────────────────
    FlowMonitorHelper flowmonHelper;
    Ptr<FlowMonitor> monitor = flowmonHelper.InstallAll();

    Simulator::Stop(Seconds(simTime));
    Simulator::Run();

    // ── Metrics ───────────────────────────────────────────────────────────
    monitor->CheckForLostPackets();
    Ptr<Ipv4FlowClassifier> classifier =
        DynamicCast<Ipv4FlowClassifier>(flowmonHelper.GetClassifier());
    FlowMonitor::FlowStatsContainer stats = monitor->GetFlowStats();

    double   totalThroughput = 0, totalDelay = 0;
    double   totalTx = 0,        totalRx = 0,    totalDrop = 0;
    uint32_t flowCount = 0;

    for (auto& flow : stats)
    {
        if (flow.second.txPackets == 0)
            continue;
        totalThroughput += flow.second.rxBytes * 8.0 / simTime / 1e6;
        if (flow.second.rxPackets > 0)
            totalDelay += flow.second.delaySum.GetSeconds() / flow.second.rxPackets;
        totalTx   += flow.second.txPackets;
        totalRx   += flow.second.rxPackets;
        totalDrop += flow.second.lostPackets;
        flowCount++;
    }

    double pdr      = (totalTx > 0) ? (totalRx / totalTx) * 100.0  : 0.0;
    double pdrOp    = (totalTx > 0) ? (totalDrop / totalTx) * 100.0 : 0.0;
    double avgDelay = (flowCount > 0) ? totalDelay / flowCount * 1000.0 : 0.0; // ms

    // ── Energy consumed ───────────────────────────────────────────────────
    double totalEnergy = 0.0;
    for (uint32_t i = 0; i < nNodes; i++)
    {
        Ptr<BasicEnergySource> src =
            DynamicCast<BasicEnergySource>(sources.Get(i));
        if (src)
            totalEnergy += 100.0 - src->GetRemainingEnergy();
    }

    // ── CSV output ────────────────────────────────────────────────────────
    std::ofstream out(outputFile, std::ios::app);
    if (out.is_open())
    {
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
              << " PDR="    << pdr      << "%"
              << " PDRop="  << pdrOp    << "%"
              << " Energy=" << totalEnergy << " J"
              << std::endl;

    Simulator::Destroy();
    return 0;
}