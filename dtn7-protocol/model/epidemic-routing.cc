#include "epidemic-routing.h"
#include "ns3/log.h"
#include <sstream>
#include <functional> // 添加这行来支持哈希函数
namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("EpidemicRouting");

namespace dtn7 {

NS_OBJECT_ENSURE_REGISTERED (EpidemicRouting);

TypeId 
EpidemicRouting::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::dtn7::EpidemicRouting")
    .SetParent<RoutingAlgorithm> ()
    .SetGroupName ("Dtn7")
    .AddConstructor<EpidemicRouting> ()
  ;
  return tid;
}

EpidemicRouting::EpidemicRouting ()
{
  NS_LOG_FUNCTION (this);
}

EpidemicRouting::~EpidemicRouting ()
{
  NS_LOG_FUNCTION (this);
}

void 
EpidemicRouting::NotifyNewBundle (Ptr<Bundle> bundle, const NodeID& source)
{
  NS_LOG_FUNCTION (this << bundle << source.ToString ());
  
  // Store the bundle
  if (!m_store->Push (bundle))
    {
      NS_LOG_ERROR ("Failed to store bundle");
      return;
    }
  
  // Update bundle descriptor
  BundleDescriptor& desc = UpdateBundleDescriptor (bundle);
  
  // Mark as sent to source node
  desc.AddSentNode (source);
  
  NS_LOG_INFO ("Added new bundle from " << source.ToString () << " to store");
}

void 
EpidemicRouting::NotifyPeerAppeared (const PeerInfo& peer)
{
  NS_LOG_FUNCTION (this << peer.nodeID.ToString ());
  
  // Store peer information
  {
    std::lock_guard<std::mutex> lock (m_peersMutex);
    m_peers[peer.nodeID] = peer;
  }
  
  NS_LOG_INFO ("Peer appeared: " << peer.nodeID.ToString ());
  
  // Dispatch bundles to the new peer
  DispatchBundles ();
}

void 
EpidemicRouting::NotifyPeerDisappeared (const NodeID& peer)
{
  NS_LOG_FUNCTION (this << peer.ToString ());
  
  // Remove peer information
  {
    std::lock_guard<std::mutex> lock (m_peersMutex);
    m_peers.erase (peer);
  }
  
  NS_LOG_INFO ("Peer disappeared: " << peer.ToString ());
}

void 
EpidemicRouting::DispatchBundles ()
{
  NS_LOG_FUNCTION (this);
  
  // Get all bundles from the store
  std::vector<Ptr<Bundle>> bundles = m_store->GetAll ();
  
  NS_LOG_INFO ("Dispatching " << bundles.size () << " bundles to peers");
  
  // Get all active peers
  std::vector<PeerInfo> activePeers;
  {
    std::lock_guard<std::mutex> lock (m_peersMutex);
    for (const auto& pair : m_peers)
      {
        if (pair.second.IsActive ())
          {
            activePeers.push_back (pair.second);
          }
      }
  }
  
  // Try to send each bundle to each peer
  for (const auto& bundle : bundles)
    {
      BundleID id = bundle->GetId ();
      
      // Skip expired bundles
      std::lock_guard<std::mutex> lock (m_bundlesMutex);
      auto it = m_bundles.find (id);
      if (it != m_bundles.end () && it->second.IsExpired ())
        {
          NS_LOG_INFO ("Skipping expired bundle: " << id.ToString ());
          continue;
        }
      
      for (const auto& peer : activePeers)
        {
          // Skip if destination is local
          if (bundle->GetPrimaryBlock ().GetDestinationEID () == m_localNodeID)
            {
              continue;
            }
          
          // Skip if destination is this peer and it's not the final destination
          if (bundle->GetPrimaryBlock ().GetDestinationEID () != peer.nodeID &&
              peer.nodeID == m_localNodeID)
            {
              continue;
            }
          
          // Skip if already sent to this peer
          if (it != m_bundles.end () && it->second.SentTo (peer.nodeID))
            {
              continue;
            }
          
          // Send the bundle to the peer
          SendBundle (bundle, peer.nodeID, peer.endpoint);
        }
    }
}

std::string 
EpidemicRouting::GetName () const
{
  return "EpidemicRouting";
}

std::string 
EpidemicRouting::GetStats () const
{
  std::stringstream ss;
  
  ss << "EpidemicRouting(";
  ss << "peers=" << m_peers.size ();
  ss << ", bundles=" << m_bundles.size ();
  ss << ", sent=" << m_sentBundles;
  ss << ", failed=" << m_failedBundles;
  ss << ")";
  
  return ss.str ();
}

} // namespace dtn7

} // namespace ns3