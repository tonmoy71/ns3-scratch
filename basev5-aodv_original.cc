/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *
 * Net topology
 *
 * With a 3x3 mesh grid (size can vary):
 *
 *               n2   n3   n4
 *
 *
 * n0----n1 )))  n5   n6   n7
 *
 *
 *               n8   n9   n10
 *
 *
 *
 *  n0: internet node.
 *  n1: access node (a static mesh node with a P2P link).
 *  n2-n10: mesh nodes.
 *
 *  n0 has ip 1.1.1.1 to emulate internet access.
 *  n1 is always at the middle of the mesh, horizontal distance to the mesh can be set.
 *
 *  There are traffic generators in each node but n1 (no need)
 *  so they can emulate uploads and downloads to internet.
 *
 *  A IPv4 ping app is enable to check conectivity and keep track of
 *  simulation as it runs. It pings "internet" (AKA 1.1.1.1) from the last node every 1 sec.
 *
 *  In this scenario, AODV routes are used as L3 routing protocol for everyone.
 *  This solves L2 routing as well inside the mesh.
 *
 *  Performance is measured with Flowmon. Every node (but n1) has sink for packets.
 *  Flows are output in a .CSV file, using tabs as separators
 *
 *  Author: Carlos Cordero 
 *  Email: carlos.cordero.0@gmail.com
 *
 */

#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/internet-module.h"
#include "ns3/netanim-module.h"
#include "ns3/v4ping-helper.h"
#include "ns3/aodv-helper.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/ipv4-flow-classifier.h"
#include "ns3/flow-monitor.h"
#include "ns3/animation-interface.h"
//#include "ns3/wifi-phy.h"
//#include <iostream>
//#include <sstream>
//#include <fstream>
//#include <vector>
//#include <stdlib.h>


using namespace ns3;

int main (int argc, char *argv[])
{
  ns3::PacketMetadata::Enable ();
  NS_LOG_COMPONENT_DEFINE ("TesisBase");
//  LogComponentEnable ("Time", LOG_LEVEL_ALL);
//  LogComponentEnableAll (LOG_LEVEL_DEBUG);
//  LogComponentEnable ("aodvRoutingProtocol", LOG_LEVEL_ALL);
//  LogComponentEnable ("PacketSink", LOG_LEVEL_DEBUG);
  LogComponentEnable ("V4Ping", LOG_LEVEL_DEBUG);


// Variable definition (all generic variables starts with "m_")
  int m_xNodes = 3; // Mesh width in number of nodes
  int m_yNodes = 3; // Mesh heigth in number of nodes
  int m_distNodes = 75; // Distance between nodes (mts)
  int m_distAP = 100; // Distance between access nodes and mesh (mts)
  int m_totalTime = 90; // Time to simulate (segs)
  int m_packetSize = 512; // Packet size for tests
  double m_rss = 88; //86 minimum for 100mts range 11a, 88 for 11g
  bool m_mobile = false; // Mesh nodes are mobile
  bool m_newFlowFile = false; // Clear flow .csv file
  bool m_drawAnim = false; // Enable netanim .xls output
  std::string m_txAppRate = "128kbps"; // Transmision speed for apps
  std::string m_txInternetRate = "1Mbps"; // Transmision speed to n0 <-> n1 link
  std::string m_animFile = "resultados/basev4-aodv.xml"; // File for .xml
  std::string m_routeFile = "resultados/basev4-aodv-route.xml"; // File for .xml routing
  std::string m_statsFile = "resultados/basev4-aodv-3x3"; // Prefix for statistics output files
  std::string m_flowmonFile = "resultados/basev4-aodv.flowmon"; // File for flowmon output
  int tmp_x;
  char tmp_char [30] = "";

// Command line options
  CommandLine cmd;
  cmd.AddValue ("mesh-width", "Number of node in mesh width", m_xNodes);
  cmd.AddValue ("mesh-height", "Number of node in mesh height", m_yNodes);
  cmd.AddValue ("node-distance", "Distance between nodes (horizontally / vertically)", m_distNodes);
  cmd.AddValue ("ap-distance", "Distance between access nodes and mesh (horizontally)", m_distAP);
  cmd.AddValue ("time", "Total time to simulate", m_totalTime);
  cmd.AddValue ("mobile", "Set movement to mesh nodes", m_mobile);
  cmd.AddValue ("app-packet-size", "Set packet size to tx from apps", m_packetSize);
  cmd.AddValue ("app-tx-rate", "Set speed of traffic generation", m_txAppRate);
  cmd.AddValue ("link-speed", "Transmision speed over P2P link", m_txInternetRate);
  cmd.AddValue ("enable-anim", "Enable output for .xml animation", m_drawAnim);
  cmd.AddValue ("anim-file", "Set output name for .xml animation file", m_animFile);
  cmd.AddValue ("route-file", "Set output name for route file", m_routeFile);
  cmd.AddValue ("stats-file", "Set output prefix for .csv flows results file", m_statsFile);
  cmd.AddValue ("new-flow-file", "Clear .csv flows results file", m_newFlowFile);
  cmd.AddValue ("flow-file", "Set output name for flow monitor .flowmon file", m_flowmonFile);
  cmd.Parse (argc, argv);

// Node container creation (all node containers starts with "nc_")
  NodeContainer nc_all; // Contains every node (starting with internet node, access nodes and then mesh nodes
  NodeContainer nc_mesh; // Contains only mesh nodes (x*y nodes)
  NodeContainer nc_wireless; // Contains access and mesh nodes
  NodeContainer nc_internet; // Contains internet nodes
  NodeContainer nc_destiny; // Contains nodes with sink for apps (internet and mesh nodes)
  nc_mesh.Create (m_xNodes * m_yNodes);
  nc_all.Create (2);
  nc_all.Add (nc_mesh);
  nc_wireless.Add (nc_all.Get (1));
  nc_wireless.Add (nc_mesh);
  nc_internet.Add (nc_all.Get (0));
  nc_internet.Add (nc_all.Get (1));
  nc_destiny.Add (nc_all.Get (0));
  nc_destiny.Add (nc_mesh);

// Create P2P links between n0 <-> n1 (all devices starts with "de_")
  PointToPointHelper p2pinternet;
  p2pinternet.SetDeviceAttribute ("DataRate", StringValue (m_txInternetRate));
  p2pinternet.SetChannelAttribute ("Delay", StringValue ("10ms"));
  NetDeviceContainer de_internet;
  de_internet = p2pinternet.Install (nc_internet);

// Set YansWifiChannel
  WifiHelper wifi;
  YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default ();
  YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default ();
  NqosWifiMacHelper wifiMac = NqosWifiMacHelper::Default ();
  wifi.SetStandard (WIFI_PHY_STANDARD_80211g);
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                "DataMode",StringValue ("ErpOfdmRate6Mbps"));
  wifiPhy.SetPcapDataLinkType (YansWifiPhyHelper::DLT_IEEE802_11_RADIO); 
  wifiPhy.Set ("RxGain", DoubleValue (m_rss));
  wifiChannel.AddPropagationLoss ("ns3::FriisPropagationLossModel");

  wifiMac.SetType ("ns3::AdhocWifiMac");
  wifiPhy.SetChannel (wifiChannel.Create ());
  NetDeviceContainer de_wireless = wifi.Install (wifiPhy, wifiMac, nc_wireless);

// Set startup positions for nodes
  MobilityHelper mobilityMesh; //Position mesh nodes
  mobilityMesh.SetPositionAllocator ("ns3::GridPositionAllocator",
                                    "MinX", DoubleValue (m_distAP),
                                    "MinY", DoubleValue (0.0),
                                    "DeltaX", DoubleValue (m_distNodes),
                                    "DeltaY", DoubleValue (m_distNodes),
                                    "GridWidth", UintegerValue (m_xNodes),
                                    "LayoutType", StringValue ("RowFirst"));
  if (m_mobile) // Set mobility for mesh
  { //if walking nodes
    int t1 = m_distAP / 2;
    int t2 = m_distAP + ((m_xNodes - 1) * m_distNodes) + (m_distAP / 2);
    int t3 = m_distAP / -2;
    int t4 = (m_yNodes * m_distNodes) + (m_distAP / 2);
    sprintf (tmp_char, "%d|%d|%d|%d", t1, t2, t3, t4);
    mobilityMesh.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
                                  "Mode", StringValue ("Time"),
                                  "Time", StringValue ("2s"),
                                  "Bounds", StringValue (tmp_char));
  }
  else
  { //if standing nodes
    mobilityMesh.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  }
  mobilityMesh.Install (nc_mesh);

  MobilityHelper mobilityInternet; //Position internet and access node
  mobilityInternet.SetPositionAllocator ("ns3::GridPositionAllocator",
                                    "MinX", DoubleValue (m_distAP * -1),
                                    "MinY", DoubleValue ((m_distNodes * (m_yNodes - 1)) / 2),
                                    "DeltaX", DoubleValue (m_distAP),
                                    "DeltaY", DoubleValue (0.0),
                                    "GridWidth", UintegerValue (2),
                                    "LayoutType", StringValue ("RowFirst"));
  mobilityInternet.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobilityInternet.Install (nc_internet);

// Set Internet stack and config IPs for interfaces (all interfaces starts with "if_")
  InternetStackHelper internetStack;
  AodvHelper aodv;
  internetStack.SetRoutingHelper (aodv);
  internetStack.Install (nc_all);

  Ipv4AddressHelper addrMesh;
  addrMesh.SetBase ("10.0.0.0", "255.255.255.0");
  Ipv4InterfaceContainer if_mesh = addrMesh.Assign (de_wireless);

  Ipv4AddressHelper addrInternet;
  addrInternet.SetBase ("1.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer if_internet = addrInternet.Assign (de_internet);

// Install applications
// Set sinks for receiving data
  PacketSinkHelper sinkTcp ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), 9));
  ApplicationContainer ac_sinkTcp = sinkTcp.Install (nc_destiny);
  ac_sinkTcp.Start (Seconds (10.0));
  ac_sinkTcp.Stop (Seconds (m_totalTime - 1));

// Set traffic generator apps
  Config::SetDefault ("ns3::OnOffApplication::PacketSize", UintegerValue (m_packetSize));
  Config::SetDefault ("ns3::OnOffApplication::DataRate", StringValue (m_txAppRate));
  std::ostringstream oss_crv;
  oss_crv << "ns3::ConstantRandomVariable[Constant=" << std::rand () % 10 << "]";
  Config::SetDefault ("ns3::OnOffApplication::OnTime", StringValue (oss_crv.str ()));
  oss_crv << "ns3::ConstantRandomVariable[Constant=" << std::rand () % 10 << "]";
  Config::SetDefault ("ns3::OnOffApplication::OffTime", StringValue (oss_crv.str ()));
  //Config::SetDefault ("ns3::OnOffApplication::OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=10.0]"));
  //Config::SetDefault ("ns3::OnOffApplication::OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=2.0]"));

  ApplicationContainer ac_onoffMesh; // Creates 1 OnOff App in each mesh node TO internet
  OnOffHelper onoffMesh ("ns3::TcpSocketFactory", Address (InetSocketAddress (if_internet.GetAddress (0), 9)));
  //onoffMesh.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=3.0]"));
  //onoffMesh.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=5.0]"));
  ac_onoffMesh = onoffMesh.Install (nc_mesh);
  ac_onoffMesh.Start (Seconds (30));
  ac_onoffMesh.Stop (Seconds (m_totalTime - 10));

  ApplicationContainer ac_onoffInternet [m_xNodes * m_yNodes]; // Creates 1 OnOff App for each mesh node FROM internet
  for (tmp_x = 0; tmp_x < m_xNodes * m_yNodes; tmp_x++)
  {
    OnOffHelper onoffInternet ("ns3::TcpSocketFactory", Address (InetSocketAddress (if_mesh.GetAddress (tmp_x), 9)));
    ac_onoffInternet [tmp_x] = onoffInternet.Install (nc_all.Get (0));
    ac_onoffInternet [tmp_x].Start (Seconds (30));
    ac_onoffInternet [tmp_x].Stop (Seconds (m_totalTime - 5));
  }

/////PING FOR TESTS
  V4PingHelper ping1 (if_internet.GetAddress (0));
  ping1.SetAttribute ("Verbose", BooleanValue (true));
  ping1.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
  ApplicationContainer appPingInternet1 = ping1.Install (nc_all.Get(m_xNodes * m_yNodes + 1));
  appPingInternet1.Start (Seconds (0));
  appPingInternet1.Stop (Seconds (m_totalTime - 1));

  V4PingHelper ping2 (if_mesh.GetAddress (0));
  ping2.SetAttribute ("Verbose", BooleanValue (true));
  ping2.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
  ApplicationContainer appPingInternet2 = ping2.Install (nc_all.Get(m_xNodes * m_yNodes + 1));
  appPingInternet2.Start (Seconds (0));
  appPingInternet2.Stop (Seconds (m_totalTime - 1));

  
  
/////END PING FOR TESTS


//// Sets animation configs and create .xml file for NetAnim
//  Simulator::Stop (Seconds (m_totalTime));
//
//  for (tmp_x = 2; tmp_x < m_xNodes * m_yNodes + 2; tmp_x++)
//  {
//    sprintf (tmp_char, "%d-STA", tmp_x);
//    AnimationInterface::SetNodeDescription (nc_all.Get (tmp_x), tmp_char);
//  }
//  AnimationInterface::SetNodeDescription (nc_all.Get (0), "0-Internet");
//  AnimationInterface::SetNodeDescription (nc_all.Get (1), "1-AP");
//  AnimationInterface::SetNodeColor (nc_mesh, 0, 255, 0);
//  AnimationInterface::SetNodeColor (nc_internet.Get (0), 255, 0, 0);
//  AnimationInterface::SetNodeColor (nc_internet.Get (1), 0, 0, 255);
//  if (m_drawAnim)
//  {
//    AnimationInterface anim (m_animFile);
//    anim.EnablePacketMetadata(true);
////    anim.EnableIpv4RouteTracking (m_routeFile, Seconds (0), Seconds (m_totalTime), Seconds (0.25));
//  }

// Flow Monitor
  FlowMonitorHelper flowmon;
  Ptr<FlowMonitor> monitor = flowmon.Install (nc_destiny);
  monitor->Start (Seconds (0));
  monitor->Stop (Seconds (m_totalTime));
	monitor->SerializeToXmlFile (m_flowmonFile, true, true);

// Run the simulation
  Simulator::Run ();

//////////// Log data

// If requested, clean the flow file for a new use
  if (m_newFlowFile)
  {
    std::ostringstream os_clear;
    os_clear << m_statsFile << "_flows.csv";
    std::ofstream of_clear (os_clear.str().c_str(), std::ios::out | std::ios::trunc);
    of_clear << """Src IP""\t""Dest IP""\t";
    of_clear << """Src Port""\t""Dest Port""\t";
    of_clear << """First Tx Pkt""\t""Last Tx Pkt""\t";
    of_clear << """First Rx Pkt""\t""Last Rx Pkt""\t";
    of_clear << """Total Tx Bytes""\t""Total Rx Bytes""\t";
    of_clear << """Total Tx Packets""\t""Total Rx Packets""\n";
    of_clear.close ();
  }

// Open file to append new data
  std::ostringstream os;
  os << m_statsFile << "_flows.csv";
  std::ofstream of (os.str().c_str(), std::ios::out | std::ios::app);

//Print per flow statistics
  monitor->CheckForLostPackets ();
  Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmon.GetClassifier ());
  std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats ();
  for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin (); i != stats.end (); ++i)
  {
    Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (i->first);
    of << t.sourceAddress << "\t" << t.destinationAddress << "\t";
    of << t.sourcePort << "\t" << t.destinationPort << "\t";
    of << i->second.timeFirstTxPacket.GetSeconds() << "\t" << i->second.timeLastTxPacket.GetSeconds() << "\t";
    of << i->second.timeFirstRxPacket.GetSeconds() << "\t" << i->second.timeLastTxPacket.GetSeconds() << "\t";
    of << i->second.txBytes << "\t" << i->second.rxBytes << "\t";
    of << i->second.txPackets << "\t" << i->second.rxPackets << "\n";
  }
  of << """AODV""\t" << m_xNodes << "x" << m_yNodes <<"\n";
  of.close ();
//////////// End Log data

  Simulator::Destroy ();
  return 0;
}

