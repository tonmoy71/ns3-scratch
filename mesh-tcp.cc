
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/config-store-module.h"
#include "ns3/applications-module.h"
#include "ns3/mobility-module.h"
#include "ns3/wifi-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/mesh-module.h"
#include "ns3/mesh-helper.h"
#include "ns3/propagation-loss-model.h"
#include "ns3/csma-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/netanim-module.h"

#include "mesh-tcp.h"

#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <sstream>

using namespace ns3;

//-----------------------------------FUNCTIONS FOR CREATING SOCKETS

void createTcpSocket(NodeContainer c, Ipv4InterfaceContainer ifcont, int sink, int source, int sinkPort, double startTime, double stopTime, uint32_t packetSize, uint32_t numPackets, std::string dataRate)
{
	
	Address sinkAddress1 (InetSocketAddress (ifcont.GetAddress (sink), sinkPort));
	PacketSinkHelper packetSinkHelper1 ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), sinkPort));
	ApplicationContainer sinkApps1 = packetSinkHelper1.Install (c.Get (sink));
	sinkApps1.Start (Seconds (0.));
	sinkApps1.Stop (Seconds (stopTime));
	
	Ptr<Socket> ns3TcpSocket1 = Socket::CreateSocket (c.Get (source), TcpSocketFactory::GetTypeId ());
	Ptr<MyApp> app1 = CreateObject<MyApp> ();
	app1->Setup (ns3TcpSocket1, sinkAddress1, packetSize, numPackets, DataRate (dataRate));
	c.Get (source)->AddApplication (app1);
	app1->SetStartTime (Seconds (startTime));
	app1->SetStopTime (Seconds (stopTime));
	
}

void createUdpSocket(NodeContainer c, Ipv4InterfaceContainer ifcont, int sink, int source, int sinkPort, double startTime, double stopTime, uint32_t packetSize, uint32_t numPackets, std::string dataRate)
{
	
	Address sinkAddress1 (InetSocketAddress (ifcont.GetAddress (sink), sinkPort));
	PacketSinkHelper packetSinkHelper1 ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), sinkPort));
	ApplicationContainer sinkApps1 = packetSinkHelper1.Install (c.Get (sink));
	sinkApps1.Start (Seconds (0.));
	sinkApps1.Stop (Seconds (stopTime));
	
	Ptr<Socket> ns3UdpSocket1 = Socket::CreateSocket (c.Get (source), UdpSocketFactory::GetTypeId ());
	Ptr<MyApp> app1 = CreateObject<MyApp> ();
	app1->Setup (ns3UdpSocket1, sinkAddress1, packetSize, numPackets, DataRate (dataRate));
	c.Get (source)->AddApplication (app1);
	app1->SetStartTime (Seconds (startTime));
	app1->SetStopTime (Seconds (stopTime));
	
}

//-----------------------------------FUNCTION FOR SETTING WIFI PHY PARAMETERS

void setPhy(YansWifiPhyHelper &wifiPhy)
{
	wifiPhy.Set ("TxPowerStart", DoubleValue(1000.0));
	wifiPhy.Set ("TxPowerEnd", DoubleValue(1000.0));
	wifiPhy.Set ("TxPowerLevels", UintegerValue(1000));
	wifiPhy.Set ("TxGain", DoubleValue(1000.0));
	wifiPhy.Set ("RxGain", DoubleValue(1000.0));
	wifiPhy.Set ("EnergyDetectionThreshold", DoubleValue(-60));
	wifiPhy.Set ("CcaMode1Threshold", DoubleValue(-65));
}
	
int main (int argc, char *argv[])
{
         //LogComponentEnable ("YansWifiPhy", LOG_LEVEL_ALL);
	uint32_t packetSize = 1024;
        
        ns3::PacketMetadata::Enable ();
	
	//-----------------------------------CREATE NODES
	
	NodeContainer genMesh;
	NetDeviceContainer genMeshDevice;

	genMesh.Create(6);
	
	//-----------------------------------SETUP WIFI PHY AND CHANNEL
	
	YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default ();
	wifiPhy.SetPcapDataLinkType (YansWifiPhyHelper::DLT_IEEE802_11);
	setPhy(wifiPhy);
	
	YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default ();
	wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
	wifiChannel.AddPropagationLoss ("ns3::FixedRssLossModel");
        //wifiChannel.AddPropagationLoss ("ns3::FriisPropagationLossModel");
	
	wifiPhy.SetChannel (wifiChannel.Create ());
	
	//-----------------------------------SET PEER LINK OPTIONS
	
	Config::SetDefault ("ns3::dot11s::PeerLink::MaxBeaconLoss", 
						UintegerValue (2)); 
	Config::SetDefault ("ns3::dot11s::PeerLink::MaxRetries", UintegerValue 
						(4)); 
	Config::SetDefault ("ns3::dot11s::PeerLink::MaxPacketFailure", 
						UintegerValue (1)); 
	
	//-----------------------------------SET PEER MGMT PROTOCOL OPTIONS
	
	Config::SetDefault 
	("ns3::dot11s::PeerManagementProtocol::EnableBeaconCollisionAvoidance",BooleanValue 
	 (true));
	
	//-----------------------------------SET HWMP OPTIONS
	
	Config::SetDefault 
	("ns3::dot11s::HwmpProtocol::Dot11MeshHWMPactivePathTimeout",TimeValue 
	 (Seconds (100))); 
	Config::SetDefault 
	("ns3::dot11s::HwmpProtocol::Dot11MeshHWMPactiveRootTimeout",TimeValue 
	 (Seconds (100))); 
	Config::SetDefault 
	("ns3::dot11s::HwmpProtocol::Dot11MeshHWMPmaxPREQretries",UintegerValue 
	 (5)); 
	Config::SetDefault 
	("ns3::dot11s::HwmpProtocol::UnicastPreqThreshold",UintegerValue 
	 (10)); 
	Config::SetDefault 
	("ns3::dot11s::HwmpProtocol::UnicastDataThreshold",UintegerValue (5)); 
	Config::SetDefault ("ns3::dot11s::HwmpProtocol::DoFlag", BooleanValue 
						(true)); 
	Config::SetDefault ("ns3::dot11s::HwmpProtocol::RfFlag", BooleanValue 
						(false)); 
	
	//-----------------------------------SETUP MESH HELPER
	
	std::string m_stack = "ns3::Dot11sStack";
	std::string m_root = "ff:ff:ff:ff:ff:ff";
	MeshHelper mesh;
	mesh = MeshHelper::Default ();
	if (!Mac48Address (m_root.c_str ()).IsBroadcast ())
    {
		mesh.SetStackInstaller (m_stack, "Root", Mac48AddressValue (Mac48Address (m_root.c_str ())));
    }
	else
    {
		mesh.SetStackInstaller (m_stack);
    }
	mesh.SetSpreadInterfaceChannels (MeshHelper::SPREAD_CHANNELS);
	mesh.SetNumberOfInterfaces (1);
	mesh.SetMacType ("RandomStart", TimeValue (Seconds(0.1))); 
	//mesh.SetRemoteStationManager ("ns3::ConstantRateWifiManager", 
	//							  "DataMode", StringValue ("ErpOfdmRate6Mbps"), "RtsCtsThreshold", UintegerValue 
	//							  (2500)); 
	
	NetDeviceContainer meshDevice;
	genMeshDevice = mesh.Install (wifiPhy, genMesh);
	
	//-----------------------------------MOBILITY HELPER
	
	MobilityHelper mobility;
	mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
								   "MinX", DoubleValue (0.0),
								   "MinY", DoubleValue (0.0),
								   "DeltaX", DoubleValue (50.0),
								   "DeltaY", DoubleValue (50.0),
								   "GridWidth", UintegerValue (3),
								   "LayoutType", StringValue ("RowFirst"));
	mobility.Install (genMesh);
	
	//-----------------------------------INSTALL INTERNET STACK AND ADDRESSES
	
        AnimationInterface animation ("mesh-tcp.xml");
        
        animation.EnablePacketMetadata(false);
        
	InternetStackHelper stack;
	stack.Install (genMesh);

	
	Ipv4AddressHelper address;
	
	address.SetBase ("10.1.1.0", "255.255.255.0");
	Ipv4InterfaceContainer genInterfaces;
	genInterfaces = address.Assign (genMeshDevice);
	
	//-----------------------------------CREATE A TCP SOCKET
	
	createTcpSocket(genMesh, genInterfaces, 1, 0, 8080, 1.0, 100.0, packetSize, 1000, "250Kbps");
	
	//-----------------------------------INSTALL FLOWMONITOR
	
	FlowMonitorHelper flowmon;
	Ptr<FlowMonitor> monitor = flowmon.InstallAll();
	
	//-----------------------------------SETUP SIMULATION
	
	Simulator::Stop (Seconds (100.0));
	Simulator::Run ();
	
	monitor->CheckForLostPackets ();
	Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmon.GetClassifier ());
	std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats ();
	
	for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator iter = stats.begin (); iter != stats.end (); ++iter)
    {
		Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (iter->first);
		
		NS_LOG_UNCOND("Flow ID: " << iter->first << " Src Addr " << t.sourceAddress << " Dst Addr " << t.destinationAddress);
		NS_LOG_UNCOND("Tx Packets = " << iter->second.txPackets);
		NS_LOG_UNCOND("Rx Packets = " << iter->second.rxPackets);
		NS_LOG_UNCOND("Throughput: " << iter->second.rxBytes * 8.0 / (iter->second.timeLastRxPacket.GetSeconds()-iter->second.timeFirstTxPacket.GetSeconds()) / packetSize  << " Kbps");
    }
	
	Simulator::Destroy ();
	return 0;
	
}

	
	