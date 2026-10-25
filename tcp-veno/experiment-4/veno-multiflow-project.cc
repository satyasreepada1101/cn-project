
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/tcp-veno.h"

#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <cmath>

using namespace ns3;

int main(int argc, char *argv[])
{
    uint32_t packetSize = 1024;
    uint16_t basePort = 5000;

    double simulationTime = 20.0;
    double senderStartTime = 1.0;

    double appRate = 5.0;
    double linkRateMbps = 10.0;
    double delayMs = 10.0;

    uint32_t numFlows = 1;

    std::string outputFile =
        "scratch/results/experiment-4/veno_1flow.csv";

    CommandLine cmd;

    cmd.AddValue("numFlows",
                "Number of competing TCP flows",
                numFlows);

    cmd.AddValue("linkRateMbps",
                "Link bandwidth in Mbps",
                linkRateMbps);

    cmd.AddValue("delayMs",
                "Propagation delay in milliseconds",
                delayMs);

    cmd.AddValue("outputFile",
                "CSV output file",
                outputFile);

    cmd.Parse(argc, argv);

    if (numFlows == 0)
    {
        std::cerr << "Number of flows must be greater than zero."
                  << std::endl;
        return 1;
    }

    // Select TCP Veno
    Config::SetDefault(
        "ns3::TcpL4Protocol::SocketType",
        TypeIdValue(TcpVeno::GetTypeId()));

    // Create two nodes
    NodeContainer nodes;
    nodes.Create(2);

    // Point-to-point shared link
    PointToPointHelper pointToPoint;

    pointToPoint.SetDeviceAttribute(
        "DataRate",
        StringValue(std::to_string(linkRateMbps) + "Mbps"));

    pointToPoint.SetChannelAttribute(
        "Delay",
        StringValue(std::to_string(delayMs) + "ms"));

    NetDeviceContainer devices =
        pointToPoint.Install(nodes);

    // Internet stack
    InternetStackHelper internet;
    internet.Install(nodes);

    // Assign IP addresses
    Ipv4AddressHelper address;

    address.SetBase("10.1.1.0", "255.255.255.0");

    Ipv4InterfaceContainer interfaces =
        address.Assign(devices);

    // Store receiver applications
    ApplicationContainer sinkApps;

    // Store sender applications
    ApplicationContainer senderApps;

    // Create multiple TCP flows
    for (uint32_t i = 0; i < numFlows; i++)
    {
        uint16_t port = basePort + i;

        // Receiver
        PacketSinkHelper sinkHelper(
            "ns3::TcpSocketFactory",
            InetSocketAddress(
                Ipv4Address::GetAny(),
                port));

        ApplicationContainer sink =
            sinkHelper.Install(nodes.Get(1));

        sink.Start(Seconds(0.0));
        sink.Stop(Seconds(simulationTime));

        sinkApps.Add(sink);

        
        // Sender
        OnOffHelper senderHelper(
            "ns3::TcpSocketFactory",
            InetSocketAddress(
                interfaces.GetAddress(1),
                port));

        senderHelper.SetAttribute(
            "DataRate",
            StringValue(std::to_string(appRate) + "Mbps"));

        senderHelper.SetAttribute(
            "PacketSize",
            UintegerValue(packetSize));

        senderHelper.SetAttribute(
            "OnTime",
            StringValue(
                "ns3::ConstantRandomVariable[Constant=1]"));

        senderHelper.SetAttribute(
            "OffTime",
            StringValue(
                "ns3::ConstantRandomVariable[Constant=0]"));

        ApplicationContainer senderApp =
            senderHelper.Install(nodes.Get(0));

        senderApp.Start(Seconds(senderStartTime));
        senderApp.Stop(Seconds(simulationTime));

        senderApps.Add(senderApp);

        
    }

    // Run simulation
    Simulator::Stop(Seconds(simulationTime));

    Simulator::Run();

    // Measurement window
    double measurementWindow =
        simulationTime - senderStartTime;

    // Store per-flow throughput
    std::vector<double> flowThroughputs;

    double totalThroughput = 0.0;

    double sumThroughput = 0.0;
    double sumSquaredThroughput = 0.0;

    std::cout << "\n========== TCP VENO MULTI-FLOW RESULTS ==========\n";

    std::cout << "TCP Algorithm: Veno\n";
    std::cout << "Number of Flows: " << numFlows << "\n";
    std::cout << "Link Bandwidth: " << linkRateMbps << " Mbps\n";
    std::cout << "Propagation Delay: " << delayMs << " ms\n";
    std::cout << "Application Rate per Flow: " << appRate << " Mbps\n";

    // Calculate throughput for each flow
    for (uint32_t i = 0; i < numFlows; i++)
    {
        Ptr<PacketSink> sink =
            DynamicCast<PacketSink>(sinkApps.Get(i));

        uint64_t receivedBytes =
            sink->GetTotalRx();

        double throughputMbps =
            (receivedBytes * 8.0) /
            (measurementWindow * 1000000.0);

        flowThroughputs.push_back(throughputMbps);

        totalThroughput += throughputMbps;

        sumThroughput += throughputMbps;

        sumSquaredThroughput +=
            throughputMbps * throughputMbps;

        std::cout << "Flow " << i + 1
                  << " Received Bytes: "
                  << receivedBytes << "\n";

        std::cout << "Flow " << i + 1
                  << " Throughput: "
                  << throughputMbps << " Mbps\n";
    }

    // Jain's Fairness Index
    double fairnessIndex = 0.0;

    if (sumSquaredThroughput > 0.0)
    {
        fairnessIndex =
            (sumThroughput * sumThroughput) /
            (numFlows * sumSquaredThroughput);
    }

    std::cout << "Total Throughput: "
              << totalThroughput << " Mbps\n";

    std::cout << "Jain's Fairness Index: "
              << fairnessIndex << "\n";

    std::cout << "Measurement Window: "
              << measurementWindow << " seconds\n";

    std::cout << "===============================================\n";

    // Save results to CSV
    std::ofstream csvFile(outputFile);

    if (!csvFile.is_open())
    {
        std::cerr << "Error opening output file: "
                  << outputFile << std::endl;

        Simulator::Destroy();
        return 1;
    }

    csvFile
        << "Algorithm,"
        << "NumberOfFlows,"
        << "Bandwidth_Mbps,"
        << "Delay_ms,"
        << "PacketSize_bytes,"
        << "SimulationTime_s,"
        << "MeasurementWindow_s,"
        << "FlowID,"
        << "ReceivedBytes,"
        << "Throughput_Mbps,"
        << "TotalThroughput_Mbps,"
        << "JainFairnessIndex\n";

    // Write one row per flow
    for (uint32_t i = 0; i < numFlows; i++)
    {
        Ptr<PacketSink> sink =
            DynamicCast<PacketSink>(sinkApps.Get(i));

        uint64_t receivedBytes =
            sink->GetTotalRx();

        double throughputMbps =
            flowThroughputs[i];

        csvFile
            << "Veno,"
            << numFlows << ","
            << linkRateMbps << ","
            << delayMs << ","
            << packetSize << ","
            << simulationTime << ","
            << measurementWindow << ","
            << i + 1 << ","
            << receivedBytes << ","
            << throughputMbps << ","
            << totalThroughput << ","
            << fairnessIndex << "\n";
    }

    csvFile.close();

    std::cout << "Results saved to: "
              << outputFile << std::endl;

    Simulator::Destroy();

    return 0;
}
