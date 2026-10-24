#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/tcp-vegas.h"

#include <fstream>
#include <iostream>
#include <string>
#include <vector>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("VegasExperiment4");

static bool
RunSimulation(uint32_t flowCount, std::ofstream& results)
{
    NodeContainer nodes;
    nodes.Create(2);

    PointToPointHelper p2p;
    p2p.SetDeviceAttribute("DataRate", StringValue("10Mbps"));
    p2p.SetChannelAttribute("Delay", StringValue("10ms"));

    NetDeviceContainer devices = p2p.Install(nodes);

    InternetStackHelper internet;
    internet.Install(nodes);

    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces = address.Assign(devices);

    std::vector<Ptr<PacketSink>> sinks;
    sinks.reserve(flowCount);
    for (uint32_t flowId = 0; flowId < flowCount; ++flowId)
    {
        const uint16_t port = static_cast<uint16_t>(5000 + flowId);
        PacketSinkHelper sinkHelper(
            "ns3::TcpSocketFactory",
            InetSocketAddress(Ipv4Address::GetAny(), port));
        ApplicationContainer sinkApp = sinkHelper.Install(nodes.Get(1));
        sinkApp.Start(Seconds(0.0));
        sinkApp.Stop(Seconds(10.0));
        sinks.push_back(DynamicCast<PacketSink>(sinkApp.Get(0)));

        OnOffHelper client("ns3::TcpSocketFactory",
                           InetSocketAddress(interfaces.GetAddress(1), port));
        client.SetAttribute("DataRate", StringValue("5Mbps"));
        client.SetAttribute("PacketSize", UintegerValue(1024));
        ApplicationContainer clientApp = client.Install(nodes.Get(0));
        clientApp.Start(Seconds(1.0));
        clientApp.Stop(Seconds(10.0));
    }

    Simulator::Stop(Seconds(10.0));
    Simulator::Run();

    std::vector<uint64_t> receivedBytes;
    std::vector<double> throughputsMbps;
    receivedBytes.reserve(flowCount);
    throughputsMbps.reserve(flowCount);

    uint64_t aggregateBytes = 0;
    double aggregateThroughputMbps = 0.0;
    double throughputSquaresSum = 0.0;
    for (const auto& sink : sinks)
    {
        const uint64_t bytes = sink->GetTotalRx();
        const double throughputMbps = (bytes * 8.0) / (10.0 * 1000000.0);
        receivedBytes.push_back(bytes);
        throughputsMbps.push_back(throughputMbps);
        aggregateBytes += bytes;
        aggregateThroughputMbps += throughputMbps;
        throughputSquaresSum += throughputMbps * throughputMbps;
    }

    const double throughputSum = aggregateThroughputMbps;
    const double fairness =
        flowCount == 1
            ? 1.0
            : (throughputSum * throughputSum) /
                  (flowCount * throughputSquaresSum);

    for (uint32_t flowId = 0; flowId < flowCount; ++flowId)
    {
        results << "TCP Vegas," << flowCount << ',' << flowId + 1 << ','
                << receivedBytes[flowId] << ',' << throughputsMbps[flowId]
                << ',' << aggregateThroughputMbps << ',' << fairness << '\n';
        std::cout << "Flows: " << flowCount << ", flow " << flowId + 1
                  << ", received bytes: " << receivedBytes[flowId]
                  << ", throughput: " << throughputsMbps[flowId] << " Mbps"
                  << std::endl;
    }
    std::cout << "Flows: " << flowCount
              << ", aggregate received bytes: " << aggregateBytes
              << ", aggregate throughput: " << aggregateThroughputMbps
              << " Mbps, Jain fairness: " << fairness << std::endl;

    Simulator::Destroy();
    return static_cast<bool>(results);
}

int
main(int argc, char* argv[])
{
    CommandLine cmd(__FILE__);
    cmd.Parse(argc, argv);

    Config::SetDefault("ns3::TcpL4Protocol::SocketType",
                       TypeIdValue(TcpVegas::GetTypeId()));

    const std::string resultsPath =
        "scratch/results/tcp-vegas/experiment-4/summary.csv";
    std::ofstream results(resultsPath);
    if (!results.is_open())
    {
        std::cerr << "Could not open results file: " << resultsPath << std::endl;
        return 1;
    }

    results << "Algorithm,Number_of_Flows,Flow_ID,Received_Bytes,"
               "Throughput_Mbps,Aggregate_Throughput_Mbps,Jain_Fairness\n";

    for (const uint32_t flowCount : {1, 2, 4})
    {
        if (!RunSimulation(flowCount, results))
        {
            std::cerr << "Failed while writing results file: " << resultsPath
                      << std::endl;
            return 1;
        }
    }

    std::cout << "Results saved to " << resultsPath << std::endl;
    return 0;
}
