/*
 * sim-fanet-phase2-pktsize.cc
 * BONUS: FF-AODV Phase 2 FANET simulation for packet-size sweep.
 *
 * This is intentionally separate from speed-sweep scripts so existing
 * pipelines remain unchanged.
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

#include <algorithm>
#include <fstream>
#include <sstream>

using namespace ns3;

int main(int argc, char* argv[])
{
    // Tunable parameters
    uint32_t nNodes = 60;
    uint32_t nFlows = 15;
    uint32_t pktPerSec = 20;
    uint32_t packetSize = 512;
    uint32_t runId = 1;
    double speed = 15.0;
    double speedSpread = 0.6;
    double simTime = 60.0;
    std::string outputFile = "results/fanet_bonus/vary_packet_size.csv";

    CommandLine cmd;
    cmd.AddValue("nNodes", "Number of nodes", nNodes);
    cmd.AddValue("nFlows", "Number of flows", nFlows);
    cmd.AddValue("pktPerSec", "Packets per second", pktPerSec);
    cmd.AddValue("packetSize", "UDP packet size in bytes", packetSize);
    cmd.AddValue("runId", "Run identifier for multi-seed aggregation", runId);
    cmd.AddValue("speed", "Mean node speed (m/s)", speed);
    cmd.AddValue("speedSpread", "Relative +/- spread around speed for node velocity", speedSpread);
    cmd.AddValue("simTime", "Simulation duration (s)", simTime);
    cmd.AddValue("outputFile", "Output CSV path", outputFile);
    cmd.Parse(argc, argv);

    NodeContainer nodes;
    nodes.Create(nNodes);

    // 802.11a ad-hoc PHY/MAC
    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211a);
    wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager",
                                 "DataMode",
                                 StringValue("OfdmRate6Mbps"),
                                 "ControlMode",
                                 StringValue("OfdmRate6Mbps"));

    YansWifiPhyHelper phy;
    YansWifiChannelHelper channel = YansWifiChannelHelper::Default();
    phy.SetChannel(channel.Create());

    WifiMacHelper mac;
    mac.SetType("ns3::AdhocWifiMac");

    NetDeviceContainer devices = wifi.Install(phy, mac, nodes);

    // Energy model
    BasicEnergySourceHelper energyHelper;
    energyHelper.Set("BasicEnergySourceInitialEnergyJ", DoubleValue(100.0));
    EnergySourceContainer sources = energyHelper.Install(nodes);

    WifiRadioEnergyModelHelper radioEnergyHelper;
    radioEnergyHelper.Install(devices, sources);

    // Gauss-Markov mobility
    MobilityHelper mobility;
    double minNodeSpeed = std::max(0.5, speed * (1.0 - speedSpread));
    double maxNodeSpeed = std::max(minNodeSpeed + 0.1, speed * (1.0 + speedSpread));

    std::ostringstream velocityRv;
    velocityRv << "ns3::UniformRandomVariable[Min=" << minNodeSpeed << "|Max=" << maxNodeSpeed
               << "]";

    mobility.SetPositionAllocator("ns3::RandomRectanglePositionAllocator",
                                  "X",
                                  StringValue("ns3::UniformRandomVariable[Min=0|Max=500]"),
                                  "Y",
                                  StringValue("ns3::UniformRandomVariable[Min=0|Max=500]"));
    mobility.SetMobilityModel("ns3::GaussMarkovMobilityModel",
                              "Bounds",
                              BoxValue(Box(0, 500, 0, 500, 0, 100)),
                              "TimeStep",
                              TimeValue(Seconds(0.5)),
                              "Alpha",
                              DoubleValue(0.85),
                              "MeanVelocity",
                              StringValue(velocityRv.str()),
                              "MeanDirection",
                              StringValue("ns3::UniformRandomVariable[Min=0|Max=6.283185]"),
                              "MeanPitch",
                              StringValue("ns3::ConstantRandomVariable[Constant=0.0]"));
    mobility.Install(nodes);

    // FF-AODV Phase 2 (velocity-aware)
    AodvHelper aodv;
    aodv.Set("Alpha", DoubleValue(0.5));
    aodv.Set("Beta", DoubleValue(0.3));
    aodv.Set("Gamma", DoubleValue(0.2));
    aodv.Set("InitialEnergy", DoubleValue(100.0));
    aodv.Set("MaxVelocity", DoubleValue(50.0));

    InternetStackHelper internet;
    internet.SetRoutingHelper(aodv);
    internet.Install(nodes);

    Ipv4AddressHelper address;
    address.SetBase("10.1.4.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces = address.Assign(devices);

    // Traffic
    uint16_t port = 9;
    double dataRate = static_cast<double>(pktPerSec) * packetSize * 8.0;

    ApplicationContainer serverApps, clientApps;
    uint32_t actualFlows = std::min(nFlows, nNodes / 2);

    for (uint32_t i = 0; i < actualFlows; i++)
    {
        uint32_t src = i;
        uint32_t dest = nNodes - 1 - i;

        PacketSinkHelper sink("ns3::UdpSocketFactory",
                              InetSocketAddress(Ipv4Address::GetAny(), port + i));
        serverApps.Add(sink.Install(nodes.Get(dest)));

        OnOffHelper onoff("ns3::UdpSocketFactory",
                          InetSocketAddress(interfaces.GetAddress(dest), port + i));
        onoff.SetAttribute("DataRate", DataRateValue(DataRate(static_cast<uint64_t>(dataRate))));
        onoff.SetAttribute("PacketSize", UintegerValue(packetSize));
        onoff.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
        onoff.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
        clientApps.Add(onoff.Install(nodes.Get(src)));
    }

    serverApps.Start(Seconds(1.0));
    serverApps.Stop(Seconds(simTime));
    clientApps.Start(Seconds(2.0));
    clientApps.Stop(Seconds(simTime - 1.0));

    FlowMonitorHelper flowmonHelper;
    Ptr<FlowMonitor> monitor = flowmonHelper.InstallAll();

    Simulator::Stop(Seconds(simTime));
    Simulator::Run();

    // Metrics
    monitor->CheckForLostPackets();
    FlowMonitor::FlowStatsContainer stats = monitor->GetFlowStats();

    double totalThroughput = 0.0;
    double totalDelay = 0.0;
    double totalTx = 0.0;
    double totalRx = 0.0;
    double totalDrop = 0.0;
    uint32_t flowCount = 0;

    for (auto& flow : stats)
    {
        if (flow.second.txPackets == 0)
        {
            continue;
        }

        totalThroughput += flow.second.rxBytes * 8.0 / simTime / 1e6;
        if (flow.second.rxPackets > 0)
        {
            totalDelay += flow.second.delaySum.GetSeconds() / flow.second.rxPackets;
        }
        totalTx += flow.second.txPackets;
        totalRx += flow.second.rxPackets;
        totalDrop += flow.second.lostPackets;
        flowCount++;
    }

    double pdr = (totalTx > 0.0) ? (totalRx / totalTx) * 100.0 : 0.0;
    double pdrop = (totalTx > 0.0) ? (totalDrop / totalTx) * 100.0 : 0.0;
    double avgDelay = (flowCount > 0) ? totalDelay / flowCount * 1000.0 : 0.0;

    double totalEnergy = 0.0;
    for (uint32_t i = 0; i < nNodes; i++)
    {
        Ptr<BasicEnergySource> src = DynamicCast<BasicEnergySource>(sources.Get(i));
        if (src)
        {
            totalEnergy += 100.0 - src->GetRemainingEnergy();
        }
    }

    std::ofstream out(outputFile, std::ios::app);
    if (out.is_open())
    {
        out << nNodes << "," << nFlows << "," << pktPerSec << "," << packetSize << "," << speed
            << "," << totalThroughput << "," << avgDelay << "," << pdr << "," << pdrop << ","
            << totalEnergy << "," << runId << "\n";
        out.close();
    }

    std::cout << "Run=" << runId << " PktSize=" << packetSize << " Speed=" << speed
              << " Tput=" << totalThroughput << " Mbps"
              << " Delay=" << avgDelay << " ms"
              << " PDR=" << pdr << "%"
              << " PDrop=" << pdrop << "%"
              << " Energy=" << totalEnergy << " J" << std::endl;

    Simulator::Destroy();
    return 0;
}
