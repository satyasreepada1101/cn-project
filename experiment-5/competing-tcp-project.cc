
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"

#include <fstream>
#include <iostream>
#include <vector>
#include <cmath>

using namespace ns3;

int main(int argc, char *argv[])
{
    // Simulation parameters
    double bandwidth = 10.0;
    double delay = 10.0;
    double appRate = 5.0;

    uint32_t packetSize = 1024;

    double simulationTime = 20.0;
    double senderStartTime = 1.0;
    double measurementWindow =
        simulationTime - senderStartTime;

    // Create nodes
    NodeContainer nodes;
    nodes.Create(5);

    // Node 0: Vegas sender
    // Node 1: Veno sender
    // Node 2: YeAH sender
    // Node 3: Router
    // Node 4: Receiver

    NodeContainer vegasNode(nodes.Get(0));
    NodeContainer venoNode(nodes.Get(1));
    NodeContainer yeahNode(nodes.Get(2));

    NodeContainer router(nodes.Get(3));
    NodeContainer receiver(nodes.Get(4));

    // Install Internet stack on all nodes
    InternetStackHelper internet;
    internet.Install(nodes);

    // Access links
    PointToPointHelper accessLink;

    accessLink.SetDeviceAttribute(
        "DataRate",
        StringValue("100Mbps"));

    accessLink.SetChannelAttribute(
        "Delay",
        StringValue("1ms"));

    // Shared bottleneck
    PointToPointHelper bottleneck;

    bottleneck.SetDeviceAttribute(
        "DataRate",
        StringValue("10Mbps"));

    bottleneck.SetChannelAttribute(
        "Delay",
        StringValue("10ms"));

    // Connect each sender to router
    NetDeviceContainer vegasDevices =
        accessLink.Install(nodes.Get(0), nodes.Get(3));

    NetDeviceContainer venoDevices =
        accessLink.Install(nodes.Get(1), nodes.Get(3));

    NetDeviceContainer yeahDevices =
        accessLink.Install(nodes.Get(2), nodes.Get(3));

    // Connect router to receiver
    NetDeviceContainer bottleneckDevices =
        bottleneck.Install(nodes.Get(3), nodes.Get(4));

    // Assign IP addresses
    Ipv4AddressHelper address;

    address.SetBase(
        "10.1.1.0",
        "255.255.255.0");

    Ipv4InterfaceContainer vegasInterfaces =
        address.Assign(vegasDevices);

    address.SetBase(
        "10.1.2.0",
        "255.255.255.0");

    Ipv4InterfaceContainer venoInterfaces =
        address.Assign(venoDevices);

    address.SetBase(
        "10.1.3.0",
        "255.255.255.0");

    Ipv4InterfaceContainer yeahInterfaces =
        address.Assign(yeahDevices);

    address.SetBase(
        "10.1.4.0",
        "255.255.255.0");

    Ipv4InterfaceContainer bottleneckInterfaces =
        address.Assign(bottleneckDevices);

    // Populate routing tables
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    // Configure TCP algorithms individually
    Config::Set(
        "/NodeList/0/$ns3::TcpL4Protocol/SocketType",
        TypeIdValue(TcpVegas::GetTypeId()));

    Config::Set(
        "/NodeList/1/$ns3::TcpL4Protocol/SocketType",
        TypeIdValue(TcpVeno::GetTypeId()));

    Config::Set(
        "/NodeList/2/$ns3::TcpL4Protocol/SocketType",
        TypeIdValue(TcpYeah::GetTypeId()));

    // Receiver applications
    uint16_t ports[3] = {5001, 5002, 5003};

    ApplicationContainer sinkApps;

    for (uint32_t i = 0; i < 3; i++)
    {
        PacketSinkHelper sinkHelper(
            "ns3::TcpSocketFactory",
            InetSocketAddress(
                Ipv4Address::GetAny(),
                ports[i]));

        ApplicationContainer app =
            sinkHelper.Install(receiver);

        app.Start(Seconds(0.0));
        app.Stop(Seconds(simulationTime));

        sinkApps.Add(app);
    }

    // Vegas sender
    OnOffHelper vegasSender(
        "ns3::TcpSocketFactory",
        InetSocketAddress(
            bottleneckInterfaces.GetAddress(1),
            ports[0]));

    vegasSender.SetAttribute(
        "DataRate",
        StringValue("5Mbps"));

    vegasSender.SetAttribute(
        "PacketSize",
        UintegerValue(packetSize));

    vegasSender.SetAttribute(
        "OnTime",
        StringValue(
            "ns3::ConstantRandomVariable[Constant=1]"));

    vegasSender.SetAttribute(
        "OffTime",
        StringValue(
            "ns3::ConstantRandomVariable[Constant=0]"));

    ApplicationContainer vegasApp =
        vegasSender.Install(vegasNode);

    vegasApp.Start(Seconds(senderStartTime));
    vegasApp.Stop(Seconds(simulationTime));

    // Veno sender
    OnOffHelper venoSender(
        "ns3::TcpSocketFactory",
        InetSocketAddress(
            bottleneckInterfaces.GetAddress(1),
            ports[1]));

    venoSender.SetAttribute(
        "DataRate",
        StringValue("5Mbps"));

    venoSender.SetAttribute(
        "PacketSize",
        UintegerValue(packetSize));

    venoSender.SetAttribute(
        "OnTime",
        StringValue(
            "ns3::ConstantRandomVariable[Constant=1]"));

    venoSender.SetAttribute(
        "OffTime",
        StringValue(
            "ns3::ConstantRandomVariable[Constant=0]"));

    ApplicationContainer venoApp =
        venoSender.Install(venoNode);

    venoApp.Start(Seconds(senderStartTime));
    venoApp.Stop(Seconds(simulationTime));

    // YeAH sender
    OnOffHelper yeahSender(
        "ns3::TcpSocketFactory",
        InetSocketAddress(
            bottleneckInterfaces.GetAddress(1),
            ports[2]));

    yeahSender.SetAttribute(
        "DataRate",
        StringValue("5Mbps"));

    yeahSender.SetAttribute(
        "PacketSize",
        UintegerValue(packetSize));

    yeahSender.SetAttribute(
        "OnTime",
        StringValue(
            "ns3::ConstantRandomVariable[Constant=1]"));

    yeahSender.SetAttribute(
        "OffTime",
        StringValue(
            "ns3::ConstantRandomVariable[Constant=0]"));

    ApplicationContainer yeahApp =
        yeahSender.Install(yeahNode);

    yeahApp.Start(Seconds(senderStartTime));
    yeahApp.Stop(Seconds(simulationTime));

    // Run simulation
    Simulator::Stop(Seconds(simulationTime));
    Simulator::Run();

    // Collect throughput
    std::vector<double> throughput;

    double totalThroughput = 0.0;

    std::vector<std::string> algorithms =
        {"Vegas", "Veno", "Yeah"};

    uint64_t receivedBytes[3];

    for (uint32_t i = 0; i < 3; i++)
    {
        Ptr<PacketSink> sink =
            DynamicCast<PacketSink>(
                sinkApps.Get(i));

        receivedBytes[i] = sink->GetTotalRx();

        double flowThroughput =
            receivedBytes[i] * 8.0 /
            (measurementWindow * 1000000.0);

        throughput.push_back(flowThroughput);

        totalThroughput += flowThroughput;

        std::cout
            << algorithms[i]
            << " received bytes: "
            << receivedBytes[i]
            << std::endl;

        std::cout
            << algorithms[i]
            << " throughput: "
            << flowThroughput
            << " Mbps"
            << std::endl;
    }

    // Jain's Fairness Index
    double sum = 0.0;
    double sumSquares = 0.0;

    for (double value : throughput)
    {
        sum += value;
        sumSquares += value * value;
    }

    double fairness =
        (sum * sum) /
        (3.0 * sumSquares);

    // Create results directory before running
    std::ofstream csv(
        "scratch/results/experiment-5/summary.csv");

    csv
        << "Algorithm,FlowID,ReceivedBytes,"
        << "Throughput_Mbps,TotalThroughput_Mbps,"
        << "JainFairnessIndex\n";

    for (uint32_t i = 0; i < 3; i++)
    {
        csv
            << algorithms[i] << ","
            << i + 1 << ","
            << receivedBytes[i] << ","
            << throughput[i] << ","
            << totalThroughput << ","
            << fairness << "\n";
    }

    csv.close();

    std::cout
        << "\nTotal throughput: "
        << totalThroughput
        << " Mbps"
        << std::endl;

    std::cout
        << "Jain Fairness Index: "
        << fairness
        << std::endl;

    Simulator::Destroy();

    return 0;
}
