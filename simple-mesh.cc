/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/* Net topology
 *
 * With a 3x3 mesh grid:
 *
 * n0----n1 ))) n2
 *
 *  n0: internet node
 *  n1: access node
 *  n2: mesh node
 *
 *  n1 has 2 interfaces (P2P and wireless)
 *  n0 has ip 1.1.1.1 to emulate internet access
 *  n1 has ip 1.1.1.2 and 10.0.0.1
 *  n2 has ip 10.0.0.2
 *
 */

#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/netanim-module.h"
#include "ns3/v4ping-helper.h"
#include "ns3/olsr-helper.h"
#include "ns3/aodv-helper.h"

using namespace ns3;

int main (int argc, char *argv[])
{
  ns3::PacketMetadata::Enable ();
  NS_LOG_COMPONENT_DEFINE ("SimpleMesh");
  //LogComponentEnable ("V4Ping", LOG_LEVEL_DEBUG);
  LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
  LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);
//  LogComponentEnable ("AodvRoutingProtocol", LOG_LEVEL_DEBUG);
//  LogComponentEnable ("YansWifiPhy", LOG_LEVEL_DEBUG);
//  LogComponentEnableAll (LOG_LEVEL_DEBUG);

  NodeContainer nc_all; // Contains every node in order
  NodeContainer nc_p2p; // Contains n0 and n1 nodes
  NodeContainer nc_mesh; // Contains n1 and n2 nodes
  nc_all.Create (3);
  nc_p2p.Add (nc_all.Get (0));
  nc_p2p.Add (nc_all.Get (1));
  nc_mesh.Add (nc_all.Get (1));
  nc_mesh.Add (nc_all.Get (2));

/*  PointToPointHelper p2ph; // Set p2p link between node 0 and 1 
  p2ph.SetDeviceAttribute ("DataRate", StringValue ("1Mbps"));
  //p2ph.SetChannelAttribute ("DataRate", StringValue ("1000000"));
  p2ph.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (2)));
  NetDeviceContainer de_p2p = p2ph.Install (nc_p2p);*/

  WifiHelper wifi2; // Set up extra wifi internet interface
  YansWifiPhyHelper wifiPhy2 = YansWifiPhyHelper::Default ();
  YansWifiChannelHelper wifiChannel2 = YansWifiChannelHelper::Default ();
  NqosWifiMacHelper wifiMac2 = NqosWifiMacHelper::Default ();
  wifi2.SetStandard (WIFI_PHY_STANDARD_80211a);
  wifi2.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                "DataMode",StringValue ("OfdmRate6Mbps"),
                                "ControlMode",StringValue ("OfdmRate6Mbps"));
  wifiPhy2.SetPcapDataLinkType (YansWifiPhyHelper::DLT_IEEE802_11_RADIO); 
  wifiChannel2.AddPropagationLoss ("ns3::RangePropagationLossModel","MaxRange", DoubleValue (100));
  wifiMac2.SetType ("ns3::AdhocWifiMac");
  wifiPhy2.SetChannel (wifiChannel2.Create ());
/*  wifiPhy.Set ("TxPowerStart", DoubleValue(16.0206));
  wifiPhy.Set ("TxPowerEnd", DoubleValue(16.0206));
  wifiPhy.Set ("TxPowerLevels", UintegerValue(1));
  wifiPhy.Set ("TxGain", DoubleValue(0) );
  wifiPhy.Set ("RxGain", DoubleValue (0) );*/
  NetDeviceContainer de_p2p = wifi2.Install (wifiPhy2, wifiMac2, nc_p2p);



  WifiHelper wifi; // Set up wifi adhoc interface
  YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default ();
  YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default ();
  NqosWifiMacHelper wifiMac = NqosWifiMacHelper::Default ();
  wifi.SetStandard (WIFI_PHY_STANDARD_80211a);
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                "DataMode",StringValue ("OfdmRate6Mbps"),
                                "ControlMode",StringValue ("OfdmRate6Mbps"));
  wifiPhy.SetPcapDataLinkType (YansWifiPhyHelper::DLT_IEEE802_11_RADIO); 
  wifiChannel.AddPropagationLoss ("ns3::RangePropagationLossModel","MaxRange", DoubleValue (100));
  wifiMac.SetType ("ns3::AdhocWifiMac");
  wifiPhy.SetChannel (wifiChannel.Create ());
/*  wifiPhy.Set ("TxPowerStart", DoubleValue(16.0206));
  wifiPhy.Set ("TxPowerEnd", DoubleValue(16.0206));
  wifiPhy.Set ("TxPowerLevels", UintegerValue(1));
  wifiPhy.Set ("TxGain", DoubleValue(0) );
  wifiPhy.Set ("RxGain", DoubleValue (0) );*/
  NetDeviceContainer de_mesh = wifi.Install (wifiPhy, wifiMac, nc_mesh);

  MobilityHelper mobilityInternet; //Position nodes
  mobilityInternet.SetPositionAllocator ("ns3::GridPositionAllocator",
                                    "MinX", DoubleValue (0),
                                    "MinY", DoubleValue (0),
                                    "DeltaX", DoubleValue (75),
                                    "DeltaY", DoubleValue (75),
                                    "GridWidth", UintegerValue (5),
                                    "LayoutType", StringValue ("RowFirst"));
  mobilityInternet.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobilityInternet.Install (nc_all);

  InternetStackHelper internetStack; //Config IP, addresses and routing protocol
  /*OlsrHelper routingProtocol;
  internetStack.SetRoutingHelper (routingProtocol);*/
  internetStack.Install (nc_all);

  Ipv4AddressHelper addrMesh;
  addrMesh.SetBase ("10.0.0.0", "255.255.255.0");
  Ipv4InterfaceContainer if_mesh;
  if_mesh = addrMesh.Assign (de_mesh);

  Ipv4AddressHelper addrInternet;
  addrInternet.SetBase ("1.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer if_p2p;
  if_p2p = addrInternet.Assign (de_p2p);

  /*V4PingHelper ping1 (if_p2p.GetAddress (0)); // Install App ping - from mesh node to internet
  ping1.SetAttribute ("Verbose", BooleanValue (true));
  ping1.SetAttribute ("Interval", TimeValue (Seconds (5.0)));
  ApplicationContainer appPingInternet1 = ping1.Install (nc_all.Get(2));
  appPingInternet1.Start (Seconds (4));
  appPingInternet1.Stop (Seconds (25));*/

  UdpEchoServerHelper echoServer (9);
  ApplicationContainer serverApps = echoServer.Install (nc_all.Get (0));

  serverApps.Start (Seconds (1.0));
  serverApps.Stop (Seconds (30.0));
  UdpEchoClientHelper echoClient (if_p2p.GetAddress (0), 9);
  echoClient.SetAttribute ("MaxPackets", UintegerValue (5));
  echoClient.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
  echoClient.SetAttribute ("PacketSize", UintegerValue (1024));

  ApplicationContainer clientApps = echoClient.Install (nc_all.Get (2));
  clientApps.Start (Seconds (2.0));
  clientApps.Stop (Seconds (30.0));

  Simulator::Stop (Seconds (30));
  AnimationInterface anim ("simple-mesh.xml");
  anim.EnablePacketMetadata(true);
  anim.EnableIpv4RouteTracking ("SimpleMesh-route.xml", Seconds (0), Seconds (10), Seconds (0.25));

  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  // Run the simulation

  Simulator::Run ();
  Simulator::Destroy ();
  return 0;

}
