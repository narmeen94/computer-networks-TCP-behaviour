

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
#include "ns3/traffic-control-module.h"
#include "ns3/flow-monitor-module.h"


using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("Part1-Q1-ECE-545");

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



int main (int argc, char *argv [])
{
  time_t rawtime;
  struct tm * timeinfo;
  char buffer [80];
  time (&rawtime);
  timeinfo = localtime (&rawtime);
  strftime (buffer, sizeof (buffer), "%d-%m-%Y-%I-%M-%S", timeinfo);
  std::string currentTime (buffer);

  
  
  Time stopTime = Seconds (10);
  LogComponentEnable("VisualSimulatorImpl", LOG_LEVEL_INFO);
  //LogComponentEnable("PacketSinkHelper", LOG_LEVEL_INFO);


  CommandLine cmd (__FILE__);
  
  cmd.Parse (argc, argv);

  Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (1448)); //packet size
  
  

  NodeContainer sender, receiver; //sender is node 0, receiver is node 1
  NodeContainer routers; // router 1 is node 2 , router 2 is node 3 as in visualizer . the flow starts from top to bottom.
  sender.Create (1);
  receiver.Create (1);
  routers.Create (2);

  // Create the point-to-point link helpers
  PointToPointHelper bottleneckLink;
  bottleneckLink.SetDeviceAttribute  ("DataRate", StringValue ("1.5Mbps"));
  bottleneckLink.SetChannelAttribute ("Delay", StringValue ("10ms"));
  bottleneckLink.SetQueue ("ns3::DropTailQueue", "MaxSize", QueueSizeValue(QueueSize("50p"))); //reducing queue size to 1p

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
  std::cout<<"the address of the receiver is: ";
  std::cout<<ir1.GetAddress(1)<<std::endl;
  std::cout<<"the address of the sender is: ";
  std::cout<<is1.GetAddress(0)<<std::endl;

  source.SetAttribute ("MaxBytes", UintegerValue (0));
  ApplicationContainer sourceApps = source.Install (sender.Get (0));
  sourceApps.Start (Seconds (0.0));
  sourceApps.Stop (stopTime);

  // Install application on the receiver
  PacketSinkHelper sink ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), port));
  ApplicationContainer sinkApps = sink.Install (receiver.Get (0));
  sinkApps.Start (Seconds (0.0));
  sinkApps.Stop (stopTime);

  // Create a new directory to store the output of the program
  dir = "part1q1thruput-results/" + currentTime + "/";
  std::string dirToSave = "mkdir -p " + dir;
  if (system (dirToSave.c_str ()) == -1)
    {
      exit (1);
    }


  // Check for throuput using Flow Monitor
  FlowMonitorHelper flowmon;
  Ptr<FlowMonitor> monitor = flowmon.InstallAll ();
  Simulator::Schedule (Seconds (1), &TraceThroughput, monitor);

  

  Simulator::Stop (stopTime + TimeStep (1));

  //for packet drop , we use tracemetrics:
  AsciiTraceHelper ascii;
  bottleneckLink.EnableAsciiAll(ascii.CreateFileStream("bottleneck_qsize50_wtraffic.tr"));
  edgeLink.EnableAsciiAll(ascii.CreateFileStream("edge_qsize50_wtraffic.tr"));

  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}
