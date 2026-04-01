#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-layout-module.h"
#include "ns3/point-to-point-module.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

using namespace ns3;

namespace
{

std::ofstream g_cwndStream;
std::ofstream g_throughputStream;
bool g_firstCwndSample = true;
uint64_t g_lastRxBytes = 0;

void
CwndTracer(uint32_t oldValue, uint32_t newValue)
{
    if (g_firstCwndSample)
    {
        g_cwndStream << 0.0 << " " << oldValue << '\n';
        g_firstCwndSample = false;
    }

    g_cwndStream << Simulator::Now().GetSeconds() << " " << newValue << '\n';
}

void
SampleThroughput(Ptr<PacketSink> sink, double intervalSeconds)
{
    const uint64_t currentRxBytes = sink->GetTotalRx();
    const double throughputMbps = (currentRxBytes - g_lastRxBytes) * 8.0 / intervalSeconds / 1e6;

    g_throughputStream << Simulator::Now().GetSeconds() << " " << throughputMbps << '\n';
    g_lastRxBytes = currentRxBytes;

    Simulator::Schedule(Seconds(intervalSeconds), &SampleThroughput, sink, intervalSeconds);
}

void
TryConnectCwndTrace(Ptr<OnOffApplication> tcpSourceApp)
{
    Ptr<Socket> socket = tcpSourceApp->GetSocket();
    if (socket == nullptr)
    {
        Simulator::Schedule(MilliSeconds(1), &TryConnectCwndTrace, tcpSourceApp);
        return;
    }

    socket->TraceConnectWithoutContext("CongestionWindow", MakeCallback(&CwndTracer));
}

TypeId
ResolveTcpType(const std::string& tcpVariant)
{
    if (tcpVariant == "reno")
    {
        return TcpNewReno::GetTypeId();
    }

    if (tcpVariant == "cubic")
    {
        return TcpCubic::GetTypeId();
    }

    NS_FATAL_ERROR("Unsupported tcpVariant value: use reno or cubic");
}

} // namespace

int
main(int argc, char* argv[])
{
    std::string tcpVariant = "reno";
    std::string outputPrefix = "scratch/activities/results/activity2";
    double simulationTime = 20.0;
    double startTime = 1.0;
    double sampleInterval = 0.1;

    CommandLine cmd;
    cmd.AddValue("tcpVariant", "reno or cubic", tcpVariant);
    cmd.AddValue("outputPrefix", "Prefix for data files", outputPrefix);
    cmd.AddValue("simulationTime", "Simulation time in seconds", simulationTime);
    cmd.AddValue("startTime", "Application start time in seconds", startTime);
    cmd.AddValue("sampleInterval", "Throughput sampling interval in seconds", sampleInterval);
    cmd.Parse(argc, argv);

    std::filesystem::create_directories(std::filesystem::path(outputPrefix).parent_path());

    Config::SetDefault("ns3::TcpL4Protocol::SocketType", TypeIdValue(ResolveTcpType(tcpVariant)));
    Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue(1448));
    Config::SetDefault("ns3::TcpSocket::DelAckCount", UintegerValue(1));

    PointToPointHelper leafLink;
    leafLink.SetDeviceAttribute("DataRate", StringValue("10Mbps"));
    leafLink.SetChannelAttribute("Delay", StringValue("40ms"));

    PointToPointHelper bottleneckLink;
    bottleneckLink.SetDeviceAttribute("DataRate", StringValue("10Mbps"));
    bottleneckLink.SetChannelAttribute("Delay", StringValue("20ms"));

    PointToPointDumbbellHelper dumbbell(2, leafLink, 2, leafLink, bottleneckLink);
    InternetStackHelper stack;
    dumbbell.InstallStack(stack);

    Ipv4AddressHelper leftIp;
    Ipv4AddressHelper rightIp;
    Ipv4AddressHelper routerIp;
    leftIp.SetBase("10.1.1.0", "255.255.255.0");
    rightIp.SetBase("10.2.1.0", "255.255.255.0");
    routerIp.SetBase("10.3.1.0", "255.255.255.0");
    dumbbell.AssignIpv4Addresses(leftIp, rightIp, routerIp);

    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    const uint16_t tcpPort = 5000;
    const uint16_t udpPort = 5001;

    Address tcpSinkAddress(InetSocketAddress(Ipv4Address::GetAny(), tcpPort));
    PacketSinkHelper tcpSinkHelper("ns3::TcpSocketFactory", tcpSinkAddress);
    ApplicationContainer tcpSinkApp = tcpSinkHelper.Install(dumbbell.GetRight(0));
    tcpSinkApp.Start(Seconds(0.0));
    tcpSinkApp.Stop(Seconds(simulationTime));

    Address udpSinkAddress(InetSocketAddress(Ipv4Address::GetAny(), udpPort));
    PacketSinkHelper udpSinkHelper("ns3::UdpSocketFactory", udpSinkAddress);
    ApplicationContainer udpSinkApp = udpSinkHelper.Install(dumbbell.GetRight(1));
    udpSinkApp.Start(Seconds(0.0));
    udpSinkApp.Stop(Seconds(simulationTime));

    OnOffHelper tcpSourceHelper("ns3::TcpSocketFactory", Address());
    tcpSourceHelper.SetAttribute("OnTime",
                                 StringValue("ns3::ConstantRandomVariable[Constant=1]"));
    tcpSourceHelper.SetAttribute("OffTime",
                                 StringValue("ns3::ConstantRandomVariable[Constant=0]"));
    tcpSourceHelper.SetAttribute("DataRate", DataRateValue(DataRate("5Mbps")));
    tcpSourceHelper.SetAttribute("PacketSize", UintegerValue(1000));
    tcpSourceHelper.SetAttribute("MaxBytes", UintegerValue(0));
    tcpSourceHelper.SetAttribute("Remote",
                                 AddressValue(InetSocketAddress(dumbbell.GetRightIpv4Address(0),
                                                               tcpPort)));

    ApplicationContainer tcpSourceApp = tcpSourceHelper.Install(dumbbell.GetLeft(0));
    tcpSourceApp.Start(Seconds(startTime));
    tcpSourceApp.Stop(Seconds(simulationTime));

    OnOffHelper udpSourceHelper("ns3::UdpSocketFactory", Address());
    udpSourceHelper.SetAttribute("OnTime",
                                 StringValue("ns3::ConstantRandomVariable[Constant=1]"));
    udpSourceHelper.SetAttribute("OffTime",
                                 StringValue("ns3::ConstantRandomVariable[Constant=0]"));
    udpSourceHelper.SetAttribute("DataRate", DataRateValue(DataRate("8Mbps")));
    udpSourceHelper.SetAttribute("PacketSize", UintegerValue(1000));
    udpSourceHelper.SetAttribute("MaxBytes", UintegerValue(0));
    udpSourceHelper.SetAttribute("Remote",
                                 AddressValue(InetSocketAddress(dumbbell.GetRightIpv4Address(1),
                                                               udpPort)));

    ApplicationContainer udpSourceApp = udpSourceHelper.Install(dumbbell.GetLeft(1));
    udpSourceApp.Start(Seconds(startTime));
    udpSourceApp.Stop(Seconds(simulationTime));

    const std::string variantTag = (tcpVariant == "reno") ? "reno" : "cubic";
    const std::string cwndFileName = outputPrefix + "-" + variantTag + "-cwnd.dat";
    const std::string throughputFileName = outputPrefix + "-" + variantTag + "-throughput.dat";

    g_cwndStream.open(cwndFileName, std::ios::out | std::ios::trunc);
    g_throughputStream.open(throughputFileName, std::ios::out | std::ios::trunc);
    g_cwndStream << "time_s cwnd_bytes" << '\n';
    g_throughputStream << "time_s throughput_mbps" << '\n';

    Ptr<OnOffApplication> tcpSource = DynamicCast<OnOffApplication>(tcpSourceApp.Get(0));
    Simulator::Schedule(Seconds(startTime), &TryConnectCwndTrace, tcpSource);

    Ptr<PacketSink> tcpSink = DynamicCast<PacketSink>(tcpSinkApp.Get(0));
    g_lastRxBytes = 0;
    Simulator::Schedule(Seconds(sampleInterval), &SampleThroughput, tcpSink, sampleInterval);

    Simulator::Stop(Seconds(simulationTime));
    Simulator::Run();

    const double activeTime = simulationTime - startTime;
    const double averageThroughputMbps = tcpSink->GetTotalRx() * 8.0 / activeTime / 1e6;

    std::cout << "TCP variant: " << variantTag << '\n';
    std::cout << "TCP sink bytes: " << tcpSink->GetTotalRx() << '\n';
    std::cout << "Average TCP throughput (Mbps): " << averageThroughputMbps << '\n';

    Simulator::Destroy();
    g_cwndStream.close();
    g_throughputStream.close();

    return 0;
}