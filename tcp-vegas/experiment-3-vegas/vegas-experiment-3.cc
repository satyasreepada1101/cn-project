#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/error-model.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/tcp-vegas.h"

#include <fstream>
#include <iostream>
#include <string>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("VegasExperiment3");

static uint64_t
RunSimulation(double lossPercent)
{
    NodeContainer nodes;
    nodes.Create(2);

    PointToPointHelper p2p;
    p2p.SetDeviceAttribute("DataRate", StringValue("10Mbps"));
    p2p.SetChannelAttribute("Delay", StringValue("10ms"));

    NetDeviceContainer devices = p2p.Install(nodes);
    Ptr<RateErrorModel> errorModel = CreateObject<RateErrorModel>();
    errorModel->SetUnit(RateErrorModel::ERROR_UNIT_PACKET);
    errorModel->SetRate(lossPercent / 100.0);
    errorModel->AssignStreams(1);
    devices.Get(1)->SetAttribute("ReceiveErrorModel", PointerValue(errorModel));

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
        "scratch/results/tcp-vegas/experiment-3/summary.csv";
    std::ofstream results(resultsPath);
    if (!results.is_open())
    {
        std::cerr << "Could not open results file: " << resultsPath << std::endl;
        return 1;
    }

    results << "Algorithm,Packet_Loss_Percent,Received_Bytes,Throughput_Mbps\n";
    for (const double lossPercent : {0.0, 0.1, 0.5, 1.0})
    {
        const uint64_t receivedBytes = RunSimulation(lossPercent);
        const double throughputMbps =
            (receivedBytes * 8.0) / (10.0 * 1000000.0);

        results << "TCP Vegas," << lossPercent << ',' << receivedBytes << ','
                << throughputMbps << '\n';
        std::cout << "Packet loss: " << lossPercent << "%, received bytes: "
                  << receivedBytes << ", throughput: " << throughputMbps
                  << " Mbps" << std::endl;
    }

    if (!results)
    {
        std::cerr << "Failed while writing results file: " << resultsPath
                  << std::endl;
        return 1;
    }

    std::cout << "Packet loss is applied at the receiver-side device of the "
                 "bottleneck point-to-point link."
              << std::endl;
    std::cout << "Results saved to " << resultsPath << std::endl;
    return 0;
}
