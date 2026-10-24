#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/tcp-vegas.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include <fstream>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("VegasProject");
std::ofstream cwndFile;

static void
CwndChange(uint32_t oldCwnd, uint32_t newCwnd)
{
    cwndFile << Simulator::Now().GetSeconds()
             << "\t"
             << newCwnd
             << std::endl;
}
int main(int argc, char *argv[])
{
    double bandwidth = 10.0;

    CommandLine cmd(__FILE__);
    cmd.AddValue("bandwidth", "Bottleneck bandwidth in Mbps", bandwidth);
    cmd.Parse(argc, argv);

    NS_LOG_UNCOND("TCP Vegas Experiment 1 Started");
    NS_LOG_UNCOND("Bottleneck bandwidth: " << bandwidth << " Mbps");

Config::SetDefault("ns3::TcpL4Protocol::SocketType",
                   TypeIdValue(TcpVegas::GetTypeId()));

    NodeContainer nodes;
    nodes.Create(2);

    PointToPointHelper p2p;
    p2p.SetDeviceAttribute(
    "DataRate",
    DataRateValue(DataRate(std::to_string(bandwidth) + "Mbps")));
    p2p.SetChannelAttribute("Delay", StringValue("10ms"));

    NetDeviceContainer devices = p2p.Install(nodes);

    InternetStackHelper internet;
    internet.Install(nodes);

    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");

    Ipv4InterfaceContainer interfaces = address.Assign(devices);
uint16_t port = 5000;

PacketSinkHelper sinkHelper(
    "ns3::TcpSocketFactory",
    InetSocketAddress(Ipv4Address::GetAny(), port));

ApplicationContainer sinkApp = sinkHelper.Install(nodes.Get(1));
sinkApp.Start(Seconds(0.0));
sinkApp.Stop(Seconds(10.0));

OnOffHelper client(
    "ns3::TcpSocketFactory",
    InetSocketAddress(interfaces.GetAddress(1), port));

client.SetAttribute("DataRate", StringValue("5Mbps"));
client.SetAttribute("PacketSize", UintegerValue(1024));

ApplicationContainer clientApp = client.Install(nodes.Get(0));
clientApp.Start(Seconds(1.0));
clientApp.Stop(Seconds(10.0));
Ptr<Socket> ns3TcpSocket =
    Socket::CreateSocket(nodes.Get(0),
                         TcpSocketFactory::GetTypeId());

ns3TcpSocket->TraceConnectWithoutContext(
    "CongestionWindow",
    MakeCallback(&CwndChange));
    NS_LOG_UNCOND("Created 2 nodes");
    NS_LOG_UNCOND("Connected nodes with "
              << bandwidth << "Mbps, 10ms link");

    NS_LOG_UNCOND("Node 0 IP: " << interfaces.GetAddress(0));
    NS_LOG_UNCOND("Node 1 IP: " << interfaces.GetAddress(1));

Simulator::Stop(Seconds(10.0));
NS_LOG_UNCOND("TCP Vegas traffic started");
Simulator::Run();

Ptr<PacketSink> sink =
    DynamicCast<PacketSink>(sinkApp.Get(0));

double throughput =
    (sink->GetTotalRx() * 8.0) / (10.0 * 1000000);

NS_LOG_UNCOND("Total Bytes Received: "
              << sink->GetTotalRx());

NS_LOG_UNCOND("Throughput: "
              << throughput
              << " Mbps");

Simulator::Destroy();

    return 0;
}
