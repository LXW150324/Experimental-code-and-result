/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/mobility-module.h"
#include "ns3/netanim-module.h"
#include "ns3/udp-client-server-helper.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("SimpleDelayTolerantExample");

int 
main (int argc, char *argv[])
{
  // 基本参数
  uint32_t nNodes = 5;
  Time simTime = Seconds(100);

  CommandLine cmd (__FILE__);
  cmd.AddValue ("nNodes", "Number of nodes", nNodes);
  cmd.AddValue ("simTime", "Simulation time", simTime);
  cmd.Parse (argc, argv);

  // 创建节点
  NodeContainer nodes;
  nodes.Create (nNodes);

  // 设置移动性
  MobilityHelper mobility;
  mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                                "MinX", DoubleValue (0.0),
                                "MinY", DoubleValue (0.0),
                                "DeltaX", DoubleValue (100.0),
                                "DeltaY", DoubleValue (100.0),
                                "GridWidth", UintegerValue (3),
                                "LayoutType", StringValue ("RowFirst"));
  
  mobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
                            "Bounds", RectangleValue (Rectangle (-500, 500, -500, 500)),
                            "Speed", StringValue ("ns3::ConstantRandomVariable[Constant=5.0]"));
  mobility.Install (nodes);

  // 创建链接
  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("10Mbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms"));

  NetDeviceContainer devices;
  for (uint32_t i = 0; i < nNodes - 1; i++)
    {
      devices.Add (pointToPoint.Install (nodes.Get (i), nodes.Get (i + 1)));
    }

  // 安装Internet栈
  InternetStackHelper internet;
  internet.Install (nodes);

  // 分配IP地址
  Ipv4AddressHelper address;
  address.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer interfaces = address.Assign (devices);

  // 在节点上安装应用
  UdpServerHelper server(9);
  ApplicationContainer serverApps = server.Install(nodes.Get(nNodes-1));
  serverApps.Start(Seconds(1.0));
  serverApps.Stop(simTime);

  UdpClientHelper client(interfaces.GetAddress(nNodes-1), 9);
  client.SetAttribute("MaxPackets", UintegerValue(100));
  client.SetAttribute("Interval", TimeValue(Seconds(1.0)));
  client.SetAttribute("PacketSize", UintegerValue(1024));

  ApplicationContainer clientApps = client.Install(nodes.Get(0));
  clientApps.Start(Seconds(2.0));
  clientApps.Stop(simTime - Seconds(1.0));

  // 可视化
  AnimationInterface anim("simple-dtn-animation.xml");
  for (uint32_t i = 0; i < nNodes; i++)
    {
      anim.UpdateNodeDescription(nodes.Get(i), "Node " + std::to_string(i));
      anim.UpdateNodeColor(nodes.Get(i), 0, 0, 255); // Blue
    }
  
  anim.UpdateNodeColor(nodes.Get(0), 255, 0, 0); // 源节点为红色
  anim.UpdateNodeColor(nodes.Get(nNodes-1), 0, 255, 0); // 目标节点为绿色

  // 运行仿真
  Simulator::Stop (simTime);
  Simulator::Run ();
  Simulator::Destroy ();
  
  return 0;
}