#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/tcp-quick-vegas.h"
#include "ns3/traffic-control-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("QuickVegasBasic");

static std::ofstream g_cwndFile;
static std::ofstream g_queueFile;
static std::ofstream g_throughputFile;
static std::ofstream g_rttFile;  // NEW

static uint64_t g_lastRxBytes = 0;
static Ptr<PacketSink> g_sink;


// NEW: RTT tracer callback
static void
RttTracer(Time oldRtt, Time newRtt)
{
    g_rttFile << Simulator::Now().GetSeconds() << "\t" 
              << newRtt.GetMilliSeconds() << "\n";
}

// NEW: Connect RTT trace
static void
ConnectRttTrace()
{
    Config::ConnectWithoutContext(
        "/NodeList/0/$ns3::TcpL4Protocol/SocketList/0/RTT",
        MakeCallback(&RttTracer));
}






static void
CwndTracer(uint32_t oldVal, uint32_t newVal)
{
    uint32_t segSize = 1000;
    double cwndPackets = (double)newVal / segSize;
    g_cwndFile << Simulator::Now().GetSeconds() << "\t" << cwndPackets << "\n";
}

static void
ThroughputSampler()
{
    uint64_t rx = g_sink->GetTotalRx();
    double mbps = (rx - g_lastRxBytes) * 8.0 / 1e6;
    g_lastRxBytes = rx;
    g_throughputFile << Simulator::Now().GetSeconds() << "\t" << mbps << "\n";
    Simulator::Schedule(Seconds(1.0), &ThroughputSampler);
}

static void
QueueSampler(Ptr<QueueDisc> qdisc)
{
    uint32_t qlen = qdisc->GetNPackets();
    g_queueFile << Simulator::Now().GetSeconds() << "\t" << qlen << "\n";
    Simulator::Schedule(Seconds(0.1), &QueueSampler, qdisc);
}

static void
ConnectCwndTrace()
{
    Config::ConnectWithoutContext(
        "/NodeList/0/$ns3::TcpL4Protocol/SocketList/0/CongestionWindow",
        MakeCallback(&CwndTracer));
}

int
main(int argc, char* argv[])
{
    LogComponentEnable("TcpQuickVegas", LOG_LEVEL_DEBUG);
    LogComponentEnable("QuickVegasBasic", LOG_LEVEL_INFO);

    std::string tcpVariant = "TcpVegas";

    CommandLine cmd;
    cmd.AddValue("tcpVariant", "TCP variant: TcpVegas or TcpQuickVegas", tcpVariant);
    cmd.Parse(argc, argv);

    Config::SetDefault("ns3::TcpL4Protocol::SocketType",
                       StringValue("ns3::" + tcpVariant));
    Config::SetDefault("ns3::TcpVegas::Alpha", UintegerValue(2));
    Config::SetDefault("ns3::TcpVegas::Beta",  UintegerValue(4));
    Config::SetDefault("ns3::TcpVegas::Gamma", UintegerValue(1));
    Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue(1000));

    Config::SetDefault("ns3::TcpSocket::SndBufSize", UintegerValue(1048576));
    Config::SetDefault("ns3::TcpSocket::RcvBufSize", UintegerValue(1048576));

    
    uint32_t nCbr = 1;
    uint32_t n = 1 + nCbr;

    NodeContainer sources, routers, destinations;
    sources.Create(n);
    routers.Create(2);
    destinations.Create(n);

    PointToPointHelper access;
    access.SetDeviceAttribute("DataRate", StringValue("1Gbps"));
    access.SetChannelAttribute("Delay",   StringValue("1ms"));

    std::vector<NetDeviceContainer> devS_R1(n);
    std::vector<NetDeviceContainer> devR2_D(n);
    for (uint32_t i = 0; i < n; i++)
    {
        devS_R1[i] = access.Install(sources.Get(i), routers.Get(0));
        devR2_D[i] = access.Install(routers.Get(1), destinations.Get(i));
    }

    PointToPointHelper bottleneck;
    bottleneck.SetDeviceAttribute("DataRate", StringValue("50Mbps"));
    bottleneck.SetChannelAttribute("Delay",   StringValue("48ms"));
  
    bottleneck.SetQueue("ns3::DropTailQueue", "MaxSize", StringValue("1p"));
    NetDeviceContainer devR1R2 = bottleneck.Install(routers.Get(0), routers.Get(1));

    InternetStackHelper stack;
    stack.Install(sources);
    stack.Install(routers);
    stack.Install(destinations);

    Ipv4AddressHelper address;
    std::vector<Ipv4InterfaceContainer> ifS_R1(n);
    std::vector<Ipv4InterfaceContainer> ifR2_D(n);

    for (uint32_t i = 0; i < n; i++)
    {
        std::ostringstream sub;
        sub << "10.1." << (i + 1) << ".0";
        address.SetBase(sub.str().c_str(), "255.255.255.0");
        ifS_R1[i] = address.Assign(devS_R1[i]);
    }

    address.SetBase("10.2.0.0", "255.255.255.0");
    Ipv4InterfaceContainer ifR1R2 = address.Assign(devR1R2);

    for (uint32_t i = 0; i < n; i++)
    {
        std::ostringstream sub;
        sub << "10.3." << (i + 1) << ".0";
        address.SetBase(sub.str().c_str(), "255.255.255.0");
        ifR2_D[i] = address.Assign(devR2_D[i]);
    }

    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    // TCP flow S1 to D1 
    uint16_t tcpPort = 50001;
    BulkSendHelper bulk("ns3::TcpSocketFactory",
                        InetSocketAddress(ifR2_D[0].GetAddress(1), tcpPort));
    bulk.SetAttribute("MaxBytes", UintegerValue(0));
    ApplicationContainer tcpSrcApp = bulk.Install(sources.Get(0));
    tcpSrcApp.Start(Seconds(0.0));
    tcpSrcApp.Stop(Seconds(240.0));

    PacketSinkHelper tcpSink("ns3::TcpSocketFactory",
                             InetSocketAddress(Ipv4Address::GetAny(), tcpPort));
    ApplicationContainer tcpSinkApp = tcpSink.Install(destinations.Get(0));
    tcpSinkApp.Start(Seconds(0.0));
    tcpSinkApp.Stop(Seconds(240.0));

    // traffic
    //double cbrPerFlow = 50.0 / nCbr; 
    double cbrPerFlow = 25.0 / nCbr; 
    std::ostringstream cbrRate;
    cbrRate << cbrPerFlow << "Mbps";

    for (uint32_t i = 1; i <= nCbr; i++)
    {
        uint16_t udpPort = 50001 + i;

        OnOffHelper cbr("ns3::UdpSocketFactory",
                        InetSocketAddress(ifR2_D[i].GetAddress(1), udpPort));
        cbr.SetAttribute("DataRate",   StringValue(cbrRate.str()));
        cbr.SetAttribute("PacketSize", UintegerValue(1000));
        cbr.SetAttribute("OnTime",  StringValue("ns3::ConstantRandomVariable[Constant=1]"));
        cbr.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));

        ApplicationContainer cbrSrcApp = cbr.Install(sources.Get(i));
        cbrSrcApp.Start(Seconds(80.0));
        cbrSrcApp.Stop(Seconds(90.0));

        PacketSinkHelper udpSink("ns3::UdpSocketFactory",
                                 InetSocketAddress(Ipv4Address::GetAny(), udpPort));
        ApplicationContainer cbrSinkApp = udpSink.Install(destinations.Get(i));
        cbrSinkApp.Start(Seconds(0.0));
        cbrSinkApp.Stop(Seconds(240.0));
    }

  
    TrafficControlHelper tchClean;
    tchClean.Uninstall(devR1R2);

    TrafficControlHelper tch;
    tch.SetRootQueueDisc("ns3::FifoQueueDisc",
                         "MaxSize", StringValue("500p"));
    QueueDiscContainer qdiscs = tch.Install(devR1R2.Get(0));

  
    std::string prefix = tcpVariant + "-basic";
    g_cwndFile.open(prefix + "-cwnd.dat");
    g_queueFile.open(prefix + "-queue.dat");
    g_throughputFile.open(prefix + "-throughput.dat");
    g_rttFile.open(prefix + "-rtt.dat");  // NEW

    g_cwndFile       << "# Time(s)\tCwnd(packets)\n";
    g_queueFile      << "# Time(s)\tQueueLen(packets)\n";
    g_throughputFile << "# Time(s)\tThroughput(Mbps)\n";
    g_rttFile        << "# Time(s)\tRTT(ms)\n";  // NEW


    g_sink = DynamicCast<PacketSink>(tcpSinkApp.Get(0));

    Simulator::Schedule(Seconds(0.001), &ConnectCwndTrace);
    Simulator::Schedule(Seconds(0.1),   &QueueSampler, qdiscs.Get(0));
    Simulator::Schedule(Seconds(1.0),   &ThroughputSampler);
    Simulator::Schedule(Seconds(1), &ConnectRttTrace);  // NEW

    Simulator::Stop(Seconds(240.0));
    Simulator::Run();

    g_cwndFile.close();
    g_queueFile.close();
    g_throughputFile.close();
    g_rttFile.close();  // NEW

    std::cout << "Queue drops: "
              << qdiscs.Get(0)->GetStats().nTotalDroppedPackets << std::endl;

    double totalRx  = g_sink->GetTotalRx();
    double avgTput  = (totalRx * 8.0) / (240.0 * 1e6);
    std::cout << "=== " << tcpVariant << " Basic Behavior ===" << std::endl;
    std::cout << "Total bytes received : " << totalRx << " bytes" << std::endl;
    std::cout << "Average throughput   : " << avgTput << " Mbps" << std::endl;

    Simulator::Destroy();
    return 0;
}




