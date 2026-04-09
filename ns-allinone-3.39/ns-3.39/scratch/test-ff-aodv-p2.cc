#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/applications-module.h"
#include "ns3/energy-module.h"
#include "ns3/aodv-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/netanim-module.h"
#include <cmath>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("TestFfAodvV2");

// ── Dynamic energy-based color update ────────────────────────────────────────
// Called every 1 second. Nodes shift Green→Orange→Red as energy depletes.
void UpdateEnergyColors(AnimationInterface*    anim,
                        NodeContainer*         nodes,
                        EnergySourceContainer* energySources,
                        uint32_t               nNodes)
{
    for (uint32_t i = 0; i < nNodes; i++)
    {
        double pct = energySources->Get(i)->GetRemainingEnergy()
                   / energySources->Get(i)->GetInitialEnergy();

        uint8_t r, g, b;
        if      (pct > 0.70) { r = 0;   g = 200; b = 0;   }  // Green  = healthy
        else if (pct > 0.30) { r = 255; g = 165; b = 0;   }  // Orange = medium
        else                 { r = 220; g = 0;   b = 0;   }  // Red    = critical

        anim->UpdateNodeColor(nodes->Get(i), r, g, b);
    }
    Simulator::Schedule(Seconds(1.0), &UpdateEnergyColors,
                        anim, nodes, energySources, nNodes);
}

// ─────────────────────────────────────────────────────────────────────────────

int main(int argc, char* argv[])
{
    uint32_t nNodes  = 15;
    double   simTime = 120.0;  // longer = more energy consumed = visible color changes
    bool     verbose = false;

    CommandLine cmd(__FILE__);
    cmd.AddValue("nodes",   "Number of UAV nodes",    nNodes);
    cmd.AddValue("time",    "Simulation time (s)",    simTime);
    cmd.AddValue("verbose", "Enable AODV log output", verbose);
    cmd.Parse(argc, argv);

    if (verbose)
        LogComponentEnable("AodvRoutingProtocol", LOG_LEVEL_LOGIC);

    // ── 1. Nodes ──────────────────────────────────────────────────────────────
    NodeContainer nodes;
    nodes.Create(nNodes);
    std::cout << "[1] Created " << nNodes << " nodes.\n";

    // ── 2. Wi-Fi ad-hoc ───────────────────────────────────────────────────────
    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211a);
    wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager",
                                 "DataMode",    StringValue("OfdmRate6Mbps"),
                                 "ControlMode", StringValue("OfdmRate6Mbps"));

    WifiMacHelper mac;
    mac.SetType("ns3::AdhocWifiMac");

    YansWifiPhyHelper phy;
    phy.Set("TxPowerStart", DoubleValue(20.0));  // ~250 m range
    phy.Set("TxPowerEnd",   DoubleValue(20.0));

    YansWifiChannelHelper channel = YansWifiChannelHelper::Default();
    phy.SetChannel(channel.Create());

    NetDeviceContainer devices = wifi.Install(phy, mac, nodes);
    std::cout << "[2] Wi-Fi installed (TxPower=20 dBm, ~250 m range).\n";

    // ── 3. Mobility ───────────────────────────────────────────────────────────
    // Small 200x200m area + slow 1-3 m/s = nodes stay in range = packets flow
    MobilityHelper mobility;
    mobility.SetPositionAllocator(
        "ns3::RandomRectanglePositionAllocator",
        "X", StringValue("ns3::UniformRandomVariable[Min=0|Max=200]"),
        "Y", StringValue("ns3::UniformRandomVariable[Min=0|Max=200]"));
    mobility.SetMobilityModel(
        "ns3::GaussMarkovMobilityModel",
        "Bounds",        BoxValue(Box(0, 200, 0, 200, 0, 0)),
        "MeanVelocity",  StringValue("ns3::UniformRandomVariable[Min=1|Max=3]"),
        "MeanDirection", StringValue("ns3::UniformRandomVariable[Min=0|Max=6.283185]"));
    mobility.Install(nodes);
    std::cout << "[3] Mobility installed (200x200m, 1-3 m/s).\n";

    // ── 4. Energy ─────────────────────────────────────────────────────────────
    // 20 J initial — low enough that depletion is visible within 120s
    BasicEnergySourceHelper energyHelper;
    energyHelper.Set("BasicEnergySourceInitialEnergyJ", DoubleValue(20.0));
    EnergySourceContainer energySources = energyHelper.Install(nodes);

    WifiRadioEnergyModelHelper radioEnergyHelper;
    for (uint32_t i = 0; i < nNodes; i++)
        radioEnergyHelper.Install(devices.Get(i), energySources.Get(i));

    std::cout << "[4] Energy sources (20 J each) installed.\n";

    // ── 5. FF-AODV ────────────────────────────────────────────────────────────
    AodvHelper aodv;
    aodv.Set("Alpha",         DoubleValue(0.5));
    aodv.Set("Beta",          DoubleValue(0.3));
    aodv.Set("Gamma",         DoubleValue(0.2));
    aodv.Set("InitialEnergy", DoubleValue(20.0));  // must match energy source
    aodv.Set("MaxVelocity",   DoubleValue(3.0));   // must match mobility max
    aodv.Set("EnableHello",   BooleanValue(true));

    InternetStackHelper internet;
    internet.SetRoutingHelper(aodv);
    internet.Install(nodes);

    Ipv4AddressHelper addressHelper;
    addressHelper.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces = addressHelper.Assign(devices);
    std::cout << "[5] FF-AODV v2 installed (a=0.5 b=0.3 g=0.2).\n";

    // ── 6. Applications ───────────────────────────────────────────────────────
    // Start traffic at 15s+ so AODV has time to discover routes first.
    // High data rate (512 kbps) drains energy fast = color changes visible.

    // Flow 1: Node 0 -> Node 14
    uint16_t port1 = 9;
    PacketSinkHelper sink1("ns3::UdpSocketFactory",
                           InetSocketAddress(Ipv4Address::GetAny(), port1));
    sink1.Install(nodes.Get(nNodes - 1)).Start(Seconds(0.0));

    OnOffHelper onoff1("ns3::UdpSocketFactory",
                       InetSocketAddress(interfaces.GetAddress(nNodes - 1), port1));
    onoff1.SetAttribute("DataRate",   StringValue("512kbps"));
    onoff1.SetAttribute("PacketSize", UintegerValue(512));
    onoff1.SetAttribute("OnTime",  StringValue("ns3::ConstantRandomVariable[Constant=1]"));
    onoff1.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
    onoff1.Install(nodes.Get(0)).Start(Seconds(15.0));

    // Flow 2: Node 1 -> Node 13
    uint16_t port2 = 10;
    PacketSinkHelper sink2("ns3::UdpSocketFactory",
                           InetSocketAddress(Ipv4Address::GetAny(), port2));
    sink2.Install(nodes.Get(nNodes - 2)).Start(Seconds(0.0));

    OnOffHelper onoff2("ns3::UdpSocketFactory",
                       InetSocketAddress(interfaces.GetAddress(nNodes - 2), port2));
    onoff2.SetAttribute("DataRate",   StringValue("512kbps"));
    onoff2.SetAttribute("PacketSize", UintegerValue(512));
    onoff2.SetAttribute("OnTime",  StringValue("ns3::ConstantRandomVariable[Constant=1]"));
    onoff2.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
    onoff2.Install(nodes.Get(1)).Start(Seconds(20.0));

    // Flow 3: Node 2 -> Node 12 (extra load to stress energy)
    uint16_t port3 = 11;
    PacketSinkHelper sink3("ns3::UdpSocketFactory",
                           InetSocketAddress(Ipv4Address::GetAny(), port3));
    sink3.Install(nodes.Get(nNodes - 3)).Start(Seconds(0.0));

    OnOffHelper onoff3("ns3::UdpSocketFactory",
                       InetSocketAddress(interfaces.GetAddress(nNodes - 3), port3));
    onoff3.SetAttribute("DataRate",   StringValue("256kbps"));
    onoff3.SetAttribute("PacketSize", UintegerValue(512));
    onoff3.Install(nodes.Get(2)).Start(Seconds(25.0));

    std::cout << "[6] 3 flows configured (start at 15s / 20s / 25s).\n";

    // ── 7. FlowMonitor ────────────────────────────────────────────────────────
    FlowMonitorHelper flowHelper;
    Ptr<FlowMonitor> flowMonitor = flowHelper.InstallAll();

    // ── 8. NetAnim ────────────────────────────────────────────────────────────
    AnimationInterface anim("ff-aodv-animation.xml");
    anim.SetMaxPktsPerTraceFile(1000000);

    for (uint32_t i = 0; i < nNodes; i++)
        anim.UpdateNodeSize(i, 5, 5);

    // Label key nodes
    anim.UpdateNodeDescription(nodes.Get(0),          "SRC-0");
    anim.UpdateNodeDescription(nodes.Get(1),          "SRC-1");
    anim.UpdateNodeDescription(nodes.Get(2),          "SRC-2");
    anim.UpdateNodeDescription(nodes.Get(nNodes - 1), "DST-14");
    anim.UpdateNodeDescription(nodes.Get(nNodes - 2), "DST-13");
    anim.UpdateNodeDescription(nodes.Get(nNodes - 3), "DST-12");

    // Start energy color updater — nodes begin GREEN, shift to ORANGE then RED
    Simulator::Schedule(Seconds(0.0), &UpdateEnergyColors,
                        &anim, &nodes, &energySources, nNodes);

    // ── 9. Run ────────────────────────────────────────────────────────────────
    std::cout << "\n=== Simulation starting (" << simTime << "s) ===\n";
    std::cout << "    Nodes start GREEN -> ORANGE -> RED as energy depletes\n\n";

    Simulator::Stop(Seconds(simTime));
    Simulator::Run();

    // ── 10. Results ───────────────────────────────────────────────────────────
    std::cout << "\n=== Complete ===\n\n--- Node Energy ---\n";
    double total = 0.0;
    for (uint32_t i = 0; i < nNodes; i++)
    {
        Ptr<EnergySource> es = energySources.Get(i);
        double rem  = es->GetRemainingEnergy();
        double init = es->GetInitialEnergy();
        double pct  = rem / init * 100.0;
        total += (init - rem);
        std::string col = (pct>70)?"GREEN":(pct>30)?"ORANGE":"RED";
        std::cout << "  Node " << i << ": " << rem << "/" << init
                  << " J (" << pct << "%) [" << col << "]\n";
    }
    std::cout << "  Total: " << total << " J  Avg: " << total/nNodes << " J\n";

    std::cout << "\n--- Flow Statistics ---\n";
    flowMonitor->CheckForLostPackets();
    Ptr<Ipv4FlowClassifier> classifier =
        DynamicCast<Ipv4FlowClassifier>(flowHelper.GetClassifier());
    for (auto& e : flowMonitor->GetFlowStats())
    {
        auto ft = classifier->FindFlow(e.first);
        std::cout << "\n  Flow " << e.first << " ("
                  << ft.sourceAddress << " -> " << ft.destinationAddress << ")\n";
        std::cout << "    Tx=" << e.second.txPackets
                  << " Rx=" << e.second.rxPackets;
        if (e.second.txPackets > 0)
            std::cout << " PDR=" << (double)e.second.rxPackets/e.second.txPackets*100.0 << "%";
        if (e.second.rxPackets > 0)
            std::cout << " Delay=" << e.second.delaySum.GetSeconds()/e.second.rxPackets*1000.0
                      << "ms Tput=" << e.second.rxBytes*8.0/(simTime*1000.0) << "kbps";
        std::cout << "\n";
    }

    std::cout << "\nOpen ff-aodv-animation.xml in NetAnim to see energy color changes.\n";
    Simulator::Destroy();
    return 0;
}