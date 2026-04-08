#include "ns3/aodv-module.h"
#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/energy-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/network-module.h"
#include "ns3/wifi-module.h"

#include <algorithm>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <set>
#include <sstream>
#include <string>
#include <vector>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("ExperimentFfAodv");

namespace
{

struct FlowDefinition
{
    uint32_t src;
    uint32_t dst;
    uint16_t port;
    double startTime;
};

static std::string
ConstantRv(double value)
{
    std::ostringstream oss;
    oss << "ns3::ConstantRandomVariable[Constant=" << value << "]";
    return oss.str();
}

static bool
NeedsCsvHeader(const std::string& csvFile)
{
    std::ifstream in(csvFile.c_str(), std::ios::in);
    if (!in.good())
    {
        return true;
    }
    return in.peek() == std::ifstream::traits_type::eof();
}

} // namespace

int
main(int argc, char* argv[])
{
    uint32_t nNodes = 20;
    uint32_t nFlows = 10;
    uint32_t packetsPerSecond = 100;
    double speed = 5.0;
    uint32_t protocolVersion = 1;
    uint32_t runId = 1;

    double simTime = 60.0;
    uint32_t packetSize = 256;
    double txRange = 120.0;
    double coverageScale = 3.0;
    double initialEnergyJ = 100.0;
    std::string csvFile = "ff-aodv-experiment-results.csv";
    bool verbose = false;

    CommandLine cmd(__FILE__);
    cmd.AddValue("nNodes", "Number of nodes", nNodes);
    cmd.AddValue("nFlows", "Number of UDP flows", nFlows);
    cmd.AddValue("packetsPerSecond", "Packets per second per flow", packetsPerSecond);
    cmd.AddValue("speed", "Gauss-Markov mean speed in m/s", speed);
    cmd.AddValue("protocolVersion", "1: Phase 1, 2: Phase 2", protocolVersion);
    cmd.AddValue("runId", "RNG run identifier", runId);
    cmd.AddValue("simTime", "Simulation time in seconds", simTime);
    cmd.AddValue("packetSize", "UDP payload bytes", packetSize);
    cmd.AddValue("txRange", "Radio transmission range for range-loss model", txRange);
    cmd.AddValue("coverageScale", "Area side = coverageScale * txRange", coverageScale);
    cmd.AddValue("initialEnergy", "Initial node energy in Joules", initialEnergyJ);
    cmd.AddValue("csvFile", "CSV output file path", csvFile);
    cmd.AddValue("verbose", "Enable AODV logic logs", verbose);
    cmd.Parse(argc, argv);

    if (protocolVersion != 1 && protocolVersion != 2)
    {
        NS_LOG_UNCOND("ERROR: protocolVersion must be 1 or 2");
        return 1;
    }
    if (nNodes < 2 || nFlows == 0 || packetsPerSecond == 0 || simTime <= 0.0 || txRange <= 0.0 ||
        coverageScale <= 0.0)
    {
        NS_LOG_UNCOND("ERROR: invalid simulation parameters");
        return 1;
    }

    uint64_t maxPairs = static_cast<uint64_t>(nNodes) * static_cast<uint64_t>(nNodes - 1);
    if (nFlows > maxPairs)
    {
        NS_LOG_UNCOND("ERROR: nFlows exceeds unique source-destination pairs");
        return 1;
    }

    if (verbose)
    {
        LogComponentEnable("AodvRoutingProtocol", LOG_LEVEL_LOGIC);
    }

    RngSeedManager::SetSeed(12345);
    RngSeedManager::SetRun(runId);

    double areaSide = coverageScale * txRange;

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
    phy.Set("TxPowerStart", DoubleValue(16.0));
    phy.Set("TxPowerEnd", DoubleValue(16.0));

    YansWifiChannelHelper channel;
    channel.SetPropagationDelay("ns3::ConstantSpeedPropagationDelayModel");
    channel.AddPropagationLoss("ns3::RangePropagationLossModel", "MaxRange", DoubleValue(txRange));
    phy.SetChannel(channel.Create());

    NetDeviceContainer devices = wifi.Install(phy, mac, nodes);

    MobilityHelper mobility;
    mobility.SetPositionAllocator("ns3::RandomRectanglePositionAllocator",
                                  "X",
                                  StringValue("ns3::UniformRandomVariable[Min=0.0|Max=" +
                                              std::to_string(areaSide) + "]"),
                                  "Y",
                                  StringValue("ns3::UniformRandomVariable[Min=0.0|Max=" +
                                              std::to_string(areaSide) + "]"));

    if (speed <= 0.0)
    {
        mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    }
    else
    {
        mobility.SetMobilityModel("ns3::GaussMarkovMobilityModel",
                                  "Bounds",
                                  BoxValue(Box(0, areaSide, 0, areaSide, 0, 0)),
                                  "TimeStep",
                                  TimeValue(Seconds(0.5)),
                                  "Alpha",
                                  DoubleValue(0.85),
                                  "MeanVelocity",
                                  StringValue(ConstantRv(speed)),
                                  "MeanDirection",
                                  StringValue("ns3::UniformRandomVariable[Min=0.0|Max=6.283185307]"),
                                  "MeanPitch",
                                  StringValue(ConstantRv(0.0)),
                                  "NormalVelocity",
                                  StringValue("ns3::NormalRandomVariable[Mean=0.0|Variance=0.5|Bound=1.0]"),
                                  "NormalDirection",
                                  StringValue("ns3::NormalRandomVariable[Mean=0.0|Variance=0.5|Bound=1.0]"),
                                  "NormalPitch",
                                  StringValue("ns3::NormalRandomVariable[Mean=0.0|Variance=0.0|Bound=0.0]"));
    }
    mobility.Install(nodes);

    BasicEnergySourceHelper energyHelper;
    energyHelper.Set("BasicEnergySourceInitialEnergyJ", DoubleValue(initialEnergyJ));
    EnergySourceContainer energySources = energyHelper.Install(nodes);

    WifiRadioEnergyModelHelper radioEnergyHelper;
    for (uint32_t i = 0; i < nNodes; ++i)
    {
        radioEnergyHelper.Install(devices.Get(i), energySources.Get(i));
    }

    AodvHelper aodv;
    if (protocolVersion == 1)
    {
        aodv.Set("Alpha", DoubleValue(0.6));
        aodv.Set("Beta", DoubleValue(0.4));
        aodv.Set("Gamma", DoubleValue(0.0));
    }
    else
    {
        aodv.Set("Alpha", DoubleValue(0.5));
        aodv.Set("Beta", DoubleValue(0.3));
        aodv.Set("Gamma", DoubleValue(0.2));
    }
    aodv.Set("InitialEnergy", DoubleValue(initialEnergyJ));
    aodv.Set("MaxVelocity", DoubleValue(std::max(50.0, speed)));
    aodv.Set("EnableHello", BooleanValue(true));

    InternetStackHelper internet;
    internet.SetRoutingHelper(aodv);
    internet.Install(nodes);

    Ipv4AddressHelper ipv4;
    ipv4.SetBase("10.1.0.0", "255.255.0.0");
    Ipv4InterfaceContainer interfaces = ipv4.Assign(devices);

    std::vector<FlowDefinition> flows;
    flows.reserve(nFlows);
    std::set<uint64_t> usedPairs;
    Ptr<UniformRandomVariable> pairRv = CreateObject<UniformRandomVariable>();

    uint16_t firstPort = 9000;
    uint32_t attempts = 0;
    while (flows.size() < nFlows && attempts < nFlows * 200)
    {
        ++attempts;
        uint32_t src = pairRv->GetInteger(0, nNodes - 1);
        uint32_t dst = pairRv->GetInteger(0, nNodes - 1);
        if (src == dst)
        {
            continue;
        }

        uint64_t key = (static_cast<uint64_t>(src) << 32) | static_cast<uint64_t>(dst);
        if (!usedPairs.insert(key).second)
        {
            continue;
        }

        FlowDefinition flow;
        flow.src = src;
        flow.dst = dst;
        flow.port = static_cast<uint16_t>(firstPort + flows.size());
        flow.startTime = 5.0 + 0.02 * static_cast<double>(flows.size());
        flows.push_back(flow);
    }

    if (flows.size() != nFlows)
    {
        NS_LOG_UNCOND("ERROR: unable to generate enough unique flows");
        return 1;
    }

    double appStop = std::max(1.0, simTime - 1.0);
    for (const auto& flow : flows)
    {
        PacketSinkHelper sink("ns3::UdpSocketFactory",
                              InetSocketAddress(Ipv4Address::GetAny(), flow.port));
        ApplicationContainer sinkApps = sink.Install(nodes.Get(flow.dst));
        sinkApps.Start(Seconds(0.0));
        sinkApps.Stop(Seconds(simTime));

        double start = std::min(flow.startTime, appStop - 0.01);
        double duration = std::max(0.1, appStop - start);
        uint32_t maxPackets = std::max<uint32_t>(
            1,
            static_cast<uint32_t>(duration * static_cast<double>(packetsPerSecond)));

        UdpClientHelper client(interfaces.GetAddress(flow.dst), flow.port);
        client.SetAttribute("MaxPackets", UintegerValue(maxPackets));
        client.SetAttribute("Interval",
                            TimeValue(Seconds(1.0 / static_cast<double>(packetsPerSecond))));
        client.SetAttribute("PacketSize", UintegerValue(packetSize));

        ApplicationContainer srcApps = client.Install(nodes.Get(flow.src));
        srcApps.Start(Seconds(start));
        srcApps.Stop(Seconds(appStop));
    }

    FlowMonitorHelper flowHelper;
    Ptr<FlowMonitor> monitor = flowHelper.InstallAll();

    Simulator::Stop(Seconds(simTime));
    Simulator::Run();

    monitor->CheckForLostPackets();
    Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(flowHelper.GetClassifier());
    FlowMonitor::FlowStatsContainer stats = monitor->GetFlowStats();

    uint64_t totalTxPackets = 0;
    uint64_t totalRxPackets = 0;
    uint64_t totalTxBytes = 0;
    uint64_t totalRxBytes = 0;
    double totalDelaySeconds = 0.0;

    uint16_t lastPort = static_cast<uint16_t>(firstPort + nFlows - 1);
    for (const auto& entry : stats)
    {
        Ipv4FlowClassifier::FiveTuple tuple = classifier->FindFlow(entry.first);
        if (tuple.protocol != 17)
        {
            continue;
        }
        if (tuple.destinationPort < firstPort || tuple.destinationPort > lastPort)
        {
            continue;
        }

        totalTxPackets += entry.second.txPackets;
        totalRxPackets += entry.second.rxPackets;
        totalTxBytes += entry.second.txBytes;
        totalRxBytes += entry.second.rxBytes;
        totalDelaySeconds += entry.second.delaySum.GetSeconds();
    }

    double activeDuration = std::max(0.1, simTime - 5.0);
    double throughputMbps = (totalRxBytes * 8.0) / (activeDuration * 1e6);
    double avgDelayMs =
        (totalRxPackets > 0) ? (totalDelaySeconds / static_cast<double>(totalRxPackets)) * 1000.0 : 0.0;
    double pdr = (totalTxPackets > 0)
                     ? static_cast<double>(totalRxPackets) / static_cast<double>(totalTxPackets)
                     : 0.0;
    uint64_t droppedPackets = (totalTxPackets > totalRxPackets) ? (totalTxPackets - totalRxPackets) : 0;
    double dropRatio = (totalTxPackets > 0)
                           ? static_cast<double>(droppedPackets) / static_cast<double>(totalTxPackets)
                           : 0.0;

    double totalEnergyConsumed = 0.0;
    for (uint32_t i = 0; i < energySources.GetN(); ++i)
    {
        Ptr<EnergySource> source = energySources.Get(i);
        totalEnergyConsumed +=
            std::max(0.0, source->GetInitialEnergy() - source->GetRemainingEnergy());
    }

    bool writeHeader = NeedsCsvHeader(csvFile);
    std::ofstream out(csvFile.c_str(), std::ios::app);
    if (!out.good())
    {
        NS_LOG_UNCOND("ERROR: cannot open CSV file: " << csvFile);
        Simulator::Destroy();
        return 1;
    }

    if (writeHeader)
    {
        out << "protocol_version,n_nodes,n_flows,packets_per_second,speed_mps,tx_range_m,coverage_scale,sim_time_s,"
            << "tx_packets,rx_packets,throughput_mbps,avg_e2e_delay_ms,pdr,drop_ratio,total_energy_consumption_j\n";
    }

    out << std::fixed << std::setprecision(6) << protocolVersion << ',' << nNodes << ',' << nFlows
        << ',' << packetsPerSecond << ',' << speed << ',' << txRange << ',' << coverageScale << ','
        << simTime << ',' << totalTxPackets << ',' << totalRxPackets << ',' << throughputMbps << ','
        << avgDelayMs << ',' << pdr << ',' << dropRatio << ',' << totalEnergyConsumed << '\n';
    out.close();

    NS_LOG_UNCOND("Run complete: protocol=" << protocolVersion << " nodes=" << nNodes
                                            << " flows=" << nFlows << " pps=" << packetsPerSecond
                                            << " speed=" << speed << " m/s");
    NS_LOG_UNCOND("  Throughput=" << throughputMbps << " Mbps"
                                   << " AvgDelay=" << avgDelayMs << " ms"
                                   << " PDR=" << pdr << " DropRatio=" << dropRatio
                                   << " EnergyConsumed=" << totalEnergyConsumed << " J");
    NS_LOG_UNCOND("  CSV=" << csvFile);

    Simulator::Destroy();
    return 0;
}
