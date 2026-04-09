#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/energy-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/internet-module.h"
#include "ns3/lr-wpan-module.h"
#include "ns3/mobility-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/sixlowpan-module.h"
#include "ns3/tcp-quick-vegas.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <map>
#include <numeric>
#include <random>
#include <set>
#include <sstream>
#include <string>
#include <vector>

using namespace ns3;
using namespace ns3::energy;

NS_LOG_COMPONENT_DEFINE("QuickVegas802154StarMesh");

namespace
{

constexpr uint16_t kSinkPort = 9;
constexpr double kTxPowerW = 31.32e-3;
constexpr double kRxPowerW = 39.98e-3;
constexpr double kSupplyVoltageV = 3.0;
constexpr double kTxCurrentA = kTxPowerW / kSupplyVoltageV;
constexpr double kRxCurrentA = kRxPowerW / kSupplyVoltageV;

struct ExperimentConfig
{
    uint32_t nodes = 40;
    uint32_t flows = 20;
    uint32_t packetsPerSecond = 200;
    uint32_t coverage = 2;
    double txRangeMeters = 100.0;
    double simDuration = 300.0;
    uint32_t packetSize = 114;
    uint32_t tcpSegment = 114;
    uint32_t routerBuffer = 1000;
    uint32_t rngSeed = 42;
};

struct RunMetrics
{
    double throughputMbps = 0.0;
    double delayMs = 0.0;
    double pdr = 0.0;
    double dropRatio = 0.0;
    double energyJoules = 0.0;
};

struct DatRow
{
    uint32_t xValue = 0;
    std::string algorithm;
    double throughputMbps = 0.0;
    double delayMs = 0.0;
    double pdr = 0.0;
    double dropRatio = 0.0;
    double energyJoules = 0.0;
    uint32_t nodes = 0;
    uint32_t flows = 0;
    uint32_t packetsPerSecond = 0;
    uint32_t coverage = 0;
    double simDuration = 0.0;
};

struct SweepDefinition
{
    std::string id;
    std::string xLabel;
    std::vector<uint32_t> levels;
};

struct EnergyContext
{
    Ptr<BasicEnergySource> source;
    Ptr<SimpleDeviceEnergyModel> model;
};

class PeriodicTcpSender : public Application
{
  public:
    PeriodicTcpSender() = default;

    void Setup(const Address& peer, uint32_t packetSize, uint32_t packetsPerSecond)
    {
        m_peer = peer;
        m_packetSize = packetSize;
        m_interval = Seconds(1.0 / static_cast<double>(packetsPerSecond));
    }

  private:
    void StartApplication() override
    {
        if (!m_socket)
        {
            m_socket = Socket::CreateSocket(GetNode(), TcpSocketFactory::GetTypeId());
            m_socket->SetConnectCallback(
                MakeCallback(&PeriodicTcpSender::HandleConnectSuccess, this),
                MakeCallback(&PeriodicTcpSender::HandleConnectFail, this));
            m_socket->SetSendCallback(MakeCallback(&PeriodicTcpSender::HandleSendReady, this));
            m_socket->Connect(m_peer);
        }
    }

    void StopApplication() override
    {
        m_sendEvent.Cancel();
        m_retryEvent.Cancel();
        if (m_socket)
        {
            m_socket->Close();
            m_socket = nullptr;
        }
        m_connected = false;
    }

    void HandleConnectSuccess(Ptr<Socket> socket)
    {
        m_connected = true;
        ScheduleNextTx();
    }

    void HandleConnectFail(Ptr<Socket> socket)
    {
        m_connected = false;
        m_retryEvent.Cancel();
        m_retryEvent = Simulator::Schedule(Seconds(1.0), &PeriodicTcpSender::RetryConnect, this);
    }

    void RetryConnect()
    {
        if (m_socket && !m_connected)
        {
            m_socket->Connect(m_peer);
        }
    }

    void HandleSendReady(Ptr<Socket> socket, uint32_t available)
    {
        if (m_connected && !m_sendEvent.IsRunning())
        {
            ScheduleNextTx();
        }
    }

    void ScheduleNextTx()
    {
        if (!m_sendEvent.IsRunning())
        {
            m_sendEvent = Simulator::Schedule(m_interval, &PeriodicTcpSender::SendPacket, this);
        }
    }

    void SendPacket()
    {
        if (!m_connected || !m_socket)
        {
            return;
        }

        Ptr<Packet> packet = Create<Packet>(m_packetSize);
        const int sent = m_socket->Send(packet);
        if (sent >= 0)
        {
            ScheduleNextTx();
        }
        else
        {
            
            m_sendEvent = Simulator::Schedule(MilliSeconds(50), &PeriodicTcpSender::SendPacket, this);
        }
    }

    Ptr<Socket> m_socket;
    Address m_peer;
    uint32_t m_packetSize = 0;
    Time m_interval = Seconds(1.0);
    bool m_connected = false;
    EventId m_sendEvent;
    EventId m_retryEvent;
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

    if (algorithmName == "TcpQuickVegas")
    {
        NS_ABORT_MSG_UNLESS(TypeId::LookupByNameFailSafe("ns3::TcpQuickVegas", &tid),
                            "ns3::TcpQuickVegas is not registered in this build.");
        return "ns3::TcpQuickVegas";
    }

    NS_ABORT_MSG("Unsupported TCP algorithm name: " << algorithmName);
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

uint32_t
ScenarioSeed(const ExperimentConfig& cfg)
{
    uint32_t seed = cfg.rngSeed;
    seed ^= cfg.nodes * 2654435761u;
    seed ^= cfg.flows * 40503u;
    seed ^= cfg.packetsPerSecond * 97u;
    seed ^= cfg.coverage * 811u;
    return seed;
}

void
OnPhyStateChange(Ptr<SimpleDeviceEnergyModel> model,
                 Time time,
                 lrwpan::PhyEnumeration oldState,
                 lrwpan::PhyEnumeration newState)
{
    (void)time;
    (void)oldState;

    double currentA = 0.0;
    if (newState == lrwpan::IEEE_802_15_4_PHY_BUSY_TX)
    {
        currentA = kTxCurrentA;
    }
    else if (newState == lrwpan::IEEE_802_15_4_PHY_BUSY_RX)
    {
        currentA = kRxCurrentA;
    }

    model->SetCurrentA(currentA);
}

void
PlaceCoordinatorAndSensors(Ptr<Node> coordinator,
                           const NodeContainer& sensors,
                           uint32_t sensorCount,
                           double spacing)
{
    const uint32_t columns = static_cast<uint32_t>(std::ceil(std::sqrt(static_cast<double>(sensorCount))));
    const uint32_t rows = static_cast<uint32_t>(std::ceil(static_cast<double>(sensorCount) / columns));
    const uint32_t centerColumn = columns / 2;
    const uint32_t centerRow = rows / 2;
    const double width = (columns > 1) ? (columns - 1) * spacing : 0.0;
    const double height = (rows > 1) ? (rows - 1) * spacing : 0.0;

    Ptr<ConstantPositionMobilityModel> coordMobility = coordinator->GetObject<ConstantPositionMobilityModel>();
    coordMobility->SetPosition(Vector(width / 2.0, height / 2.0, 0.0));

    uint32_t sensorIndex = 0;
    for (uint32_t row = 0; row < rows && sensorIndex < sensorCount; ++row)
    {
        for (uint32_t col = 0; col < columns && sensorIndex < sensorCount; ++col)
        {
            if (row == centerRow && col == centerColumn)
            {
                continue;
            }

            Ptr<ConstantPositionMobilityModel> mobility =
                sensors.Get(sensorIndex)->GetObject<ConstantPositionMobilityModel>();
            mobility->SetPosition(Vector(col * spacing, row * spacing, 0.0));
            sensorIndex++;
        }
    }

    while (sensorIndex < sensorCount)
    {
        const uint32_t extra = sensorIndex - rows * columns;
        Ptr<ConstantPositionMobilityModel> mobility =
            sensors.Get(sensorIndex)->GetObject<ConstantPositionMobilityModel>();
        mobility->SetPosition(Vector(width + spacing, extra * spacing, 0.0));
        sensorIndex++;
    }
}

void
ConfigureTcpVariant(const std::string& algorithmName, uint32_t tcpSegment)
{
    const std::string socketType = ResolveSocketType(algorithmName);
    Config::SetDefault("ns3::TcpL4Protocol::SocketType", StringValue(socketType));
    Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue(tcpSegment));
    Config::SetDefault("ns3::TcpSocket::SndBufSize", UintegerValue(256 * 1024));
    Config::SetDefault("ns3::TcpSocket::RcvBufSize", UintegerValue(256 * 1024));

    if (algorithmName == "TcpQuickVegas")
    {
        Config::SetDefault("ns3::TcpVegas::Alpha", UintegerValue(3));
        Config::SetDefault("ns3::TcpVegas::Beta", UintegerValue(5));
        Config::SetDefault("ns3::TcpVegas::Gamma", UintegerValue(2));
        if (HasAttribute("ns3::TcpQuickVegas", "SlowStartMode"))
        {
            Config::SetDefault("ns3::TcpQuickVegas::SlowStartMode", UintegerValue(2));
        }
    }
    else
    {
        Config::SetDefault("ns3::TcpVegas::Alpha", UintegerValue(2));
        Config::SetDefault("ns3::TcpVegas::Beta", UintegerValue(4));
        Config::SetDefault("ns3::TcpVegas::Gamma", UintegerValue(1));
    }
}

RunMetrics
RunSingleScenario(const ExperimentConfig& cfg, const std::string& algorithmName)
{
    RunMetrics metrics;

    SeedManager::SetSeed(cfg.rngSeed);
    SeedManager::SetRun(1);
    RngSeedManager::SetSeed(cfg.rngSeed);
    RngSeedManager::SetRun(1);

    ConfigureTcpVariant(algorithmName, cfg.tcpSegment);

    Ptr<Node> wiredSource = CreateObject<Node>();
    Ptr<Node> wiredRouter = CreateObject<Node>();
    Ptr<Node> coordinator = CreateObject<Node>();

    NodeContainer sensorNodes;
    sensorNodes.Create(cfg.nodes);

    NodeContainer allNodes;
    allNodes.Add(wiredSource);
    allNodes.Add(wiredRouter);
    allNodes.Add(coordinator);
    allNodes.Add(sensorNodes);

    MobilityHelper mobility;
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(allNodes);

    wiredSource->GetObject<ConstantPositionMobilityModel>()->SetPosition(Vector(-40.0, 0.0, 0.0));
    wiredRouter->GetObject<ConstantPositionMobilityModel>()->SetPosition(Vector(-20.0, 0.0, 0.0));

    const double spacing = cfg.coverage * cfg.txRangeMeters /
                           std::ceil(std::sqrt(static_cast<double>(cfg.nodes)));
    PlaceCoordinatorAndSensors(coordinator, sensorNodes, cfg.nodes, spacing);

    PointToPointHelper p2p;
    p2p.SetDeviceAttribute("DataRate", StringValue("100Mbps"));
    p2p.SetChannelAttribute("Delay", StringValue("2ms"));
    p2p.SetQueue("ns3::DropTailQueue",
                 "MaxSize",
                 QueueSizeValue(QueueSize(std::to_string(cfg.routerBuffer) + "p")));

    NetDeviceContainer sourceRouterDevices = p2p.Install(wiredSource, wiredRouter);
    NetDeviceContainer routerCoordinatorDevices = p2p.Install(wiredRouter, coordinator);

    NodeContainer wirelessNodes;
    wirelessNodes.Add(coordinator);
    wirelessNodes.Add(sensorNodes);

    LrWpanHelper lrWpanHelper;
    lrWpanHelper.SetPropagationDelayModel("ns3::ConstantSpeedPropagationDelayModel");
    lrWpanHelper.AddPropagationLossModel("ns3::LogDistancePropagationLossModel");
    NetDeviceContainer lrwpanDevices = lrWpanHelper.Install(wirelessNodes);
    lrWpanHelper.CreateAssociatedPan(lrwpanDevices, 0x1234);

    InternetStackHelper internet;
    internet.SetIpv4StackInstall(false);
    internet.Install(allNodes);

    SixLowPanHelper sixlowpan;
    NetDeviceContainer sixlowpanDevices = sixlowpan.Install(lrwpanDevices);
    for (uint32_t i = 0; i < sixlowpanDevices.GetN(); ++i)
    {
        sixlowpanDevices.Get(i)->SetAttribute("UseMeshUnder", BooleanValue(true));
        sixlowpanDevices.Get(i)->SetAttribute("MeshUnderRadius", UintegerValue(10));
    }

    Ipv6AddressHelper ipv6;
    ipv6.SetBase(Ipv6Address("2001:1::"), Ipv6Prefix(64));
    Ipv6InterfaceContainer sourceRouterIfaces = ipv6.Assign(sourceRouterDevices);
    ipv6.SetBase(Ipv6Address("2001:2::"), Ipv6Prefix(64));
    Ipv6InterfaceContainer routerCoordinatorIfaces = ipv6.Assign(routerCoordinatorDevices);
    ipv6.SetBase(Ipv6Address("2001:3::"), Ipv6Prefix(64));
    Ipv6InterfaceContainer wirelessIfaces = ipv6.Assign(sixlowpanDevices);

    sourceRouterIfaces.SetForwarding(1, true);
    routerCoordinatorIfaces.SetForwarding(0, true);
    routerCoordinatorIfaces.SetForwarding(1, true);
    wirelessIfaces.SetForwarding(0, true);

    Ipv6StaticRoutingHelper routingHelper;

    Ptr<Ipv6StaticRouting> sourceRouting =
        routingHelper.GetStaticRouting(wiredSource->GetObject<Ipv6>());
    sourceRouting->SetDefaultRoute(
        sourceRouterIfaces.GetAddress(1, 1),
        wiredSource->GetObject<Ipv6>()->GetInterfaceForDevice(sourceRouterDevices.Get(0)));

    Ptr<Ipv6StaticRouting> routerRouting =
        routingHelper.GetStaticRouting(wiredRouter->GetObject<Ipv6>());
    routerRouting->AddNetworkRouteTo(
        Ipv6Address("2001:3::"),
        Ipv6Prefix(64),
        routerCoordinatorIfaces.GetAddress(1, 1),
        wiredRouter->GetObject<Ipv6>()->GetInterfaceForDevice(routerCoordinatorDevices.Get(0)));

    Ptr<Ipv6StaticRouting> coordinatorRouting =
        routingHelper.GetStaticRouting(coordinator->GetObject<Ipv6>());
    coordinatorRouting->SetDefaultRoute(
        routerCoordinatorIfaces.GetAddress(0, 1),
        coordinator->GetObject<Ipv6>()->GetInterfaceForDevice(routerCoordinatorDevices.Get(1)));

    for (uint32_t i = 0; i < cfg.nodes; ++i)
    {
        Ptr<Node> sensor = sensorNodes.Get(i);
        Ptr<Ipv6StaticRouting> sensorRouting =
            routingHelper.GetStaticRouting(sensor->GetObject<Ipv6>());
        sensorRouting->SetDefaultRoute(
            wirelessIfaces.GetAddress(0, 1),
            sensor->GetObject<Ipv6>()->GetInterfaceForDevice(sixlowpanDevices.Get(i + 1)));
    }

    BasicEnergySourceHelper energySourceHelper;
    energySourceHelper.Set("BasicEnergySourceInitialEnergyJ", DoubleValue(10000.0));
    energySourceHelper.Set("BasicEnergySupplyVoltageV", DoubleValue(kSupplyVoltageV));
    EnergySourceContainer energySources = energySourceHelper.Install(sensorNodes);

    std::vector<EnergyContext> energyContexts(cfg.nodes);
    for (uint32_t i = 0; i < cfg.nodes; ++i)
    {
        Ptr<BasicEnergySource> source = DynamicCast<BasicEnergySource>(energySources.Get(i));
        Ptr<SimpleDeviceEnergyModel> model = CreateObject<SimpleDeviceEnergyModel>();
        model->SetNode(sensorNodes.Get(i));
        model->SetEnergySource(source);
        model->SetCurrentA(0.0);
        source->AppendDeviceEnergyModel(model);
        energyContexts[i] = {source, model};

        Ptr<lrwpan::LrWpanNetDevice> sensorDevice =
            DynamicCast<lrwpan::LrWpanNetDevice>(lrwpanDevices.Get(i + 1));
        sensorDevice->GetPhy()->TraceConnectWithoutContext(
            "TrxState",
            MakeBoundCallback(&OnPhyStateChange, model));
    }

    PacketSinkHelper sinkHelper(
        "ns3::TcpSocketFactory",
        Inet6SocketAddress(Ipv6Address::GetAny(), kSinkPort));
    ApplicationContainer sinkApp = sinkHelper.Install(wiredSource);
    sinkApp.Start(Seconds(0.0));
    sinkApp.Stop(Seconds(cfg.simDuration + 1.0));

    const Ipv6Address sinkAddress = sourceRouterIfaces.GetAddress(0, 1);

    std::mt19937 rng(ScenarioSeed(cfg));
    std::uniform_int_distribution<uint32_t> sensorPick(0, cfg.nodes - 1);

    ApplicationContainer sourceApps;
    for (uint32_t flow = 0; flow < cfg.flows; ++flow)
    {
        const uint32_t sensorIndex = sensorPick(rng);
        Ptr<PeriodicTcpSender> app = CreateObject<PeriodicTcpSender>();
        app->Setup(Inet6SocketAddress(sinkAddress, kSinkPort), cfg.packetSize, cfg.packetsPerSecond);
        sensorNodes.Get(sensorIndex)->AddApplication(app);
        app->SetStartTime(Seconds(1.0 + 0.01 * flow));
        app->SetStopTime(Seconds(cfg.simDuration));
        sourceApps.Add(app);
    }

    FlowMonitorHelper flowHelper;
    Ptr<FlowMonitor> flowMonitor = flowHelper.InstallAll();
    Ptr<Ipv6FlowClassifier> classifier = DynamicCast<Ipv6FlowClassifier>(flowHelper.GetClassifier6());

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
        const Ipv6FlowClassifier::FiveTuple tuple = classifier->FindFlow(flowId);
        if (tuple.protocol != 6)
        {
            continue;
        }
        if (tuple.destinationPort != kSinkPort)
        {
            continue;
        }
        if (tuple.destinationAddress != sinkAddress)
        {
            continue;
        }

        totalTxPackets += stat.txPackets;
        totalRxPackets += stat.rxPackets;
        totalRxBytes += stat.rxBytes;
        totalDelay += stat.delaySum;

        uint64_t lostPackets = stat.lostPackets;
        if (lostPackets == 0 && stat.txPackets > stat.rxPackets)
        {
            lostPackets = stat.txPackets - stat.rxPackets;
        }
        totalLostPackets += lostPackets;
    }

    metrics.throughputMbps = (static_cast<double>(totalRxBytes) * 8.0) / cfg.simDuration / 1e6;
    if (totalRxPackets > 0)
    {
        metrics.delayMs = totalDelay.GetSeconds() * 1000.0 / static_cast<double>(totalRxPackets);
    }
    if (totalTxPackets > 0)
    {
        metrics.pdr = static_cast<double>(totalRxPackets) / static_cast<double>(totalTxPackets);
        metrics.dropRatio = static_cast<double>(totalLostPackets) / static_cast<double>(totalTxPackets);
    }

    for (const auto& ctx : energyContexts)
    {
        metrics.energyJoules += 10000.0 - ctx.source->GetRemainingEnergy();
    }

    Simulator::Destroy();
    return metrics;
}

void
WriteDat(const std::string& outputDir,
         const std::string& fileName,
         const std::string& xLabel,
         const std::vector<DatRow>& rows)
{
    std::filesystem::create_directories(outputDir);
    const std::string path = outputDir + "/" + fileName;
    std::ofstream out(path, std::ios::trunc);
    NS_ABORT_MSG_UNLESS(out.is_open(), "Unable to open output DAT: " << path);

    out << "# x_label\t" << xLabel << "\n";
    out << "# columns: x_value algorithm throughput_mbps delay_ms pdr drop_ratio energy_joules "
           "nodes flows packets_per_sec coverage sim_duration\n";
    out << std::fixed << std::setprecision(6);

    for (const auto& row : rows)
    {
        out << row.xValue << '\t' << row.algorithm << '\t' << row.throughputMbps << '\t'
            << row.delayMs << '\t' << row.pdr << '\t' << row.dropRatio << '\t' << row.energyJoules << '\t'
            << row.nodes << '\t' << row.flows << '\t' << row.packetsPerSecond << '\t' << row.coverage << '\t'
            << row.simDuration << '\n';
    }
}

} // namespace

int
main(int argc, char* argv[])
{
    std::string outputDir = "results/quick-vegas-802154";
    std::string dataDir = "results/quick-vegas-802154/data";
    bool enableLogging = true;

    CommandLine cmd(__FILE__);
    cmd.AddValue("outputDir", "Directory for CSV result files", outputDir);
    cmd.AddValue("dataDir", "Directory for sweep .dat result files", dataDir);
    cmd.AddValue("enableLogging", "Enable per-run logging", enableLogging);
    cmd.Parse(argc, argv);

    if (enableLogging)
    {
        LogComponentEnable("QuickVegas802154StarMesh", LOG_LEVEL_INFO);
    }

    const ExperimentConfig baseline;
    const std::vector<std::string> algorithms = {"TcpVegas", "TcpQuickVegas"};
    const std::vector<SweepDefinition> sweeps = {
        {"nodes", "nodes", {20, 40, 60, 80, 100}},
        {"flows", "flows", {10, 20, 30, 40, 50}},
        {"pkts", "packets_per_sec", {100, 200, 300, 400, 500}},
        {"coverage", "coverage", {1, 2, 3, 4, 5}},
    };

    std::map<std::string, std::vector<DatRow>> rowsPerSweep;

    for (const auto& sweep : sweeps)
    {
        for (uint32_t level : sweep.levels)
        {
            ExperimentConfig cfg = baseline;
            if (sweep.id == "nodes")
            {
                cfg.nodes = level;
            }
            else if (sweep.id == "flows")
            {
                cfg.flows = level;
            }
            else if (sweep.id == "pkts")
            {
                cfg.packetsPerSecond = level;
            }
            else if (sweep.id == "coverage")
            {
                cfg.coverage = level;
            }

            for (const auto& algorithm : algorithms)
            {
                NS_LOG_INFO("Running sweep=" << sweep.id << " level=" << level
                            << " algorithm=" << algorithm);
                const RunMetrics metrics = RunSingleScenario(cfg, algorithm);
                rowsPerSweep[sweep.id].push_back({level,
                                                  algorithm,
                                                  metrics.throughputMbps,
                                                  metrics.delayMs,
                                                  metrics.pdr,
                                                  metrics.dropRatio,
                                                  metrics.energyJoules,
                                                  cfg.nodes,
                                                  cfg.flows,
                                                  cfg.packetsPerSecond,
                                                  cfg.coverage,
                                                  cfg.simDuration});
            }
        }
    }

    WriteDat(dataDir, "results_802154_sweep_nodes.dat", "nodes", rowsPerSweep["nodes"]);
    WriteDat(dataDir, "results_802154_sweep_flows.dat", "flows", rowsPerSweep["flows"]);
    WriteDat(dataDir, "results_802154_sweep_pkts.dat", "packets_per_sec", rowsPerSweep["pkts"]);
    WriteDat(dataDir, "results_802154_sweep_coverage.dat", "coverage", rowsPerSweep["coverage"]);

    std::ofstream readme(outputDir + "/README.txt", std::ios::trunc);
    if (readme.is_open())
    {
        readme << "QuickVegas vs TcpVegas over static IEEE 802.15.4 star-mesh with wired backbone\n";
        readme << "Data files are written in: " << dataDir << "\n";
        readme << "Files: results_802154_sweep_nodes.dat, results_802154_sweep_flows.dat, "
                  "results_802154_sweep_pkts.dat, results_802154_sweep_coverage.dat\n";
        readme << "Note: IPv6 is used end-to-end over both the wired and 6LoWPAN segments so the "
                  "benchmark remains runnable with Ipv6FlowClassifier.\n";
        readme << "Note: packets_per_sec is enforced with TCP OnOff applications because BulkSendHelper "
                  "does not provide an application rate knob for the requested sweep.\n";
    }

    return 0;
}
