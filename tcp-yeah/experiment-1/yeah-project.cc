#include <fstream>
#include <iostream>
#include <string>

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/tcp-yeah.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("YeahProject");

int main(int argc, char *argv[])
{
    uint32_t packetSize = 1024;
    uint16_t port = 5000;

    double simulationTime = 10.0;
    double senderStartTime = 1.0;
    double appRate = 5.0;
    double linkRateMbps = 10.0;

    std::string outputFile =
        "results/yeah/experiment-1/yeah_10Mbps.csv";

    CommandLine cmd(__FILE__);
    cmd.AddValue("linkRateMbps", "Bottleneck link rate in Mbps", linkRateMbps);
    cmd.AddValue("outputFile", "CSV output file", outputFile);
    cmd.Parse(argc, argv);

    // Select TCP YeAH
    Config::SetDefault(
        "ns3::TcpL4Protocol::SocketType",
        TypeIdValue(TcpYeah::GetTypeId()));

    // Create nodes
    NodeContainer nodes;
    nodes.Create(2);

    // Point-to-point link
    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute(
        "DataRate",
        StringValue(std::to_string(linkRateMbps) + "Mbps"));
    pointToPoint.SetChannelAttribute(
        "Delay",
        StringValue("10ms"));

    NetDeviceContainer devices;
    devices = pointToPoint.Install(nodes);

    // Install Internet stack
    InternetStackHelper internet;
    internet.Install(nodes);

    // Assign IP addresses
    Ipv4AddressHelper address;
    address.SetBase(
        "10.1.1.0",
        "255.255.255.0");

    Ipv4InterfaceContainer interfaces;
    interfaces = address.Assign(devices);

    // Packet sink on receiver
    PacketSinkHelper sinkHelper(
        "ns3::TcpSocketFactory",
        InetSocketAddress(Ipv4Address::GetAny(), port));

    ApplicationContainer sinkApp = sinkHelper.Install(nodes.Get(1));
    sinkApp.Start(Seconds(0.0));
    sinkApp.Stop(Seconds(simulationTime));

    // TCP sender
    OnOffHelper client(
        "ns3::TcpSocketFactory",
        InetSocketAddress(interfaces.GetAddress(1), port));

    client.SetAttribute(
        "DataRate",
        DataRateValue(DataRate(
            std::to_string(appRate) + "Mbps")));

    client.SetAttribute(
        "PacketSize",
        UintegerValue(packetSize));

    client.SetAttribute(
        "OnTime",
        StringValue("ns3::ConstantRandomVariable[Constant=1]"));

    client.SetAttribute(
        "OffTime",
        StringValue("ns3::ConstantRandomVariable[Constant=0]"));

    ApplicationContainer clientApp = client.Install(nodes.Get(0));

    clientApp.Start(Seconds(senderStartTime));
    clientApp.Stop(Seconds(simulationTime));

    // Run simulation
    Simulator::Stop(Seconds(simulationTime));
    Simulator::Run();

    // Get received bytes
    Ptr<PacketSink> sink =
        DynamicCast<PacketSink>(sinkApp.Get(0));

    uint64_t totalBytesReceived = sink->GetTotalRx();

    // Measurement window
    double measurementWindow =
        simulationTime - senderStartTime;

    // Calculate throughput
    double throughput =
        (totalBytesReceived * 8.0) /
        (measurementWindow * 1000000.0);

    // Print results
    std::cout << "\n========== TCP YeAH Results ==========\n";
    std::cout << "Algorithm: YeAH\n";
    std::cout << "Link bandwidth: "
              << linkRateMbps << " Mbps\n";
    std::cout << "Application rate: "
              << appRate << " Mbps\n";
    std::cout << "Total bytes received: "
              << totalBytesReceived << "\n";
    std::cout << "Throughput: "
              << throughput << " Mbps\n";
    std::cout << "Measurement window: "
              << measurementWindow << " seconds\n";
    std::cout << "======================================\n";

    // Create output directory
    system("mkdir -p results/yeah/experiment-1");

    // Write CSV
    std::ofstream outFile(outputFile);

    outFile << "Algorithm,Bandwidth_Mbps,Delay_ms,"
            << "PacketSize_bytes,SimulationTime_s,"
            << "MeasurementWindow_s,TotalBytesReceived,"
            << "Throughput_Mbps\n";

    outFile << "YeAH,"
            << linkRateMbps << ","
            << "10,"
            << packetSize << ","
            << simulationTime << ","
            << measurementWindow << ","
            << totalBytesReceived << ","
            << throughput << "\n";

    outFile.close();

    Simulator::Destroy();

    return 0;
}
