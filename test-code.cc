

// This program simulates the following topology:
//
//           3 Mbps            1.5 Mbps          3 Mbps
//  Sender -------------- R1 -------------- R2 -------------- Receiver
//              10ms               10ms               10ms
//
// The link between R1 and R2 is a bottleneck link with 1.5 Mbps. All other
// links are 3 Mbps.
//



#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"

//#include "ns3/traffic-control-module.h"
//#include "ns3/flow-monitor-module.h"

using namespace ns3;

// std::string dir;
// uint32_t prev = 0;
// Time prevTime = Seconds (0);

// Calculate throughput
// static void
// TraceThroughput (Ptr<FlowMonitor> monitor)
// {
//   FlowMonitor::FlowStatsContainer stats = monitor->GetFlowStats ();
//   auto itr = stats.begin ();
//   Time curTime = Now ();
//   std::ofstream thr (dir + "/throughput.dat", std::ios::out | std::ios::app);
//   thr <<  curTime << " " << 8 * (itr->second.txBytes - prev) / (1000 * 1000 * (curTime.GetSeconds () - prevTime.GetSeconds ())) << std::endl;
//   prevTime = curTime;
//   prev = itr->second.txBytes;
//   Simulator::Schedule (Seconds (0.2), &TraceThroughput, monitor);
// }

// // Check the queue size
// void CheckQueueSize (Ptr<QueueDisc> qd)
// {
//   uint32_t qsize = qd->GetCurrentSize ().GetValue ();
//   Simulator::Schedule (Seconds (0.2), &CheckQueueSize, qd);
//   std::ofstream q (dir + "/queueSize.dat", std::ios::out | std::ios::app);
//   q << Simulator::Now ().GetSeconds () << " " << qsize << std::endl;
//   q.close ();
// }

// // Trace congestion window
// static void CwndTracer (Ptr<OutputStreamWrapper> stream, uint32_t oldval, uint32_t newval)
// {
//   *stream->GetStream () << Simulator::Now ().GetSeconds () << " " << newval / 1448.0 << std::endl;
// }

// void TraceCwnd (uint32_t nodeId, uint32_t socketId)
// {
//   AsciiTraceHelper ascii;
//   Ptr<OutputStreamWrapper> stream = ascii.CreateFileStream (dir + "/cwnd.dat");
//   Config::ConnectWithoutContext ("/NodeList/" + std::to_string (nodeId) + "/$ns3::TcpL4Protocol/SocketList/" + std::to_string (socketId) + "/CongestionWindow", MakeBoundCallback (&CwndTracer, stream));
// }

int main (int argc, char *argv [])
{
  // Naming the output directory using local system time
  // time_t rawtime;
  // struct tm * timeinfo;
  // char buffer [80];
  // time (&rawtime);
  // timeinfo = localtime (&rawtime);
  // strftime (buffer, sizeof (buffer), "%d-%m-%Y-%I-%M-%S", timeinfo);
  // std::string currentTime (buffer);

  // std::string tcpTypeId = "TcpReno";
  std::string queueDisc = "FifoQueueDisc";
  // uint32_t delAckCount = 2;
  // bool bql = true;
  // bool enablePcap = false;
   Time stopTime = Seconds (20);

  CommandLine cmd (__FILE__);
  // cmd.AddValue ("tcpTypeId", "Transport protocol to use: TcpNewReno, TcpBbr", tcpTypeId);
  // cmd.AddValue ("delAckCount", "Delayed ACK count", delAckCount);
  // cmd.AddValue ("enablePcap", "Enable/Disable pcap file generation", enablePcap);
  // cmd.AddValue ("stopTime", "Stop time for applications / simulation time will be stopTime + 1", stopTime);
  cmd.Parse (argc, argv);

  //queueDisc = std::string ("ns3::") + queueDisc;

  Config::SetDefault ("ns3::TcpL4Protocol::SocketType", StringValue ("ns3::TcpNewReno"));
  // Config::SetDefault ("ns3::TcpSocket::SndBufSize", UintegerValue (4194304));
  // Config::SetDefault ("ns3::TcpSocket::RcvBufSize", UintegerValue (6291456));
  // Config::SetDefault ("ns3::TcpSocket::InitialCwnd", UintegerValue (10));
  // Config::SetDefault ("ns3::TcpSocket::DelAckCount", UintegerValue (delAckCount));
  Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (1448)); //packet size
  Config::SetDefault ("ns3::DropTailQueue<Packet>::MaxSize", QueueSizeValue (QueueSize ("50p"))); //changed to 50 from 1
  //Config::SetDefault (queueDisc + "::MaxSize", QueueSizeValue (QueueSize ("100p")));

  NodeContainer sender, receiver; //sender is node 0, receiver is node 1
  NodeContainer routers; // router 1 is node 2 , router 2 is node 3 as in visualizer . the flow starts from top to bottom.
  sender.Create (1);
  receiver.Create (1);
  routers.Create (2);

  // Create the point-to-point link helpers
  PointToPointHelper bottleneckLink;
  bottleneckLink.SetDeviceAttribute  ("DataRate", StringValue ("1.5Mbps"));
  bottleneckLink.SetChannelAttribute ("Delay", StringValue ("10ms"));

  PointToPointHelper edgeLink;
  edgeLink.SetDeviceAttribute  ("DataRate", StringValue ("3Mbps"));
  edgeLink.SetChannelAttribute ("Delay", StringValue ("10ms"));

  // Create NetDevice containers
  NetDeviceContainer senderEdge = edgeLink.Install (sender.Get (0), routers.Get (0));
  NetDeviceContainer r1r2 = bottleneckLink.Install (routers.Get (0), routers.Get (1));
  NetDeviceContainer receiverEdge = edgeLink.Install (routers.Get (1), receiver.Get (0));

  // Install Stack
  InternetStackHelper internet;
  internet.Install (sender);
  internet.Install (receiver);
  internet.Install (routers);

  // Configure the root queue discipline
  // TrafficControlHelper tch;
  // tch.SetRootQueueDisc (queueDisc);

  // if (bql)
  //   {
  //     tch.SetQueueLimits ("ns3::DynamicQueueLimits", "HoldTime", StringValue ("1000ms"));
  //   }

  // tch.Install (senderEdge);
  // tch.Install (receiverEdge);

  // Assign IP addresses
  Ipv4AddressHelper ipv4;
  ipv4.SetBase ("10.0.0.0", "255.255.255.0");

  Ipv4InterfaceContainer i1i2 = ipv4.Assign (r1r2);

  ipv4.NewNetwork ();
  Ipv4InterfaceContainer is1 = ipv4.Assign (senderEdge);

  ipv4.NewNetwork ();
  Ipv4InterfaceContainer ir1 = ipv4.Assign (receiverEdge);

  // Populate routing tables
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  // Select sender side port
  uint16_t port = 50001;

  // Install application on the sender
  BulkSendHelper source ("ns3::TcpSocketFactory", InetSocketAddress (ir1.GetAddress (1), port));
  source.SetAttribute ("MaxBytes", UintegerValue (0));
  ApplicationContainer sourceApps = source.Install (sender.Get (0));
  sourceApps.Start (Seconds (0.0));
  //Simulator::Schedule (Seconds (0.2), &TraceCwnd, 0, 0);
  sourceApps.Stop (stopTime);

  // Install application on the receiver
  PacketSinkHelper sink ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), port));
  ApplicationContainer sinkApps = sink.Install (receiver.Get (0));
  sinkApps.Start (Seconds (0.0));
  sinkApps.Stop (stopTime);

  // Create a new directory to store the output of the program
  // dir = "bbr-results/" + currentTime + "/";
  // std::string dirToSave = "mkdir -p " + dir;
  // if (system (dirToSave.c_str ()) == -1)
  //   {
  //     exit (1);
  //   }

  // The plotting scripts are provided in the following repository, if needed:
  // https://github.com/mohittahiliani/BBR-Validation/
  //
  // Download 'PlotScripts' directory (which is inside ns-3 scripts directory)
  // from the link given above and place it in the ns-3 root directory.
  // Uncomment the following three lines to generate plots for Congestion
  // Window, sender side throughput and queue occupancy on the bottleneck link.
  //
  // system (("cp -R PlotScripts/gnuplotScriptCwnd " + dir).c_str ());
  // system (("cp -R PlotScripts/gnuplotScriptThroughput " + dir).c_str ());
  // system (("cp -R PlotScripts/gnuplotScriptQueueSize " + dir).c_str ());

  // Trace the queue occupancy on the second interface of R1
  // tch.Uninstall (routers.Get (0)->GetDevice (1));
  // QueueDiscContainer qd;
  // qd = tch.Install (routers.Get (0)->GetDevice (1));
  // Simulator::ScheduleNow (&CheckQueueSize, qd.Get (0));

  // // Generate PCAP traces if it is enabled
  // if (enablePcap)
  //   {
  //     if (system ((dirToSave + "/pcap/").c_str ()) == -1)
  //       {
  //         exit (1);
  //       }
  //     bottleneckLink.EnablePcapAll (dir + "/pcap/bbr", true);
  //   }

  // // Check for dropped packets using Flow Monitor
  // FlowMonitorHelper flowmon;
  // Ptr<FlowMonitor> monitor = flowmon.InstallAll ();
  // Simulator::Schedule (Seconds (0 + 0.000001), &TraceThroughput, monitor);

  Simulator::Stop (stopTime + TimeStep (1));
  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}
