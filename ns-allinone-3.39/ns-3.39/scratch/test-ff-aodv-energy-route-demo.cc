#include "ns3/aodv-module.h"
#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/energy-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/netanim-module.h"
#include "ns3/network-module.h"
#include "ns3/udp-header.h"
#include "ns3/wifi-module.h"

#include <algorithm>
#include <limits>
#include <set>
#include <sstream>
#include <string>
#include <vector>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("FfAodvEnergyRouteDemo");

static Ipv4Address g_srcIp;
static Ipv4Address g_dstIp;
static uint16_t g_appPort = 9000;
static uint16_t g_warmupPort = 9001;
static std::set<uint32_t> g_forwarders;

static uint32_t
ExtractNodeId(const std::string& context)
{
    const std::string token = "/NodeList/";
    std::size_t start = context.find(token);
    if (start == std::string::npos)
    {
        return std::numeric_limits<uint32_t>::max();
    }
    start += token.size();
    std::size_t end = context.find('/', start);
    if (end == std::string::npos)
    {
        return std::numeric_limits<uint32_t>::max();
    }
    return static_cast<uint32_t>(std::stoul(context.substr(start, end - start)));
}

static void
TrackDataForward(std::string context,
                 const Ipv4Header& ipHeader,
                 Ptr<const Packet> packet,
                 uint32_t interface)
{
    (void)interface;

    if (ipHeader.GetProtocol() != 17)
    {
        return;
    }
    if (ipHeader.GetSource() != g_srcIp || ipHeader.GetDestination() != g_dstIp)
    {
        return;
    }

    Ptr<Packet> copy = packet->Copy();
    UdpHeader udp;
    if (copy->PeekHeader(udp) == 0)
    {
        return;
    }
    if (udp.GetDestinationPort() != g_appPort)
    {
        return;
    }

    uint32_t nodeId = ExtractNodeId(context);
    if (nodeId != std::numeric_limits<uint32_t>::max())
    {
        g_forwarders.insert(nodeId);
    }
}

int
main(int argc, char* argv[])
{
    std::string mode = "energy"; // energy or hop
    double simTime = 30.0;
    bool verbose = false;

    CommandLine cmd(__FILE__);
    cmd.AddValue("mode", "Routing preference: energy or hop", mode);
    cmd.AddValue("time", "Simulation time in seconds", simTime);
    cmd.AddValue("verbose", "Enable AODV logic logs", verbose);
    cmd.Parse(argc, argv);

    if (mode != "energy" && mode != "hop")
    {
        NS_LOG_UNCOND("Invalid mode. Use --mode=energy or --mode=hop");
        return 1;
    }

    if (verbose)
    {
        LogComponentEnable("AodvRoutingProtocol", LOG_LEVEL_LOGIC);
    }

    Config::SetDefault("ns3::aodv::RoutingProtocol::TtlStart", UintegerValue(10));
    Config::SetDefault("ns3::aodv::RoutingProtocol::TtlIncrement", UintegerValue(1));
    Config::SetDefault("ns3::aodv::RoutingProtocol::TtlThreshold", UintegerValue(10));

    NodeContainer nodes;
    nodes.Create(6);

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
    channel.AddPropagationLoss("ns3::RangePropagationLossModel", "MaxRange", DoubleValue(45.0));
    phy.SetChannel(channel.Create());

    NetDeviceContainer devices = wifi.Install(phy, mac, nodes);

    MobilityHelper mobility;
    Ptr<ListPositionAllocator> positions = CreateObject<ListPositionAllocator>();

    // Topology:
    // Short path (2 hops): 0 -> 1 -> 5, where node 1 is intentionally low-energy.
    // Longer path (4 hops): 0 -> 2 -> 3 -> 4 -> 5, with healthy-energy nodes.
    positions->Add(Vector(0.0, 0.0, 0.0));   // 0: source
    positions->Add(Vector(40.0, 0.0, 0.0));  // 1: low-energy shortcut node
    positions->Add(Vector(0.0, 40.0, 0.0));  // 2
    positions->Add(Vector(40.0, 50.0, 0.0)); // 3
    positions->Add(Vector(80.0, 40.0, 0.0)); // 4
    positions->Add(Vector(80.0, 0.0, 0.0));  // 5: destination

    mobility.SetPositionAllocator(positions);
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(nodes);

    BasicEnergySourceHelper energyHelper;
    energyHelper.Set("BasicEnergySourceInitialEnergyJ", DoubleValue(100.0));
    EnergySourceContainer sources = energyHelper.Install(nodes);

    // Force the shortcut relay to have low residual energy.
    Ptr<BasicEnergySource> lowNodeEnergy = DynamicCast<BasicEnergySource>(sources.Get(1));
    NS_ASSERT(lowNodeEnergy);
    lowNodeEnergy->SetInitialEnergy(5.0);

    double alpha = (mode == "energy") ? 0.85 : 0.0;
    double beta = (mode == "energy") ? 0.15 : 1.0;

    AodvHelper aodv;
    aodv.Set("Alpha", DoubleValue(alpha));
    aodv.Set("Beta", DoubleValue(beta));
    aodv.Set("Gamma", DoubleValue(0.0));
    aodv.Set("InitialEnergy", DoubleValue(100.0));
    aodv.Set("MaxVelocity", DoubleValue(1.0));
    aodv.Set("DestinationOnly", BooleanValue(true));

    InternetStackHelper internet;
    internet.SetRoutingHelper(aodv);
    internet.Install(nodes);

    Ipv4AddressHelper ipv4;
    ipv4.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer ifaces = ipv4.Assign(devices);

    g_srcIp = ifaces.GetAddress(0);
    g_dstIp = ifaces.GetAddress(5);

    PacketSinkHelper sinkHelper("ns3::UdpSocketFactory",
                                InetSocketAddress(Ipv4Address::GetAny(), g_appPort));
    ApplicationContainer sinkApps = sinkHelper.Install(nodes.Get(5));
    sinkApps.Start(Seconds(0.0));
    sinkApps.Stop(Seconds(simTime));

    PacketSinkHelper warmupSinkHelper("ns3::UdpSocketFactory",
                                      InetSocketAddress(Ipv4Address::GetAny(), g_warmupPort));
    ApplicationContainer warmupSinkApps = warmupSinkHelper.Install(nodes.Get(5));
    warmupSinkApps.Start(Seconds(0.0));
    warmupSinkApps.Stop(Seconds(simTime));

    // Warm-up probe to trigger route discovery before the tracked flow starts.
    UdpClientHelper warmupClient(g_dstIp, g_warmupPort);
    warmupClient.SetAttribute("MaxPackets", UintegerValue(1));
    warmupClient.SetAttribute("Interval", TimeValue(Seconds(1.0)));
    warmupClient.SetAttribute("PacketSize", UintegerValue(64));
    ApplicationContainer warmupClientApps = warmupClient.Install(nodes.Get(0));
    warmupClientApps.Start(Seconds(2.0));
    warmupClientApps.Stop(Seconds(3.0));

    OnOffHelper onoff("ns3::UdpSocketFactory", InetSocketAddress(g_dstIp, g_appPort));
    onoff.SetAttribute("DataRate", StringValue("128kbps"));
    onoff.SetAttribute("PacketSize", UintegerValue(256));
    onoff.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
    onoff.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
    ApplicationContainer srcApps = onoff.Install(nodes.Get(0));
    srcApps.Start(Seconds(5.0));
    srcApps.Stop(Seconds(simTime - 1.0));

    Config::Connect("/NodeList/*/$ns3::Ipv4L3Protocol/UnicastForward",
                    MakeCallback(&TrackDataForward));

    std::string animFile =
        (mode == "energy") ? "ff-energy-route-demo-energy.xml" : "ff-energy-route-demo-hop.xml";
    AnimationInterface anim(animFile);
    anim.EnablePacketMetadata(true);
    anim.SetMaxPktsPerTraceFile(1000000);

    for (uint32_t i = 0; i < nodes.GetN(); ++i)
    {
        anim.UpdateNodeSize(i, 8.0, 8.0);
    }

    anim.UpdateNodeDescription(0, "SRC");
    anim.UpdateNodeDescription(1, "LOW-E shortcut");
    anim.UpdateNodeDescription(2, "ALT-1");
    anim.UpdateNodeDescription(3, "ALT-2");
    anim.UpdateNodeDescription(4, "ALT-3");
    anim.UpdateNodeDescription(5, "DST");

    anim.UpdateNodeColor(0, 0, 120, 255);
    anim.UpdateNodeColor(5, 0, 120, 255);
    anim.UpdateNodeColor(1, 220, 0, 0);
    anim.UpdateNodeColor(2, 0, 170, 0);
    anim.UpdateNodeColor(3, 0, 170, 0);
    anim.UpdateNodeColor(4, 0, 170, 0);

    std::string routeFile =
        (mode == "energy") ? "ff-energy-route-demo-energy.routes" : "ff-energy-route-demo-hop.routes";
    Ptr<OutputStreamWrapper> routeStream =
        Create<OutputStreamWrapper>(routeFile, std::ios::out);
    Ipv4RoutingHelper::PrintRoutingTableAllAt(Seconds(10.0), routeStream, Time::S);

    NS_LOG_UNCOND("=== FF-AODV Energy Route Demo ===");
    NS_LOG_UNCOND("Mode        : " << mode);
    NS_LOG_UNCOND("Alpha/Beta  : " << alpha << " / " << beta);
    NS_LOG_UNCOND("Node 1 energy: " << sources.Get(1)->GetRemainingEnergy() << " J (low)");
    NS_LOG_UNCOND("Other nodes : 100 J");
    NS_LOG_UNCOND("Candidate paths:");
    NS_LOG_UNCOND("  Short (low energy): 0 -> 1 -> 5");
    NS_LOG_UNCOND("  Long  (healthy)   : 0 -> 2 -> 3 -> 4 -> 5");

    Simulator::Stop(Seconds(simTime));
    Simulator::Run();

    Ptr<PacketSink> sink = DynamicCast<PacketSink>(sinkApps.Get(0));
    NS_ASSERT(sink);

    std::vector<uint32_t> forwarderList(g_forwarders.begin(), g_forwarders.end());
    std::sort(forwarderList.begin(), forwarderList.end());

    NS_LOG_UNCOND("\n--- Data Forwarders for Flow 0->5 (UDP/" << g_appPort << ") ---");
    if (forwarderList.empty())
    {
        NS_LOG_UNCOND("No forwarders observed.");
    }
    else
    {
        std::ostringstream oss;
        for (std::size_t i = 0; i < forwarderList.size(); ++i)
        {
            if (i > 0)
            {
                oss << ", ";
            }
            oss << forwarderList[i];
        }
        NS_LOG_UNCOND("Forwarding node IDs: [" << oss.str() << "]");
    }

    bool usedShortcut = g_forwarders.count(1) > 0;
    bool usedLongRoute = g_forwarders.count(2) && g_forwarders.count(3) && g_forwarders.count(4);

    NS_LOG_UNCOND("Route hint  : "
                  << (usedLongRoute ? "Long healthy path observed" : "Long healthy path not fully seen")
                  << ", " << (usedShortcut ? "Shortcut used" : "Shortcut not used"));
    NS_LOG_UNCOND("Total Rx at destination: " << sink->GetTotalRx() << " bytes");
    NS_LOG_UNCOND("NetAnim file: " << animFile);
    NS_LOG_UNCOND("Route table file: " << routeFile);

    Simulator::Destroy();
    return 0;
}
