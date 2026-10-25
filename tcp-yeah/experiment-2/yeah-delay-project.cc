#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/tcp-yeah.h"

#include <fstream>
#include <iostream>
#include <string>

using namespace ns3;

int main(int argc, char *argv[])
{
    uint32_t packetSize = 1024;
    uint16_t port = 5000;

    double simulationTime = 10.0;
    double senderStartTime = 1.0;

    double appRate = 5.0;
    double linkRateMbps = 10.0;
    double delayMs = 10.0;

    std::string outputFile =
        "scratch/results/yeah/experiment-2/yeah_10ms.csv";

    CommandLine cmd;

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

    // Select TCP YeAH
    Config::SetDefault(
        "ns3::TcpL4Protocol::SocketType",
        TypeIdValue(TcpYeah::GetTypeId()));

    // Create two nodes
    NodeContainer nodes;
    nodes.Create(2);

    // Point-to-point link
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

    // TCP receiver
    PacketSinkHelper sinkHelper(
        "ns3::TcpSocketFactory",
        InetSocketAddress(Ipv4Address::GetAny(), port));

    ApplicationContainer sinkApp =
        sinkHelper.Install(nodes.Get(1));

    sinkApp.Start(Seconds(0.0));
    sinkApp.Stop(Seconds(simulationTime));

    // TCP sender
    OnOffHelper sender(
        "ns3::TcpSocketFactory",
        InetSocketAddress(interfaces.GetAddress(1), port));

    sender.SetAttribute(
        "DataRate",
        StringValue(std::to_string(appRate) + "Mbps"));

    sender.SetAttribute(
        "PacketSize",
        UintegerValue(packetSize));

    sender.SetAttribute(
        "OnTime",
        StringValue("ns3::ConstantRandomVariable[Constant=1]"));

    sender.SetAttribute(
        "OffTime",
        StringValue("ns3::ConstantRandomVariable[Constant=0]"));

    ApplicationContainer senderApp =
        sender.Install(nodes.Get(0));

    senderApp.Start(Seconds(senderStartTime));
    senderApp.Stop(Seconds(simulationTime));

    // Run simulation
    Simulator::Stop(Seconds(simulationTime));

    Simulator::Run();

    // Retrieve received bytes
    Ptr<PacketSink> sink =
        DynamicCast<PacketSink>(sinkApp.Get(0));

    uint64_t totalBytesReceived =
        sink->GetTotalRx();

    double measurementWindow =
        simulationTime - senderStartTime;

    double throughputMbps =
        (totalBytesReceived * 8.0) /
        (measurementWindow * 1000000.0);

    // Display results
    std::cout << "\n========== TCP YeAH RESULTS ==========\n";

    std::cout << "TCP Algorithm: YeAH\n";

    std::cout << "Link Bandwidth: "
              << linkRateMbps << " Mbps\n";

    std::cout << "Propagation Delay: "
              << delayMs << " ms\n";

    std::cout << "Application Rate: "
              << appRate << " Mbps\n";

    std::cout << "Total Bytes Received: "
              << totalBytesReceived << " bytes\n";

    std::cout << "Throughput: "
              << throughputMbps << " Mbps\n";

    std::cout << "Measurement Window: "
              << measurementWindow << " seconds\n";

    std::cout << "===================================\n";

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
        << "Bandwidth_Mbps,"
        << "Delay_ms,"
        << "PacketSize_bytes,"
        << "SimulationTime_s,"
        << "MeasurementWindow_s,"
        << "TotalBytesReceived,"
        << "Throughput_Mbps\n";

    csvFile
        << "YeAH,"
        << linkRateMbps << ","
        << delayMs << ","
        << packetSize << ","
        << simulationTime << ","
        << measurementWindow << ","
        << totalBytesReceived << ","
        << throughputMbps << "\n";

    csvFile.close();

    std::cout << "Results saved to: "
              << outputFile << std::endl;

    Simulator::Destroy();

    return 0;
}
