#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/tcp-vegas.h"

#include <fstream>
#include <iostream>
#include <string>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("VegasExperiment2");

static uint64_t
RunSimulation(uint32_t delayMs)
{
    NodeContainer nodes;
    nodes.Create(2);

    PointToPointHelper p2p;
    p2p.SetDeviceAttribute("DataRate", StringValue("10Mbps"));
    p2p.SetChannelAttribute("Delay", StringValue(std::to_string(delayMs) + "ms"));

    NetDeviceContainer devices = p2p.Install(nodes);

    InternetStackHelper internet;
    internet.Install(nodes);

    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces = address.Assign(devices);

    constexpr uint16_t port = 5000;
    PacketSinkHelper sinkHelper("ns3::TcpSocketFactory",
                                InetSocketAddress(Ipv4Address::GetAny(), port));
    ApplicationContainer sinkApp = sinkHelper.Install(nodes.Get(1));
    sinkApp.Start(Seconds(0.0));
    sinkApp.Stop(Seconds(10.0));

    OnOffHelper client("ns3::TcpSocketFactory",
                       InetSocketAddress(interfaces.GetAddress(1), port));
    client.SetAttribute("DataRate", StringValue("5Mbps"));
    client.SetAttribute("PacketSize", UintegerValue(1024));
    ApplicationContainer clientApp = client.Install(nodes.Get(0));
    clientApp.Start(Seconds(1.0));
    clientApp.Stop(Seconds(10.0));

    Simulator::Stop(Seconds(10.0));
    Simulator::Run();

    Ptr<PacketSink> sink = DynamicCast<PacketSink>(sinkApp.Get(0));
    const uint64_t receivedBytes = sink->GetTotalRx();
    Simulator::Destroy();

    return receivedBytes;
}

int
main(int argc, char* argv[])
{
    CommandLine cmd(__FILE__);
    cmd.Parse(argc, argv);

    Config::SetDefault("ns3::TcpL4Protocol::SocketType",
                       TypeIdValue(TcpVegas::GetTypeId()));

    const std::string resultsPath =
        "scratch/results/tcp-vegas/experiment-2/summary.csv";
    std::ofstream results(resultsPath);
    if (!results.is_open())
    {
        std::cerr << "Could not open results file: " << resultsPath << std::endl;
        return 1;
    }

    results << "Algorithm,Bottleneck_Delay_ms,Received_Bytes,Throughput_Mbps\n";
    for (const uint32_t delayMs : {10, 50, 100, 200})
    {
        const uint64_t receivedBytes = RunSimulation(delayMs);
        const double throughputMbps =
            (receivedBytes * 8.0) / (10.0 * 1000000.0);

        results << "TCP Vegas," << delayMs << ',' << receivedBytes << ','
                << throughputMbps << '\n';
        std::cout << "Delay: " << delayMs << " ms, received bytes: "
                  << receivedBytes << ", throughput: " << throughputMbps
                  << " Mbps" << std::endl;
    }

    if (!results)
    {
        std::cerr << "Failed while writing results file: " << resultsPath
                  << std::endl;
        return 1;
    }

    std::cout << "Results saved to " << resultsPath << std::endl;
    return 0;
}
