#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mesh-module.h"
#include "ns3/mobility-module.h"
#include "ns3/mesh-helper.h"
#include "ns3/netanim-module.h"
#include "src/point-to-point/helper/point-to-point-helper.h"
#include "src/csma/helper/csma-helper.h"
#include "ns3/olsr-helper.h"
#include "ns3/ipv4-global-routing-helper.h"

#include <iostream>
#include <sstream>
#include <fstream>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("infrastructure-mesh");

class MeshTest
{
public:
  /// Init test
  MeshTest ();
  /// Configure test from command line arguments
  void Configure (int argc, char ** argv);
  /// Run test
  int Run ();
private:
  int m_backbone;
  int m_gw;
  int m_mr;
  int m_ap;
  int m_sta;
  double m_step;
  double m_randomStart;
  double m_totalTime;
  double m_packetInterval;
  uint16_t m_packetSize;
  uint32_t m_nIfaces;
  bool m_chan;
  bool m_pcap;
  std::string m_stack;
  std::string m_root;

  /// NodeContainer for individual nodes
  NodeContainer nc_sta1, nc_sta2;
  NodeContainer nc_ap1, nc_ap2;
  NodeContainer nc_mr1, nc_mr2;
  NodeContainer nc_gw1, nc_gw2;
  NodeContainer nc_bb1;

  // NodeContainer for categorical nodes
  NodeContainer nc_sta, nc_ap;

  // NodeContainer for connected nodes
  NodeContainer nc_sta1Ap1, nc_sta2Ap2;
  NodeContainer nc_ap1Mr1, nc_ap2Mr2;
  NodeContainer nc_mr1Gw1, nc_mr2Gw2;
  NodeContainer nc_gw1Bb1, nc_gw2Bb1;


  // List of categorical NetDevice Container
  NetDeviceContainer de_sta1, de_sta2;
  NetDeviceContainer de_ap1, de_ap2;

  // List of WiFi NetDevice Container
  NetDeviceContainer de_wifi_sta1Ap1;
  NetDeviceContainer de_wifi_sta2Ap2;

  // List of mesh NetDevice Container
  NetDeviceContainer de_mesh_mr1Gw1;
  NetDeviceContainer de_mesh_mr2Gw2;

  // List of CSMA NetDevice Container
  NetDeviceContainer de_csma_ap1Mr1;
  NetDeviceContainer de_csma_ap2Mr2;

  // List of p2p NetDevice Container
  NetDeviceContainer de_p2p_gw1Bb1;
  NetDeviceContainer de_p2p_gw2Bb1;

  // List of interface container
  Ipv4InterfaceContainer if_wifi_sta1Ap1;
  Ipv4InterfaceContainer if_wifi_sta2Ap2;

  Ipv4InterfaceContainer if_mesh_mr1Gw1;
  Ipv4InterfaceContainer if_mesh_mr2Gw2;

  Ipv4InterfaceContainer if_csma_ap1Mr1;
  Ipv4InterfaceContainer if_csma_ap2Mr2;

  Ipv4InterfaceContainer if_p2p_gw1Bb1;
  Ipv4InterfaceContainer if_p2p_gw2Bb1;

  // Helper
  MeshHelper meshHelper;
  PointToPointHelper p2pHelper;
  CsmaHelper csmaHelper;
  Ipv4AddressHelper address;

private:
  /// Create nodes and setup their mobility
  void CreateNodes ();
  /// Install internet m_stack on nodes
  void InstallInternetStack ();
  /// Install applications
  void InstallApplication ();
  /// Print mesh devices diagnostics
  void Report ();
};

MeshTest::MeshTest () :
m_backbone (1),
//m_mesh (6),
m_gw (2),
m_mr (2),
m_ap (2),
m_sta (2),
m_step (100.0),
m_randomStart (0.1),
m_totalTime (5.0),
m_packetInterval (0.1),
m_packetSize (1024),
m_nIfaces (1),
m_chan (true),
m_pcap (false),
m_stack ("ns3::Dot11sStack"),
m_root ("ff:ff:ff:ff:ff:ff") { }

void
MeshTest::Configure (int argc, char *argv[])
{
  CommandLine cmd;
  cmd.AddValue ("step", "Size of edge in our grid, meters. [100 m]", m_step);
  /*
   * As soon as starting node means that it sends a beacon,
   * simultaneous start is not good.
   */
  cmd.AddValue ("start", "Maximum random start delay, seconds. [0.1 s]", m_randomStart);
  cmd.AddValue ("time", "Simulation time, seconds [100 s]", m_totalTime);
  cmd.AddValue ("packet-interval", "Interval between packets in UDP ping, seconds [0.001 s]", m_packetInterval);
  cmd.AddValue ("packet-size", "Size of packets in UDP ping", m_packetSize);
  cmd.AddValue ("interfaces", "Number of radio interfaces used by each mesh point. [1]", m_nIfaces);
  cmd.AddValue ("channels", "Use different frequency channels for different interfaces. [0]", m_chan);
  cmd.AddValue ("pcap", "Enable PCAP traces on interfaces. [0]", m_pcap);
  cmd.AddValue ("stack", "Type of protocol stack. ns3::Dot11sStack by default", m_stack);
  cmd.AddValue ("root", "Mac address of root mesh point in HWMP", m_root);

  cmd.Parse (argc, argv);
  //NS_LOG_DEBUG("Grid:" << m_xSize << "*" << m_ySize);
  //NS_LOG_DEBUG("Simulation time: " << m_totalTime << " s");
}

void
MeshTest::CreateNodes ()
{
  // Create individual nodes in their node container
  nc_sta1.Create (1);
  nc_sta2.Create (1);
  nc_ap1.Create (1);
  nc_ap2.Create (1);
  nc_mr1.Create (1);
  nc_mr2.Create (1);
  nc_gw1.Create (1);
  nc_gw2.Create (1);
  nc_bb1.Create (1);

  // Create categorical Node Container
  nc_sta = NodeContainer (nc_sta1, nc_sta2);
  nc_ap = NodeContainer (nc_ap1, nc_ap2);

  // Create connected nodes in their node container
  nc_sta1Ap1 = NodeContainer (nc_sta1, nc_ap1);
  nc_sta2Ap2 = NodeContainer (nc_sta2, nc_ap2);
  nc_ap1Mr1 = NodeContainer (nc_ap1, nc_mr1);
  nc_ap2Mr2 = NodeContainer (nc_ap2, nc_mr2);
  nc_mr1Gw1 = NodeContainer (nc_mr1, nc_gw1);
  nc_mr2Gw2 = NodeContainer (nc_mr2, nc_gw2);
  nc_gw1Bb1 = NodeContainer (nc_gw1, nc_bb1);
  nc_gw2Bb1 = NodeContainer (nc_gw2, nc_bb1);


  // Create p2p links between backbone (bb1) and gateways (gw1, gw2)
  p2pHelper.SetDeviceAttribute ("DataRate", StringValue ("100Mbps"));
  p2pHelper.SetChannelAttribute ("Delay", StringValue ("10ms"));
  de_p2p_gw1Bb1 = p2pHelper.Install (nc_gw1Bb1);
  de_p2p_gw2Bb1 = p2pHelper.Install (nc_gw2Bb1);

  // Create CSMA connection between MRs (mr1, mr2) and APs (ap1, ap2)
  csmaHelper.SetChannelAttribute ("DataRate", StringValue ("100Mbps"));
  csmaHelper.SetChannelAttribute ("Delay", TimeValue (NanoSeconds (6560)));
  de_csma_ap1Mr1 = csmaHelper.Install (nc_ap1Mr1);
  de_csma_ap2Mr2 = csmaHelper.Install (nc_ap2Mr2);

  // Configure YansWifiChannel
  YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default ();
  YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default ();
  wifiPhy.SetChannel (wifiChannel.Create ());

  /*
   * Create mesh helper and set stack installer to it
   * Stack installer creates all needed protocols and install them to
   * mesh point device
   */
  meshHelper = MeshHelper::Default ();
  if (!Mac48Address (m_root.c_str ()).IsBroadcast ())
    {
      meshHelper.SetStackInstaller (m_stack, "Root", Mac48AddressValue (Mac48Address (m_root.c_str ())));
    }
  else
    {
      //If root is not set, we do not use "Root" attribute, because it
      //is specified only for 11s
      meshHelper.SetStackInstaller (m_stack);
    }
  if (m_chan)
    {
      meshHelper.SetSpreadInterfaceChannels (MeshHelper::SPREAD_CHANNELS);
    }
  else
    {
      meshHelper.SetSpreadInterfaceChannels (MeshHelper::ZERO_CHANNEL);
    }
  meshHelper.SetMacType ("RandomStart", TimeValue (Seconds (m_randomStart)));
  // Set number of interfaces - default is single-interface mesh point
  meshHelper.SetNumberOfInterfaces (m_nIfaces);
  // Install protocols and return container if MeshPointDevices
  de_mesh_mr1Gw1 = meshHelper.Install (wifiPhy, nc_mr1Gw1);
  de_mesh_mr2Gw2 = meshHelper.Install (wifiPhy, nc_mr2Gw2);

  // TODO: Setup Mobility for mesh nodes

  // Setup WiFi network
  WifiHelper wifi = WifiHelper::Default ();
  wifi.SetRemoteStationManager ("ns3::AarfWifiManager");

  NqosWifiMacHelper mac = NqosWifiMacHelper::Default ();

  // TODO: Change SSID for different networks
  // Install on different ap1 <--> sta1, ap2 <--> sta2

  // STA1 and AP1 are initialized for network 1
  Ssid ssid = Ssid ("network-1");
  mac.SetType ("ns3::StaWifiMac",
               "Ssid", SsidValue (ssid),
               "ActiveProbing", BooleanValue (false));

  de_sta1 = wifi.Install (wifiPhy, mac, nc_sta1);

  // Setup AP for network 1
  mac.SetType ("ns3::ApWifiMac",
               "Ssid", SsidValue (ssid));

  de_ap1 = wifi.Install (wifiPhy, mac, nc_ap1);


  // STA and APs are initialized for network 2
  ssid = Ssid ("network-2");
  mac.SetType ("ns3::StaWifiMac",
               "Ssid", SsidValue (ssid),
               "ActiveProbing", BooleanValue (false));

  de_sta2 = wifi.Install (wifiPhy, mac, nc_sta2);

  // Setup AP for network 2
  mac.SetType ("ns3::ApWifiMac",
               "Ssid", SsidValue (ssid));

  de_ap2 = wifi.Install (wifiPhy, mac, nc_ap2);

  // Net Device container for STA and AP in network 1
  de_wifi_sta1Ap1.Add (de_sta1);
  de_wifi_sta1Ap1.Add (de_ap1);

  // Net Device container for STA and AP in network 2
  de_wifi_sta2Ap2.Add (de_sta2);
  de_wifi_sta2Ap2.Add (de_ap2);



  // Setup mobility for WiFi nodes
  MobilityHelper mobility;
  mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                                 "MinX", DoubleValue (0.0),
                                 "MinY", DoubleValue (0.0),
                                 "DeltaX", DoubleValue (m_step),
                                 "DeltaY", DoubleValue (m_step),
                                 "GridWidth", UintegerValue (5),
                                 "LayoutType", StringValue ("RowFirst"));
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (nc_sta1);
  mobility.Install (nc_ap1);
  mobility.Install (nc_mr1);
  mobility.Install (nc_gw1);
  mobility.Install (nc_bb1);
  mobility.Install (nc_sta2);
  mobility.Install (nc_ap2);
  mobility.Install (nc_mr2);
  mobility.Install (nc_gw2);
  


}

void
MeshTest::InstallInternetStack ()
{

  InternetStackHelper internetStackHelper;
  OlsrHelper routingProtocol;
  internetStackHelper.SetRoutingHelper (routingProtocol);  
  
  internetStackHelper.Install (nc_sta1);
  internetStackHelper.Install (nc_sta2);
  internetStackHelper.Install (nc_ap1);
  internetStackHelper.Install (nc_ap2);
  internetStackHelper.Install (nc_mr1);
  internetStackHelper.Install (nc_mr2);
  internetStackHelper.Install (nc_gw1);
  internetStackHelper.Install (nc_gw2);
  internetStackHelper.Install (nc_bb1);



  // Network 1 (left)
  address.SetBase ("10.1.1.0", "255.255.255.0");
  if_wifi_sta1Ap1 = address.Assign (de_wifi_sta1Ap1);

  address.SetBase ("10.1.2.0", "255.255.255.0");
  if_csma_ap1Mr1 = address.Assign (de_csma_ap1Mr1);

  address.SetBase ("10.1.3.0", "255.255.255.0");
  if_mesh_mr1Gw1 = address.Assign (de_mesh_mr1Gw1);

  address.SetBase ("10.1.4.0", "255.255.255.0");
  if_p2p_gw1Bb1 = address.Assign (de_p2p_gw1Bb1);

  // Network 2 (right)
  address.SetBase ("20.1.1.0", "255.255.255.0");
  if_wifi_sta2Ap2 = address.Assign (de_wifi_sta2Ap2);

  address.SetBase ("20.1.2.0", "255.255.255.0");
  if_csma_ap2Mr2 = address.Assign (de_csma_ap2Mr2);

  address.SetBase ("20.1.3.0", "255.255.255.0");
  if_mesh_mr2Gw2 = address.Assign (de_mesh_mr2Gw2);

  address.SetBase ("20.1.4.0", "255.255.255.0");
  if_p2p_gw2Bb1 = address.Assign (de_p2p_gw2Bb1);

}

void
MeshTest::InstallApplication ()
{
  // Server is set on STA2 in network 2 (right)
  UdpEchoServerHelper echoServer (9);
  ApplicationContainer serverApps = echoServer.Install (nc_mr1.Get (0));
  serverApps.Start (Seconds (0.0));
  serverApps.Stop (Seconds (m_totalTime));

  // Client is set on STA1 in network 1 (left)
  UdpEchoClientHelper echoClient (if_csma_ap1Mr1.GetAddress (1), 9);
  echoClient.SetAttribute ("MaxPackets", UintegerValue ((uint32_t) (m_totalTime * (1 / m_packetInterval))));
  echoClient.SetAttribute ("Interval", TimeValue (Seconds (m_packetInterval)));
  echoClient.SetAttribute ("PacketSize", UintegerValue (m_packetSize));
  ApplicationContainer clientApps = echoClient.Install (nc_ap1.Get (0));
  clientApps.Start (Seconds (0.0));
  clientApps.Stop (Seconds (m_totalTime));
  
  //Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
}

int
MeshTest::Run ()
{

  CreateNodes ();
  InstallInternetStack ();
  InstallApplication ();
  Simulator::Stop (Seconds (m_totalTime));
  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}

int
main (int argc, char *argv[])
{
  ns3::PacketMetadata::Enable ();
  LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
  LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);
  MeshTest t;
  t.Configure (argc, argv);
  return t.Run ();
}
