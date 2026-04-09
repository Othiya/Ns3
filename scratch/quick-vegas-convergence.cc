#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/point-to-point-net-device.h"
#include "ns3/traffic-control-module.h"
#include "ns3/tcp-quick-vegas.h"

#include <algorithm>
#include <cmath>
#include <deque>
#include <filesystem>
#include <iomanip>
#include <numeric>
#include <sstream>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("QuickVegasBasic");

static std::ofstream g_cwndFile;
static std::ofstream g_rttFile;
static std::ofstream g_convergenceFile;

static uint32_t g_lastCwnd     = 0;
static uint32_t g_segmentSize  = 1000;
static double   g_samplePeriod = 0.05;
static double   g_bdpKb        = 0.0;
static Time     g_minRtt        = Seconds(0);
static bool     g_haveMinRtt    = false;
static std::deque<double> g_cwndHistory;
static std::vector<std::pair<double, double>> g_cwndSamples;

struct ConvergenceEvent
{
    std::string name;
    double      eventTimeSec = 0.0;
};

static std::vector<ConvergenceEvent> g_events;

static std::string
RateMbpsString(double rateMbps)
{
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(6) << rateMbps << "Mbps";
    return oss.str();
}

static void
ChangeBottleneckRate(Ptr<PointToPointNetDevice> devA,
                     Ptr<PointToPointNetDevice> devB,
                     const std::string& rate)
{
    if (devA)
    {
        devA->SetDataRate(DataRate(rate));
    }
    if (devB)
    {
        devB->SetDataRate(DataRate(rate));
    }
    NS_LOG_INFO("Bottleneck rate changed to " << rate << " at "
                << Simulator::Now().GetSeconds() << " s");
}

static std::vector<double>
ParseCsvDoubles(const std::string& csv)
{
    std::vector<double> values;
    std::stringstream ss(csv);
    std::string item;
    while (std::getline(ss, item, ','))
    {
        std::stringstream token(item);
        double val = 0.0;
        token >> val;
        if (val > 0.0)
        {
            values.push_back(val);
        }
    }
    return values;
}

static void
MarkEventStart(const std::string& eventName)
{
    for (auto& e : g_events)
    {
        if (e.name == eventName)
        {
            e.eventTimeSec = Simulator::Now().GetSeconds();
            NS_LOG_INFO("Convergence event started: " << eventName << " at " << e.eventTimeSec << " s");
            return;
        }
    }
}

static void
WriteConvergenceRecord(const ConvergenceEvent& ev, double convergenceSec)
{
    if (!g_convergenceFile.is_open())
    {
        return;
    }

    double baseRttSec = g_haveMinRtt ? g_minRtt.GetSeconds() : 0.0;
    double convergenceBaseRtts = (baseRttSec > 0.0) ? (convergenceSec / baseRttSec) : 0.0;

    g_convergenceFile << ev.eventTimeSec << "\t"
                      << g_bdpKb << "\t"
                      << convergenceSec << "\t"
                      << ev.name << "\t"
                      << baseRttSec << "\t"
                      << convergenceBaseRtts << "\n";
    g_convergenceFile.flush();

    NS_LOG_INFO("Convergence logged for " << ev.name
                << " | BDP=" << g_bdpKb << " Kb"
                << " | convergence=" << convergenceSec << " s"
                << " | baseRTT=" << baseRttSec << " s"
                << " | convergence(baseRTT)=" << convergenceBaseRtts);
}

static double
MedianInWindow(double startSec, double endSec)
{
    std::vector<double> vals;
    for (const auto& s : g_cwndSamples)
    {
        if (s.first >= startSec && s.first <= endSec)
        {
            vals.push_back(s.second);
        }
    }
    if (vals.empty())
    {
        return 0.0;
    }

    std::sort(vals.begin(), vals.end());
    const size_t n = vals.size();
    if ((n % 2) == 0)
    {
        return 0.5 * (vals[n / 2 - 1] + vals[n / 2]);
    }
    return vals[n / 2];
}

static double
MeanInWindow(double startSec, double endSec)
{
    double sum = 0.0;
    uint32_t cnt = 0;
    for (const auto& s : g_cwndSamples)
    {
        if (s.first >= startSec && s.first <= endSec)
        {
            sum += s.second;
            cnt++;
        }
    }
    if (cnt == 0)
    {
        return 0.0;
    }
    return sum / static_cast<double>(cnt);
}

static double
FindConvergenceSec(double eventStart,
                   double phaseEnd,
                   double preSteady,
                   double postSteady,
                   double baseRttSec)
{
    const double delta = postSteady - preSteady;
    if (std::abs(delta) < 1e-9)
    {
        return std::max(0.0, phaseEnd - eventStart);
    }

    const double holdSec = std::max(1.5, 8.0 * baseRttSec);
    const double guardSec = std::max(0.5, 2.0 * baseRttSec);
    const double targetFrac = 0.90;
    const double band = 0.08 * std::max(1.0, std::abs(postSteady));

    for (const auto& s : g_cwndSamples)
    {
        const double t = s.first;
        if (t < eventStart + guardSec || t > phaseEnd - holdSec)
        {
            continue;
        }

        const double avg = MeanInWindow(std::max(eventStart, t - 2.0 * baseRttSec), t);
        const double progress = (avg - preSteady) / delta;
        if (progress < targetFrac)
        {
            continue;
        }

        bool holds = true;
        for (const auto& f : g_cwndSamples)
        {
            if (f.first < t || f.first > t + holdSec)
            {
                continue;
            }
            if (std::abs(f.second - postSteady) > band)
            {
                holds = false;
                break;
            }
        }

        if (holds)
        {
            return t - eventStart;
        }
    }

    return std::max(0.0, phaseEnd - eventStart);
}

static double
GetEventStart(const std::string& eventName)
{
    for (const auto& e : g_events)
    {
        if (e.name == eventName)
        {
            return e.eventTimeSec;
        }
    }
    return 0.0;
}

static void
PostProcessConvergence(double simStopSec)
{
    const double baseRttSec = g_haveMinRtt ? g_minRtt.GetSeconds() : 0.1;

    const double tNew = GetEventStart("new_connection");
    const double tHalf = GetEventStart("bandwidth_halved");
    const double tDouble = GetEventStart("bandwidth_doubled");

    ConvergenceEvent evNew{"new_connection", tNew};
    ConvergenceEvent evHalf{"bandwidth_halved", tHalf};
    ConvergenceEvent evDouble{"bandwidth_doubled", tDouble};

    const double preNew = MeanInWindow(tNew, tNew + std::max(1.0, 4.0 * baseRttSec));
    const double postNew = MedianInWindow(std::max(tNew, tHalf - 20.0), tHalf - 1.0);

    const double preHalf = MedianInWindow(std::max(tNew, tHalf - 20.0), tHalf - 1.0);
    const double postHalf = MedianInWindow(std::max(tHalf, tDouble - 20.0), tDouble - 1.0);

    const double preDouble = MedianInWindow(std::max(tHalf, tDouble - 20.0), tDouble - 1.0);
    const double postDouble = MedianInWindow(std::max(tDouble, simStopSec - 20.0), simStopSec - 1.0);

    WriteConvergenceRecord(evNew, FindConvergenceSec(tNew, tHalf, preNew, postNew, baseRttSec));
    WriteConvergenceRecord(evHalf, FindConvergenceSec(tHalf, tDouble, preHalf, postHalf, baseRttSec));
    WriteConvergenceRecord(evDouble, FindConvergenceSec(tDouble, simStopSec, preDouble, postDouble, baseRttSec));
}

static void
RttTracer(Time oldRtt, Time newRtt)
{
    if (newRtt.IsStrictlyPositive() && (!g_haveMinRtt || newRtt < g_minRtt))
    {
        g_minRtt = newRtt;
        g_haveMinRtt = true;
    }

    if (g_rttFile.is_open())
    {
        g_rttFile << Simulator::Now().GetSeconds() << "\t" << newRtt.GetMilliSeconds() << "\n";
    }
}

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
    g_lastCwnd = newVal;
}

static void
CwndSampler()
{
    const double nowSec = Simulator::Now().GetSeconds();
    const double cwndPkts = static_cast<double>(g_lastCwnd) / static_cast<double>(g_segmentSize);

    if (g_cwndFile.is_open())
    {
        g_cwndFile << nowSec << "\t" << cwndPkts << "\n";
    }

    g_cwndHistory.push_back(cwndPkts);
    const uint32_t historyMax = 30;
    if (g_cwndHistory.size() > historyMax)
    {
        g_cwndHistory.pop_front();
    }

    g_cwndSamples.emplace_back(nowSec, cwndPkts);

    Simulator::Schedule(Seconds(g_samplePeriod), &CwndSampler);
}

static void
ConnectCwndTrace()
{
    Config::ConnectWithoutContext(
        "/NodeList/0/$ns3::TcpL4Protocol/SocketList/0/CongestionWindow",
        MakeCallback(&CwndTracer));
}

static void
ResetGlobals(double bdpKb)
{
    g_lastCwnd = 0;
    g_bdpKb = bdpKb;
    g_minRtt = Seconds(0);
    g_haveMinRtt = false;
    g_cwndHistory.clear();
    g_cwndSamples.clear();

    g_events = {
        {"new_connection", 0.0},
        {"bandwidth_halved", 0.0},
        {"bandwidth_doubled", 0.0},
    };
}

static bool
RunOneBdpScenario(const std::string& tcpVariant,
                  uint32_t vegasAlpha,
                  uint32_t vegasBeta,
                  uint32_t vegasGamma,
                  double bdpKb,
                  const std::string& outputDir,
                  double baseRttSec,
                  double simStopSec)
{
    ResetGlobals(bdpKb);

    std::string socketType = "ns3::" + tcpVariant;
    TypeId tid;
    if (!TypeId::LookupByNameFailSafe(socketType, &tid))
    {
        NS_LOG_ERROR("Requested tcpVariant=" << tcpVariant
                     << " is not registered in this ns-3 build.");
        return false;
    }

    Config::SetDefault("ns3::TcpL4Protocol::SocketType", StringValue(socketType));
    Config::SetDefault("ns3::TcpVegas::Alpha", UintegerValue(vegasAlpha));
    Config::SetDefault("ns3::TcpVegas::Beta", UintegerValue(vegasBeta));
    Config::SetDefault("ns3::TcpVegas::Gamma", UintegerValue(vegasGamma));
    Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue(g_segmentSize));
    Config::SetDefault("ns3::TcpSocket::SndBufSize", UintegerValue(1048576));
    Config::SetDefault("ns3::TcpSocket::RcvBufSize", UintegerValue(1048576));

    const uint32_t n = 1;

    const double bottleneckRateMbps = bdpKb / (baseRttSec * 1000.0);

    NodeContainer sources, routers, destinations;
    sources.Create(n);
    routers.Create(2);
    destinations.Create(n);

    PointToPointHelper access;
    access.SetDeviceAttribute("DataRate", StringValue("1Gbps"));
    access.SetChannelAttribute("Delay", StringValue("1ms"));

    std::vector<NetDeviceContainer> devS_R1(n);
    std::vector<NetDeviceContainer> devR2_D(n);
    for (uint32_t i = 0; i < n; i++)
    {
        devS_R1[i] = access.Install(sources.Get(i), routers.Get(0));
        devR2_D[i] = access.Install(routers.Get(1), destinations.Get(i));
    }

    PointToPointHelper bottleneck;
    std::ostringstream rate;
    rate << bottleneckRateMbps << "Mbps";
    bottleneck.SetDeviceAttribute("DataRate", StringValue(rate.str()));
    bottleneck.SetChannelAttribute("Delay", StringValue("48ms"));
    bottleneck.SetQueue("ns3::DropTailQueue", "MaxSize", StringValue("1000p"));
    NetDeviceContainer devR1R2 = bottleneck.Install(routers.Get(0), routers.Get(1));
    Ptr<PointToPointNetDevice> bottleneckDevA = DynamicCast<PointToPointNetDevice>(devR1R2.Get(0));
    Ptr<PointToPointNetDevice> bottleneckDevB = DynamicCast<PointToPointNetDevice>(devR1R2.Get(1));

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
    address.Assign(devR1R2);

    for (uint32_t i = 0; i < n; i++)
    {
        std::ostringstream sub;
        sub << "10.3." << (i + 1) << ".0";
        address.SetBase(sub.str().c_str(), "255.255.255.0");
        ifR2_D[i] = address.Assign(devR2_D[i]);
    }

    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    const uint16_t tcpPort = 50001;
    BulkSendHelper bulk("ns3::TcpSocketFactory", InetSocketAddress(ifR2_D[0].GetAddress(1), tcpPort));
    bulk.SetAttribute("MaxBytes", UintegerValue(0));
    ApplicationContainer tcpSrcApp = bulk.Install(sources.Get(0));
    tcpSrcApp.Start(Seconds(0.0));
    tcpSrcApp.Stop(Seconds(simStopSec));

    PacketSinkHelper tcpSink("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), tcpPort));
    ApplicationContainer tcpSinkApp = tcpSink.Install(destinations.Get(0));
    tcpSinkApp.Start(Seconds(0.0));
    tcpSinkApp.Stop(Seconds(simStopSec));

    TrafficControlHelper tchClean;
    tchClean.Uninstall(devR1R2);

    TrafficControlHelper tch;
    tch.SetRootQueueDisc("ns3::FifoQueueDisc", "MaxSize", StringValue("500p"));
    tch.Install(devR1R2.Get(0));

    std::ostringstream suffix;
    suffix << outputDir << "/" << tcpVariant << "-bdp" << static_cast<uint32_t>(bdpKb);
    g_cwndFile.open(suffix.str() + "-cwnd.dat");
    g_rttFile.open(suffix.str() + "-rtt.dat");

    g_cwndFile << "# Time(s)\tCWND(packets)\n";
    g_rttFile << "# Time(s)\tRTT(ms)\n";

    Simulator::Schedule(Seconds(0.001), &ConnectCwndTrace);
    Simulator::Schedule(Seconds(0.25), &ConnectRttTrace);
    Simulator::Schedule(Seconds(g_samplePeriod), &CwndSampler);

    Simulator::Schedule(Seconds(0.0), &MarkEventStart, std::string("new_connection"));
    Simulator::Schedule(Seconds(80.0), &MarkEventStart, std::string("bandwidth_halved"));
    Simulator::Schedule(Seconds(80.0),
                        &ChangeBottleneckRate,
                        bottleneckDevA,
                        bottleneckDevB,
                        RateMbpsString(bottleneckRateMbps / 2.0));
    Simulator::Schedule(Seconds(160.0), &MarkEventStart, std::string("bandwidth_doubled"));
    Simulator::Schedule(Seconds(160.0),
                        &ChangeBottleneckRate,
                        bottleneckDevA,
                        bottleneckDevB,
                        RateMbpsString(bottleneckRateMbps));

    Simulator::Stop(Seconds(simStopSec));
    Simulator::Run();

    PostProcessConvergence(simStopSec);

    g_cwndFile.close();
    g_rttFile.close();
    Simulator::Destroy();
    return true;
}

int
main(int argc, char* argv[])
{
    LogComponentEnable("QuickVegasBasic", LOG_LEVEL_INFO);
    //LogComponentEnable("TcpQuickVegas", LOG_LEVEL_DEBUG);

    std::string tcpVariant = "TcpQuickVegas";
    //std::string bdpList = "500,1000,2000,3000,4000,5000,6000,7000,8000";

    std::string bdpList = "500,1000,2000,3000,4000,5000";

    std::string outputDir = "convergence";
    uint32_t vegasAlpha = 20;
    uint32_t vegasBeta = 40;
    uint32_t vegasGamma = 10;
    double baseRttSec = 0.1;
    double simStopSec = 240.0;

    CommandLine cmd;
    cmd.AddValue("tcpVariant", "TCP variant: TcpVegas or TcpQuickVegas", tcpVariant);
    cmd.AddValue("bdpList", "Comma-separated BDP list in Kb (example: 500,1000,2000,4000)", bdpList);
    cmd.AddValue("outputDir", "Output directory for .dat logs", outputDir);
    cmd.AddValue("vegasAlpha", "Vegas alpha", vegasAlpha);
    cmd.AddValue("vegasBeta", "Vegas beta", vegasBeta);
    cmd.AddValue("vegasGamma", "Vegas gamma", vegasGamma);
    cmd.AddValue("baseRttSec", "Base RTT in seconds for BDP-rate mapping", baseRttSec);
    cmd.AddValue("simStopSec", "Simulation stop time in seconds", simStopSec);
    cmd.Parse(argc, argv);

    if (!std::filesystem::exists(outputDir))
    {
        std::filesystem::create_directories(outputDir);
    }

    const std::vector<double> bdpValues = ParseCsvDoubles(bdpList);
    if (bdpValues.empty())
    {
        NS_LOG_ERROR("No valid BDP values supplied through --bdpList");
        return 1;
    }

    std::string convergencePath = outputDir + "/" + tcpVariant + "-convergence-vs-bdp.dat";
    g_convergenceFile.open(convergencePath, std::ios::out);
    g_convergenceFile << "# Time(s)\tBDP(Kb)\tConvergenceTime(s)\tEvent\tBaseRTT(s)\tConvergence(BaseRTTs)\n";

    std::cout << "Running " << tcpVariant << " over " << bdpValues.size()
              << " BDP points. Results folder: " << outputDir << "\n";
    for (double bdpKb : bdpValues)
    {
        std::cout << "  -> Simulating BDP=" << bdpKb << " Kb\n";
        if (!RunOneBdpScenario(tcpVariant,
                               vegasAlpha,
                               vegasBeta,
                               vegasGamma,
                               bdpKb,
                               outputDir,
                               baseRttSec,
                               simStopSec))
        {
            g_convergenceFile.close();
            std::cerr << "TCP variant '" << tcpVariant
                      << "' is not available in this build.\n";
            return 2;
        }
    }

    g_convergenceFile.close();
    std::cout << "Convergence log written to: " << convergencePath << "\n";
    return 0;
}