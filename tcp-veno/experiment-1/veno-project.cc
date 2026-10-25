
#include <fstream>
#include <iostream>
#include <string>

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/tcp-veno.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("VenoProject");

int
main(int argc, char *argv[])
{
    // ==========================================
    // 1. Simulation configuration
    // ==========================================

    uint32_t packetSize = 1024;
    uint32_t port = 5000;

    double simulationTime = 10.0;
    double senderStartTime = 1.0;

    double appRate = 5.0;

    // ==========================================
    // 2. Experiment parameters
    // ==========================================

    double linkRateMbps = 10.0;

    std::string outputFile =
        "results/experiment-1/veno_10Mbps.csv";

    // ==========================================
    // 3. Command-line configuration
    // ==========================================

    CommandLine cmd;

    cmd.AddValue(
        "linkRateMbps",
        "Bottleneck link bandwidth in Mbps",
        linkRateMbps);

    cmd.AddValue(
        "outputFile",
        "File to save experiment results",
        outputFile);

    cmd.Parse(argc, argv);

    // ==========================================
    // 4. Select TCP Veno
    // ==========================================

    Config::SetDefault(
        "ns3::TcpL4Protocol::SocketType",
        TypeIdValue(TcpVeno::GetTypeId()));

    // ==========================================
    // 5. Create sender and receiver nodes
    // ==========================================

    NodeContainer nodes;
    nodes.Create(2);

    // ==========================================
    // 6. Configure point-to-point link
    // ==========================================

    PointToPointHelper pointToPoint;

    pointToPoint.SetDeviceAttribute(
        "DataRate",
        StringValue(
            std::to_string(linkRateMbps) + "Mbps"));

    pointToPoint.SetChannelAttribute(
        "Delay",
        StringValue("10ms"));

    NetDeviceContainer devices;

    devices = pointToPoint.Install(nodes);

    // ==========================================
    // 7. Install TCP/IP stack
    // ==========================================

    InternetStackHelper internet;

    internet.Install(nodes);

    // ==========================================
    // 8. Assign IP addresses
    // ==========================================

    Ipv4AddressHelper address;

    address.SetBase(
        "10.1.1.0",
        "255.255.255.0");

    Ipv4InterfaceContainer interfaces;

    interfaces = address.Assign(devices);

    // ==========================================
    // 9. Configure receiver application
    // ==========================================

    PacketSinkHelper sinkHelper(
        "ns3::TcpSocketFactory",
        InetSocketAddress(
            Ipv4Address::GetAny(),
            port));

    ApplicationContainer sinkApps;

    sinkApps = sinkHelper.Install(nodes.Get(1));

    sinkApps.Start(Seconds(0.0));

    sinkApps.Stop(Seconds(simulationTime));

    // ==========================================
    // 10. Configure sender application
    // ==========================================

    OnOffHelper sender(
        "ns3::TcpSocketFactory",
        InetSocketAddress(
            interfaces.GetAddress(1),
            port));

    sender.SetAttribute(
        "DataRate",
        DataRateValue(
            DataRate(
                std::to_string(appRate) + "Mbps")));

    sender.SetAttribute(
        "PacketSize",
        UintegerValue(packetSize));

    sender.SetAttribute(
        "OnTime",
        StringValue(
            "ns3::ConstantRandomVariable[Constant=1]"));

    sender.SetAttribute(
        "OffTime",
        StringValue(
            "ns3::ConstantRandomVariable[Constant=0]"));

    ApplicationContainer senderApps;

    senderApps = sender.Install(nodes.Get(0));

    senderApps.Start(Seconds(senderStartTime));

    senderApps.Stop(Seconds(simulationTime));

    // ==========================================
    // 11. Run simulation
    // ==========================================

    Simulator::Stop(Seconds(simulationTime));

    Simulator::Run();

    // ==========================================
    // 12. Get received bytes
    // ==========================================

    Ptr<PacketSink> sink;

    sink = DynamicCast<PacketSink>(
        sinkApps.Get(0));

    uint64_t totalBytesReceived =
        sink->GetTotalRx();

    // ==========================================
    // 13. Calculate throughput
    // ==========================================

    double measurementWindow =
        simulationTime - senderStartTime;

    double throughputMbps =
        (totalBytesReceived * 8.0) /
        (measurementWindow * 1000000.0);

    // ==========================================
    // 14. Print results
    // ==========================================

    std::cout
        << "\n========== TCP VENO RESULTS ==========\n";

    std::cout
        << "TCP Algorithm: Veno\n";

    std::cout
        << "Link Bandwidth: "
        << linkRateMbps
        << " Mbps\n";

    std::cout
        << "Application Rate: "
        << appRate
        << " Mbps\n";

    std::cout
        << "Total Bytes Received: "
        << totalBytesReceived
        << " bytes\n";

    std::cout
        << "Throughput: "
        << throughputMbps
        << " Mbps\n";

    std::cout
        << "Measurement Window: "
        << measurementWindow
        << " seconds\n";

    std::cout
        << "===================================\n";

    // ==========================================
    // 15. Save results to CSV
    // ==========================================

    std::ofstream resultFile(outputFile);

    if (resultFile.is_open())
    {
        resultFile
            << "Algorithm,"
            << "Bandwidth_Mbps,"
            << "Delay_ms,"
            << "PacketSize_bytes,"
            << "SimulationTime_s,"
            << "MeasurementWindow_s,"
            << "TotalBytesReceived,"
            << "Throughput_Mbps\n";

        resultFile
            << "Veno,"
            << linkRateMbps << ","
            << 10 << ","
            << packetSize << ","
            << simulationTime << ","
            << measurementWindow << ","
            << totalBytesReceived << ","
            << throughputMbps
            << "\n";

        resultFile.close();

        std::cout
            << "Results saved to: "
            << outputFile
            << "\n";
    }
    else
    {
        std::cerr
            << "Error: Could not open result file: "
            << outputFile
            << "\n";
    }

    // ==========================================
    // 16. Destroy simulation
    // ==========================================

    Simulator::Destroy();

    return 0;
}
