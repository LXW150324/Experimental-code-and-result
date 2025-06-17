#include "routing.h"
#include "ns3/simulator.h"
#include <sstream>
#include <functional> // 添加这行来支持哈希函数
namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("RoutingAlgorithm");

namespace dtn7 {

// BundleDescriptor implementation

bool 
BundleDescriptor::SentTo (const NodeID& node) const
{
  for (const auto& sentNode : sentNodes)
    {
      if (sentNode == node)
        {
          return true;
        }
    }
  
  return false;
}

void 
BundleDescriptor::AddSentNode (const NodeID& node)
{
  if (!SentTo (node))
    {
      sentNodes.push_back (node);
    }
}

bool 
BundleDescriptor::IsExpired () const
{
  return Simulator::Now () > expirationTime;
}

// PeerInfo implementation

bool 
PeerInfo::IsActive () const
{
  // Consider a peer active if it's reachable and was seen recently
  return reachable && (Simulator::Now () - lastSeen < Time (Minutes (5)));
}

// RoutingAlgorithm implementation

NS_OBJECT_ENSURE_REGISTERED (RoutingAlgorithm);

TypeId 
RoutingAlgorithm::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::dtn7::RoutingAlgorithm")
    .SetParent<Object> ()
    .SetGroupName ("Dtn7")
    .AddTraceSource ("BundleSent",
                     "Trace source for sent bundles",
                     MakeTraceSourceAccessor (&RoutingAlgorithm::m_bundleSentTrace),
                     "ns3::dtn7::RoutingAlgorithm::BundleTracedCallback")
  ;
  return tid;
}

RoutingAlgorithm::RoutingAlgorithm ()
  : m_sentBundles (0),
    m_failedBundles (0)
{
  NS_LOG_FUNCTION (this);
}

RoutingAlgorithm::~RoutingAlgorithm ()
{
  NS_LOG_FUNCTION (this);
}

void 
RoutingAlgorithm::Initialize (Ptr<BundleStore> store,
                              std::vector<Ptr<ConvergenceSender>> senders,
                              NodeID localNodeID)
{
  NS_LOG_FUNCTION (this << store << senders.size () << localNodeID.ToString ());
  
  m_store = store;
  m_senders = senders;
  m_localNodeID = localNodeID;
}

void
RoutingAlgorithm::DispatchBundles ()
{
  NS_LOG_FUNCTION (this);
  NS_LOG_INFO ("Default implementation of DispatchBundles - should be overridden by subclasses");
  // This default implementation doesn't do anything
  // Subclasses should override this with their specific dispatch logic
}

bool 
RoutingAlgorithm::SendBundle (Ptr<Bundle> bundle, const NodeID& receiver, const std::string& endpoint)
{
  NS_LOG_FUNCTION (this << bundle << receiver.ToString () << endpoint);
  
  // Find a suitable convergence layer sender
  for (const auto& sender : m_senders)
    {
      if (sender->IsEndpointReachable (endpoint))
        {
          NS_LOG_INFO ("Sending bundle to " << receiver.ToString () << " via " << endpoint);
          
          // Update previous node block
          Ptr<PreviousNodeBlock> prevNode = DynamicCast<PreviousNodeBlock> (bundle->GetBlockByType (BlockType::PREVIOUS_NODE_BLOCK));
          if (!prevNode)
            {
              prevNode = Create<PreviousNodeBlock> (m_localNodeID);
              bundle->AddBlock (prevNode);
            }
          else
            {
              prevNode->SetPreviousNode (m_localNodeID);
            }
          
          // Send the bundle
          bool success = sender->Send (bundle, endpoint);
          
          if (success)
            {
              // Update bundle descriptor
              {
                std::lock_guard<std::mutex> lock (m_bundlesMutex);
                BundleID id = bundle->GetId ();
                auto it = m_bundles.find (id);
                if (it != m_bundles.end ())
                  {
                    it->second.AddSentNode (receiver);
                  }
              }
              
              m_sentBundles++;
              m_bundleSentTrace (bundle, receiver);
              return true;
            }
          else
            {
              NS_LOG_ERROR ("Failed to send bundle to " << receiver.ToString () << " via " << endpoint);
              m_failedBundles++;
              return false;
            }
        }
    }
  
  NS_LOG_ERROR ("No reachable convergence layer for endpoint " << endpoint);
  m_failedBundles++;
  return false;
}

BundleDescriptor& 
RoutingAlgorithm::UpdateBundleDescriptor (Ptr<Bundle> bundle)
{
  NS_LOG_FUNCTION (this << bundle);
  
  BundleID id = bundle->GetId ();
  
  std::lock_guard<std::mutex> lock (m_bundlesMutex);
  auto it = m_bundles.find (id);
  
  if (it != m_bundles.end ())
    {
      return it->second;
    }
  
  // Create new descriptor
  BundleDescriptor desc;
  desc.id = id;
  desc.receiver = bundle->GetPrimaryBlock ().GetDestinationEID ();
  desc.expirationTime = CalculateExpirationTime (bundle);
  
  m_bundles[id] = desc;
  return m_bundles[id];
}

Time 
RoutingAlgorithm::CalculateExpirationTime (Ptr<Bundle> bundle) const
{
  NS_LOG_FUNCTION (this << bundle);
  
  const PrimaryBlock& primary = bundle->GetPrimaryBlock ();
  Time creationTime = primary.GetCreationTimestamp ().ToTime ();
  Time lifetime = primary.GetLifetime ();
  
  return creationTime + lifetime;
}

} // namespace dtn7

} // namespace ns3