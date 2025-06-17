#include "spray-routing.h"
#include "ns3/log.h"
#include "ns3/uinteger.h" // Add this for UintegerValue
#include "ns3/object.h"   // Add this for type accessors and checkers
#include <sstream>
#include <functional> // 添加这行来支持哈希函数
namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("SprayAndWaitRouting");

namespace dtn7 {

NS_OBJECT_ENSURE_REGISTERED (SprayAndWaitRouting);

TypeId 
SprayAndWaitRouting::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::dtn7::SprayAndWaitRouting")
    .SetParent<RoutingAlgorithm> ()
    .SetGroupName ("Dtn7")
    .AddConstructor<SprayAndWaitRouting> ()
    .AddAttribute ("MaxCopies",
                   "Maximum number of copies per bundle",
                   UintegerValue (6),
                   MakeUintegerAccessor (&SprayAndWaitRouting::m_maxCopies),
                   MakeUintegerChecker<uint32_t> (1))
  ;
  return tid;
}

SprayAndWaitRouting::SprayAndWaitRouting ()
  : m_maxCopies (6)
{
  NS_LOG_FUNCTION (this);
}

SprayAndWaitRouting::~SprayAndWaitRouting ()
{
  NS_LOG_FUNCTION (this);
}

void 
SprayAndWaitRouting::NotifyNewBundle (Ptr<Bundle> bundle, const NodeID& source)
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
  
  // Set initial copy count
  BundleID id = bundle->GetId ();
  uint32_t count = m_maxCopies;
  
  // If this node is the source, use max copies
  if (bundle->GetPrimaryBlock ().GetSourceNodeEID () == m_localNodeID)
    {
      NS_LOG_INFO ("Local node is source, setting max copies: " << count);
    }
  else
    {
      // If this is a received bundle, use half the copies from source
      count = std::max(1U, count / 2);
      NS_LOG_INFO ("Received from remote, setting half copies: " << count);
    }
  
  SetCopyCount (id, count);
  
  NS_LOG_INFO ("Added new bundle from " << source.ToString () << " to store with " << count << " copies");
}

void 
SprayAndWaitRouting::NotifyPeerAppeared (const PeerInfo& peer)
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
SprayAndWaitRouting::NotifyPeerDisappeared (const NodeID& peer)
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
SprayAndWaitRouting::DispatchBundles ()
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
      
      uint32_t copyCount = GetCopyCount (id);
      
      if (copyCount <= 1)
        {
          // Wait phase: only forward directly to destination
          const EndpointID& destinationEID = bundle->GetPrimaryBlock ().GetDestinationEID ();
          
          for (const auto& peer : activePeers)
            {
              // Only send to final destination
              if (peer.nodeID == destinationEID)
                {
                  if (it != m_bundles.end () && !it->second.SentTo (peer.nodeID))
                    {
                      NS_LOG_INFO ("Sending bundle directly to destination: " << peer.nodeID.ToString ());
                      SendBundle (bundle, peer.nodeID, peer.endpoint);
                    }
                }
            }
        }
      else
        {
          // Spray phase: forward to peers we haven't sent to yet
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
              
              // Decrease copy count
              uint32_t newCount = DecreaseCopyCount (id);
              
              // Split the remaining copies
              uint32_t peerCopies = newCount / 2;
              uint32_t localCopies = newCount - peerCopies;
              
              NS_LOG_INFO ("Spraying bundle to " << peer.nodeID.ToString () 
                          << ", copies: local=" << localCopies 
                          << ", peer=" << peerCopies);
              
              // Send the bundle to the peer
              if (SendBundle (bundle, peer.nodeID, peer.endpoint))
                {
                  // Update local copy count
                  SetCopyCount (id, localCopies);
                  
                  // If we're out of copies, break
                  if (localCopies <= 1)
                    {
                      break;
                    }
                }
              else
                {
                  // Failed to send, restore copy count
                  SetCopyCount (id, newCount);
                }
            }
        }
    }
}

void 
SprayAndWaitRouting::SetCopyCount (const BundleID& id, uint32_t count)
{
  NS_LOG_FUNCTION (this << id.ToString () << count);
  
  std::lock_guard<std::mutex> lock (m_copiesMutex);
  m_copies[id] = count;
}

uint32_t 
SprayAndWaitRouting::GetCopyCount (const BundleID& id) const
{
  NS_LOG_FUNCTION (this << id.ToString ());
  
  std::lock_guard<std::mutex> lock (m_copiesMutex);
  auto it = m_copies.find (id);
  if (it != m_copies.end ())
    {
      return it->second;
    }
  
  return 1; // Default to 1 copy
}

uint32_t 
SprayAndWaitRouting::DecreaseCopyCount (const BundleID& id)
{
  NS_LOG_FUNCTION (this << id.ToString ());
  
  std::lock_guard<std::mutex> lock (m_copiesMutex);
  
  auto it = m_copies.find (id);
  if (it != m_copies.end () && it->second > 1)
    {
      it->second--;
      return it->second;
    }
  
  return 1; // Minimum is 1 copy
}

std::string 
SprayAndWaitRouting::GetName () const
{
  return "SprayAndWaitRouting";
}

std::string 
SprayAndWaitRouting::GetStats () const
{
  std::stringstream ss;
  
  ss << "SprayAndWaitRouting(";
  ss << "maxCopies=" << m_maxCopies;
  ss << ", peers=" << m_peers.size ();
  ss << ", bundles=" << m_bundles.size ();
  ss << ", sent=" << m_sentBundles;
  ss << ", failed=" << m_failedBundles;
  ss << ")";
  
  return ss.str ();
}

} // namespace dtn7

} // namespace ns3