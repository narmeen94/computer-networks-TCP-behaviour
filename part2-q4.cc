/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

 //question 2 part 4: the base program is tcp-validation.cc and has been modified for project requirements.


//
//              ----   bottleneck link    ----
//  servers ---| WR |--------------------| LR |--- clients
//              ----                      ----
//  ns-3 node IDs:
//  nodes 0-2    3                         4        5-7
//




#include <iostream>
#include <fstream>
#include <string>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/traffic-control-module.h"
#include "ns3/internet-module.h"
#include "ns3/internet-apps-module.h"
#include "ns3/point-to-point-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("part2-q2");

// These variables are declared outside of main() so that they can
// be used in trace sinks.
uint32_t g_firstBytesReceived = 0;
uint32_t g_secondBytesReceived = 0;
uint32_t g_marksObserved = 0;
uint32_t g_dropsObserved = 0;




void
TraceFirstRx (Ptr<const Packet> packet, const Address &address)
{
  g_firstBytesReceived += packet->GetSize ();
}

void
TraceSecondRx (Ptr<const Packet> packet, const Address &address)
{
  g_secondBytesReceived += packet->GetSize ();
}




void
TraceFirstThroughput (std::ofstream* ofStream, Time throughputInterval)
{
  double throughput = g_firstBytesReceived * 8 / throughputInterval.GetSeconds () / 1e6;
  
    
  *ofStream << Simulator::Now ().GetSeconds () << " " << throughput << std::endl;
    
  g_firstBytesReceived = 0;
  Simulator::Schedule (throughputInterval, &TraceFirstThroughput, ofStream, throughputInterval);
  
}

void
TraceSecondThroughput (std::ofstream* ofStream, Time throughputInterval)
{
  
 *ofStream << Simulator::Now ().GetSeconds () << " " << g_secondBytesReceived * 8 / throughputInterval.GetSeconds () / 1e6 << std::endl;
    
  g_secondBytesReceived = 0;
  Simulator::Schedule (throughputInterval, &TraceSecondThroughput, ofStream, throughputInterval);
}


void
ScheduleFirstPacketSinkConnection (void)
{
  Config::ConnectWithoutContextFailSafe ("/NodeList/4/ApplicationList/*/$ns3::PacketSink/Rx", MakeCallback (&TraceFirstRx));
}

void
ScheduleSecondPacketSinkConnection (void)
{
  Config::ConnectWithoutContext ("/NodeList/5/ApplicationList/*/$ns3::PacketSink/Rx", MakeCallback (&TraceSecondRx));
}

int
main (int argc, char *argv[])
{
  ////////////////////////////////////////////////////////////
  // variables not configured at command line               //
  ////////////////////////////////////////////////////////////
  
  Time throughputSamplingInterval = MilliSeconds (200);
  
  std::string firstTcpThroughputTraceFile = "tcp-validation-first-tcp-throughput.dat";
  
  std::string secondTcpThroughputTraceFile = "tcp-validation-second-tcp-throughput.dat";
  

  ////////////////////////////////////////////////////////////
  // variables configured at command line                   //
  ////////////////////////////////////////////////////////////
  
  
  Time stopTime = Seconds (70);
  

  ////////////////////////////////////////////////////////////
  // Override ns-3 defaults                                 //
  ////////////////////////////////////////////////////////////
  Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (1448));
  Config::SetDefault ("ns3::TcpSocketBase::MaxWindowSize", UintegerValue (3*1448));
  Config::SetDefault ("ns3::TcpSocket::InitialCwnd", UintegerValue (10));
  

  ////////////////////////////////////////////////////////////
  // command-line argument parsing                          //
  ////////////////////////////////////////////////////////////
  CommandLine cmd;
  
  
  cmd.Parse (argc, argv);

  
  

  TypeId firstTcpTypeId= TcpNewReno::GetTypeId ();
  
  TypeId secondTcpTypeId=TcpWestwood::GetTypeId ();
  
  // Report on configuration
  NS_LOG_DEBUG ("first TCP: " << firstTcpTypeId.GetName () << "; second TCP: " << secondTcpTypeId.GetName () );

  
  std::ofstream firstTcpThroughputOfStream;
  
  std::ofstream secondTcpThroughputOfStream;
  
  
    
  firstTcpThroughputOfStream.open (firstTcpThroughputTraceFile.c_str (), std::ofstream::out);
  secondTcpThroughputOfStream.open (secondTcpThroughputTraceFile.c_str (), std::ofstream::out);
          

  ////////////////////////////////////////////////////////////
  // scenario setup                                         //
  ////////////////////////////////////////////////////////////
  
  Ptr<Node> Server1 = CreateObject<Node> ();
  Ptr<Node> Server2 = CreateObject<Node> ();

  Ptr<Node> Router1 = CreateObject<Node> ();
  Ptr<Node> Router2 = CreateObject<Node> ();
  
  Ptr<Node> Client1 = CreateObject<Node> ();
  Ptr<Node> Client2 = CreateObject<Node> ();

  

  // Create the point-to-point link helpers
  PointToPointHelper bottleneckLink;
  bottleneckLink.SetDeviceAttribute  ("DataRate", StringValue ("1.5Mbps"));
  bottleneckLink.SetChannelAttribute ("Delay", StringValue ("10ms"));
  //bottleneckLink.SetQueue ("ns3::DropTailQueue", "MaxSize", QueueSizeValue(QueueSize("50p"))); //reducing queue size to 1p

  PointToPointHelper edgeLink;
  edgeLink.SetDeviceAttribute  ("DataRate", StringValue ("3Mbps"));
  edgeLink.SetChannelAttribute ("Delay", StringValue ("10ms"));

  // Create NetDevice containers
  NetDeviceContainer senderEdge1 = edgeLink.Install (Server1,Router1);
  NetDeviceContainer senderEdge2 = edgeLink.Install (Server2,Router1);
  NetDeviceContainer r1r2 = bottleneckLink.Install (Router1,Router2);
  NetDeviceContainer receiverEdge1 = edgeLink.Install (Router2,Client1);
  NetDeviceContainer receiverEdge2 = edgeLink.Install (Router2,Client2);

  // Install Stack
  
  InternetStackHelper stackHelper;

  Ptr<TcpL4Protocol> proto;
  stackHelper.Install (Server1);
  proto = Server1->GetObject<TcpL4Protocol> ();
  proto->SetAttribute ("SocketType", TypeIdValue (firstTcpTypeId));

  stackHelper.Install (Client1);
  proto = Client1->GetObject<TcpL4Protocol> ();
  proto->SetAttribute ("SocketType", TypeIdValue (firstTcpTypeId));


  
  stackHelper.Install (Router1);
  stackHelper.Install (Router2);

  stackHelper.Install (Server2);
  proto = Server2->GetObject<TcpL4Protocol> ();
  proto->SetAttribute ("SocketType", TypeIdValue (secondTcpTypeId));
  
  stackHelper.Install (Client2);
  proto = Client2->GetObject<TcpL4Protocol> ();  
  proto->SetAttribute ("SocketType", TypeIdValue (secondTcpTypeId));
  
  Ipv4AddressHelper ipv4;
  ipv4.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer firstServerIfaces = ipv4.Assign (senderEdge1);

  ipv4.SetBase ("10.1.2.0", "255.255.255.0");
  Ipv4InterfaceContainer secondServerIfaces = ipv4.Assign (senderEdge2);
  
  ipv4.SetBase ("172.16.1.0", "255.255.255.0");
  Ipv4InterfaceContainer r1r2Ifaces = ipv4.Assign (r1r2);

  ipv4.SetBase ("192.168.1.0", "255.255.255.0");
  Ipv4InterfaceContainer firstClientIfaces = ipv4.Assign (receiverEdge1);
  ipv4.SetBase ("192.168.2.0", "255.255.255.0");
  Ipv4InterfaceContainer secondClientIfaces = ipv4.Assign (receiverEdge2);

  //disabling traffic helper:
  TrafficControlHelper tc;
  tc.Uninstall (senderEdge1);
  tc.Uninstall (receiverEdge1);
  tc.Uninstall (senderEdge2);
  tc.Uninstall (receiverEdge2);
  tc.Uninstall (r1r2);
 

  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  
  // application setup                                      
  
  
  uint16_t firstPort = 5000;
  BulkSendHelper tcp1 ("ns3::TcpSocketFactory", Address ());
  tcp1.SetAttribute ("MaxBytes", UintegerValue (0));

  // Configure first TCP client/server pair

  //server1
  InetSocketAddress firstDestAddress (firstClientIfaces.GetAddress (1), firstPort);
  tcp1.SetAttribute ("Remote", AddressValue (firstDestAddress));
  ApplicationContainer firstApp;
  firstApp = tcp1.Install (Server1);
  firstApp.Start (Seconds (0.0));
  firstApp.Stop (stopTime);

  //client 1
  Address firstSinkAddress (InetSocketAddress (Ipv4Address::GetAny (), firstPort));
  ApplicationContainer firstSinkApp;
  PacketSinkHelper firstSinkHelper ("ns3::TcpSocketFactory", firstSinkAddress);
  firstSinkApp = firstSinkHelper.Install (Client1);
  firstSinkApp.Start (Seconds(0.0));
  firstSinkApp.Stop (stopTime);

  // Configure second TCP client/server pair

  BulkSendHelper tcp2 ("ns3::TcpSocketFactory", Address ());
  tcp1.SetAttribute ("MaxBytes", UintegerValue (0));
  uint16_t secondPort = 5000;
  
  InetSocketAddress secondDestAddress (secondClientIfaces.GetAddress (1), secondPort);
  tcp2.SetAttribute ("Remote", AddressValue (secondDestAddress));
  ApplicationContainer secondApp;
  secondApp = tcp2.Install (Server2);
  secondApp.Start (Seconds(1.0));   //second flow will start after 1 second
  secondApp.Stop (stopTime);

  Address secondSinkAddress (InetSocketAddress (Ipv4Address::GetAny (), secondPort));
  PacketSinkHelper secondSinkHelper ("ns3::TcpSocketFactory", secondSinkAddress);
  ApplicationContainer secondSinkApp;
  secondSinkApp = secondSinkHelper.Install (Client2);
  secondSinkApp.Start (Seconds (1.0));
  secondSinkApp.Stop (stopTime);
  
    
      

  
  // Setup scheduled traces; TCP traces must be hooked after socket creation
  
  Simulator::Schedule (Seconds (0.0) + MilliSeconds (100), &ScheduleFirstPacketSinkConnection);
  
  Simulator::Schedule (throughputSamplingInterval, &TraceFirstThroughput, &firstTcpThroughputOfStream, throughputSamplingInterval);
  
    
 // Setup scheduled traces; TCP traces must be hooked after socket creation

      
  Simulator::Schedule (Seconds (0.0) + MilliSeconds (100), &ScheduleSecondPacketSinkConnection);
  Simulator::Schedule (throughputSamplingInterval, &TraceSecondThroughput, &secondTcpThroughputOfStream, throughputSamplingInterval);
      
  //Simulator::Schedule (marksSamplingInterval, &TraceMarksFrequency, &queueMarksFrequencyOfStream, marksSamplingInterval);

//   if (enablePcap)
//     {
//       p2p.EnablePcapAll ("tcp-validation", false);
//     }

  Simulator::Stop (stopTime);
  Simulator::Run ();
  Simulator::Destroy ();

  firstTcpThroughputOfStream.close ();
  secondTcpThroughputOfStream.close ();
          
}

