#include "ns3/aodv-module.h"
#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/energy-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/network-module.h"
#include "ns3/wifi-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("TestFfAodvPhase1");

int
main(int argc, char* argv[])
{
    uint32_t nNodes = 10;
    double simTime = 30.0;
    bool verbose = false;

    CommandLine cmd(__FILE__);
    cmd.AddValue("nodes", "Number of nodes", nNodes);
    cmd.AddValue("time", "Simulation time (s)", simTime);
    cmd.AddValue("verbose", "Enable AODV logic logging", verbose);
    cmd.Parse(argc, argv);

    if (verbose)
    {
        LogComponentEnable("AodvRoutingProtocol", LOG_LEVEL_LOGIC);
    }

    NodeContainer nodes;
    nodes.Create(nNodes);

    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211a);
    wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager",
                                 "DataMode",
                                 StringValue("OfdmRate6Mbps"),
                                 "ControlMode",
                                 StringValue("OfdmRate6Mbps"));

    WifiMacHelper mac;
    mac.SetType("ns3::AdhocWifiMac");

    YansWifiPhyHelper phy;
    phy.Set("TxPowerStart", DoubleValue(17.0));
    phy.Set("TxPowerEnd", DoubleValue(17.0));

    YansWifiChannelHelper channel;
    channel.SetPropagationDelay("ns3::ConstantSpeedPropagationDelayModel");
    channel.AddPropagationLoss("ns3::RangePropagationLossModel", "MaxRange", DoubleValue(120.0));
    phy.SetChannel(channel.Create());

    NetDeviceContainer devices = wifi.Install(phy, mac, nodes);

    MobilityHelper mobility;
    mobility.SetPositionAllocator("ns3::RandomRectanglePositionAllocator",
                                  "X",
                                  StringValue("ns3::UniformRandomVariable[Min=0|Max=250]"),
                                  "Y",
                                  StringValue("ns3::UniformRandomVariable[Min=0|Max=250]"));
    mobility.SetMobilityModel("ns3::GaussMarkovMobilityModel",
                              "Bounds",
                              BoxValue(Box(0, 250, 0, 250, 0, 0)),
                              "MeanVelocity",
                              StringValue("ns3::UniformRandomVariable[Min=5|Max=15]"),
                              "MeanDirection",
                              StringValue("ns3::UniformRandomVariable[Min=0|Max=6.283185]"));
    mobility.Install(nodes);

    BasicEnergySourceHelper energyHelper;
    energyHelper.Set("BasicEnergySourceInitialEnergyJ", DoubleValue(100.0));
    EnergySourceContainer energySources = energyHelper.Install(nodes);

    WifiRadioEnergyModelHelper radioEnergyHelper;
    for (uint32_t i = 0; i < nNodes; ++i)
    {
        radioEnergyHelper.Install(devices.Get(i), energySources.Get(i));
    }

    AodvHelper aodv;
    aodv.Set("Alpha", DoubleValue(0.6));
    aodv.Set("Beta", DoubleValue(0.4));
    aodv.Set("Gamma", DoubleValue(0.0));
    aodv.Set("InitialEnergy", DoubleValue(100.0));
    aodv.Set("MaxVelocity", DoubleValue(50.0));
    aodv.Set("EnableHello", BooleanValue(true));

    InternetStackHelper internet;
    internet.SetRoutingHelper(aodv);
    internet.Install(nodes);

    Ipv4AddressHelper ipv4;
    ipv4.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces = ipv4.Assign(devices);

    uint16_t port = 9000;
    PacketSinkHelper sinkHelper("ns3::UdpSocketFactory",
                                InetSocketAddress(Ipv4Address::GetAny(), port));
    ApplicationContainer sinkApps = sinkHelper.Install(nodes.Get(nNodes - 1));
    sinkApps.Start(Seconds(0.0));
    sinkApps.Stop(Seconds(simTime));

    OnOffHelper onoff("ns3::UdpSocketFactory",
                      InetSocketAddress(interfaces.GetAddress(nNodes - 1), port));
    onoff.SetAttribute("DataRate", StringValue("96kbps"));
    onoff.SetAttribute("PacketSize", UintegerValue(512));
    onoff.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1.0]"));
    onoff.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0.0]"));
    ApplicationContainer srcApps = onoff.Install(nodes.Get(0));
    srcApps.Start(Seconds(2.0));
    srcApps.Stop(Seconds(simTime - 1.0));

    FlowMonitorHelper flowHelper;
    Ptr<FlowMonitor> flowMonitor = flowHelper.InstallAll();

    Simulator::Stop(Seconds(simTime));
    Simulator::Run();

    flowMonitor->CheckForLostPackets();
    Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(flowHelper.GetClassifier());
    FlowMonitor::FlowStatsContainer stats = flowMonitor->GetFlowStats();

    std::cout << "=== FF-AODV Phase 1 (Energy + Hop Fitness) ===\n";
    std::cout << "Alpha=" << 0.6 << " Beta=" << 0.4 << " Gamma=" << 0.0 << "\n";

    for (const auto& entry : stats)
    {
        auto tuple = classifier->FindFlow(entry.first);
        if (tuple.protocol != 17 || tuple.destinationPort != port)
        {
            continue;
        }

        std::cout << "Flow " << entry.first << " " << tuple.sourceAddress << " -> "
                  << tuple.destinationAddress << "\n";
        std::cout << "  TxPackets=" << entry.second.txPackets
                  << " RxPackets=" << entry.second.rxPackets << "\n";

        if (entry.second.txPackets > 0)
        {
            double pdr = 100.0 * static_cast<double>(entry.second.rxPackets) /
                         static_cast<double>(entry.second.txPackets);
            std::cout << "  PDR=" << pdr << "%\n";
        }

        if (entry.second.rxPackets > 0)
        {
            double avgDelayMs = (entry.second.delaySum.GetSeconds() /
                                 static_cast<double>(entry.second.rxPackets)) *
                                1000.0;
            double throughputKbps = (entry.second.rxBytes * 8.0) / (simTime * 1000.0);
            std::cout << "  AvgDelay=" << avgDelayMs << " ms\n";
            std::cout << "  Throughput=" << throughputKbps << " kbps\n";
        }
    }

    double totalConsumed = 0.0;
    for (uint32_t i = 0; i < nNodes; ++i)
    {
        Ptr<EnergySource> src = energySources.Get(i);
        totalConsumed += (src->GetInitialEnergy() - src->GetRemainingEnergy());
    }
    std::cout << "TotalEnergyConsumed=" << totalConsumed << " J\n";

    Simulator::Destroy();
    return 0;
}
