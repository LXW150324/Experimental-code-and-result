#ifndef DTN7_ROUTING_H
#define DTN7_ROUTING_H

#include "ns3/object.h"
#include "ns3/traced-callback.h"
#include "ns3/log.h"

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <functional> // 添加这行

#include "bundle.h"
#include "endpoint.h"
#include "convergence-layer.h"
#include "bundle-store.h"

namespace ns3 {

namespace dtn7 {

/**
 * \ingroup dtn7
 * \brief Bundle descriptor for routing decisions
 */
struct BundleDescriptor
{
  BundleID id;              //!< Bundle ID
  EndpointID receiver;      //!< Receiver endpoint
  std::vector<NodeID> sentNodes; //!< Nodes the bundle was sent to
  Time expirationTime;      //!< Time when the bundle expires
  
  /**
   * \brief Check if bundle was sent to a node
   * \param node Node to check
   * \return true if bundle was sent to the node
   */
  bool SentTo (const NodeID& node) const;
  
  /**
   * \brief Add a node to the list of nodes the bundle was sent to
   * \param node Node to add
   */
  void AddSentNode (const NodeID& node);
  
  /**
   * \brief Check if bundle is expired
   * \return true if bundle is expired
   */
  bool IsExpired () const;
};

/**
 * \ingroup dtn7
 * \brief Peer information for routing decisions
 */
struct PeerInfo
{
  NodeID nodeID;          //!< Peer node ID
  Time lastSeen;          //!< Last time the peer was seen
  Time receptionTime;     //!< Time when the peer was discovered
  bool reachable;         //!< Whether the peer is reachable
  std::string cla;        //!< Convergence layer used for the peer
  std::string endpoint;   //!< Endpoint address
  
  /**
   * \brief Check if peer is active
   * \return true if peer is active
   */
  bool IsActive () const;
};

/**
 * \ingroup dtn7
 * \brief Interface for routing algorithms
 */
class RoutingAlgorithm : public Object
{
public:
  /**
   * \brief Get the type ID
   * \return Type ID
   */
  static TypeId GetTypeId ();
  
  /**
   * \brief Default constructor
   */
  RoutingAlgorithm ();
  
  /**
   * \brief Destructor
   */
  virtual ~RoutingAlgorithm ();
  
  /**
   * \brief Initialize routing algorithm
   * \param store Bundle store
   * \param senders Convergence layer senders
   * \param localNodeID Local node ID
   */
  virtual void Initialize (Ptr<BundleStore> store,
                          std::vector<Ptr<ConvergenceSender>> senders,
                          NodeID localNodeID);
  
  /**
   * \brief Process a newly received bundle
   * \param bundle Received bundle
   * \param source Source of the bundle
   */
  virtual void NotifyNewBundle (Ptr<Bundle> bundle, const NodeID& source) = 0;
  
  /**
   * \brief Update routing information for a peer
   * \param peer Peer information
   */
  virtual void NotifyPeerAppeared (const PeerInfo& peer) = 0;
  
  /**
   * \brief Update routing information when a peer disconnects
   * \param peer Peer node ID
   */
  virtual void NotifyPeerDisappeared (const NodeID& peer) = 0;
  
  /**
   * \brief Dispatch bundles to be sent
   */
  virtual void DispatchBundles ();
  
  /**
   * \brief Get algorithm name
   * \return Algorithm name
   */
  virtual std::string GetName () const = 0;
  
  /**
   * \brief Get routing stats
   * \return Statistics string
   */
  virtual std::string GetStats () const = 0;

protected:
  NodeID m_localNodeID;                                  //!< Local node ID
  Ptr<BundleStore> m_store;                              //!< Bundle store
  std::vector<Ptr<ConvergenceSender>> m_senders;         //!< Convergence layer senders
  std::unordered_map<NodeID, PeerInfo> m_peers;          //!< Known peers
  std::unordered_map<BundleID, BundleDescriptor> m_bundles; //!< Bundle descriptors
  mutable std::mutex m_peersMutex;                       //!< Mutex for peers
  mutable std::mutex m_bundlesMutex;                     //!< Mutex for bundles
  
  uint64_t m_sentBundles;                                //!< Number of sent bundles
  uint64_t m_failedBundles;                              //!< Number of failed sendings
  
  TracedCallback<Ptr<Bundle>, NodeID> m_bundleSentTrace; //!< Trace for sent bundles
  
  /**
   * \brief Send a bundle to a receiver
   * \param bundle Bundle to send
   * \param receiver Receiver node ID
   * \param endpoint Endpoint address
   * \return true if bundle was sent
   */
  bool SendBundle (Ptr<Bundle> bundle, const NodeID& receiver, const std::string& endpoint);
  
  /**
   * \brief Update or create a bundle descriptor
   * \param bundle Bundle
   * \return Bundle descriptor
   */
  BundleDescriptor& UpdateBundleDescriptor (Ptr<Bundle> bundle);
  
  /**
   * \brief Calculate expiration time for a bundle
   * \param bundle Bundle
   * \return Expiration time
   */
  Time CalculateExpirationTime (Ptr<Bundle> bundle) const;
};

} // namespace dtn7

} // namespace ns3

#endif /* DTN7_ROUTING_H */