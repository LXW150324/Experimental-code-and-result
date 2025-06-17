#include "dtn7-helper.h"
#include "ns3/log.h"
#include "ns3/string.h"
#include "ns3/ipv4.h"
#include "ns3/names.h"
#include "../model/tcp-convergence-layer.h" 

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("Dtn7Helper");

namespace dtn7 {

Dtn7Helper::Dtn7Helper ()
{
  NS_LOG_FUNCTION (this);
  
  m_nodeFactory.SetTypeId ("ns3::dtn7::DtnNode");
  m_storeFactory.SetTypeId ("ns3::dtn7::MemoryBundleStore");
  m_routingFactory.SetTypeId ("ns3::dtn7::EpidemicRouting");
  m_claFactory.SetTypeId ("ns3::dtn7::TcpConvergenceLayer");
}

void 
Dtn7Helper::SetRoutingAlgorithm (std::string routingType,
                                std::string n0, const AttributeValue &v0,
                                std::string n1, const AttributeValue &v1,
                                std::string n2, const AttributeValue &v2,
                                std::string n3, const AttributeValue &v3,
                                std::string n4, const AttributeValue &v4,
                                std::string n5, const AttributeValue &v5,
                                std::string n6, const AttributeValue &v6,
                                std::string n7, const AttributeValue &v7)
{
  NS_LOG_FUNCTION (this << routingType);
  
  m_routingFactory.SetTypeId (routingType);
  
  if (n0 != "")
    {
      m_routingFactory.Set (n0, v0);
    }
  if (n1 != "")
    {
      m_routingFactory.Set (n1, v1);
    }
  if (n2 != "")
    {
      m_routingFactory.Set (n2, v2);
    }
  if (n3 != "")
    {
      m_routingFactory.Set (n3, v3);
    }
  if (n4 != "")
    {
      m_routingFactory.Set (n4, v4);
    }
  if (n5 != "")
    {
      m_routingFactory.Set (n5, v5);
    }
  if (n6 != "")
    {
      m_routingFactory.Set (n6, v6);
    }
  if (n7 != "")
    {
      m_routingFactory.Set (n7, v7);
    }
}

void 
Dtn7Helper::SetBundleStore (std::string storeType,
                           std::string n0, const AttributeValue &v0,
                           std::string n1, const AttributeValue &v1,
                           std::string n2, const AttributeValue &v2,
                           std::string n3, const AttributeValue &v3,
                           std::string n4, const AttributeValue &v4,
                           std::string n5, const AttributeValue &v5,
                           std::string n6, const AttributeValue &v6,
                           std::string n7, const AttributeValue &v7)
{
  NS_LOG_FUNCTION (this << storeType);
  
  m_storeFactory.SetTypeId (storeType);
  
  if (n0 != "")
    {
      m_storeFactory.Set (n0, v0);
    }
  if (n1 != "")
    {
      m_storeFactory.Set (n1, v1);
    }
  if (n2 != "")
    {
      m_storeFactory.Set (n2, v2);
    }
  if (n3 != "")
    {
      m_storeFactory.Set (n3, v3);
    }
  if (n4 != "")
    {
      m_storeFactory.Set (n4, v4);
    }
  if (n5 != "")
    {
      m_storeFactory.Set (n5, v5);
    }
  if (n6 != "")
    {
      m_storeFactory.Set (n6, v6);
    }
  if (n7 != "")
    {
      m_storeFactory.Set (n7, v7);
    }
}

void 
Dtn7Helper::SetConvergenceLayer (std::string claType,
                                std::string n0, const AttributeValue &v0,
                                std::string n1, const AttributeValue &v1,
                                std::string n2, const AttributeValue &v2,
                                std::string n3, const AttributeValue &v3,
                                std::string n4, const AttributeValue &v4,
                                std::string n5, const AttributeValue &v5,
                                std::string n6, const AttributeValue &v6,
                                std::string n7, const AttributeValue &v7)
{
  NS_LOG_FUNCTION (this << claType);
  
  m_claFactory.SetTypeId (claType);
  
  if (n0 != "")
    {
      m_claFactory.Set (n0, v0);
    }
  if (n1 != "")
    {
      m_claFactory.Set (n1, v1);
    }
  if (n2 != "")
    {
      m_claFactory.Set (n2, v2);
    }
  if (n3 != "")
    {
      m_claFactory.Set (n3, v3);
    }
  if (n4 != "")
    {
      m_claFactory.Set (n4, v4);
    }
  if (n5 != "")
    {
      m_claFactory.Set (n5, v5);
    }
  if (n6 != "")
    {
      m_claFactory.Set (n6, v6);
    }
  if (n7 != "")
    {
      m_claFactory.Set (n7, v7);
    }
}

ApplicationContainer 
Dtn7Helper::Install (NodeContainer c)
{
  NS_LOG_FUNCTION (this);
  
  ApplicationContainer apps;
  
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      apps.Add (InstallPriv (*i));
    }
  
  return apps;
}

ApplicationContainer 
Dtn7Helper::Install (Ptr<Node> node)
{
  NS_LOG_FUNCTION (this << node);
  
  ApplicationContainer apps;
  apps.Add (InstallPriv (node));
  return apps;
}

ApplicationContainer 
Dtn7Helper::Install (std::string nodeName)
{
  NS_LOG_FUNCTION (this << nodeName);
  
  Ptr<Node> node = Names::Find<Node> (nodeName);
  return Install (node);
}

Ptr<Application> 
Dtn7Helper::InstallPriv (Ptr<Node> node)
{
  NS_LOG_FUNCTION (this << node);
  
  if (!node) {
    NS_LOG_ERROR("Cannot install on null node");
    return nullptr;
  }
  
  // 创建DTN节点
  Ptr<DtnNode> app = m_nodeFactory.Create<DtnNode> ();
  if (!app) {
    NS_LOG_ERROR("Failed to create DtnNode");
    return nullptr;
  }
  
  // 基于节点ID或IP地址设置节点ID
  std::stringstream ss;
  ss << "dtn://node-" << node->GetId () << "/";
  app->SetNodeId (NodeID (ss.str ()));
  
  // 创建bundle存储
  Ptr<BundleStore> store = m_storeFactory.Create<BundleStore> ();
  if (!store) {
    NS_LOG_ERROR("Failed to create BundleStore");
    return app;  // 返回app但没有存储配置
  }
  app->SetBundleStore (store);
  
  // 创建路由算法
  Ptr<RoutingAlgorithm> routing = m_routingFactory.Create<RoutingAlgorithm> ();
  if (!routing) {
    NS_LOG_ERROR("Failed to create RoutingAlgorithm");
    return app;  // 返回app但没有路由配置
  }
  app->SetRoutingAlgorithm (routing);
  
  // 使用正确方法创建收敛层
  Ptr<Object> claObj = m_claFactory.Create<Object> ();
  Ptr<ConvergenceLayer> cla = DynamicCast<ConvergenceLayer> (claObj);
  
  if (!cla)
    {
      NS_LOG_ERROR ("Failed to create convergence layer");
      return app;  // 返回app但没有CLA配置
    }
  
  // 使用节点的IP地址配置TCP收敛层
  Ptr<TcpConvergenceLayer> tcpCla = DynamicCast<TcpConvergenceLayer> (cla);
  if (tcpCla)
    {
      // 为TcpConvergenceLayer设置节点指针
      tcpCla->SetNode(node); // 确保TcpConvergenceLayer有一个公开的m_node成员

      // 获取第一个IPv4地址
      Ptr<Ipv4> ipv4 = node->GetObject<Ipv4> ();
      if (ipv4 && ipv4->GetNInterfaces () > 0)
        {
          // 使用第一个非回环接口
          for (uint32_t i = 0; i < ipv4->GetNInterfaces (); ++i)
            {
              if (ipv4->GetAddress (i, 0).GetLocal () != Ipv4Address::GetLoopback ())
                {
                  Ipv4Address addr = ipv4->GetAddress (i, 0).GetLocal ();
                  tcpCla->SetAttribute ("LocalAddress", Ipv4AddressValue (addr));
                  break;
                }
            }
        }
    }
  
  app->AddConvergenceLayer (cla);
  
  // 在节点上安装应用
  node->AddApplication (app);
  
  return app;
}

} // namespace dtn7

} // namespace ns3