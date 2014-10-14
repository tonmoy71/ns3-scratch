/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/* Net topology
 *
 * With a 3x3 mesh grid:
 *
 * 
 *       n1 ))) 
 *       |
 * n0----|          n3
 *       |
 *       n2 )))
 * 
 * 
 *  n0: internet node
 *  n1: access node
 *  n2: access node
 *  n3: mesh node
 *
 *  n1 has 2 interfaces (P2P and wireless)
 *  n2 has 2 interfaces (P2P and wireless)
 *  n0 has ip 1.1.1.1 to emulate internet access
 *  n1 has ip 1.1.1.2 and 10.0.0.1
 *  n2 has ip 1.1.1.3 and 10.0.0.2
 *  n3 has ip 10.0.0.3
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
#include "src/core/model/string.h"

using namespace ns3;

int
main (int argc, char *argv[])
{
  ns3::PacketMetadata::Enable ();
  NS_LOG_COMPONENT_DEFINE ("mesh-handover");
  //LogComponentEnable ("V4Ping", LOG_LEVEL_DEBUG);
  LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
  LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);
  //  LogComponentEnable ("AodvRoutingProtocol", LOG_LEVEL_DEBUG);
  //  LogComponentEnable ("YansWifiPhy", LOG_LEVEL_DEBUG);
  //  LogComponentEnableAll (LOG_LEVEL_DEBUG);

  NodeContainer nc_all; // Contains every node in order
  NodeContainer nc_p2p; // Contains n0, n1 and n2 nodes
  NodeContainer nc_mesh; // Contains n1, n2 and n3 nodes
  nc_all.Create (4);
  // Contains n0, n1 and n2
  nc_p2p.Add (nc_all.Get (0));
  nc_p2p.Add (nc_all.Get (1));
  nc_p2p.Add (nc_all.Get (2));
  // Contains n1, n2 and n3
  nc_mesh.Add (nc_all.Get (1));
  nc_mesh.Add (nc_all.Get (2));
  nc_mesh.Add (nc_all.Get (3));

  /*  PointToPointHelper p2ph; // Set p2p link between node 0 and 1 
    p2ph.SetDeviceAttribute ("DataRate", StringValue ("1Mbps"));
    //p2ph.SetChannelAttribute ("DataRate", StringValue ("1000000"));
    p2ph.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (2)));
    NetDeviceContainer de_p2p = p2ph.Install (nc_p2p);*/

  WifiHelper wifiInternet; // Set up extra wifi internet interface
  YansWifiPhyHelper wifiInternetPhy = YansWifiPhyHelper::Default ();
  YansWifiChannelHelper wifiInternetChannel = YansWifiChannelHelper::Default ();
  NqosWifiMacHelper wifiInternetMac = NqosWifiMacHelper::Default ();
  wifiInternet.SetStandard (WIFI_PHY_STANDARD_80211a);
  wifiInternet.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                        "DataMode", StringValue ("OfdmRate6Mbps"),
                                        "ControlMode", StringValue ("OfdmRate6Mbps"));
  wifiInternetPhy.SetPcapDataLinkType (YansWifiPhyHelper::DLT_IEEE802_11_RADIO);
  wifiInternetChannel.AddPropagationLoss ("ns3::RangePropagationLossModel", "MaxRange", DoubleValue (100));
  wifiInternetMac.SetType ("ns3::AdhocWifiMac");
  wifiInternetPhy.SetChannel (wifiInternetChannel.Create ());
  /*  wifiPhy.Set ("TxPowerStart", DoubleValue(16.0206));
    wifiPhy.Set ("TxPowerEnd", DoubleValue(16.0206));
    wifiPhy.Set ("TxPowerLevels", UintegerValue(1));
    wifiPhy.Set ("TxGain", DoubleValue(0) );
    wifiPhy.Set ("RxGain", DoubleValue (0) );*/
  NetDeviceContainer p2pDevice = wifiInternet.Install (wifiInternetPhy, wifiInternetMac, nc_p2p);



  WifiHelper wifiMesh; // Set up wifi adhoc interface for mesh nodes
  YansWifiPhyHelper wifiMeshPhy = YansWifiPhyHelper::Default ();
  YansWifiChannelHelper wifiMeshChannel = YansWifiChannelHelper::Default ();
  NqosWifiMacHelper wifiMesMac = NqosWifiMacHelper::Default ();
  wifiMesh.SetStandard (WIFI_PHY_STANDARD_80211a);
  wifiMesh.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                    "DataMode", StringValue ("OfdmRate6Mbps"),
                                    "ControlMode", StringValue ("OfdmRate6Mbps"));
  wifiMeshPhy.SetPcapDataLinkType (YansWifiPhyHelper::DLT_IEEE802_11_RADIO);
  wifiMeshChannel.AddPropagationLoss ("ns3::RangePropagationLossModel", "MaxRange", DoubleValue (100));
  wifiMesMac.SetType ("ns3::AdhocWifiMac");
  wifiMeshPhy.SetChannel (wifiMeshChannel.Create ());
  /*  wifiPhy.Set ("TxPowerStart", DoubleValue(16.0206));
    wifiPhy.Set ("TxPowerEnd", DoubleValue(16.0206));
    wifiPhy.Set ("TxPowerLevels", UintegerValue(1));
    wifiPhy.Set ("TxGain", DoubleValue(0) );
    wifiPhy.Set ("RxGain", DoubleValue (0) );*/
  NetDeviceContainer meshDevice = wifiMesh.Install (wifiMeshPhy, wifiMesMac, nc_mesh);

  // Set mobility for the internet node and access points
  MobilityHelper constantMobility; //Position nodes
  constantMobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                                         "MinX", DoubleValue (0),
                                         "MinY", DoubleValue (0),
                                         "DeltaX", DoubleValue (75),
                                         "DeltaY", DoubleValue (75),
                                         "GridWidth", UintegerValue (5),
                                         "LayoutType", StringValue ("RowFirst"));
  constantMobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  constantMobility.Install (nc_p2p);

  // Set mobility for mesh node
  MobilityHelper randomMobility; //Position nodes
  randomMobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                                       "MinX", DoubleValue (0),
                                       "MinY", DoubleValue (0),
                                       "DeltaX", DoubleValue (75),
                                       "DeltaY", DoubleValue (75),
                                       "GridWidth", UintegerValue (5),
                                       "LayoutType", StringValue ("RowFirst"));
  randomMobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel", "Bounds", RectangleValue (Rectangle (-500, 500, -500, 500)),
                                   "Speed", StringValue ("ns3::ConstantRandomVariable[Constant=1.0]"),
                                   "Mode", StringValue("Time"));
  randomMobility.Install (nc_mesh.Get (2));

  InternetStackHelper internetStack; //Config IP, addresses and routing protocol
  OlsrHelper routingProtocol;
  internetStack.SetRoutingHelper (routingProtocol);
  internetStack.Install (nc_all);

  Ipv4AddressHelper addrMesh;
  addrMesh.SetBase ("10.0.0.0", "255.255.255.0");
  Ipv4InterfaceContainer meshInterface;
  meshInterface = addrMesh.Assign (meshDevice);

  Ipv4AddressHelper addrInternet;
  addrInternet.SetBase ("1.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer p2pInterface;
  p2pInterface = addrInternet.Assign (p2pDevice);

  /*V4PingHelper ping1 (if_p2p.GetAddress (0)); // Install App ping - from mesh node to internet
  ping1.SetAttribute ("Verbose", BooleanValue (true));
  ping1.SetAttribute ("Interval", TimeValue (Seconds (5.0)));
  ApplicationContainer appPingInternet1 = ping1.Install (nc_all.Get(2));
  appPingInternet1.Start (Seconds (4));
  appPingInternet1.Stop (Seconds (25));*/

  UdpEchoServerHelper echoServer (9);
  ApplicationContainer serverApps = echoServer.Install (nc_all.Get (0));    // Server is on n0 node

  serverApps.Start (Seconds (7.0));
  serverApps.Stop (Seconds (50.0));
  UdpEchoClientHelper echoClient (p2pInterface.GetAddress (0), 9);
  echoClient.SetAttribute ("MaxPackets", UintegerValue (5));
  echoClient.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
  echoClient.SetAttribute ("PacketSize", UintegerValue (1024));

  ApplicationContainer clientApps = echoClient.Install (nc_all.Get (3));    // Client is on n3 node
  clientApps.Start (Seconds (7.0));
  clientApps.Stop (Seconds (50.0));

  Simulator::Stop (Seconds (50));
  AnimationInterface animation ("mesh-handover.xml");
  
  // Set position for the nodes
  animation.SetConstantPosition (nc_all.Get (0), 150.0, 37.0);          // n0 -> internet node
  animation.SetConstantPosition (nc_all.Get (1), 75.0, 0.0);            // n1 -> AP
  animation.SetConstantPosition (nc_all.Get (2), 75.0, 75.0);           // n2 -> AP
  animation.SetConstantPosition (nc_all.Get (3), 0.0, 0.0);             // n3 -> Mesh Node
  
  animation.EnablePacketMetadata (true);
  animation.EnableIpv4RouteTracking ("mesh-handover-route.xml", Seconds (0), Seconds (10), Seconds (0.25));

  //Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  // Run the simulation

  Simulator::Run ();
  Simulator::Destroy ();
  return 0;

}
