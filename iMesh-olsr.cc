/* Net topology
 *
 *
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
 *  n0: Internet node
 *  n1: access node (a static mesh node with a P2P link)
 *  n2-n10: mesh nodes
 *
 */



#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mesh-module.h"
#include "ns3/mobility-module.h"
#include "ns3/mesh-helper.h"
#include "ns3/point-to-point-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/netanim-module.h"
#include "ns3/olsr-helper.h"
#include <iostream>
#include <sstream>
#include <fstream>
// Needed for ping and test
#include "ns3/test.h"
#include "ns3/abort.h"
#include "ns3/v4ping.h"
#include "ns3/config.h"
#include "ns3/names.h"
#include "src/core/model/log.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("iMesh");

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
  int m_xSize;
  int m_ySize;
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

  NodeContainer nc_all; // Contains every node (starting with internet node, access nodes and then mesh nodes
  NodeContainer nc_mesh; // Contains mesh nodes
  NodeContainer nc_wireless; // Contains access and mesh nodes
  NodeContainer nc_internet; // Contains internet nodes
  NodeContainer nc_destination; // Contains nodes with sink for apps (internet and mesh nodes)

  /// List of all mesh point devices
  NetDeviceContainer meshDevices;
  NetDeviceContainer internetDevices;

  //Addresses of meshInterfaces:
  Ipv4InterfaceContainer meshInterfaces;
  Ipv4InterfaceContainer p2pInterfaces; /// Needed for n0-----n1

  // MeshHelper. Report is not static methods
  MeshHelper meshHelper;
  PointToPointHelper p2pHelper;

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
m_xSize (3),
m_ySize (3),
m_step (100.0),
m_randomStart (0.1),
m_totalTime (30.0),
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
  cmd.AddValue ("meshInterfaces", "Number of radio interfaces used by each meshHelper point. [1]", m_nIfaces);
  cmd.AddValue ("channels", "Use different frequency channels for different meshInterfaces. [0]", m_chan);
  cmd.AddValue ("pcap", "Enable PCAP traces on meshInterfaces. [0]", m_pcap);
  cmd.AddValue ("stack", "Type of protocol stack. ns3::Dot11sStack by default", m_stack);
  cmd.AddValue ("root", "Mac address of root meshHelper point in HWMP", m_root);

  cmd.Parse (argc, argv);
  NS_LOG_DEBUG ("Grid:" << m_xSize << "*" << m_ySize);
  NS_LOG_DEBUG ("Simulation time: " << m_totalTime << " s");
  
  LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
  LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);
}

void
MeshTest::CreateNodes ()
{

  // Node container creation (all node containers starts with "nc_")

  /*
   * Create m_ySize*m_xSize stations to form a grid topology
   */
  nc_all.Create (2);

  nc_mesh.Create (m_ySize * m_xSize);

  /// Needed to add mesh nodes and wireless nodes to the internet nodes
  nc_all.Add (nc_mesh);
  nc_mesh.Add (nc_all.Get (1)); /// n1---{meshHelper nodes}
  nc_wireless.Add (nc_all.Get (1));
  nc_wireless.Add (nc_mesh);
  nc_internet.Add (nc_all.Get (0));
  nc_internet.Add (nc_all.Get (1));
  nc_destination.Add (nc_all.Get (0));
  nc_destination.Add (nc_mesh);

  //Create P2P links between n0 <-> n1 (all devices starts with "de_")

  p2pHelper.SetDeviceAttribute ("DataRate", StringValue ("1Mbps"));
  p2pHelper.SetChannelAttribute ("Delay", StringValue ("10ms"));
  internetDevices = p2pHelper.Install (nc_internet);

  // Configure YansWifiChannel
  YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default ();
  YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default ();
  wifiPhy.SetChannel (wifiChannel.Create ());
  /*
   * Create mesh helper and set stack installer to it
   * Stack installer creates all needed protocols and install them to
   * meshHelper point device
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
  // Set number of interfaces - default is single-interface meshHelper point
  meshHelper.SetNumberOfInterfaces (m_nIfaces);
  // Install protocols and return container if MeshPointDevices
  meshDevices = meshHelper.Install (wifiPhy, nc_mesh);
  // Setup mobility - static grid topology
  MobilityHelper mobilityHelper;
  mobilityHelper.SetPositionAllocator ("ns3::GridPositionAllocator",
                                       "MinX", DoubleValue (0.0),
                                       "MinY", DoubleValue (0.0),
                                       "DeltaX", DoubleValue (m_step),
                                       "DeltaY", DoubleValue (m_step),
                                       "GridWidth", UintegerValue (m_xSize),
                                       "LayoutType", StringValue ("RowFirst"));
  mobilityHelper.SetMobilityModel ("ns3::RandomWalk2dMobilityModel", "Bounds", RectangleValue(Rectangle(-1000, 1000, -1000, 1000)));
  mobilityHelper.Install (nc_mesh);
  if (m_pcap)
    wifiPhy.EnablePcapAll (std::string ("mp-"));

  /*AnimationInterface animation("iMesh.xml");

  animation.SetConstantPosition(nc_internet.Get(0), -200.0, 100.0);
  animation.SetConstantPosition(nc_internet.Get(1), -100.0, 100.0);*/
}

void
MeshTest::InstallInternetStack ()
{

  InternetStackHelper internetStackHelper;

  OlsrHelper routingProtocol;
  internetStackHelper.SetRoutingHelper (routingProtocol);

  internetStackHelper.Install (nc_mesh);
  internetStackHelper.Install (nc_all.Get (0)); /// Setting up protocol stack on node 0

  Ipv4AddressHelper meshAddress;
  meshAddress.SetBase ("10.1.1.0", "255.255.255.0");
  meshInterfaces = meshAddress.Assign (meshDevices);

  Ipv4AddressHelper p2pAddress;
  p2pAddress.SetBase ("1.1.1.0", "255.255.255.0");
  p2pInterfaces = p2pAddress.Assign (internetDevices);

}

void
MeshTest::InstallApplication ()
{

  UdpEchoServerHelper echoServer (9);
  ApplicationContainer serverApps = echoServer.Install (nc_all.Get (0));
  serverApps.Start (Seconds (0.0));
  serverApps.Stop (Seconds (m_totalTime));

  UdpEchoClientHelper echoClient (p2pInterfaces.GetAddress (0), 9);
  //echoClient.SetAttribute ("MaxPackets", UintegerValue ((uint32_t) (m_totalTime * (1 / m_packetInterval))));
  echoClient.SetAttribute ("MaxPackets", UintegerValue (20));
  echoClient.SetAttribute ("Interval", TimeValue (Seconds (m_packetInterval)));
  echoClient.SetAttribute ("PacketSize", UintegerValue (m_packetSize));

  ApplicationContainer clientApps1 = echoClient.Install (nc_mesh.Get (m_xSize * m_ySize - 1));
  //ApplicationContainer clientApps2 = echoClient.Install (nc_mesh.Get (m_xSize * m_ySize - 2));



  clientApps1.Start (Seconds (0.0));
  clientApps1.Stop (Seconds (m_totalTime));

  //  clientApps2.Start (Seconds (12));
  //  clientApps2.Stop (Seconds (m_totalTime));

  /*for (tmp_x = 2; tmp_x < m_xSize * m_ySize + 2; tmp_x++)
  {
    sprintf (tmp_char, "%d-STA", tmp_x);
    AnimationInterface::SetNodeDescription (nc_all.Get (tmp_x), tmp_char);
  }
  AnimationInterface::SetNodeDescription (nc_all.Get (0), "0-Internet");
  AnimationInterface::SetNodeDescription (nc_all.Get (1), "1-AP");
  AnimationInterface::SetNodeColor (nc_mesh, 0, 255, 0);
  AnimationInterface::SetNodeColor (nc_internet.Get (0), 255, 0, 0);
  AnimationInterface::SetNodeColor (nc_internet.Get (1), 0, 0, 255);

   */



}

int
MeshTest::Run ()
{
  CreateNodes ();
  InstallInternetStack ();
  InstallApplication ();


  AnimationInterface animation ("iMesh.xml");

  animation.SetConstantPosition (nc_all.Get (0), 0.0, 0.0);
  animation.SetConstantPosition (nc_internet.Get (1), 37.0, 0.0);
  animation.SetConstantPosition (nc_mesh.Get (0), 75.0, 75.0);
  animation.SetConstantPosition (nc_mesh.Get (1), 150.0, 75.0);
  animation.SetConstantPosition (nc_mesh.Get (2), 225.0, 75.0);
  animation.SetConstantPosition (nc_mesh.Get (3), 75.0, 150.0);
  animation.SetConstantPosition (nc_mesh.Get (4), 150.0, 150.0);
  animation.SetConstantPosition (nc_mesh.Get (5), 225.0, 150.0);
  animation.SetConstantPosition (nc_mesh.Get (6), 75.0, 0.0);
  animation.SetConstantPosition (nc_mesh.Get (7), 150.0, 0.0);
  animation.SetConstantPosition (nc_mesh.Get (8), 225.0, 0.0);

  //animation.EnablePacketMetadata(true);
  //anim.EnableIpv4RouteTracking (m_routeFile, Seconds (0), Seconds (m_totalTime), Seconds (0.25));





  Simulator::Schedule (Seconds (m_totalTime), &MeshTest::Report, this);
  Simulator::Stop (Seconds (m_totalTime));
  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}

void
MeshTest::Report () {
  //  unsigned n (0);
  //  for (NetDeviceContainer::Iterator i = meshDevices.Begin (); i != meshDevices.End (); ++i, ++n)
  //    {
  //      std::ostringstream os;
  //      os << "mp-report-" << n << ".xml";
  //      std::cerr << "Printing meshHelper point device #" << n << " diagnostics to " << os.str () << "\n";
  //      std::ofstream of;
  //      of.open (os.str ().c_str ());
  //      if (!of.is_open ())
  //        {
  //          std::cerr << "Error: Can't open file " << os.str () << "\n";
  //          return;
  //        }
  //      meshHelper.Report (*i, of);
  //      of.close ();
  //    }
}

int
main (int argc, char *argv[])
{

  ns3::PacketMetadata::Enable ();
  MeshTest t;
  t.Configure (argc, argv);
  return t.Run ();
}
