#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mesh-module.h"
#include "ns3/mobility-module.h"
#include "ns3/mesh-helper.h"
#include "ns3/flow-monitor-module.h"
#include <iomanip>
#include <string>
#include <iostream>
#include <sstream>
#include <fstream>
#include <ns3/flow-monitor-helper.h>
#include "ns3/gnuplot.h"
#include "ns3/netanim-module.h"
#include "src/point-to-point/helper/point-to-point-helper.h"
#include "src/csma/helper/csma-helper.h"
#include "ns3/olsr-helper.h"
#include "ns3/ipv4-global-routing-helper.h"

#include "ns3/netanim-module.h"
#include "src/network/model/packet-metadata.h"
//#include "mesh.h"

#include <iostream>
#include <sstream>
#include <fstream>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("mesh-internet-handoff");
void
ThroughputMonitor (FlowMonitorHelper *fmhelper, Ptr<FlowMonitor> flowMon, Gnuplot2dDataset DataSet);
//NS_LOG_COMPONENT_DEFINE ("TestMeshScript");

// Method for setting mobility using (x,y) position for the nodes

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
  int m_sta1, m_sta2, m_gw1, m_gw2, m_bb1;
  int m_xSize;
  int m_ySize;
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
  NodeContainer nc_mbb1, nc_mbb2;
  NodeContainer nc_gw1, nc_gw2;
  NodeContainer nc_bb1;

  // NodeContainer for categorical nodes
  NodeContainer nc_sta, nc_mesh1, nc_mesh2;

  // NodeContainer for connected nodes
  NodeContainer nc_sta1Mbb1, nc_sta2Mbb2;
  NodeContainer nc_mbb1Gw1, nc_mbb2Gw2;
  NodeContainer nc_gw1Bb1, nc_gw2Bb1;

  // List of categorical NetDevice Container
  NetDeviceContainer de_sta1, de_sta2, de_ap1, de_ap2;

  // List of WiFi NetDevice Container
  NetDeviceContainer de_wifi_sta1Ap1;
  NetDeviceContainer de_wifi_sta2Ap2;

  // List of mesh NetDevice Container
  NetDeviceContainer de_mesh1;
  NetDeviceContainer de_mesh2;




  // List of p2p NetDevice Container
  NetDeviceContainer de_p2p_gw1Bb1;
  NetDeviceContainer de_p2p_gw2Bb1;

  // List of interface container

  Ipv4InterfaceContainer if_mesh1;
  Ipv4InterfaceContainer if_mesh2;

  Ipv4InterfaceContainer if_p2p_gw1Bb1;
  Ipv4InterfaceContainer if_p2p_gw2Bb1;

  // Helper
  MeshHelper meshHelper1, meshHelper2;
  PointToPointHelper p2pHelper;
  Ipv4AddressHelper address;


private:
  /// Create nodes and setup their mobility
  void CreateNodes ();
  /// Install internet m_stack on nodes
  void InstallInternetStack ();
  /// Install applications
  void InstallApplication ();
  /// Setup mobility
  void SetupMobility ();
  /// Print mesh devices diagnostics
  void Report ();
  /// Setup FlowMonitor
  void InstallFlowMonitor ();
};

MeshTest::MeshTest () :

m_sta1 (1),
m_sta2 (1),
m_gw1 (1),
m_gw2 (1),
m_bb1 (1),

m_xSize (2),
m_ySize (2),

//m_ap (2),
m_sta (1),
m_step (50.0),
m_randomStart (0.1),
m_totalTime (25.0),
m_packetInterval (1.0),
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

  cmd.AddValue ("sta1", "", m_sta1);
  cmd.AddValue ("sta2", "", m_sta2);

  //
  cmd.AddValue ("x-size", "Number of nodes in a row grid. [6]", m_xSize);
  cmd.AddValue ("y-size", "Number of rows in a grid. [6]", m_ySize);
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
  NS_LOG_DEBUG ("Grid:" << m_xSize << "*" << m_ySize);
  NS_LOG_DEBUG ("Simulation time: " << m_totalTime << " s");
}

void
MeshTest::CreateNodes ()
{

  nc_sta1.Create (m_sta1);
  nc_sta2.Create (m_sta2);
  nc_mbb1.Create (m_ySize * m_xSize);
  nc_mbb2.Create (m_ySize * m_xSize);
  nc_gw1.Create (m_gw1);
  nc_gw2.Create (m_gw2);
  nc_bb1.Create (m_bb1);


  nc_sta = NodeContainer (nc_sta1, nc_sta2);
  nc_sta1Mbb1 = NodeContainer (nc_sta1, nc_mbb1);
  nc_sta2Mbb2 = NodeContainer (nc_sta2, nc_mbb2);


  nc_gw1Bb1 = NodeContainer (nc_gw1, nc_bb1);
  nc_gw2Bb1 = NodeContainer (nc_gw2, nc_bb1);


  nc_mesh1 = NodeContainer (nc_sta1, nc_mbb1, nc_gw1);
  nc_mesh2 = NodeContainer (nc_sta2, nc_mbb2, nc_gw2);




  // Create p2p links between backbone (bb1) and gateways (gw1, gw2)
  p2pHelper.SetDeviceAttribute ("DataRate", StringValue ("100Mbps"));
  p2pHelper.SetChannelAttribute ("Delay", StringValue ("10ms"));
  de_p2p_gw1Bb1 = p2pHelper.Install (nc_gw1Bb1);
  de_p2p_gw2Bb1 = p2pHelper.Install (nc_gw2Bb1);


  // Configure YansWifiChannel
  YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default ();
  YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default ();
  wifiPhy.SetChannel (wifiChannel.Create ());



  meshHelper1 = MeshHelper::Default ();
  if (!Mac48Address (m_root.c_str ()).IsBroadcast ())
    {
      meshHelper1.SetStackInstaller (m_stack, "Root", Mac48AddressValue (Mac48Address (m_root.c_str ())));
    }
  else
    {
      //If root is not set, we do not use "Root" attribute, because it
      //is specified only for 11s
      meshHelper1.SetStackInstaller (m_stack);
    }
  if (m_chan)
    {
      meshHelper1.SetSpreadInterfaceChannels (MeshHelper::SPREAD_CHANNELS);
    }
  else
    {
      meshHelper1.SetSpreadInterfaceChannels (MeshHelper::ZERO_CHANNEL);
    }
  meshHelper1.SetMacType ("RandomStart", TimeValue (Seconds (m_randomStart)));
  // Set number of interfaces - default is single-interface mesh point
  meshHelper1.SetNumberOfInterfaces (m_nIfaces);
  // Install protocols and return container if MeshPointDevices
  de_mesh1 = meshHelper1.Install (wifiPhy, nc_mesh1);

  //----------------------mesh router2 ------------------------------------------

  meshHelper2 = MeshHelper::Default ();
  if (!Mac48Address (m_root.c_str ()).IsBroadcast ())
    {
      meshHelper2.SetStackInstaller (m_stack, "Root", Mac48AddressValue (Mac48Address (m_root.c_str ())));
    }
  else
    {
      //If root is not set, we do not use "Root" attribute, because it
      //is specified only for 11s
      meshHelper2.SetStackInstaller (m_stack);
    }
  if (m_chan)
    {
      meshHelper2.SetSpreadInterfaceChannels (MeshHelper::SPREAD_CHANNELS);
    }
  else
    {
      meshHelper2.SetSpreadInterfaceChannels (MeshHelper::ZERO_CHANNEL);
    }
  meshHelper2.SetMacType ("RandomStart", TimeValue (Seconds (m_randomStart)));
  // Set number of interfaces - default is single-interface mesh point
  meshHelper2.SetNumberOfInterfaces (m_nIfaces);

  de_mesh2 = meshHelper2.Install (wifiPhy, nc_mesh2);




  // Setup WiFi for network 1
  WifiHelper wifi1 = WifiHelper::Default ();
  wifi1.SetStandard (WIFI_PHY_STANDARD_80211b);
  wifi1.SetRemoteStationManager ("ns3::AarfWifiManager");

  NqosWifiMacHelper mac1 = NqosWifiMacHelper::Default ();





  // STA1 is initialized for network 1
  Ssid ssid1 = Ssid ("network-1");
  mac1.SetType ("ns3::StaWifiMac",
                "Ssid", SsidValue (ssid1),
                "ActiveProbing", BooleanValue (true));

  de_sta1 = wifi1.Install (wifiPhy, mac1, nc_sta1);


  // Setup AP for network 1
  mac1.SetType ("ns3::ApWifiMac",
                "Ssid", SsidValue (ssid1));

  //de_ap1 = wifi1.Install (wifiPhy, mac1, nc_mbb1.Get (0));
  for (int i = 0; i < (m_xSize*m_ySize); i += m_xSize)
    {
      de_ap1.Add (wifi1.Install (wifiPhy, mac1, nc_mbb1.Get (i)));
    }



  // Setup WiFi for network 2
  WifiHelper wifi2 = WifiHelper::Default ();
  wifi2.SetStandard (WIFI_PHY_STANDARD_80211b);
  wifi2.SetRemoteStationManager ("ns3::AarfWifiManager");

  NqosWifiMacHelper mac2 = NqosWifiMacHelper::Default ();

  // STA is initialized for network 2
  Ssid ssid2 = Ssid ("network-2");
  mac2.SetType ("ns3::StaWifiMac",
                "Ssid", SsidValue (ssid2),
                "ActiveProbing", BooleanValue (true));

  de_sta2 = wifi2.Install (wifiPhy, mac2, nc_sta2);


  // Setup AP for network 2
  mac2.SetType ("ns3::ApWifiMac",
                "Ssid", SsidValue (ssid2));

   for (int i = 0; i < (m_xSize*m_ySize); i += m_xSize)
    {
      de_ap2.Add (wifi1.Install (wifiPhy, mac1, nc_mbb2.Get (i)));
     }

  // Net Device container for STA and AP in network 1
  de_wifi_sta1Ap1.Add (de_sta1);
  de_wifi_sta1Ap1.Add (de_ap1);

  // Net Device container for STA and AP in network 2
  de_wifi_sta2Ap2.Add (de_sta2);
  de_wifi_sta2Ap2.Add (de_ap2);

}

void
MeshTest::SetupMobility ()
{
  if (m_sta1 >= m_sta2)
    m_sta = m_sta1;
  else
    m_sta = m_sta2;

  // Setup mobility for the nodes
  MobilityHelper fixedMobility1;
  fixedMobility1.SetPositionAllocator ("ns3::GridPositionAllocator",
                                       "MinX", DoubleValue ((m_sta - m_sta1) * m_step),
                                       "MinY", DoubleValue ((m_xSize - 1)*.5 * m_step),
                                       "DeltaX", DoubleValue (m_step),
                                       "DeltaY", DoubleValue (m_step),
                                       "GridWidth", UintegerValue (m_sta1),
                                       "LayoutType", StringValue ("RowFirst"));

  fixedMobility1.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
                                   "Bounds", RectangleValue (Rectangle (-500, 500, -500, 500)),
                                   "Speed", StringValue ("ns3::UniformRandomVariable[Min=20.0|Max=50.0]"),
                                   "Direction", StringValue ("ns3::UniformRandomVariable[Min=10.0|Max=26.283184]"));
  fixedMobility1.Install (nc_sta1);






  // Setup mobility for the nodes
  MobilityHelper fixedMobility2;
  fixedMobility2.SetPositionAllocator ("ns3::GridPositionAllocator",
                                       "MinX", DoubleValue ((m_sta) * m_step),
                                       "MinY", DoubleValue (0.0),
                                       "DeltaX", DoubleValue (m_step),
                                       "DeltaY", DoubleValue (m_step),
                                       "GridWidth", UintegerValue (m_xSize),
                                       "LayoutType", StringValue ("RowFirst"));


  fixedMobility2.SetMobilityModel ("ns3::ConstantPositionMobilityModel");

  fixedMobility2.Install (nc_mbb1);



  MobilityHelper fixedMobility3;
  fixedMobility3.SetPositionAllocator ("ns3::GridPositionAllocator",
                                       "MinX", DoubleValue ((m_sta + m_xSize) * m_step),
                                       "MinY", DoubleValue ((m_xSize - 1)*.5 * m_step),
                                       "DeltaX", DoubleValue (m_step),
                                       "DeltaY", DoubleValue (m_step),
                                       "GridWidth", UintegerValue (2),
                                       "LayoutType", StringValue ("RowFirst"));


  fixedMobility3.SetMobilityModel ("ns3::ConstantPositionMobilityModel");

  fixedMobility3.Install (nc_gw1);


  // Setup mobility for the nodes
  MobilityHelper fixedMobility4;
  fixedMobility4.SetPositionAllocator ("ns3::GridPositionAllocator",
                                       "MinX", DoubleValue ((m_sta - m_sta2) * m_step),
                                       "MinY", DoubleValue (((m_xSize - 1)*.5 * m_step)+(m_xSize + 1) * m_step),
                                       "DeltaX", DoubleValue (m_step),
                                       "DeltaY", DoubleValue (m_step),
                                       "GridWidth", UintegerValue (m_sta),
                                       "LayoutType", StringValue ("RowFirst"));

  fixedMobility4.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  fixedMobility4.Install (nc_sta2);






  // Setup mobility for the nodes
  MobilityHelper fixedMobility5;
  fixedMobility5.SetPositionAllocator ("ns3::GridPositionAllocator",
                                       "MinX", DoubleValue (((m_sta) * m_step)),
                                       "MinY", DoubleValue ((m_xSize + 1) * m_step),
                                       "DeltaX", DoubleValue (m_step),
                                       "DeltaY", DoubleValue (m_step),
                                       "GridWidth", UintegerValue (m_xSize),
                                       "LayoutType", StringValue ("RowFirst"));


  fixedMobility5.SetMobilityModel ("ns3::ConstantPositionMobilityModel");

  fixedMobility5.Install (nc_mbb2);

  //SetPosition (nc_gw1.Get (0),(m_sta1+m_ap1+m_xSize)*m_step , ((m_xSize-1)/2)*m_step);

  MobilityHelper fixedMobility6;
  fixedMobility6.SetPositionAllocator ("ns3::GridPositionAllocator",
                                       "MinX", DoubleValue ((m_sta + m_xSize) * m_step),
                                       "MinY", DoubleValue (((m_xSize - 1)*.5 * m_step)+(m_xSize + 1) * m_step),
                                       "DeltaX", DoubleValue (m_step),
                                       "DeltaY", DoubleValue (m_step),
                                       "GridWidth", UintegerValue (2),
                                       "LayoutType", StringValue ("RowFirst"));


  fixedMobility6.SetMobilityModel ("ns3::ConstantPositionMobilityModel");

  fixedMobility6.Install (nc_gw2);


  MobilityHelper fixedMobility7;
  fixedMobility7.SetPositionAllocator ("ns3::GridPositionAllocator",
                                       "MinX", DoubleValue ((m_sta + m_xSize + m_gw1) * m_step),
                                       "MinY", DoubleValue ((m_xSize + m_ySize)*.5 * m_step),
                                       "DeltaX", DoubleValue (m_step),
                                       "DeltaY", DoubleValue (m_step),
                                       "GridWidth", UintegerValue (2),
                                       "LayoutType", StringValue ("RowFirst"));


  fixedMobility7.SetMobilityModel ("ns3::ConstantPositionMobilityModel");

  fixedMobility7.Install (nc_bb1);





}

void
MeshTest::InstallInternetStack ()
{

  InternetStackHelper internetStackHelper;
  OlsrHelper routingProtocol;
  internetStackHelper.SetRoutingHelper (routingProtocol);


  // Setup internet stack on the nodes
  internetStackHelper.Install (nc_sta1);
  internetStackHelper.Install (nc_sta2);
  internetStackHelper.Install (nc_mbb1);
  internetStackHelper.Install (nc_mbb2);
  internetStackHelper.Install (nc_gw1);
  internetStackHelper.Install (nc_gw2);
  internetStackHelper.Install (nc_bb1);


  // Network 1 (left)

  address.SetBase ("10.1.1.0", "255.255.255.0");
  if_mesh1 = address.Assign (de_mesh1);

  address.SetBase ("10.1.2.0", "255.255.255.0");
  if_p2p_gw1Bb1 = address.Assign (de_p2p_gw1Bb1);



  // Network 2 (right)

  address.SetBase ("20.1.1.0", "255.255.255.0");
  if_mesh2 = address.Assign (de_mesh2);

  address.SetBase ("20.1.2.0", "255.255.255.0");
  if_p2p_gw2Bb1 = address.Assign (de_p2p_gw2Bb1);

}

void
MeshTest::InstallApplication ()
{

  // Server is set on STA2 in network 2 (right)
  UdpEchoServerHelper echoServer (9);
  ApplicationContainer serverApps = echoServer.Install (nc_sta2.Get (0));
  serverApps.Start (Seconds (0.0));
  serverApps.Stop (Seconds (m_totalTime));

  // Client is set on STA1 in network 1 (left)
  UdpEchoClientHelper echoClient (if_mesh2.GetAddress (0), 9);
  echoClient.SetAttribute ("MaxPackets", UintegerValue ((uint32_t) (m_totalTime * (1 / m_packetInterval))));
  echoClient.SetAttribute ("Interval", TimeValue (Seconds (m_packetInterval)));
  echoClient.SetAttribute ("PacketSize", UintegerValue (m_packetSize));
  ApplicationContainer clientApps = echoClient.Install (nc_sta1.Get (0));
  clientApps.Start (Seconds (0.0));
  clientApps.Stop (Seconds (m_totalTime));

}

int
MeshTest::Run ()
{
  CreateNodes ();
  InstallInternetStack ();
  SetupMobility ();
  InstallApplication ();

  Simulator::Stop (Seconds (m_totalTime));
  Simulator::Run ();

  Simulator::Destroy ();

  return 0;



  //InstallFlowMonitor ();


}

void
MeshTest::InstallFlowMonitor () {

  //flowMon = fmHelper.Install (nodes.Get (0));
  //flowMon = fmHelper.Install (nodes.Get (m_xSize * m_ySize - 1));
}

int
main (int argc, char *argv[])
{
  LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
  LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);
  ns3::PacketMetadata::Enable ();
  MeshTest t;
  t.Configure (argc, argv);
  return t.Run ();
}

