#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/traffic-control-module.h"
#include "ns3/tcp-quick-vegas.h"


#include <cmath>
#include <filesystem>
#include <sstream>
#include <vector>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("QueueLengthStaggeredGroups");

static std::ofstream g_queueFile;

static void
QueueSampler(Ptr<QueueDisc> queueDisc, double sampleIntervalSec)
{
    const uint32_t qlen = queueDisc ? queueDisc->GetNPackets() : 0;
    g_queueFile << Simulator::Now().GetSeconds() << "\t" << qlen << "\n";
    g_queueFile.flush();
    Simulator::Schedule(Seconds(sampleIntervalSec), &QueueSampler, queueDisc, sampleIntervalSec);
}

int
main(int argc, char* argv[])
{
    std::string tcpVariant = "TcpVegas";
    std::string outputDir  = "convergence/queue-length-staggered-groups";

    uint32_t nPerGroup       = 20;
    double group1StartSec    = 0.0;
    double group2StartSec    = 100.0;
    double group3StartSec    = 200.0;
    double activePeriodSec   = 300.0;

    std::string bottleneckRate  = "100Mbps";
    std::string bottleneckDelay = "210ms"; 
    std::string bottleneckQueue = "1250p";

    std::string accessRate  = "10Gbps";
    std::string accessDelay = "1ms";
    double queueSampleIntervalSec = 1.0;

    uint32_t vegasAlpha = 3;
    uint32_t vegasBeta  = 5;
    uint32_t vegasGamma = 2;

    double groupStartSpreadSec = 0.0;

    CommandLine cmd;
    cmd.AddValue("tcpVariant",         "TCP variant, e.g., TcpVegas / TcpNewReno / TcpQuickVegas", tcpVariant);
    cmd.AddValue("outputDir",          "Directory for queue .dat output",           outputDir);
    cmd.AddValue("nPerGroup",          "Connections per group",                     nPerGroup);
    cmd.AddValue("group1Start",        "Group-1 start time (s)",                   group1StartSec);
    cmd.AddValue("group2Start",        "Group-2 start time (s)",                   group2StartSec);
    cmd.AddValue("group3Start",        "Group-3 start time (s)",                   group3StartSec);
    cmd.AddValue("activePeriod",       "Active period of each group (s)",           activePeriodSec);
    cmd.AddValue("bottleneckRate",     "Bottleneck link rate",                      bottleneckRate);
    cmd.AddValue("bottleneckDelay",    "Bottleneck link delay",                     bottleneckDelay);
    cmd.AddValue("bottleneckQueue",    "Bottleneck queue size (e.g. 250p)",         bottleneckQueue);
    cmd.AddValue("queueSampleInterval","Queue sampling period (s)",                 queueSampleIntervalSec);
    cmd.AddValue("vegasAlpha",         "Alpha threshold (Vegas and QuickVegas)",    vegasAlpha);
    cmd.AddValue("vegasBeta",          "Beta threshold (Vegas and QuickVegas)",     vegasBeta);
    cmd.AddValue("vegasGamma",         "Gamma threshold (Vegas and QuickVegas)",    vegasGamma);
    cmd.AddValue("groupStartSpread",   "Per-group ramp-up spread in seconds",       groupStartSpreadSec);
    cmd.Parse(argc, argv);


    TypeId tid;
    std::string socketType = "ns3::" + tcpVariant;
    if (!TypeId::LookupByNameFailSafe(socketType, &tid))
    {
        NS_LOG_ERROR("tcpVariant " << tcpVariant << " is not available in this build. "
                     "For TcpQuickVegas, ensure tcp-quick-vegas.cc is listed in CMakeLists.txt.");
        return 1;
    }
    Config::SetDefault("ns3::TcpL4Protocol::SocketType", StringValue(socketType));



    Config::SetDefault("ns3::TcpVegas::Alpha", UintegerValue(vegasAlpha));
    Config::SetDefault("ns3::TcpVegas::Beta", UintegerValue(vegasBeta));
    Config::SetDefault("ns3::TcpVegas::Gamma", UintegerValue(vegasGamma));


    const uint32_t totalFlows = 3 * nPerGroup;

    NodeContainer senders, receivers, routers;
    senders.Create(totalFlows);
    receivers.Create(totalFlows);
    routers.Create(2);

    PointToPointHelper access;
    access.SetDeviceAttribute("DataRate", StringValue(accessRate));
    access.SetChannelAttribute("Delay",   StringValue(accessDelay));

    PointToPointHelper bottleneck;
    bottleneck.SetDeviceAttribute("DataRate", StringValue(bottleneckRate));
    bottleneck.SetChannelAttribute("Delay",   StringValue(bottleneckDelay));

    std::vector<NetDeviceContainer> devS_R1(totalFlows), devR2_D(totalFlows);
    for (uint32_t i = 0; i < totalFlows; ++i)
    {
        devS_R1[i] = access.Install(senders.Get(i),   routers.Get(0));
        devR2_D[i] = access.Install(routers.Get(1), receivers.Get(i));
    }
    NetDeviceContainer devR1R2 = bottleneck.Install(routers.Get(0), routers.Get(1));

    InternetStackHelper stack;
    stack.Install(senders);
    stack.Install(receivers);
    stack.Install(routers);

    TrafficControlHelper tch;
    tch.SetRootQueueDisc("ns3::FifoQueueDisc",
                         "MaxSize", StringValue(bottleneckQueue));
    QueueDiscContainer qdiscs = tch.Install(devR1R2);

    Ipv4AddressHelper addr;
    std::vector<Ipv4InterfaceContainer> ifS_R1(totalFlows), ifR2_D(totalFlows);
    for (uint32_t i = 0; i < totalFlows; ++i)
    {
        std::ostringstream s;
        s << "10.1." << (i + 1) << ".0";
        addr.SetBase(s.str().c_str(), "255.255.255.0");
        ifS_R1[i] = addr.Assign(devS_R1[i]);
    }
    addr.SetBase("10.2.0.0", "255.255.255.0");
    addr.Assign(devR1R2);
    for (uint32_t i = 0; i < totalFlows; ++i)
    {
        std::ostringstream s;
        s << "10.3." << (i + 1) << ".0";
        addr.SetBase(s.str().c_str(), "255.255.255.0");
        ifR2_D[i] = addr.Assign(devR2_D[i]);
    }

    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    const double group1StopSec = group1StartSec + activePeriodSec;
    const double group2StopSec = group2StartSec + activePeriodSec;
    const double group3StopSec = group3StartSec + activePeriodSec;
    const double simStopSec    = std::max({group1StopSec, group2StopSec, group3StopSec}) + 10.0;

    for (uint32_t i = 0; i < totalFlows; ++i)
    {
        const uint16_t port = 40000 + i;

        BulkSendHelper src("ns3::TcpSocketFactory",
                           InetSocketAddress(ifR2_D[i].GetAddress(1), port));
        src.SetAttribute("MaxBytes", UintegerValue(0));
        ApplicationContainer srcApp = src.Install(senders.Get(i));

        PacketSinkHelper sink("ns3::TcpSocketFactory",
                              InetSocketAddress(Ipv4Address::GetAny(), port));
        ApplicationContainer sinkApp = sink.Install(receivers.Get(i));

        const uint32_t idxWithinGroup = i % nPerGroup;
        const double normalized = (nPerGroup > 1)
            ? static_cast<double>(idxWithinGroup) / static_cast<double>(nPerGroup - 1)
            : 0.0;
        const double startJitter = normalized * std::max(0.0, groupStartSpreadSec);

        double startSec = group1StartSec + startJitter;
        double stopSec  = group1StopSec;
        if (i >= nPerGroup && i < 2 * nPerGroup)
        {
            startSec = group2StartSec + startJitter;
            stopSec  = group2StopSec;
        }
        else if (i >= 2 * nPerGroup)
        {
            startSec = group3StartSec + startJitter;
            stopSec  = group3StopSec;
        }

        srcApp.Start(Seconds(startSec));
        srcApp.Stop(Seconds(stopSec));
        sinkApp.Start(Seconds(0.0));
        sinkApp.Stop(Seconds(simStopSec));
    }

    std::filesystem::create_directories(outputDir);
    std::string outPath = outputDir + "/" + tcpVariant + "-queueLength-vs-time.dat";
    g_queueFile.open(outPath, std::ios::out);
    g_queueFile << "# Time(s)\tQueueLength(packets)\n";

    Ptr<QueueDisc> bottleneckQueueDisc = qdiscs.Get(0);
    Simulator::Schedule(Seconds(0.0), &QueueSampler, bottleneckQueueDisc, queueSampleIntervalSec);
    Simulator::Stop(Seconds(simStopSec));
    Simulator::Run();

    g_queueFile.close();
    Simulator::Destroy();

    std::cout << "Queue-length log written to: " << outPath << "\n";
    std::cout << "Flows: C1-C" << nPerGroup << " @ " << group1StartSec << "s, "
              << "C" << (nPerGroup + 1) << "-C" << (2 * nPerGroup) << " @ " << group2StartSec << "s, "
              << "C" << (2 * nPerGroup + 1) << "-C" << (3 * nPerGroup) << " @ " << group3StartSec << "s\n";
    if (tcpVariant == "TcpVegas" || tcpVariant == "TcpQuickVegas")
        std::cout << "Params: alpha=" << vegasAlpha << ", beta=" << vegasBeta
                  << ", gamma=" << vegasGamma << "\n";
    std::cout << "Sampling interval: " << queueSampleIntervalSec
              << "s, group start spread: " << groupStartSpreadSec << "s\n";
    return 0;
}