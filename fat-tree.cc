/*
// Network topology
//                    
//           _l9__   _l10__
//          /     \ /      \
//         /       /\       \
//        /       /  \       \
//       /       /    \       \
//       l5     l6     l7      l8
//       |\     /|     |\     /|
//       | \   / |     | \   / |
//       |  \ /  |     |  \ /  |
//       |  /\   |     |  /\   |
//       |_/  \__|     |_/  \__|
//       l1    l2      l3    l4
//       |      |       |     |
//       c1     c2      c3    c4
//*/       
// - all links are point-to-point links with indicated one-way BW/delay
// - CBR/UDP flows from n0 to n3, and n3 to n1
// - FTP/TCP flow from n0 to n3, starting at time 1.2 to 1.35 sec.
// - UDP packet size of 210 bytes, with per-packet interval 0.00375 sec.
//   (i.e., DataRate of 448,000 bps)
// - DropTail queues
// - Tracing of queues and packet receptions to file "simple-global-routing.tr"
//
#include <iostream>
#include <fstream>
#include <string>
#include <cassert>
#include "ns3/olsr-helper.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/csma-module.h"

using namespace ns3;
using namespace std;
void printRoutingTable (Ptr<Node> node)
{
    Ipv4StaticRoutingHelper helper;
    Ptr<Ipv4> stack = node -> GetObject<Ipv4>();
    Ptr<Ipv4StaticRouting> staticrouting = helper.GetStaticRouting(stack);
    uint32_t numroutes=staticrouting->GetNRoutes();
    Ipv4RoutingTableEntry entry;
    std::cout << "Routing table for device: " << Names::FindName(node) <<"\n";
    std::cout << "Destination\tMask\t\tGateway\t\tIface\n";
    for (uint32_t i =0 ; i<numroutes;i++) {
        entry =staticrouting->GetRoute(i);
        std::cout << entry.GetDestNetwork()  << "\t" << entry.GetDestNetworkMask() << "\t" << entry.GetGateway() << "\t\t" << entry.GetInterface() << "\n";
     }
    return;
}


NS_LOG_COMPONENT_DEFINE ("SimpleGlobalRoutingExample");

int 
main(int argc, char *argv[])
{
  // Users may find it convenient to turn on explicit debugging
  // for selected modules; the below line suggets how to do this
#if 0
  LogComponentEnable ("SimpleGlobalRoutingExample", LOG_LEVEL_INFO);
#endif
  // Set up some default values for the simulation. Use the
  Config::SetDefault ("ns3::OnOffApplication::PacketSize", UintegerValue(210));
  Config::SetDefault ("ns3::OnOffApplication::DataRate", StringValue ("448kb/s"));

  // DefaultValue::Bind ("DropTailQueue::m_maxPackets", 30);
  // Allow the user to override any of the default and the above
  // DafaultValue::Bind ()s at run-time, via command-line arguments
  CommandLine cmd;
 // OlsrHelper olsr;
  Ipv4StaticRoutingHelper staticRouting;
  Ipv4ListRoutingHelper listRouting;

  Ipv4ListRoutingHelper list;
  //list.Add (listRouting, 0);
  //list.Add (olsr, 10);

  bool enableFlowMonitor = false;
  cmd.AddValue("EnableMonitor", "Enable Flow Monitor", enableFlowMonitor);
  cmd.Parse(argc, argv);

  NS_LOG_INFO ("Create Nodes");
  NodeContainer c1,c2,c3,c4;
  NodeContainer l1,l2,l3,l4,l5,l6,l7,l8,l9,l10;
  c1.Create(1);
  c2.Create(1);
  c3.Create(1);
  c4.Create(1);
  l1.Create(1);
  l2.Create(1);
  l3.Create(1);
  l4.Create(1);
  l5.Create(1);
  l6.Create(1);
  l7.Create(1);
  l8.Create(1);
  l9.Create(1);
  l10.Create(1);



  NodeContainer n1 = NodeContainer (c1,l1);
  NodeContainer n2 = NodeContainer (c2,l2);
  NodeContainer n3 = NodeContainer (c3,l3);
  NodeContainer n4 = NodeContainer (c4,l4);

  NodeContainer n51 = NodeContainer (l5,l1);
  NodeContainer n52 = NodeContainer (l5,l2);

  NodeContainer n61 = NodeContainer (l6,l1);
  NodeContainer n62 = NodeContainer (l6,l2);

  NodeContainer n71 = NodeContainer (l7,l3);
  NodeContainer n72 = NodeContainer (l7,l4);

  NodeContainer n81 = NodeContainer (l8,l3);
  NodeContainer n82 = NodeContainer (l8,l4);

  NodeContainer n91 = NodeContainer (l9,l5);
  NodeContainer n92 = NodeContainer (l9,l7);

  NodeContainer nx = NodeContainer (l10,l6);
  NodeContainer ny = NodeContainer (l10,l8);

  InternetStackHelper internet;
  //internet.SetRoutingHelper (list);

  internet.Install (c1);
  internet.Install (c2);
  internet.Install (c3);
  internet.Install (c4);
  internet.Install (l1);
  internet.Install (l2);
  internet.Install (l3);
  internet.Install (l4);
  internet.Install (l5);
  internet.Install (l6);
  internet.Install (l7);
  internet.Install (l8);
  internet.Install (l9);
  internet.Install (l10);

  NS_LOG_INFO ("Create Channels");
  CsmaHelper csma;
  csma.SetChannelAttribute ("DataRate",DataRateValue (DataRate ("2Mbps")));
  csma.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (2)));
  csma.SetDeviceAttribute ("Mtu", UintegerValue (1400));
  NetDeviceContainer d1 = csma.Install (n1 );
  NetDeviceContainer d2 = csma.Install (n2 );
  NetDeviceContainer d3 = csma.Install (n3 );
  NetDeviceContainer d4 = csma.Install (n4 );
  
  
  PointToPointHelper p2p;
  p2p.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
  p2p.SetChannelAttribute ("Delay", StringValue ("10ms"));
  

  NetDeviceContainer d51 = p2p.Install(n51);
  NetDeviceContainer d52 = p2p.Install(n52);
  NetDeviceContainer d61 = p2p.Install(n61);
  NetDeviceContainer d62 = p2p.Install(n62);
  NetDeviceContainer d71 = p2p.Install(n71);
  NetDeviceContainer d72 = p2p.Install(n72);
  NetDeviceContainer d81 = p2p.Install(n81);
  NetDeviceContainer d82 = p2p.Install(n82);
  NetDeviceContainer d91 = p2p.Install(n91);
  NetDeviceContainer d92 = p2p.Install(n92);
  NetDeviceContainer dx = p2p.Install(nx);
  NetDeviceContainer dy = p2p.Install(ny);
  cout << "Control after point-to-point" << endl;

  /*p2p.SetDeviceAttribute ("DataRate", StringValue ("15Mbps"));
  p2p.SetChannelAttribute ("Delay", StringValue ("5ms"));
  NetDeviceContainer d3d2 = p2p.Install (n3n2);*/

  // Later we add IP Addresses
  NS_LOG_INFO ("Assign IP Addresses.");
  Ipv4AddressHelper ipv4;
  ipv4.SetBase("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer i1 = ipv4.Assign(d1);

  
  
  ipv4.SetBase("10.1.2.0", "255.255.255.0");
  Ipv4InterfaceContainer i2 = ipv4.Assign(d2);


  ipv4.SetBase("10.1.3.0", "255.255.255.0");
  Ipv4InterfaceContainer i3 = ipv4.Assign(d3);

  ipv4.SetBase("10.1.4.0", "255.255.255.0");
  Ipv4InterfaceContainer i4 = ipv4.Assign(d4);

  ipv4.SetBase("10.1.5.0", "255.255.255.0");
  Ipv4InterfaceContainer i51 = ipv4.Assign(d51);

  ipv4.SetBase("10.1.6.0", "255.255.255.0");
  Ipv4InterfaceContainer i52 = ipv4.Assign(d52);
 
  ipv4.SetBase("10.1.7.0", "255.255.255.0");
  Ipv4InterfaceContainer i61 = ipv4.Assign(d61);

  ipv4.SetBase("10.1.8.0", "255.255.255.0");
  Ipv4InterfaceContainer i62 = ipv4.Assign(d62);

  ipv4.SetBase("10.1.9.0", "255.255.255.0");
  Ipv4InterfaceContainer i71 = ipv4.Assign(d71);

  ipv4.SetBase("10.1.10.0", "255.255.255.0");
  Ipv4InterfaceContainer i72 = ipv4.Assign(d72);

  ipv4.SetBase("10.1.11.0", "255.255.255.0");
  Ipv4InterfaceContainer i81 = ipv4.Assign(d81);

  ipv4.SetBase("10.1.12.0", "255.255.255.0");
  Ipv4InterfaceContainer i82 = ipv4.Assign(d82);

  ipv4.SetBase("10.1.13.0", "255.255.255.0");
  Ipv4InterfaceContainer i91 = ipv4.Assign(d91);

  ipv4.SetBase("10.1.14.0", "255.255.255.0");
  Ipv4InterfaceContainer i92 = ipv4.Assign(d92);

  ipv4.SetBase("10.1.15.0", "255.255.255.0");
  Ipv4InterfaceContainer ix = ipv4.Assign(dx);

  ipv4.SetBase("10.1.16.0", "255.255.255.0");
  Ipv4InterfaceContainer iy = ipv4.Assign(dy);
  cout << " Address assigned " << endl;
 // Create router nodes, initialize routing database and set up the routing
  // tables in the nodes.
  //
 // Ipv4GlobalRouting::RandomEcmpRouting:m_randomEcmpRouting (true); 
//  Ipv4GlobalRouting::RandomEcmpRouting (true);
  Ipv4GlobalRoutingHelper::PopulateRoutingTables();
  cout << "Routing is done " << endl;
 // printRoutingTable(l1.Get(0));
 // printRoutingTable(l2.Get(0));
 // printRoutingTable(l3.Get(0));


  //
  // Create one udpServer applications on node one.
  //
    uint16_t port = 4000;
    UdpServerHelper server (port);
    ApplicationContainer apps = server.Install (c1.Get (0));
    apps.Start (Seconds (1.0));
    apps.Stop (Seconds (10.0));
 
  //
  // Create one UdpClient application to send UDP datagrams from node zero to
  // node one.
  //
    uint32_t MaxPacketSize = 1024;
    Time interPacketInterval = Seconds (0.05);
    uint32_t maxPacketCount = 320;
    UdpClientHelper client (i1.GetAddress (0), port);
    client.SetAttribute ("MaxPackets", UintegerValue (maxPacketCount));
    client.SetAttribute ("Interval", TimeValue (interPacketInterval));
    client.SetAttribute ("PacketSize", UintegerValue (MaxPacketSize));
    apps = client.Install (NodeContainer (c4.Get (0)));
    apps.Start (Seconds (2.0));
    apps.Stop (Seconds (10.0));
  
    csma.EnablePcap ("server", d1, false);
    csma.EnablePcap ("client", d4, false);
  
  //
  // Now, do the actual simulation.
  //
     NS_LOG_INFO ("Run Simulation.");
     Simulator::Stop (Seconds (20.0));
     Simulator::Run ();
     Simulator::Destroy ();
     NS_LOG_INFO ("Done.");
  //

}
