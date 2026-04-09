#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/tcp-quick-vegas.h"

#include <filesystem>
#include <fstream>
#include <cmath>
#include <iomanip>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("QuickVegasDumbbellSweeps");

namespace
{

struct ExperimentConfig
{
    uint32_t nodes = 40;
    uint32_t flows = 20;
    uint32_t packetsPerSecond = 2000;
    double simDuration = 120.0;
    uint32_t packetSize = 1000;
    uint32_t tcpSegmentSize = 1000;
    std::string accessRate = "10Gbps";
    std::string accessDelay = "1ms";
    std::string bottleneckRate = "auto";
    std::string bottleneckDelay = "250ms";
    uint32_t queuePackets = 4000;
};

struct RunMetrics
{
    double throughputMbps = 0.0;
    double delayMs = 0.0;
    double pdr = 0.0;
    double dropRatio = 0.0;
};

struct SweepDefinition
{
    std::string id;
    std::string xLabel;
    std::string title;
    std::vector<uint32_t> levels;
};

struct MetricSeries
{
    std::map<uint32_t, double> tcpVegas;
    std::map<uint32_t, double> quickVegas;
};

struct SweepResults
{
    MetricSeries throughput;
    MetricSeries delay;
    MetricSeries pdr;
    MetricSeries dropRatio;
};

std::string
ResolveSocketType(const std::string& algorithmName)
{
    TypeId tid;

    if (algorithmName == "TcpVegas")
    {
        NS_ABORT_MSG_UNLESS(TypeId::LookupByNameFailSafe("ns3::TcpVegas", &tid),
                            "ns3::TcpVegas is not registered in this build.");
        return "ns3::TcpVegas";
    }

    if (algorithmName == "QuickVegas")
    {
        if (TypeId::LookupByNameFailSafe("ns3::TcpQuickVegas", &tid))
        {
            return "ns3::TcpQuickVegas";
        }
        NS_ABORT_MSG("ns3::TcpQuickVegas is not registered in this build.");
    }

    NS_ABORT_MSG("Unsupported algorithm name: " << algorithmName);
}

bool
HasAttribute(const std::string& typeName, const std::string& attributeName)
{
    TypeId tid;
    if (!TypeId::LookupByNameFailSafe(typeName, &tid))
    {
        return false;
    }

    TypeId::AttributeInformation info;
    return tid.LookupAttributeByName(attributeName, &info);
}

std::string
DataRateFromPacketsPerSecond(uint32_t packetsPerSecond, uint32_t packetSize)
{
    const uint64_t bitsPerSecond = static_cast<uint64_t>(packetsPerSecond) * packetSize * 8ULL;
    return std::to_string(bitsPerSecond) + "bps";
}

double
OfferedLoadMbps(const ExperimentConfig& cfg)
{
    return static_cast<double>(cfg.flows) * cfg.packetsPerSecond * cfg.packetSize * 8.0 / 1e6;
}

double
ParseRateToBitsPerSecond(const std::string& rate)
{
    DataRate dataRate(rate);
    return static_cast<double>(dataRate.GetBitRate());
}

double
ParseDelayToSeconds(const std::string& delay)
{
    return Time(delay).GetSeconds();
}

uint32_t
ComputeBdpPackets(const ExperimentConfig& cfg, const std::string& bottleneckRate)
{
    const double roundTripSeconds =
        2.0 * (ParseDelayToSeconds(cfg.accessDelay) + ParseDelayToSeconds(cfg.bottleneckDelay) +
               ParseDelayToSeconds(cfg.accessDelay));
    const double bdpBits = ParseRateToBitsPerSecond(bottleneckRate) * roundTripSeconds;
    const double packetBits = static_cast<double>(cfg.tcpSegmentSize) * 8.0;
    return std::max<uint32_t>(1, static_cast<uint32_t>(std::ceil(bdpBits / packetBits)));
}

std::string
ResolveBottleneckRate(const ExperimentConfig& cfg)
{
    if (cfg.bottleneckRate != "auto")
    {
        return cfg.bottleneckRate;
    }

    const double offeredLoadMbps = OfferedLoadMbps(cfg);
    const double targetBottleneckMbps = std::max(100.0, offeredLoadMbps * 0.80);

    std::ostringstream rate;
    rate << std::fixed << std::setprecision(3) << targetBottleneckMbps << "Mbps";
    return rate.str();
}

void
WriteMetricDat(const std::string& outputDir,
               const std::string& sweepId,
               const std::string& metricName,
               const std::string& xLabel,
               const MetricSeries& series)
{
    std::filesystem::create_directories(outputDir);

    const std::string filePath = outputDir + "/" + sweepId + "-" + metricName + ".dat";
    std::ofstream out(filePath, std::ios::trunc);
    NS_ABORT_MSG_UNLESS(out.is_open(), "Unable to open output file: " << filePath);

    const bool haveTcpVegas = !series.tcpVegas.empty();
    const bool haveQuickVegas = !series.quickVegas.empty();
    NS_ABORT_MSG_UNLESS(haveTcpVegas || haveQuickVegas,
                        "No metric samples available for " << sweepId << " / " << metricName);

    out << "# " << sweepId << "\n";
    out << "# columns: " << xLabel;
    if (haveTcpVegas)
    {
        out << " TcpVegas";
    }
    if (haveQuickVegas)
    {
        out << " QuickVegas";
    }
    out << "\n";
    out << std::fixed << std::setprecision(6);

    std::set<uint32_t> levels;
    for (const auto& [level, value] : series.tcpVegas)
    {
        levels.insert(level);
    }
    for (const auto& [level, value] : series.quickVegas)
    {
        levels.insert(level);
    }

    for (uint32_t level : levels)
    {
        out << level;

        if (haveTcpVegas)
        {
            const auto vegasIt = series.tcpVegas.find(level);
            NS_ABORT_MSG_UNLESS(vegasIt != series.tcpVegas.end(),
                                "Missing TcpVegas result for level " << level << " in " << sweepId);
            out << '\t' << vegasIt->second;
        }

        if (haveQuickVegas)
        {
            const auto quickIt = series.quickVegas.find(level);
            NS_ABORT_MSG_UNLESS(quickIt != series.quickVegas.end(),
                                "Missing QuickVegas result for level " << level << " in " << sweepId);
            out << '\t' << quickIt->second;
        }

        out << '\n';
    }
}

RunMetrics
RunSingleSimulation(const ExperimentConfig& cfg, const std::string& algorithmName)
{
    RunMetrics metrics;

    const std::string socketType = ResolveSocketType(algorithmName);
    const std::string bottleneckRate = ResolveBottleneckRate(cfg);
    const uint32_t bdpPackets = ComputeBdpPackets(cfg, bottleneckRate);
    const uint32_t queuePackets = std::max(cfg.queuePackets, 2 * bdpPackets);
    const uint32_t socketBufferBytes = std::max<uint32_t>(8 * 1024 * 1024, 4 * bdpPackets * cfg.tcpSegmentSize);
    NS_LOG_INFO("Running " << algorithmName << " with offeredLoad=" << OfferedLoadMbps(cfg)
                           << " Mbps, bottleneck=" << bottleneckRate
                           << ", bottleneckDelay=" << cfg.bottleneckDelay
                           << ", estimatedBdpPackets=" << bdpPackets
                           << ", queuePackets=" << queuePackets);
    Config::SetDefault("ns3::TcpL4Protocol::SocketType", StringValue(socketType));
    Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue(cfg.tcpSegmentSize));
    Config::SetDefault("ns3::TcpSocket::SndBufSize", UintegerValue(socketBufferBytes));
    Config::SetDefault("ns3::TcpSocket::RcvBufSize", UintegerValue(socketBufferBytes));
    Config::SetDefault("ns3::DropTailQueue<Packet>::MaxSize",
                       QueueSizeValue(QueueSize(QueueSizeUnit::PACKETS, queuePackets)));

    if (algorithmName == "QuickVegas")
    {
        
        Config::SetDefault("ns3::TcpVegas::Alpha", UintegerValue(3));
        Config::SetDefault("ns3::TcpVegas::Beta", UintegerValue(5));
        Config::SetDefault("ns3::TcpVegas::Gamma", UintegerValue(2));

        TypeId quickTid;
        if (TypeId::LookupByNameFailSafe("ns3::TcpQuickVegas", &quickTid))
        {
            if (HasAttribute("ns3::TcpQuickVegas", "SlowStartMode"))
            {
                Config::SetDefault("ns3::TcpQuickVegas::SlowStartMode", UintegerValue(2));
            }
        }
    }
    else
    {
        Config::SetDefault("ns3::TcpVegas::Alpha", UintegerValue(2));
        Config::SetDefault("ns3::TcpVegas::Beta", UintegerValue(4));
        Config::SetDefault("ns3::TcpVegas::Gamma", UintegerValue(1));
    }

    NodeContainer sources;
    NodeContainer routers;
    NodeContainer destinations;
    sources.Create(cfg.nodes);
    routers.Create(2);
    destinations.Create(cfg.nodes);

    PointToPointHelper access;
    access.SetDeviceAttribute("DataRate", StringValue(cfg.accessRate));
    access.SetChannelAttribute("Delay", StringValue(cfg.accessDelay));
    access.SetQueue("ns3::DropTailQueue",
                    "MaxSize",
                    QueueSizeValue(QueueSize(std::to_string(queuePackets) + "p")));

    PointToPointHelper bottleneck;
    bottleneck.SetDeviceAttribute("DataRate", StringValue(bottleneckRate));
    bottleneck.SetChannelAttribute("Delay", StringValue(cfg.bottleneckDelay));
    bottleneck.SetQueue("ns3::DropTailQueue",
                        "MaxSize",
                        QueueSizeValue(QueueSize(std::to_string(queuePackets) + "p")));

    std::vector<NetDeviceContainer> sourceToRouter(cfg.nodes);
    std::vector<NetDeviceContainer> routerToDest(cfg.nodes);
    for (uint32_t i = 0; i < cfg.nodes; ++i)
    {
        sourceToRouter[i] = access.Install(sources.Get(i), routers.Get(0));
        routerToDest[i] = access.Install(routers.Get(1), destinations.Get(i));
    }
    NetDeviceContainer routerToRouter = bottleneck.Install(routers.Get(0), routers.Get(1));

    InternetStackHelper internet;
    internet.Install(sources);
    internet.Install(routers);
    internet.Install(destinations);

    Ipv4AddressHelper address;
    std::vector<Ipv4InterfaceContainer> sourceIfaces(cfg.nodes);
    std::vector<Ipv4InterfaceContainer> destIfaces(cfg.nodes);

    for (uint32_t i = 0; i < cfg.nodes; ++i)
    {
        std::ostringstream subnet;
        subnet << "10." << (1 + (i / 254)) << "." << (1 + (i % 254)) << ".0";
        address.SetBase(subnet.str().c_str(), "255.255.255.0");
        sourceIfaces[i] = address.Assign(sourceToRouter[i]);
    }

    address.SetBase("11.0.0.0", "255.255.255.0");
    address.Assign(routerToRouter);

    for (uint32_t i = 0; i < cfg.nodes; ++i)
    {
        std::ostringstream subnet;
        subnet << "12." << (1 + (i / 254)) << "." << (1 + (i % 254)) << ".0";
        address.SetBase(subnet.str().c_str(), "255.255.255.0");
        destIfaces[i] = address.Assign(routerToDest[i]);
    }

    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    const std::string appRate = DataRateFromPacketsPerSecond(cfg.packetsPerSecond, cfg.packetSize);
    const uint16_t basePort = 50000;
    std::set<uint16_t> sinkPorts;

    ApplicationContainer sinkApps;
    ApplicationContainer sourceApps;

    for (uint32_t flowIndex = 0; flowIndex < cfg.flows; ++flowIndex)
    {
        const uint32_t pairIndex = flowIndex % cfg.nodes;
        const uint16_t port = basePort + flowIndex;
        sinkPorts.insert(port);

        Address sinkAddress(InetSocketAddress(destIfaces[pairIndex].GetAddress(1), port));

        PacketSinkHelper sinkHelper("ns3::TcpSocketFactory",
                                    InetSocketAddress(Ipv4Address::GetAny(), port));
        sinkApps.Add(sinkHelper.Install(destinations.Get(pairIndex)));

        OnOffHelper sourceHelper("ns3::TcpSocketFactory", sinkAddress);
        sourceHelper.SetAttribute("PacketSize", UintegerValue(cfg.packetSize));
        sourceHelper.SetAttribute("DataRate", StringValue(appRate));
        sourceHelper.SetAttribute("OnTime",
                                  StringValue("ns3::ConstantRandomVariable[Constant=1]"));
        sourceHelper.SetAttribute("OffTime",
                                  StringValue("ns3::ConstantRandomVariable[Constant=0]"));
        sourceApps.Add(sourceHelper.Install(sources.Get(pairIndex)));

        const double startTime = 1.0 + 0.001 * flowIndex;
        sourceApps.Get(sourceApps.GetN() - 1)->SetStartTime(Seconds(startTime));
        sourceApps.Get(sourceApps.GetN() - 1)->SetStopTime(Seconds(cfg.simDuration));
        sinkApps.Get(sinkApps.GetN() - 1)->SetStartTime(Seconds(0.0));
        sinkApps.Get(sinkApps.GetN() - 1)->SetStopTime(Seconds(cfg.simDuration + 1.0));
    }

    FlowMonitorHelper flowHelper;
    Ptr<FlowMonitor> flowMonitor = flowHelper.InstallAll();
    Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(flowHelper.GetClassifier());

    Simulator::Stop(Seconds(cfg.simDuration + 1.0));
    Simulator::Run();

    flowMonitor->CheckForLostPackets();
    const auto stats = flowMonitor->GetFlowStats();

    uint64_t totalTxPackets = 0;
    uint64_t totalRxPackets = 0;
    uint64_t totalLostPackets = 0;
    uint64_t totalRxBytes = 0;
    Time totalDelay = Seconds(0);

    for (const auto& [flowId, stat] : stats)
    {
        const Ipv4FlowClassifier::FiveTuple tuple = classifier->FindFlow(flowId);
        if (tuple.protocol != 6)
        {
            continue;
        }
        if (sinkPorts.find(tuple.destinationPort) == sinkPorts.end())
        {
            continue;
        }

        totalTxPackets += stat.txPackets;
        totalRxPackets += stat.rxPackets;
        totalLostPackets += stat.lostPackets;
        totalRxBytes += stat.rxBytes;
        totalDelay += stat.delaySum;
    }

    if (cfg.simDuration > 0.0)
    {
        metrics.throughputMbps = (static_cast<double>(totalRxBytes) * 8.0) / cfg.simDuration / 1e6;
    }
    if (totalRxPackets > 0)
    {
        metrics.delayMs = (totalDelay.GetSeconds() * 1000.0) / static_cast<double>(totalRxPackets);
    }
    if (totalTxPackets > 0)
    {
        metrics.pdr = static_cast<double>(totalRxPackets) / static_cast<double>(totalTxPackets);
        metrics.dropRatio = static_cast<double>(totalLostPackets) / static_cast<double>(totalTxPackets);
    }

    Simulator::Destroy();
    return metrics;
}

void
StoreMetric(SweepResults& results, uint32_t level, const std::string& algorithmName, const RunMetrics& m)
{
    MetricSeries* throughput = &results.throughput;
    MetricSeries* delay = &results.delay;
    MetricSeries* pdr = &results.pdr;
    MetricSeries* dropRatio = &results.dropRatio;

    if (algorithmName == "TcpVegas")
    {
        throughput->tcpVegas[level] = m.throughputMbps;
        delay->tcpVegas[level] = m.delayMs;
        pdr->tcpVegas[level] = m.pdr;
        dropRatio->tcpVegas[level] = m.dropRatio;
    }
    else
    {
        throughput->quickVegas[level] = m.throughputMbps;
        delay->quickVegas[level] = m.delayMs;
        pdr->quickVegas[level] = m.pdr;
        dropRatio->quickVegas[level] = m.dropRatio;
    }
}

} // namespace

int
main(int argc, char* argv[])
{
    std::string outputDir = "results/quick-vegas-dumbbell";
    bool enableLogging = true;
    bool includeTcpVegas = true;
    bool includeQuickVegas = true;
    std::string bottleneckRate = "auto";
    std::string bottleneckDelay = "250ms";
    uint32_t queuePackets = 4000;

    CommandLine cmd(__FILE__);
    cmd.AddValue("outputDir", "Directory where the metric .dat files will be written", outputDir);
    cmd.AddValue("enableLogging", "Print per-run summaries to stdout", enableLogging);
    cmd.AddValue("includeTcpVegas", "Run TcpVegas and include it in the output .dat files", includeTcpVegas);
    cmd.AddValue("includeQuickVegas", "Run QuickVegas and include it in the output .dat files", includeQuickVegas);
    cmd.AddValue("bottleneckRate",
                 "Bottleneck link rate. Use 'auto' to keep the path congested while preserving a high-BDP bottleneck.",
                 bottleneckRate);
    cmd.AddValue("bottleneckDelay", "One-way bottleneck propagation delay for the high-BDP path", bottleneckDelay);
    cmd.AddValue("queuePackets", "Minimum DropTail queue size in packets on all links; actual size is at least 2x BDP", queuePackets);
    cmd.Parse(argc, argv);

    if (enableLogging)
    {
        LogComponentEnable("QuickVegasDumbbellSweeps", LOG_LEVEL_INFO);
    }

    NS_ABORT_MSG_UNLESS(includeTcpVegas || includeQuickVegas,
                        "At least one congestion control algorithm must be enabled.");

    const ExperimentConfig baseline;
    ExperimentConfig scenario = baseline;
    scenario.bottleneckRate = bottleneckRate;
    scenario.bottleneckDelay = bottleneckDelay;
    scenario.queuePackets = queuePackets;
    std::vector<std::string> algorithms;
    if (includeTcpVegas)
    {
        algorithms.push_back("TcpVegas");
    }
    if (includeQuickVegas)
    {
        algorithms.push_back("QuickVegas");
    }
    const std::vector<SweepDefinition> sweeps = {
        {"sweep1_nodes", "nodes", "Vary number of source-destination pairs", {20, 40, 60}},
        {"sweep2_flows", "flows", "Vary number of active flows", {10, 20, 30}},
        {"sweep3_packets_per_sec", "packets_per_second", "Vary application packets per second", {1000, 2000, 3000}},
        {"sweep4_flows_delay_drop", "flows", "Repeat flow sweep for delay and drop focus", {10, 20, 30}},
    };

    std::map<std::string, SweepResults> allResults;

    for (const auto& sweep : sweeps)
    {
        NS_LOG_INFO("Starting " << sweep.id << " - " << sweep.title);

        for (uint32_t level : sweep.levels)
        {
            ExperimentConfig cfg = scenario;

            if (sweep.id == "sweep1_nodes")
            {
                cfg.nodes = level;
            }
            else if (sweep.id == "sweep2_flows" || sweep.id == "sweep4_flows_delay_drop")
            {
                cfg.flows = level;
            }
            else if (sweep.id == "sweep3_packets_per_sec")
            {
                cfg.packetsPerSecond = level;
            }

            for (const auto& algorithm : algorithms)
            {
                const RunMetrics metrics = RunSingleSimulation(cfg, algorithm);
                StoreMetric(allResults[sweep.id], level, algorithm, metrics);

                NS_LOG_INFO("Completed " << sweep.id << " level=" << level << " algorithm=" << algorithm
                            << " throughput(Mbps)=" << metrics.throughputMbps
                            << " delay(ms)=" << metrics.delayMs
                            << " pdr=" << metrics.pdr
                            << " dropRatio=" << metrics.dropRatio);
            }
        }

        WriteMetricDat(outputDir, sweep.id, "throughput-mbps", sweep.xLabel, allResults[sweep.id].throughput);
        WriteMetricDat(outputDir, sweep.id, "delay-ms", sweep.xLabel, allResults[sweep.id].delay);
        WriteMetricDat(outputDir, sweep.id, "pdr", sweep.xLabel, allResults[sweep.id].pdr);
        WriteMetricDat(outputDir, sweep.id, "drop-ratio", sweep.xLabel, allResults[sweep.id].dropRatio);
    }

    std::ofstream manifest(outputDir + "/README.txt", std::ios::trunc);
    if (manifest.is_open())
    {
        manifest << "QuickVegas dumbbell experiment outputs\n";
        manifest << "Each .dat file has columns: x TcpVegas QuickVegas\n";
        manifest << "Metrics: throughput-mbps, delay-ms, pdr, drop-ratio\n";
        manifest << "Sweeps: sweep1_nodes, sweep2_flows, sweep3_packets_per_sec, sweep4_flows_delay_drop\n";
    }

    return 0;
}
