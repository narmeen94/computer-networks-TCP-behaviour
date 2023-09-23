/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

//Question 1 part 2: base program was taken from tcp-bbr-example.cc and modified for the project requirements.



// This program simulates the following topology:
//
//           3 Mbps           1.5Mbps         3 Mbps
//  Sender -------------- R1 -------------- R2 -------------- Receiver
//              10ms               10ms               10ms
//


#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/traffic-control-module.h"
#include "ns3/flow-monitor-module.h"
#include "../src/internet/model/tcp-socket-base-1.h"

using namespace ns3;

std::string dir;
uint32_t prev = 0;
Time prevTime = Seconds (0);

// Calculate throughput
static void
TraceThroughput (Ptr<FlowMonitor> monitor)
{
  FlowMonitor::FlowStatsContainer stats = monitor->GetFlowStats ();
  auto itr = stats.begin ();
  Time curTime = Now ();
  std::ofstream thr (dir + "/throughput.dat", std::ios::out | std::ios::app);
  thr <<  curTime << " " << 8 * (itr->second.txBytes - prev) / (1000 * 1000 * (curTime.GetSeconds () - prevTime.GetSeconds ())) << std::endl;
  prevTime = curTime;
  prev = itr->second.txBytes;
  Simulator::Schedule (Seconds (0.2), &TraceThroughput, monitor);
}


// Trace congestion window
static void CwndTracer (Ptr<OutputStreamWrapper> stream, uint32_t oldval, uint32_t newval)
{
  *stream->GetStream () << Simulator::Now ().GetSeconds () << " " << newval / 1448.0 << std::endl;
}

void TraceCwnd (uint32_t nodeId, uint32_t socketId)
{
  AsciiTraceHelper ascii;
  Ptr<OutputStreamWrapper> stream = ascii.CreateFileStream (dir + "/cwnd.dat");
  Config::ConnectWithoutContext ("/NodeList/" + std::to_string (nodeId) + "/$ns3::TcpL4Protocol/SocketList/" + std::to_string (socketId) + "/CongestionWindow", MakeBoundCallback (&CwndTracer, stream));
}
//Trace Estimated RTT
static void RttTracer (Ptr<OutputStreamWrapper> rttStream,Time oldval, Time newval)
{
  //*rttStream->GetStream () << "0.0 " << oldval.GetSeconds () << std::endl;
  *rttStream->GetStream () << Simulator::Now ().GetSeconds () << " " << newval.GetSeconds () << std::endl;
}

static void
TraceRtt (uint32_t nodeId, uint32_t socketId)
{
  AsciiTraceHelper ascii;
  Ptr<OutputStreamWrapper> rttstream = ascii.CreateFileStream (dir + "/rtt.dat");
  Config::ConnectWithoutContext ("/NodeList/" + std::to_string (nodeId) + "/$ns3::TcpL4Protocol/SocketList/" + std::to_string (socketId) + "/RTT", MakeBoundCallback (&RttTracer, rttstream));
  
}

//Trace real Rtt

static void
RealRttTracer (Ptr<OutputStreamWrapper> rrttStream,Time oldval, Time newval)
{
      //*rrttStream->GetStream () << "0.0 " << oldval.GetSeconds () << std::endl;
    *rrttStream->GetStream () << Simulator::Now ().GetSeconds () << " " << newval.GetSeconds () << std::endl;
}

static void
TraceRealRtt (uint32_t nodeId, uint32_t socketId)
{
  AsciiTraceHelper ascii;
  Ptr<OutputStreamWrapper> rrttstream = ascii.CreateFileStream (dir + "/rrtt.dat");
  Config::ConnectWithoutContext ("/NodeList/" + std::to_string (nodeId) + "/$ns3::TcpL4Protocol/SocketList/" + std::to_string (socketId) + "/realRTT", MakeBoundCallback (&RealRttTracer, rrttstream));
  
}

//Trace Rto i.e timeoutinterval

static void
RtoTracer (Ptr<OutputStreamWrapper> rtoStream,Time oldval, Time newval)
{
  
 // *rtoStream->GetStream () << "0.0 " << oldval.GetSeconds () << std::endl;
  *rtoStream->GetStream () << Simulator::Now ().GetSeconds () << " " << newval.GetSeconds () << std::endl;
}

static void
TraceRto (uint32_t nodeId, uint32_t socketId)
{
  AsciiTraceHelper ascii;
  Ptr<OutputStreamWrapper> rtostream = ascii.CreateFileStream (dir + "/rto.dat");
  Config::ConnectWithoutContext ("/NodeList/" + std::to_string (nodeId) + "/$ns3::TcpL4Protocol/SocketList/" + std::to_string (socketId) + "/RTO", MakeBoundCallback (&RtoTracer, rtostream));
  
}
int main (int argc, char *argv [])
{
  // Naming the output directory using local system time
  time_t rawtime;
  struct tm * timeinfo;
  char buffer [80];
  time (&rawtime);
  timeinfo = localtime (&rawtime);
  strftime (buffer, sizeof (buffer), "%d-%m-%Y-%I-%M-%S", timeinfo);
  std::string currentTime (buffer);

  std::string tcpTypeId = "TcpNewReno";
  //std::string queueDisc = "FifoQueueDisc";
  uint32_t delAckCount = 2;
  //bool bql = true;
  //bool enablePcap = false;
  Time stopTime = Seconds (200);

  CommandLine cmd (__FILE__);
  
  cmd.Parse (argc, argv);

  //queueDisc = std::string ("ns3::") + queueDisc;

  Config::SetDefault ("ns3::TcpL4Protocol::SocketType", StringValue ("ns3::" + tcpTypeId));
  Config::SetDefault ("ns3::TcpSocket::SndBufSize", UintegerValue (4194304));
  Config::SetDefault ("ns3::TcpSocket::RcvBufSize", UintegerValue (6291456));
  //Config::SetDefault ("ns3::TcpSocket::Rwnd", UintegerValue (500));
  Config::SetDefault ("ns3::TcpSocket::InitialCwnd", UintegerValue (10));
  Config::SetDefault ("ns3::TcpSocket::DelAckCount", UintegerValue (delAckCount));
  Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (1448));
  //Config::SetDefault ("ns3::DropTailQueue<Packet>::MaxSize", QueueSizeValue (QueueSize ("1p")));
  //Config::SetDefault (queueDisc + "::MaxSize", QueueSizeValue (QueueSize ("100p")));

  NodeContainer sender, receiver;
  NodeContainer routers;
  sender.Create (1);
  receiver.Create (1);
  routers.Create (2);

  // Create the point-to-point link helpers
  PointToPointHelper bottleneckLink;
  bottleneckLink.SetDeviceAttribute  ("DataRate", StringValue ("1.5Mbps"));
  bottleneckLink.SetChannelAttribute ("Delay", StringValue ("10ms"));
  bottleneckLink.SetQueue ("ns3::DropTailQueue", "MaxSize", QueueSizeValue(QueueSize("100p")));

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


  // Assign IP addresses
  Ipv4AddressHelper ipv4;
  ipv4.SetBase ("10.0.0.0", "255.255.255.0");

  Ipv4InterfaceContainer i1i2 = ipv4.Assign (r1r2);

  ipv4.NewNetwork ();
  Ipv4InterfaceContainer is1 = ipv4.Assign (senderEdge);

  ipv4.NewNetwork ();
  Ipv4InterfaceContainer ir1 = ipv4.Assign (receiverEdge);

  //disabling traffic helper:
  TrafficControlHelper tc;
  tc.Uninstall (senderEdge);
  tc.Uninstall (receiverEdge);
  tc.Uninstall (r1r2);

  // Populate routing tables
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  // Select sender side port
  uint16_t port = 50001;

  // Install application on the sender
  BulkSendHelper source ("ns3::TcpSocketFactory", InetSocketAddress (ir1.GetAddress (1), port));
  source.SetAttribute ("MaxBytes", UintegerValue (0));
  ApplicationContainer sourceApps = source.Install (sender.Get (0));
  sourceApps.Start (Seconds (0.0));

  Simulator::Schedule (Seconds (0.2), &TraceCwnd, 0, 0);
  Simulator::Schedule (Seconds (0.2), &TraceRtt, 0, 0);
  Simulator::Schedule (Seconds (0.2), &TraceRealRtt, 0, 0);
  Simulator::Schedule (Seconds (0.2), &TraceRto, 0, 0);

  sourceApps.Stop (stopTime);

  // Install application on the receiver
  PacketSinkHelper sink ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), port));
  ApplicationContainer sinkApps = sink.Install (receiver.Get (0));
  sinkApps.Start (Seconds (0.0));
  sinkApps.Stop (stopTime);

  // Create a new directory to store the output of the program
  dir = "q1part2-results/" + currentTime + "/";
  std::string dirToSave = "mkdir -p " + dir;
  if (system (dirToSave.c_str ()) == -1)
    {
      exit (1);
    }

  //for creating plots: the plotting files are copied from plotscripts to the dir defined
  system (("cp -R PlotScripts/gnuScriptCwnd " + dir).c_str ());
  system (("cp -R PlotScripts/gnuScriptRtts " + dir).c_str ());
  

  
  // Check for dropped packets using Flow Monitor
  FlowMonitorHelper flowmon;
  Ptr<FlowMonitor> monitor = flowmon.InstallAll ();
  Simulator::Schedule (Seconds (0 + 0.000001), &TraceThroughput, monitor);

  Simulator::Stop (stopTime + TimeStep (1));

  AsciiTraceHelper ascii;
  bottleneckLink.EnableAsciiAll(ascii.CreateFileStream("bottlenecktrace.tr"));
  edgeLink.EnableAsciiAll(ascii.CreateFileStream("edge_qsizetrace.tr"));

  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}
