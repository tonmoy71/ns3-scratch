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
 */



/*
	LAB Assignment #4
    1. Create a wireless mobile ad-hoc network with three nodes Node0,
    Node1 and Node2. Install the OLSR routing protocol on these nodes.

    2. Place them such that Node0 and Node2 are just out of reach of each other.

    3. Create a UDP client on Node0 and the corresponding server on Node2.

    4. Schedule Node0 to begin sending packets to Node2 at time 1s.

    5. Verify whether Node0 is able to send packets to Node2.

    6. Make Node1 move between Node0 and Node2 such that Node1 is
    visible to both 0 and 2. This should happen at time 20s. Ensure that Node1
    stays in that position for another 15s.

    7. Verify whether Node0 is able to send packets to Node1.

    8. At time 35s, move Node1 out of the region between Node0 and Node2
    such that it is out of each other's transmission ranges again.

    9. Verify whether Node0 is able to send packets to Node2.

    10. To verify whether data transmissions occur in the above scenarios,
    use either the tracing mechanism or a RecvCallback() for Node2's socket.

    11. Plot the number of bytes received versus time at Node2.

    12. Show the pcap traces at Node 1's Wifi interface, and indicate the correlation
    between Node2's packet reception timeline and Node1's mobility.


	Solution by: Konstantinos Katsaros (K.Katsaros@surrey.ac.uk)
	based on fifth.cc and simple-wifi-adhoc.cc
*/


// Network topology
//
//
//        n0 ------ n1 --- n2
//
//
// - All links are wireless IEEE 802.11b with OLSR
// - n0 and n2 are currently out of range
// - UDP flow from n0 to n2
// - n1 moving right and left

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/mobility-module.h"
#include "ns3/wifi-module.h"
#include "ns3/olsr-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/mobility-module.h"
#include "ns3/netanim-module.h"
#include "myapp.h"

NS_LOG_COMPONENT_DEFINE ("Lab4");

using namespace ns3;



static void
SetPosition (Ptr<Node> node, double x)
{
  Ptr<MobilityModel> mobility = node->GetObject<MobilityModel> ();
  Vector pos = mobility->GetPosition();
  pos.x = x;
  mobility->SetPosition(pos);
}

void
ReceivePacket(Ptr<const Packet> p, const Address & addr)
{
	std::cout << Simulator::Now ().GetSeconds () << "\t" << p->GetSize() <<"\n";
}


int main (int argc, char *argv[])
{
  ns3::PacketMetadata::Enable ();
  bool enableFlowMonitor = false;
  std::string phyMode ("DsssRate1Mbps");

  CommandLine cmd;
  cmd.AddValue ("EnableMonitor", "Enable Flow Monitor", enableFlowMonitor);
  cmd.AddValue ("phyMode", "Wifi Phy mode", phyMode);
  cmd.Parse (argc, argv);

//
// Explicitly create the nodes required by the topology (shown above).
//
  NS_LOG_INFO ("Create nodes.");
  NodeContainer c; // ALL Nodes
  c.Create(3);

  // Set up WiFi
  WifiHelper wifi;

  YansWifiPhyHelper wifiPhy =  YansWifiPhyHelper::Default ();
  wifiPhy.SetPcapDataLinkType (YansWifiPhyHelper::DLT_IEEE802_11);

  YansWifiChannelHelper wifiChannel ;
  wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
  wifiChannel.AddPropagationLoss ("ns3::TwoRayGroundPropagationLossModel",
	  	  	  	  	  	  	  	    "SystemLoss", DoubleValue(1),
		  	  	  	  	  	  	    "HeightAboveZ", DoubleValue(1.5));

  // For range near 250m
  wifiPhy.Set ("TxPowerStart", DoubleValue(33));
  wifiPhy.Set ("TxPowerEnd", DoubleValue(33));
  wifiPhy.Set ("TxPowerLevels", UintegerValue(1));
  wifiPhy.Set ("TxGain", DoubleValue(0));
  wifiPhy.Set ("RxGain", DoubleValue(0));
  wifiPhy.Set ("EnergyDetectionThreshold", DoubleValue(-61.8));
  wifiPhy.Set ("CcaMode1Threshold", DoubleValue(-64.8));

  wifiPhy.SetChannel (wifiChannel.Create ());

  // Add a non-QoS upper mac
  NqosWifiMacHelper wifiMac = NqosWifiMacHelper::Default ();
  wifiMac.SetType ("ns3::AdhocWifiMac");

  // Set 802.11b standard
  wifi.SetStandard (WIFI_PHY_STANDARD_80211b);

  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                "DataMode",StringValue(phyMode),
                                "ControlMode",StringValue(phyMode));


  NetDeviceContainer devices;
  devices = wifi.Install (wifiPhy, wifiMac, c);


//  Enable OLSR
  OlsrHelper olsr;

  // Install the routing protocol
  Ipv4ListRoutingHelper list;
  list.Add (olsr, 10);

  // Set up internet stack
  InternetStackHelper internet;
  internet.SetRoutingHelper (list);
  internet.Install (c);

  // Set up Addresses
  Ipv4AddressHelper ipv4;
  NS_LOG_INFO ("Assign IP Addresses.");
  ipv4.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer ifcont = ipv4.Assign (devices);


  NS_LOG_INFO ("Create Applications.");

  // UDP connfection from N0 to N2

  uint16_t sinkPort = 6;
  Address sinkAddress (InetSocketAddress (ifcont.GetAddress (2), sinkPort)); // interface of n2
  PacketSinkHelper packetSinkHelper ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), sinkPort));
  ApplicationContainer sinkApps = packetSinkHelper.Install (c.Get (2)); //n2 as sink
  sinkApps.Start (Seconds (0.));
  sinkApps.Stop (Seconds (100.));

  Ptr<Socket> ns3UdpSocket = Socket::CreateSocket (c.Get (0), UdpSocketFactory::GetTypeId ()); //source at n0

  // Create UDP application at n0
  Ptr<MyApp> app = CreateObject<MyApp> ();
  app->Setup (ns3UdpSocket, sinkAddress, 1040, 100000, DataRate ("250Kbps"));
  c.Get (0)->AddApplication (app);
  app->SetStartTime (Seconds (1.));
  app->SetStopTime (Seconds (100.));

// Set Mobility for all nodes

  MobilityHelper mobility;
  Ptr<ListPositionAllocator> positionAlloc = CreateObject <ListPositionAllocator>();
  positionAlloc ->Add(Vector(0, 0, 0)); // node0
  positionAlloc ->Add(Vector(1000, 0, 0)); // node1 -- starting very far away
  positionAlloc ->Add(Vector(450, 0, 0)); // node2
  mobility.SetPositionAllocator(positionAlloc);
  mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  mobility.Install(c);

  // node 1 comes in the communication range of both
  Simulator::Schedule (Seconds (20.0), &SetPosition, c.Get (1), 200.0);

  // node 1 goes out of the communication range of both
  Simulator::Schedule (Seconds (35.0), &SetPosition, c.Get (1), 1000.0);

  // Trace Received Packets
  Config::ConnectWithoutContext("/NodeList/*/ApplicationList/*/$ns3::PacketSink/Rx", MakeCallback (&ReceivePacket));

// Trace devices (pcap)
  wifiPhy.EnablePcap ("lab-4-dev", devices);
  
  AnimationInterface animation ("lab-4-solved.xml");
  animation.EnablePacketMetadata (true);


  // Flow Monitor
  Ptr<FlowMonitor> flowmon;
  if (enableFlowMonitor)
    {
      FlowMonitorHelper flowmonHelper;
      flowmon = flowmonHelper.InstallAll ();
    }

//
// Now, do the actual simulation.
//
  NS_LOG_INFO ("Run Simulation.");
  Simulator::Stop (Seconds(100.0));
  Simulator::Run ();
  if (enableFlowMonitor)
    {
	  flowmon->CheckForLostPackets ();
	  flowmon->SerializeToXmlFile("lab-4.flowmon", true, true);
    }
  
    
  Simulator::Destroy ();
  NS_LOG_INFO ("Done.");
}
