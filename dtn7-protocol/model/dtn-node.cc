#include "dtn-node.h"
#include "ns3/log.h"
#include "ns3/address.h"
#include "ns3/simulator.h"
#include "ns3/string.h"        // StringValue
#include "ns3/nstime.h"        // TimeValue, Minutes, Seconds
#include "ns3/object.h"
#include "ns3/ptr.h"
#include "ns3/trace-source-accessor.h" // MakeTraceSourceAccessor
#include "ns3/config.h"        // 添加其他可能需要的头文件
#include "ns3/object-base.h"   // 添加其他可能需要的头文件
#include "ns3/core-module.h"   // 添加核心模块包含更多辅助函数
#include <sstream>
#include <functional> // 添加这行来支持哈希函数
namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("DtnNode");

namespace dtn7 {

NS_OBJECT_ENSURE_REGISTERED (DtnNode);

TypeId 
DtnNode::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::dtn7::DtnNode")
    .SetParent<Application> ()
    .SetGroupName ("Dtn7")
    .AddConstructor<DtnNode> ()
    // 简化属性定义，仅保留必要部分
    .AddTraceSource ("BundleReceived",
                     "Trace source for received bundles",
                     MakeTraceSourceAccessor (&DtnNode::m_bundleReceivedTrace),
                     "ns3::dtn7::DtnNode::BundleTracedCallback")
    .AddTraceSource ("BundleDelivered",
                     "Trace source for delivered bundles",
                     MakeTraceSourceAccessor (&DtnNode::m_bundleDeliveredTrace),
                     "ns3::dtn7::DtnNode::BundleTracedCallback")
  ;
  return tid;
}

DtnNode::DtnNode ()
  : 
    m_nodeId ("dtn://local/"),
    m_fragmentManager (Create<FragmentationManager> ()),
    m_running (false),
    m_cleanupInterval (Minutes (1)),
    m_routingInterval (Seconds (10)),
    m_receivedBundles (0),
    m_deliveredBundles (0)
{
  NS_LOG_FUNCTION (this);
}

DtnNode::~DtnNode ()
{
  NS_LOG_FUNCTION (this);
  StopApplication ();
}

void 
DtnNode::SetNodeId (const NodeID& id)
{
  NS_LOG_FUNCTION (this << id.ToString ());
  m_nodeId = id;
}

NodeID 
DtnNode::GetNodeId () const
{
  NS_LOG_FUNCTION (this);
  return m_nodeId;
}

void 
DtnNode::AddConvergenceLayer (Ptr<ConvergenceLayer> cla)
{
  NS_LOG_FUNCTION (this << cla);
  if (cla) {  // 添加非空检查
    m_convergenceLayers.push_back (cla);
  } else {
    NS_LOG_WARN("尝试添加空的ConvergenceLayer");
  }
}

void 
DtnNode::SetRoutingAlgorithm (Ptr<RoutingAlgorithm> algorithm)
{
  NS_LOG_FUNCTION (this << algorithm);
  m_routingAlgorithm = algorithm;
}

void 
DtnNode::SetBundleStore (Ptr<BundleStore> store)
{
  NS_LOG_FUNCTION (this << store);
  m_store = store;
}

bool 
DtnNode::Send (Ptr<Bundle> bundle)
{
  NS_LOG_FUNCTION (this << bundle);
  
  if (!m_running)
    {
      NS_LOG_ERROR ("Node not running");
      return false;
    }
  
  if (!bundle)
    {
      NS_LOG_ERROR ("Invalid bundle");
      return false;
    }
  
  if (!m_routingAlgorithm) {
    NS_LOG_ERROR ("Routing algorithm not set");
    return false;
  }
  
  // 通知路由算法有关新bundle
  m_routingAlgorithm->NotifyNewBundle (bundle, m_nodeId);
  
  NS_LOG_INFO ("Added bundle to store: " << bundle->ToString ());
  
  return true;
}

bool 
DtnNode::FragmentIfNeeded (Ptr<Bundle> bundle, size_t maxFragmentSize)
{
  NS_LOG_FUNCTION (this << bundle << maxFragmentSize);
  
  if (!m_running)
    {
      NS_LOG_ERROR ("Node not running");
      return false;
    }
  
  if (!bundle)
    {
      NS_LOG_ERROR ("Invalid bundle");
      return false;
    }
  
  if (!m_fragmentManager) {
    NS_LOG_ERROR ("Fragment manager not initialized");
    return false;
  }
  
  // 检查是否需要分片
  Buffer bundleBuffer = bundle->ToCbor();
  if (bundleBuffer.GetSize() <= maxFragmentSize)
    {
      NS_LOG_INFO ("Bundle size (" << bundleBuffer.GetSize() << " bytes) <= max fragment size (" 
                  << maxFragmentSize << " bytes), no fragmentation needed");
      return false;
    }
  
  // 分片bundle
  std::vector<Ptr<Bundle>> fragments = m_fragmentManager->FragmentBundle(bundle, maxFragmentSize);
  
  if (fragments.empty())
    {
      NS_LOG_INFO ("No fragments created, possibly due to must-not-fragment flag");
      return false;
    }
  
  if (!m_routingAlgorithm) {
    NS_LOG_ERROR ("Routing algorithm not set");
    return false;
  }
  
  // 添加所有分片到路由算法
  for (const auto& fragment : fragments)
    {
      if (fragment) {  // 确保分片不为空
        m_routingAlgorithm->NotifyNewBundle (fragment, m_nodeId);
      }
    }
  
  NS_LOG_INFO ("Bundle fragmented into " << fragments.size() << " fragments");
  return true;
}

Ptr<FragmentationManager> 
DtnNode::GetFragmentationManager () const
{
  NS_LOG_FUNCTION (this);
  return m_fragmentManager;
}


bool 
DtnNode::Send (const std::string& source, 
               const std::string& destination, 
               const std::vector<uint8_t>& payload, 
               Time lifetime)
{
  NS_LOG_FUNCTION (this << source << destination << payload.size () << lifetime);
  
  if (!m_running)
    {
      NS_LOG_ERROR ("Node not running");
      return false;
    }
  
  // 创建新bundle
  DtnTime creationTime = GetDtnNow ();
  Bundle bundle = Bundle::MustNewBundle (source, destination, creationTime, lifetime, payload);
  
  Ptr<Bundle> bundlePtr = Create<Bundle> (bundle);
  if (!bundlePtr) {
    NS_LOG_ERROR ("Failed to create Bundle object");
    return false;
  }
  
  return Send (bundlePtr);
}

Ptr<BundleStore> 
DtnNode::GetBundleStore () const
{
  NS_LOG_FUNCTION (this);
  return m_store;
}

Ptr<RoutingAlgorithm> 
DtnNode::GetRoutingAlgorithm () const
{
  NS_LOG_FUNCTION (this);
  return m_routingAlgorithm;
}

std::vector<Ptr<ConvergenceLayer>> 
DtnNode::GetConvergenceLayers () const
{
  NS_LOG_FUNCTION (this);
  return m_convergenceLayers;
}

std::string 
DtnNode::GetStats () const
{
  NS_LOG_FUNCTION (this);
  
  std::stringstream ss;
  
  ss << "DtnNode(";
  ss << "id=" << m_nodeId.ToString ();
  ss << ", recv=" << m_receivedBundles;
  ss << ", delivered=" << m_deliveredBundles;
  
  if (m_store)
    {
      ss << ", store=" << m_store->GetStats ();
    }
  
  if (m_routingAlgorithm)
    {
      ss << ", routing=" << m_routingAlgorithm->GetStats ();
    }
  
  if (m_fragmentManager)
    {
      ss << ", fragmentation=" << m_fragmentManager->GetStats ();
    }
  
  for (const auto& cla : m_convergenceLayers)
    {
      if (cla) {  // 确保CLA不为空
        ss << ", cla=" << cla->GetStats ();
      }
    }
  
  ss << ")";
  
  return ss.str ();
}

void 
DtnNode::LogReceivedBundle (Ptr<Bundle> bundle)
{
  NS_LOG_FUNCTION (this << bundle);
  
  if (!bundle) {  // 添加非空检查
    NS_LOG_ERROR ("Received null bundle pointer");
    return;
  }
  
  NS_LOG_INFO ("Received bundle: " << bundle->ToString ());
  
  // 打印bundle信息
  NS_LOG_INFO ("  Source: " << bundle->GetPrimaryBlock ().GetSourceNodeEID ().ToString ());
  NS_LOG_INFO ("  Destination: " << bundle->GetPrimaryBlock ().GetDestinationEID ().ToString ());
  NS_LOG_INFO ("  Created: " << bundle->GetPrimaryBlock ().GetCreationTimestamp ().ToString ());
  NS_LOG_INFO ("  Lifetime: " << bundle->GetPrimaryBlock ().GetLifetime ().GetSeconds () << "s");
  
  // 如可能，将有效载荷打印为ASCII
  std::vector<uint8_t> payload = bundle->GetPayload ();
  std::string payloadText;
  bool isPrintable = true;
  
  for (uint8_t byte : payload)
    {
      if (byte < 32 || byte > 126)
        {
          isPrintable = false;
          break;
        }
      payloadText += static_cast<char>(byte);
    }
  
  if (isPrintable)
    {
      NS_LOG_INFO ("  Payload: \"" << payloadText << "\"");
    }
  else
    {
      NS_LOG_INFO ("  Payload: " << payload.size () << " bytes (binary)");
    }
}

void 
DtnNode::DoInitialize ()
{
  NS_LOG_FUNCTION (this);
  Application::DoInitialize ();
}

void 
DtnNode::DoDispose ()
{
  NS_LOG_FUNCTION (this);
  
  m_convergenceLayers.clear ();
  m_routingAlgorithm = nullptr;
  m_store = nullptr;
  m_fragmentManager = nullptr;
  
  Application::DoDispose ();
}

void 
DtnNode::StartApplication ()
{
  NS_LOG_FUNCTION (this);
  
  if (m_running)
    {
      return;
    }
  
  // 检查store是否已设置
  if (!m_store)
    {
      NS_LOG_ERROR ("Bundle store not set");
      return;
    }
  
  // 检查路由算法是否已设置
  if (!m_routingAlgorithm)
    {
      NS_LOG_ERROR ("Routing algorithm not set");
      return;
    }
  
  // 初始化路由算法
  std::vector<Ptr<ConvergenceSender>> senders;
  for (const auto& cla : m_convergenceLayers)
    {
      if (cla) {  // 确保CLA不为空
        senders.push_back (cla);
      } else {
        NS_LOG_WARN ("Skipping null convergence layer");
      }
    }
  
  m_routingAlgorithm->Initialize (m_store, senders, m_nodeId);
  
  // 启动收敛层
  for (const auto& cla : m_convergenceLayers)
    {
      if (!cla) {  // 跳过空CLA
        NS_LOG_WARN ("Skipping null convergence layer");
        continue;
      }
      
      // 修复歧义调用问题 - 显式指定调用ConvergenceReceiver::RegisterBundleCallback
      Ptr<ConvergenceReceiver> receiver = DynamicCast<ConvergenceReceiver> (cla);
      if (receiver)
        {
          receiver->RegisterBundleCallback (MakeCallback (&DtnNode::HandleReceivedBundle, this));
        }
      else {
        NS_LOG_WARN ("Convergence layer is not a ConvergenceReceiver");
      }
      
      // 修复歧义调用问题 - 显式指定调用ConvergenceReceiver::Start
      if (receiver)
        {
          if (!receiver->Start ()) {
            NS_LOG_ERROR ("Failed to start convergence receiver");
          }
        }
    }
  
  // 安排周期性任务
  m_cleanupEvent = Simulator::Schedule (m_cleanupInterval, &DtnNode::CleanupExpiredBundles, this);
  m_routingEvent = Simulator::Schedule (m_routingInterval, &DtnNode::RoutingTask, this);
  
  m_running = true;
  
  NS_LOG_INFO ("DTN node started with ID: " << m_nodeId.ToString ());
}

void 
DtnNode::StopApplication ()
{
  NS_LOG_FUNCTION (this);
  
  if (!m_running)
    {
      return;
    }
  
  // 取消已排定的事件
  if (m_cleanupEvent.IsPending ())
    {
      Simulator::Cancel (m_cleanupEvent);
    }
  
  if (m_routingEvent.IsPending ())
    {
      Simulator::Cancel (m_routingEvent);
    }
  
  // 停止收敛层
  for (const auto& cla : m_convergenceLayers)
    {
      if (!cla) continue;  // 跳过空CLA
      
      // 修复歧义调用问题 - 显式指定调用ConvergenceReceiver::Stop
      Ptr<ConvergenceReceiver> receiver = DynamicCast<ConvergenceReceiver> (cla);
      if (receiver)
        {
          if (!receiver->Stop ()) {
            NS_LOG_ERROR ("Failed to stop convergence receiver");
          }
        }
    }
  
  m_running = false;
  
  NS_LOG_INFO ("DTN node stopped");
}

void 
DtnNode::HandleReceivedBundle (Ptr<Bundle> bundle, NodeID source)
{
  NS_LOG_FUNCTION (this << bundle << source.ToString ());
  
  // 检查bundle是否有效
  if (!bundle)
    {
      NS_LOG_ERROR ("Invalid bundle received");
      return;
    }
  
  m_receivedBundles++;
  // 这里需要确保Bundle对象不是空指针
  m_bundleReceivedTrace (bundle);
  
  NS_LOG_INFO ("Received bundle from " << source.ToString ());
  // 检查bundle是否为片段
  if (bundle->IsFragment())
    {
      NS_LOG_INFO ("Bundle is a fragment, attempting reassembly");
      
      // 添加片段到重组过程
      Ptr<Bundle> reassembled = m_fragmentManager->AddFragment(bundle);
      
      if (reassembled)
        {
          NS_LOG_INFO ("Bundle reassembled successfully");
          
          // 处理重组的bundle
          bundle = reassembled;
        }
      else
        {
          NS_LOG_INFO ("Fragment added, but bundle not yet complete");
          
          // 存储片段以便后续重组
          m_store->Push(bundle);
          return;
        }
    }
  
  // 检查此节点是否为目标
  if (IsDeliverable (bundle))
    {
      m_deliveredBundles++;
      // 这里需要确保Bundle对象不是空指针
      m_bundleDeliveredTrace (bundle);
      
      NS_LOG_INFO ("Bundle delivered to this node");
    }
  else
    {
      // 转发bundle到路由算法
      m_routingAlgorithm->NotifyNewBundle (bundle, source);
      
      NS_LOG_INFO ("Bundle forwarded to routing algorithm");
    }
}

void 
DtnNode::CleanupExpiredBundles ()
{
  NS_LOG_FUNCTION (this);
  
  if (!m_store || !m_fragmentManager) {
    NS_LOG_ERROR ("Store or fragment manager not initialized");
    return;
  }
  
  // 清理过期的bundles
  size_t removedCount = m_store->Cleanup ();
  size_t removedFragments = m_fragmentManager->CleanupExpiredFragments();
  NS_LOG_INFO ("Cleaned up " << removedCount << " expired bundles and " 
    << removedFragments << " expired fragment sets");
  // 安排下一次清理
  m_cleanupEvent = Simulator::Schedule (m_cleanupInterval, &DtnNode::CleanupExpiredBundles, this);
}

void 
DtnNode::RoutingTask ()
{
  NS_LOG_FUNCTION (this);
  
  // 检查收敛层并发现对等节点
  for (const auto& cla : m_convergenceLayers)
    {
      if (!cla) continue;  // 跳过空CLA
      
      std::vector<std::string> activeConnections = cla->GetActiveConnections ();
      
      for (const auto& endpoint : activeConnections)
        {
          // 创建对等信息
          PeerInfo peer;
          peer.nodeID = NodeID ("dtn://peer/"); // 在实际实现中，我们会发现真实的EID
          peer.lastSeen = Simulator::Now ();
          peer.receptionTime = Simulator::Now ();
          peer.reachable = true;
          peer.cla = cla->GetEndpoint ();
          peer.endpoint = endpoint;
          
          UpdatePeer (peer);
        }
    }
  
  // 让路由算法分发bundles
  if (m_routingAlgorithm)
    {
      m_routingAlgorithm->DispatchBundles ();
    }
  
  // 安排下一次路由任务
  m_routingEvent = Simulator::Schedule (m_routingInterval, &DtnNode::RoutingTask, this);
}

void 
DtnNode::UpdatePeer (const PeerInfo& peer)
{
  NS_LOG_FUNCTION (this << peer.nodeID.ToString ());
  
  if (!m_routingAlgorithm) {
    NS_LOG_ERROR ("Routing algorithm not initialized");
    return;
  }
  
  // 通知路由算法有关对等节点
  m_routingAlgorithm->NotifyPeerAppeared (peer);
}

bool 
DtnNode::IsDeliverable (Ptr<Bundle> bundle) const
{
  NS_LOG_FUNCTION (this << bundle);
  
  if (!bundle) {
    NS_LOG_ERROR ("Null bundle in IsDeliverable");
    return false;
  }
  
  const EndpointID& destination = bundle->GetPrimaryBlock ().GetDestinationEID ();
  
  // 检查此节点是否为目标
  return destination == m_nodeId;
}

} // namespace dtn7

} // namespace ns3