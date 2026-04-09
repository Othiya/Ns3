// #include "ns3/applications-module.h"
// #include "ns3/core-module.h"
// #include "ns3/flow-monitor-module.h"
// #include "ns3/internet-module.h"
// #include "ns3/network-module.h"
// #include "ns3/point-to-point-module.h"
// #include "ns3/traffic-control-module.h"

// using namespace ns3;

// NS_LOG_COMPONENT_DEFINE("QuickVegasBasic");

// static std::ofstream g_cwndFile;
// static std::ofstream g_queueFile;
// static std::ofstream g_throughputFile;
// static std::ofstream g_rttFile;
// static std::ofstream g_flowThroughputFile; // NEW: Per-second flow throughput from FlowMonitor

// static uint64_t g_lastRxBytes = 0;
// static uint32_t g_lastCwnd    = 0;
// static Ptr<PacketSink> g_sink;

// // ── FlowMonitor globals ──────────────────────────────────────────────────────
// static Ptr<FlowMonitor>        g_flowMonitor;
// static Ptr<Ipv4FlowClassifier> g_classifier;
// static uint64_t                g_flowLastRxBytes = 0; // bytes received in last interval
// // ─────────────────────────────────────────────────────────────────────────────

// // NEW: Sample R2→D1 throughput every 1 s using FlowMonitor statistics
// // The TCP flow is FlowId 1 (first installed flow, S0 → D0).
// // We identify it by matching srcPort==50001 or simply take FlowId 1.
// static void
// FlowThroughputSampler(Ipv4Address d1Addr)
// {
//     g_flowMonitor->CheckForLostPackets();

//     const FlowMonitor::FlowStatsContainer& stats = g_flowMonitor->GetFlowStats();

//     for (auto& kv : stats)
//     {
//         Ipv4FlowClassifier::FiveTuple t = g_classifier->FindFlow(kv.first);

//         // Match: destination is D1 and protocol is TCP (6)
//         if (t.destinationAddress == d1Addr && t.protocol == 6)
//         {
//             uint64_t currentRx = kv.second.rxBytes;
//             double   mbps      = (currentRx - g_flowLastRxBytes) * 8.0 / 1e6; // 1-s interval
//             g_flowLastRxBytes  = currentRx;

//             g_flowThroughputFile << Simulator::Now().GetSeconds()
//                                  << "\t" << mbps << "\n";
//             g_flowThroughputFile.flush();
//             break; // only one TCP flow to D1
//         }
//     }

//     Simulator::Schedule(Seconds(1.0), &FlowThroughputSampler, d1Addr);
// }

// // RTT tracer callback
// static void
// RttTracer(Time oldRtt, Time newRtt)
// {
//     g_rttFile << Simulator::Now().GetSeconds() << "\t"
//               << newRtt.GetMilliSeconds() << "\n";
// }

// static void
// ConnectRttTrace()
// {
//     Config::ConnectWithoutContext(
//         "/NodeList/0/$ns3::TcpL4Protocol/SocketList/0/RTT",
//         MakeCallback(&RttTracer));
// }

// static void
// CwndTracer(uint32_t oldVal, uint32_t newVal)
// {
//     g_lastCwnd = newVal;
// }

// static void
// CwndSampler()
// {
//     uint32_t segSize  = 1000;
//     double   cwndPkts = static_cast<double>(g_lastCwnd) / segSize;
//     g_cwndFile << Simulator::Now().GetSeconds() << "\t" << cwndPkts << "\n";
//     Simulator::Schedule(Seconds(1.0), &CwndSampler);
// }

// static void
// ThroughputSampler()
// {
//     uint64_t rx   = g_sink->GetTotalRx();
//     double   mbps = (rx - g_lastRxBytes) * 8.0 / 1e6;
//     g_lastRxBytes = rx;
//     g_throughputFile << Simulator::Now().GetSeconds() << "\t" << mbps << "\n";
//     Simulator::Schedule(Seconds(1.0), &ThroughputSampler);
// }

// static void
// QueueSampler(Ptr<QueueDisc> qdisc)
// {
//     uint32_t qlen = qdisc->GetNPackets();
//     g_queueFile << Simulator::Now().GetSeconds() << "\t" << qlen << "\n";
//     Simulator::Schedule(Seconds(0.1), &QueueSampler, qdisc);
// }

// static void
// ConnectCwndTrace()
// {
//     Config::ConnectWithoutContext(
//         "/NodeList/0/$ns3::TcpL4Protocol/SocketList/0/CongestionWindow",
//         MakeCallback(&CwndTracer));
// }

// int
// main(int argc, char* argv[])
// {
//     LogComponentEnable("QuickVegasBasic", LOG_LEVEL_INFO);

//     std::string tcpVariant = "TcpVegas";
//     uint32_t    vegasAlpha = 20;
//     uint32_t    vegasBeta  = 40;
//     uint32_t    vegasGamma = 10;

//     CommandLine cmd;
//     cmd.AddValue("tcpVariant", "TCP variant: TcpVegas or TcpQuickVegas", tcpVariant);
//     cmd.AddValue("vegasAlpha", "TcpVegas alpha (queue target lower bound)", vegasAlpha);
//     cmd.AddValue("vegasBeta",  "TcpVegas beta (queue target upper bound)", vegasBeta);
//     cmd.AddValue("vegasGamma", "TcpVegas gamma (slow-start threshold)", vegasGamma);
//     cmd.Parse(argc, argv);

//     Config::SetDefault("ns3::TcpL4Protocol::SocketType",
//                        StringValue("ns3::" + tcpVariant));
//     Config::SetDefault("ns3::TcpVegas::Alpha", UintegerValue(vegasAlpha));
//     Config::SetDefault("ns3::TcpVegas::Beta",  UintegerValue(vegasBeta));
//     Config::SetDefault("ns3::TcpVegas::Gamma", UintegerValue(vegasGamma));
//     Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue(1000));
//     Config::SetDefault("ns3::TcpSocket::SndBufSize",  UintegerValue(1048576));
//     Config::SetDefault("ns3::TcpSocket::RcvBufSize",  UintegerValue(1048576));

//     uint32_t nCbr = 1;
//     uint32_t n    = 1 + nCbr;

//     NodeContainer sources, routers, destinations;
//     sources.Create(n);
//     routers.Create(2);
//     destinations.Create(n);

//     // ── Access links (1 Gbps / 1 ms) ─────────────────────────────────────────
//     PointToPointHelper access;
//     access.SetDeviceAttribute("DataRate", StringValue("1Gbps"));
//     access.SetChannelAttribute("Delay",   StringValue("1ms"));

//     std::vector<NetDeviceContainer> devS_R1(n);
//     std::vector<NetDeviceContainer> devR2_D(n);
//     for (uint32_t i = 0; i < n; i++)
//     {
//         devS_R1[i] = access.Install(sources.Get(i),  routers.Get(0));
//         devR2_D[i] = access.Install(routers.Get(1), destinations.Get(i));
//     }

//     // ── Bottleneck link (50 Mbps / 48 ms) ────────────────────────────────────
//     PointToPointHelper bottleneck;
//     bottleneck.SetDeviceAttribute("DataRate", StringValue("50Mbps"));
//     bottleneck.SetChannelAttribute("Delay",   StringValue("48ms"));
//     bottleneck.SetQueue("ns3::DropTailQueue", "MaxSize", StringValue("1000p"));
//     NetDeviceContainer devR1R2 = bottleneck.Install(routers.Get(0), routers.Get(1));

//     // ── Internet stack ────────────────────────────────────────────────────────
//     InternetStackHelper stack;
//     stack.Install(sources);
//     stack.Install(routers);
//     stack.Install(destinations);

//     // ── IP addressing ─────────────────────────────────────────────────────────
//     Ipv4AddressHelper address;
//     std::vector<Ipv4InterfaceContainer> ifS_R1(n);
//     std::vector<Ipv4InterfaceContainer> ifR2_D(n);

//     for (uint32_t i = 0; i < n; i++)
//     {
//         std::ostringstream sub;
//         sub << "10.1." << (i + 1) << ".0";
//         address.SetBase(sub.str().c_str(), "255.255.255.0");
//         ifS_R1[i] = address.Assign(devS_R1[i]);
//     }

//     address.SetBase("10.2.0.0", "255.255.255.0");
//     Ipv4InterfaceContainer ifR1R2 = address.Assign(devR1R2);

//     for (uint32_t i = 0; i < n; i++)
//     {
//         std::ostringstream sub;
//         sub << "10.3." << (i + 1) << ".0";
//         address.SetBase(sub.str().c_str(), "255.255.255.0");
//         ifR2_D[i] = address.Assign(devR2_D[i]);
//     }

//     Ipv4GlobalRoutingHelper::PopulateRoutingTables();

//     // ── TCP flow: S0 → D0 (unlimited bulk, port 50001) ───────────────────────
//     uint16_t tcpPort = 50001;
//     BulkSendHelper bulk("ns3::TcpSocketFactory",
//                         InetSocketAddress(ifR2_D[0].GetAddress(1), tcpPort));
//     bulk.SetAttribute("MaxBytes", UintegerValue(0));
//     ApplicationContainer tcpSrcApp = bulk.Install(sources.Get(0));
//     tcpSrcApp.Start(Seconds(0.0));
//     tcpSrcApp.Stop(Seconds(240.0));

//     PacketSinkHelper tcpSink("ns3::TcpSocketFactory",
//                              InetSocketAddress(Ipv4Address::GetAny(), tcpPort));
//     ApplicationContainer tcpSinkApp = tcpSink.Install(destinations.Get(0));
//     tcpSinkApp.Start(Seconds(0.0));
//     tcpSinkApp.Stop(Seconds(240.0));

//     // ── CBR/UDP cross-traffic: S1 → D1 (25 Mbps, t=80–90 s) ─────────────────
//     double cbrPerFlow = 25.0 / nCbr;
//     std::ostringstream cbrRate;
//     cbrRate << cbrPerFlow << "Mbps";

//     for (uint32_t i = 1; i <= nCbr; i++)
//     {
//         uint16_t udpPort = 50001 + i;

//         OnOffHelper cbr("ns3::UdpSocketFactory",
//                         InetSocketAddress(ifR2_D[i].GetAddress(1), udpPort));
//         cbr.SetAttribute("DataRate",   StringValue(cbrRate.str()));
//         cbr.SetAttribute("PacketSize", UintegerValue(1000));
//         cbr.SetAttribute("OnTime",  StringValue("ns3::ConstantRandomVariable[Constant=1]"));
//         cbr.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));

//         ApplicationContainer cbrSrcApp = cbr.Install(sources.Get(i));
//         cbrSrcApp.Start(Seconds(80.0));
//         cbrSrcApp.Stop(Seconds(160.0));

//         PacketSinkHelper udpSink("ns3::UdpSocketFactory",
//                                  InetSocketAddress(Ipv4Address::GetAny(), udpPort));
//         ApplicationContainer cbrSinkApp = udpSink.Install(destinations.Get(i));
//         cbrSinkApp.Start(Seconds(0.0));
//         cbrSinkApp.Stop(Seconds(240.0));
//     }

//     // ── Queue discipline on R1 egress toward R2 ───────────────────────────────
//     TrafficControlHelper tchClean;
//     tchClean.Uninstall(devR1R2);

//     TrafficControlHelper tch;
//     tch.SetRootQueueDisc("ns3::FifoQueueDisc",
//                          "MaxSize", StringValue("500p"));
//     QueueDiscContainer qdiscs = tch.Install(devR1R2.Get(0));

//     // ── Install FlowMonitor on ALL nodes ──────────────────────────────────────
//     FlowMonitorHelper flowHelper;
//     g_flowMonitor = flowHelper.InstallAll();
//     g_classifier  = DynamicCast<Ipv4FlowClassifier>(flowHelper.GetClassifier());

//     // ── Open output files ─────────────────────────────────────────────────────
//     std::string prefix = tcpVariant + "-basic";
//     g_cwndFile.open(prefix + "-TEMPcwnd.dat");
//     g_queueFile.open(prefix + "-TEMPqueue.dat");
//     g_throughputFile.open(prefix + "-TEMPthroughput.dat");
//     g_rttFile.open(prefix + "-TEMPrtt.dat");
//     g_flowThroughputFile.open(prefix + "-TEMPflow-throughput-R2D1.dat"); // NEW

//     g_cwndFile           << "# Time(s)\tCwnd(packets) [sampled @1s]\n";
//     g_queueFile          << "# Time(s)\tQueueLen(packets)\n";
//     g_throughputFile     << "# Time(s)\tThroughput(Mbps)\n";
//     g_rttFile            << "# Time(s)\tRTT(ms)\n";
//     g_flowThroughputFile << "# Time(s)\tR2-D1_Throughput(Mbps)  [FlowMonitor]\n"; // NEW

//     g_sink = DynamicCast<PacketSink>(tcpSinkApp.Get(0));

//     // ── Schedule traces ───────────────────────────────────────────────────────
//     Simulator::Schedule(Seconds(0.001), &ConnectCwndTrace);
//     Simulator::Schedule(Seconds(1.0),   &CwndSampler);
//     Simulator::Schedule(Seconds(0.1),   &QueueSampler, qdiscs.Get(0));
//     Simulator::Schedule(Seconds(1.0),   &ThroughputSampler);
//     Simulator::Schedule(Seconds(1.0),   &ConnectRttTrace);

//     // Start FlowMonitor sampler — pass D1's IP so we can filter by destination
//     Ipv4Address d1Addr = ifR2_D[0].GetAddress(1);
//     Simulator::Schedule(Seconds(1.0), &FlowThroughputSampler, d1Addr);

//     Simulator::Stop(Seconds(240.0));
//     Simulator::Run();

//     // ── Post-simulation FlowMonitor report ───────────────────────────────────
//     g_flowMonitor->CheckForLostPackets();

//     std::cout << "\n===== FlowMonitor Summary: R2 → D1 (TCP, port 50001) =====\n";

//     const FlowMonitor::FlowStatsContainer& allStats = g_flowMonitor->GetFlowStats();
//     for (auto& kv : allStats)
//     {
//         Ipv4FlowClassifier::FiveTuple t = g_classifier->FindFlow(kv.first);
//         if (t.destinationAddress == d1Addr && t.protocol == 6)
//         {
//             const FlowMonitor::FlowStats& fs = kv.second;
//             double simDuration = 240.0;
//             double avgTput     = fs.rxBytes * 8.0 / (simDuration * 1e6);

//             std::cout << "  Flow ID            : " << kv.first            << "\n";
//             std::cout << "  Src  → Dst         : "
//                       << t.sourceAddress      << ":" << t.sourcePort << " → "
//                       << t.destinationAddress << ":" << t.destinationPort << "\n";
//             std::cout << "  Tx Packets         : " << fs.txPackets          << "\n";
//             std::cout << "  Rx Packets         : " << fs.rxPackets          << "\n";
//             std::cout << "  Lost Packets       : " << fs.lostPackets        << "\n";
//             std::cout << "  Rx Bytes           : " << fs.rxBytes            << " B\n";
//             std::cout << "  Avg Throughput     : " << avgTput               << " Mbps\n";
//             std::cout << "  Mean Delay         : "
//                       << (fs.rxPackets > 0
//                               ? fs.delaySum.GetMilliSeconds() / fs.rxPackets
//                               : 0.0)                                         << " ms\n";
//             std::cout << "  Mean Jitter        : "
//                       << (fs.rxPackets > 1
//                               ? fs.jitterSum.GetMilliSeconds() / (fs.rxPackets - 1)
//                               : 0.0)                                         << " ms\n";
//             break;
//         }
//     }

//     // Save full XML report for all flows
//     g_flowMonitor->SerializeToXmlFile(prefix + "-flowmonitor.xml", true, true);
//     std::cout << "\nFull FlowMonitor XML saved to: " << prefix + "-flowmonitor.xml\n";

//     // ── Close files ───────────────────────────────────────────────────────────
//     g_cwndFile.close();
//     g_queueFile.close();
//     g_throughputFile.close();
//     g_rttFile.close();
//     g_flowThroughputFile.close();

//     // ── Existing summary ──────────────────────────────────────────────────────
//     std::cout << "\nQueue drops          : "
//               << qdiscs.Get(0)->GetStats().nTotalDroppedPackets << "\n";

//     double totalRx = g_sink->GetTotalRx();
//     double avgTput = (totalRx * 8.0) / (240.0 * 1e6);
//     std::cout << "=== " << tcpVariant << " Basic Behavior ===\n";
//     std::cout << "Total bytes received : " << totalRx << " bytes\n";
//     std::cout << "Average throughput   : " << avgTput << " Mbps\n";

//     Simulator::Destroy();
//     return 0;
// }





//Queue

// // #include "ns3/applications-module.h"
// // #include "ns3/core-module.h"
// // #include "ns3/internet-module.h"
// // #include "ns3/network-module.h"
// // #include "ns3/point-to-point-module.h"
// // #include "ns3/traffic-control-module.h"

// // #include <cmath>
// // #include <filesystem>
// // #include <sstream>
// // #include <vector>

// // using namespace ns3;

// // NS_LOG_COMPONENT_DEFINE("QueueLengthStaggeredGroups");

// // static std::ofstream g_queueFile;

// // static void
// // QueueSampler(Ptr<QueueDisc> queueDisc, double sampleIntervalSec)
// // {
// //     const uint32_t qlen = queueDisc ? queueDisc->GetNPackets() : 0;
// //     g_queueFile << Simulator::Now().GetSeconds() << "\t" << qlen << "\n";
// //     g_queueFile.flush();
// //     Simulator::Schedule(Seconds(sampleIntervalSec), &QueueSampler, queueDisc, sampleIntervalSec);
// // }

// // int
// // main(int argc, char* argv[])
// // {
// //     std::string tcpVariant = "TcpVegas";
// //     std::string outputDir  = "othiya/queue-length-staggered-groups";

// //     uint32_t nPerGroup       = 20;
// //     double group1StartSec    = 0.0;
// //     double group2StartSec    = 100.0; //100
// //     double group3StartSec    = 200.0; //200
// //     double activePeriodSec   = 300.0; //300

// //     std::string bottleneckRate  = "100Mbps";
// //     std::string bottleneckDelay = "150ms"; //important knob 

// //     // FIX 2: Buffer enlarged from 125p to 500p.
// //     //
// //     // Vegas equilibrium requires each flow to hold alpha..beta extra packets
// //     // in the network pipe at all times.  With beta=8 and up to 60 flows (3
// //     // groups × 20), the router buffer must be at least N_max × beta = 480p
// //     // for Vegas to operate in pure delay-based mode without hitting tail-drop.
// //     //
// //     // With only 125p the buffer was too small even for Group 1 alone
// //     // (20 × 8 = 160 required).  Every few RTTs a tail-drop fires, cwnd halves,
// //     // the queue drains, flows grow back — the sawtooth you saw.  500p gives
// //     // Vegas the headroom it needs, so the queue declines smoothly after each
// //     // group joins instead of bouncing between empty and full.
// //     //std::string bottleneckQueue = "500p";//150p chilo


// //     std::string bottleneckQueue = "250p";

// //     std::string accessRate  = "10Gbps";
// //     std::string accessDelay = "1ms";
// //     double queueSampleIntervalSec = 1.0;

// //     // FIX 3: Alpha/beta lowered from 7/8 to 3/4.
// //     //
// //     // You needed 7/8 before because the 125p buffer caused constant tail-drops;
// //     // higher thresholds kept cwnd large enough to avoid re-entering slow start
// //     // too aggressively — a workaround, not a fix.  With a 500p buffer and
// //     // FifoQueueDisc, drops never happen during steady state, so lower
// //     // thresholds work correctly.  Alpha=3, beta=4 gives a visible equilibrium
// //     // queue of N × 3.5 packets (70 for 20 flows) while still allowing Vegas
// //     // to back off promptly when new flows arrive.
// //     uint32_t vegasAlpha = 2; //3 chilo
// //     uint32_t vegasBeta  = 4; 
// //     uint32_t vegasGamma = 1; 

// //     // FIX 4: Start spread increased from 0.2 s to 5 s.
// //     //
// //     // With a 0.2 s spread all flows in a group complete slow start within ~2
// //     // RTTs of each other.  Their cwnd cycles are synchronised: they all detect
// //     // congestion at the same time, all back off together, drain the queue
// //     // together, then grow together — classic synchronisation-induced oscillation.
// //     // A 5 s spread staggers the slow-start phases across many RTTs, breaking
// //     // the lock-step and letting the queue settle smoothly.
// //     double groupStartSpreadSec = 0.0; //5 chilo. 10 chilo, 2.4 , 0.2 

// //     CommandLine cmd;
// //     cmd.AddValue("tcpVariant",         "TCP variant, e.g., TcpVegas / TcpNewReno", tcpVariant);
// //     cmd.AddValue("outputDir",          "Directory for queue .dat output",           outputDir);
// //     cmd.AddValue("nPerGroup",          "Connections per group",                     nPerGroup);
// //     cmd.AddValue("group1Start",        "Group-1 start time (s)",                   group1StartSec);
// //     cmd.AddValue("group2Start",        "Group-2 start time (s)",                   group2StartSec);
// //     cmd.AddValue("group3Start",        "Group-3 start time (s)",                   group3StartSec);
// //     cmd.AddValue("activePeriod",       "Active period of each group (s)",           activePeriodSec);
// //     cmd.AddValue("bottleneckRate",     "Bottleneck link rate",                      bottleneckRate);
// //     cmd.AddValue("bottleneckDelay",    "Bottleneck link delay",                     bottleneckDelay);
// //     cmd.AddValue("bottleneckQueue",    "Bottleneck queue size (e.g. 500p)",         bottleneckQueue);
// //     cmd.AddValue("queueSampleInterval","Queue sampling period (s)",                 queueSampleIntervalSec);
// //     cmd.AddValue("vegasAlpha",         "TcpVegas Alpha threshold",                  vegasAlpha);
// //     cmd.AddValue("vegasBeta",          "TcpVegas Beta threshold",                   vegasBeta);
// //     cmd.AddValue("vegasGamma",         "TcpVegas Gamma threshold",                  vegasGamma);
// //     cmd.AddValue("groupStartSpread",   "Per-group ramp-up spread in seconds",       groupStartSpreadSec);
// //     cmd.Parse(argc, argv);

// //     TypeId tid;
// //     std::string socketType = "ns3::" + tcpVariant;
// //     if (!TypeId::LookupByNameFailSafe(socketType, &tid))
// //     {
// //         NS_LOG_ERROR("tcpVariant " << tcpVariant << " is not available in this build");
// //         return 1;
// //     }
// //     Config::SetDefault("ns3::TcpL4Protocol::SocketType", StringValue(socketType));
// //     if (tcpVariant == "TcpVegas")
// //     {
// //         Config::SetDefault("ns3::TcpVegas::Alpha", UintegerValue(vegasAlpha));
// //         Config::SetDefault("ns3::TcpVegas::Beta",  UintegerValue(vegasBeta));
// //         Config::SetDefault("ns3::TcpVegas::Gamma", UintegerValue(vegasGamma));
// //     }

// //     const uint32_t totalFlows = 3 * nPerGroup;

// //     NodeContainer senders, receivers, routers;
// //     senders.Create(totalFlows);
// //     receivers.Create(totalFlows);
// //     routers.Create(2);

// //     PointToPointHelper access;
// //     access.SetDeviceAttribute("DataRate", StringValue(accessRate));
// //     access.SetChannelAttribute("Delay",   StringValue(accessDelay));

// //     // No per-device queue limit — queue management lives entirely in the
// //     // FifoQueueDisc installed below.
// //     PointToPointHelper bottleneck;
// //     bottleneck.SetDeviceAttribute("DataRate", StringValue(bottleneckRate));
// //     bottleneck.SetChannelAttribute("Delay",   StringValue(bottleneckDelay));

// //     std::vector<NetDeviceContainer> devS_R1(totalFlows), devR2_D(totalFlows);
// //     for (uint32_t i = 0; i < totalFlows; ++i)
// //     {
// //         devS_R1[i] = access.Install(senders.Get(i),   routers.Get(0));
// //         devR2_D[i] = access.Install(routers.Get(1), receivers.Get(i));
// //     }
// //     NetDeviceContainer devR1R2 = bottleneck.Install(routers.Get(0), routers.Get(1));

// //     InternetStackHelper stack;
// //     stack.Install(senders);
// //     stack.Install(receivers);
// //     stack.Install(routers);

// //     // FIX 1 (carried from previous revision): plain FifoQueueDisc instead of
// //     // CoDelQueueDisc.  CoDel actively drops packets once sojourn time exceeds
// //     // its target (~83 packets at 100 Mbps / 10 ms target).  Vegas responds to
// //     // those drops as loss events (cwnd /= 2), bypassing its delay signal.
// //     // FifoQueueDisc is a simple tail-drop FIFO with no AQM, so Vegas can rely
// //     // entirely on RTT growth as its congestion signal.
// //     TrafficControlHelper tch;
// //     tch.SetRootQueueDisc("ns3::FifoQueueDisc",
// //                          "MaxSize", StringValue(bottleneckQueue));
// //     QueueDiscContainer qdiscs = tch.Install(devR1R2);

// //     Ipv4AddressHelper addr;
// //     std::vector<Ipv4InterfaceContainer> ifS_R1(totalFlows), ifR2_D(totalFlows);
// //     for (uint32_t i = 0; i < totalFlows; ++i)
// //     {
// //         std::ostringstream s;
// //         s << "10.1." << (i + 1) << ".0";
// //         addr.SetBase(s.str().c_str(), "255.255.255.0");
// //         ifS_R1[i] = addr.Assign(devS_R1[i]);
// //     }
// //     addr.SetBase("10.2.0.0", "255.255.255.0");
// //     addr.Assign(devR1R2);
// //     for (uint32_t i = 0; i < totalFlows; ++i)
// //     {
// //         std::ostringstream s;
// //         s << "10.3." << (i + 1) << ".0";
// //         addr.SetBase(s.str().c_str(), "255.255.255.0");
// //         ifR2_D[i] = addr.Assign(devR2_D[i]);
// //     }

// //     Ipv4GlobalRoutingHelper::PopulateRoutingTables();

// //     const double group1StopSec = group1StartSec + activePeriodSec;
// //     const double group2StopSec = group2StartSec + activePeriodSec;
// //     const double group3StopSec = group3StartSec + activePeriodSec;
// //     const double simStopSec    = std::max({group1StopSec, group2StopSec, group3StopSec}) + 10.0;

// //     for (uint32_t i = 0; i < totalFlows; ++i)
// //     {
// //         const uint16_t port = 40000 + i;

// //         BulkSendHelper src("ns3::TcpSocketFactory",
// //                            InetSocketAddress(ifR2_D[i].GetAddress(1), port));
// //         src.SetAttribute("MaxBytes", UintegerValue(0));
// //         ApplicationContainer srcApp = src.Install(senders.Get(i));

// //         PacketSinkHelper sink("ns3::TcpSocketFactory",
// //                               InetSocketAddress(Ipv4Address::GetAny(), port));
// //         ApplicationContainer sinkApp = sink.Install(receivers.Get(i));

// //         // Apply the same stagger formula to all three groups.
// //         // (In the original code Group 1 had no jitter — all 20 flows fired at
// //         // exactly t=0, a synchronised burst that was never intended.)
// //         const uint32_t idxWithinGroup = i % nPerGroup;
// //         const double normalized = (nPerGroup > 1)
// //             ? static_cast<double>(idxWithinGroup) / static_cast<double>(nPerGroup - 1)
// //             : 0.0;
// //         const double startJitter = normalized * std::max(0.0, groupStartSpreadSec);

// //         double startSec = group1StartSec + startJitter;
// //         double stopSec  = group1StopSec;
// //         if (i >= nPerGroup && i < 2 * nPerGroup)
// //         {
// //             startSec = group2StartSec + startJitter;
// //             stopSec  = group2StopSec;
// //         }
// //         else if (i >= 2 * nPerGroup)
// //         {
// //             startSec = group3StartSec + startJitter;
// //             stopSec  = group3StopSec;
// //         }

// //         srcApp.Start(Seconds(startSec));
// //         srcApp.Stop(Seconds(stopSec));
// //         sinkApp.Start(Seconds(0.0));
// //         sinkApp.Stop(Seconds(simStopSec));
// //     }

// //     std::filesystem::create_directories(outputDir);
// //     std::string outPath = outputDir + "/" + tcpVariant + "-queueLength-vs-time.dat";
// //     g_queueFile.open(outPath, std::ios::out);
// //     g_queueFile << "# Time(s)\tQueueLength(packets)\n";

// //     Ptr<QueueDisc> bottleneckQueueDisc = qdiscs.Get(0);
// //     Simulator::Schedule(Seconds(0.0), &QueueSampler, bottleneckQueueDisc, queueSampleIntervalSec);
// //     Simulator::Stop(Seconds(simStopSec));
// //     Simulator::Run();

// //     g_queueFile.close();
// //     Simulator::Destroy();

// //     std::cout << "Queue-length log written to: " << outPath << "\n";
// //     std::cout << "Flows: C1-C" << nPerGroup << " @ " << group1StartSec << "s, "
// //               << "C" << (nPerGroup + 1) << "-C" << (2 * nPerGroup) << " @ " << group2StartSec << "s, "
// //               << "C" << (2 * nPerGroup + 1) << "-C" << (3 * nPerGroup) << " @ " << group3StartSec << "s\n";
// //     if (tcpVariant == "TcpVegas")
// //         std::cout << "Vegas params: alpha=" << vegasAlpha << ", beta=" << vegasBeta
// //                   << ", gamma=" << vegasGamma << "\n";
// //     std::cout << "Sampling interval: " << queueSampleIntervalSec
// //               << "s, group start spread: " << groupStartSpreadSec << "s\n";
// //     return 0;
// // }




// #include "ns3/applications-module.h"
// #include "ns3/core-module.h"
// #include "ns3/internet-module.h"
// #include "ns3/network-module.h"
// #include "ns3/point-to-point-module.h"
// #include "ns3/traffic-control-module.h"

// #include <cmath>
// #include <filesystem>
// #include <sstream>
// #include <vector>

// using namespace ns3;

// NS_LOG_COMPONENT_DEFINE("QueueLengthStaggeredGroups");

// static std::ofstream g_queueFile;

// static void
// QueueSampler(Ptr<QueueDisc> queueDisc, double sampleIntervalSec)
// {
//     const uint32_t qlen = queueDisc ? queueDisc->GetNPackets() : 0;
//     g_queueFile << Simulator::Now().GetSeconds() << "\t" << qlen << "\n";
//     g_queueFile.flush();
//     Simulator::Schedule(Seconds(sampleIntervalSec), &QueueSampler, queueDisc, sampleIntervalSec);
// }

// int
// main(int argc, char* argv[])
// {
//     std::string tcpVariant = "TcpVegas";
//     std::string outputDir  = "othiya/queue-length-staggered-groups";

//     uint32_t nPerGroup       = 20;
//     double group1StartSec    = 0.0;
//     double group2StartSec    = 100.0;
//     double group3StartSec    = 200.0;
//     double activePeriodSec   = 300.0;

//     std::string bottleneckRate  = "100Mbps";
//     std::string bottleneckDelay = "150ms";
//     std::string bottleneckQueue = "250p";

//     std::string accessRate  = "10Gbps";
//     std::string accessDelay = "1ms";
//     double queueSampleIntervalSec = 1.0;

//     uint32_t vegasAlpha = 2;
//     uint32_t vegasBeta  = 4;
//     uint32_t vegasGamma = 1;

//     double groupStartSpreadSec = 0.0;

//     CommandLine cmd;
//     cmd.AddValue("tcpVariant",         "TCP variant, e.g., TcpVegas / TcpNewReno / TcpQuickVegas", tcpVariant);
//     cmd.AddValue("outputDir",          "Directory for queue .dat output",           outputDir);
//     cmd.AddValue("nPerGroup",          "Connections per group",                     nPerGroup);
//     cmd.AddValue("group1Start",        "Group-1 start time (s)",                   group1StartSec);
//     cmd.AddValue("group2Start",        "Group-2 start time (s)",                   group2StartSec);
//     cmd.AddValue("group3Start",        "Group-3 start time (s)",                   group3StartSec);
//     cmd.AddValue("activePeriod",       "Active period of each group (s)",           activePeriodSec);
//     cmd.AddValue("bottleneckRate",     "Bottleneck link rate",                      bottleneckRate);
//     cmd.AddValue("bottleneckDelay",    "Bottleneck link delay",                     bottleneckDelay);
//     cmd.AddValue("bottleneckQueue",    "Bottleneck queue size (e.g. 250p)",         bottleneckQueue);
//     cmd.AddValue("queueSampleInterval","Queue sampling period (s)",                 queueSampleIntervalSec);
//     cmd.AddValue("vegasAlpha",         "Alpha threshold (Vegas and QuickVegas)",    vegasAlpha);
//     cmd.AddValue("vegasBeta",          "Beta threshold (Vegas and QuickVegas)",     vegasBeta);
//     cmd.AddValue("vegasGamma",         "Gamma threshold (Vegas and QuickVegas)",    vegasGamma);
//     cmd.AddValue("groupStartSpread",   "Per-group ramp-up spread in seconds",       groupStartSpreadSec);
//     cmd.Parse(argc, argv);

//     // Validate that the requested TCP variant exists in this ns-3 build.
//     // For TcpQuickVegas, make sure tcp-quick-vegas.cc is compiled in.
//     TypeId tid;
//     std::string socketType = "ns3::" + tcpVariant;
//     if (!TypeId::LookupByNameFailSafe(socketType, &tid))
//     {
//         NS_LOG_ERROR("tcpVariant " << tcpVariant << " is not available in this build. "
//                      "For TcpQuickVegas, ensure tcp-quick-vegas.cc is listed in CMakeLists.txt.");
//         return 1;
//     }
//     Config::SetDefault("ns3::TcpL4Protocol::SocketType", StringValue(socketType));

//     // Apply alpha/beta/gamma for both TcpVegas and TcpQuickVegas.
//     // TcpQuickVegas inherits from TcpVegas, so the same attribute names apply
//     // as long as your TcpQuickVegas::GetTypeId() registers them (or inherits them).
//     if (tcpVariant == "TcpVegas" || tcpVariant == "TcpQuickVegas")
//     {
//         Config::SetDefault("ns3::" + tcpVariant + "::Alpha", UintegerValue(vegasAlpha));
//         Config::SetDefault("ns3::" + tcpVariant + "::Beta",  UintegerValue(vegasBeta));
//         Config::SetDefault("ns3::" + tcpVariant + "::Gamma", UintegerValue(vegasGamma));
//     }

//     const uint32_t totalFlows = 3 * nPerGroup;

//     NodeContainer senders, receivers, routers;
//     senders.Create(totalFlows);
//     receivers.Create(totalFlows);
//     routers.Create(2);

//     PointToPointHelper access;
//     access.SetDeviceAttribute("DataRate", StringValue(accessRate));
//     access.SetChannelAttribute("Delay",   StringValue(accessDelay));

//     PointToPointHelper bottleneck;
//     bottleneck.SetDeviceAttribute("DataRate", StringValue(bottleneckRate));
//     bottleneck.SetChannelAttribute("Delay",   StringValue(bottleneckDelay));

//     std::vector<NetDeviceContainer> devS_R1(totalFlows), devR2_D(totalFlows);
//     for (uint32_t i = 0; i < totalFlows; ++i)
//     {
//         devS_R1[i] = access.Install(senders.Get(i),   routers.Get(0));
//         devR2_D[i] = access.Install(routers.Get(1), receivers.Get(i));
//     }
//     NetDeviceContainer devR1R2 = bottleneck.Install(routers.Get(0), routers.Get(1));

//     InternetStackHelper stack;
//     stack.Install(senders);
//     stack.Install(receivers);
//     stack.Install(routers);

//     TrafficControlHelper tch;
//     tch.SetRootQueueDisc("ns3::FifoQueueDisc",
//                          "MaxSize", StringValue(bottleneckQueue));
//     QueueDiscContainer qdiscs = tch.Install(devR1R2);

//     Ipv4AddressHelper addr;
//     std::vector<Ipv4InterfaceContainer> ifS_R1(totalFlows), ifR2_D(totalFlows);
//     for (uint32_t i = 0; i < totalFlows; ++i)
//     {
//         std::ostringstream s;
//         s << "10.1." << (i + 1) << ".0";
//         addr.SetBase(s.str().c_str(), "255.255.255.0");
//         ifS_R1[i] = addr.Assign(devS_R1[i]);
//     }
//     addr.SetBase("10.2.0.0", "255.255.255.0");
//     addr.Assign(devR1R2);
//     for (uint32_t i = 0; i < totalFlows; ++i)
//     {
//         std::ostringstream s;
//         s << "10.3." << (i + 1) << ".0";
//         addr.SetBase(s.str().c_str(), "255.255.255.0");
//         ifR2_D[i] = addr.Assign(devR2_D[i]);
//     }

//     Ipv4GlobalRoutingHelper::PopulateRoutingTables();

//     const double group1StopSec = group1StartSec + activePeriodSec;
//     const double group2StopSec = group2StartSec + activePeriodSec;
//     const double group3StopSec = group3StartSec + activePeriodSec;
//     const double simStopSec    = std::max({group1StopSec, group2StopSec, group3StopSec}) + 10.0;

//     for (uint32_t i = 0; i < totalFlows; ++i)
//     {
//         const uint16_t port = 40000 + i;

//         BulkSendHelper src("ns3::TcpSocketFactory",
//                            InetSocketAddress(ifR2_D[i].GetAddress(1), port));
//         src.SetAttribute("MaxBytes", UintegerValue(0));
//         ApplicationContainer srcApp = src.Install(senders.Get(i));

//         PacketSinkHelper sink("ns3::TcpSocketFactory",
//                               InetSocketAddress(Ipv4Address::GetAny(), port));
//         ApplicationContainer sinkApp = sink.Install(receivers.Get(i));

//         const uint32_t idxWithinGroup = i % nPerGroup;
//         const double normalized = (nPerGroup > 1)
//             ? static_cast<double>(idxWithinGroup) / static_cast<double>(nPerGroup - 1)
//             : 0.0;
//         const double startJitter = normalized * std::max(0.0, groupStartSpreadSec);

//         double startSec = group1StartSec + startJitter;
//         double stopSec  = group1StopSec;
//         if (i >= nPerGroup && i < 2 * nPerGroup)
//         {
//             startSec = group2StartSec + startJitter;
//             stopSec  = group2StopSec;
//         }
//         else if (i >= 2 * nPerGroup)
//         {
//             startSec = group3StartSec + startJitter;
//             stopSec  = group3StopSec;
//         }

//         srcApp.Start(Seconds(startSec));
//         srcApp.Stop(Seconds(stopSec));
//         sinkApp.Start(Seconds(0.0));
//         sinkApp.Stop(Seconds(simStopSec));
//     }

//     std::filesystem::create_directories(outputDir);
//     std::string outPath = outputDir + "/" + tcpVariant + "-queueLength-vs-time.dat";
//     g_queueFile.open(outPath, std::ios::out);
//     g_queueFile << "# Time(s)\tQueueLength(packets)\n";

//     Ptr<QueueDisc> bottleneckQueueDisc = qdiscs.Get(0);
//     Simulator::Schedule(Seconds(0.0), &QueueSampler, bottleneckQueueDisc, queueSampleIntervalSec);
//     Simulator::Stop(Seconds(simStopSec));
//     Simulator::Run();

//     g_queueFile.close();
//     Simulator::Destroy();

//     std::cout << "Queue-length log written to: " << outPath << "\n";
//     std::cout << "Flows: C1-C" << nPerGroup << " @ " << group1StartSec << "s, "
//               << "C" << (nPerGroup + 1) << "-C" << (2 * nPerGroup) << " @ " << group2StartSec << "s, "
//               << "C" << (2 * nPerGroup + 1) << "-C" << (3 * nPerGroup) << " @ " << group3StartSec << "s\n";
//     if (tcpVariant == "TcpVegas" || tcpVariant == "TcpQuickVegas")
//         std::cout << "Params: alpha=" << vegasAlpha << ", beta=" << vegasBeta
//                   << ", gamma=" << vegasGamma << "\n";
//     std::cout << "Sampling interval: " << queueSampleIntervalSec
//               << "s, group start spread: " << groupStartSpreadSec << "s\n";
//     return 0;
// }



