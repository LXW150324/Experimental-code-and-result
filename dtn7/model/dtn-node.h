#ifndef DTN7_DTN_NODE_H
#define DTN7_DTN_NODE_H

#include "ns3/application.h"
#include "ns3/nstime.h"
#include "ns3/ptr.h"
#include "ns3/traced-callback.h"
#include "ns3/log.h"

#include <vector>
#include <string>
#include <thread>
#include <atomic>
#include <mutex>

#include "bundle.h"
#include "endpoint.h"
#include "convergence-layer.h"
#include "routing.h"
#include "bundle-store.h"
#include "fragmentation-manager.h"
namespace ns3 {

namespace dtn7 {

/**
 * \ingroup dtn7
 * \brief DTN node application
 */
class DtnNode : public Application
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
  DtnNode ();
  
  /**
   * \brief Destructor
   */
  virtual ~DtnNode ();
  
  /**
   * \brief Set node ID
   * \param id Node ID
   */
  void SetNodeId (const NodeID& id);
  
  /**
   * \brief Get node ID
   * \return Node ID
   */
  NodeID GetNodeId () const;
  
  /**
   * \brief Add a convergence layer
   * \param cla Convergence layer
   */
  void AddConvergenceLayer (Ptr<ConvergenceLayer> cla);
  
  /**
   * \brief Set the routing algorithm
   * \param algorithm Routing algorithm
   */
  void SetRoutingAlgorithm (Ptr<RoutingAlgorithm> algorithm);
  
  /**
   * \brief Set the bundle store
   * \param store Bundle store
   */
  void SetBundleStore (Ptr<BundleStore> store);
  
  /**
   * \brief Send a bundle
   * \param bundle Bundle to send
   * \return true if bundle was added to the store
   */
  bool FragmentIfNeeded (Ptr<Bundle> bundle, size_t maxFragmentSize);

/**
 * \brief Get the fragmentation manager
 * \return Fragmentation manager
 */
Ptr<FragmentationManager> GetFragmentationManager () const;
  bool Send (Ptr<Bundle> bundle);
  
  /**
   * \brief Send a bundle
   * \param source Source EID
   * \param destination Destination EID
   * \param payload Payload data
   * \param lifetime Bundle lifetime
   * \return true if bundle was added to the store
   */
  bool Send (const std::string& source, 
             const std::string& destination, 
             const std::vector<uint8_t>& payload, 
             Time lifetime);
  
  /**
   * \brief Get bundle store
   * \return Bundle store
   */
  Ptr<BundleStore> GetBundleStore () const;
  
  /**
   * \brief Get routing algorithm
   * \return Routing algorithm
   */
  Ptr<RoutingAlgorithm> GetRoutingAlgorithm () const;
  
  /**
   * \brief Get convergence layers
   * \return Convergence layers
   */
  std::vector<Ptr<ConvergenceLayer>> GetConvergenceLayers () const;
  
  /**
   * \brief Get statistics
   * \return Statistics string
   */
  std::string GetStats () const;
  
  /**
   * \brief Log a received bundle
   * \param bundle Received bundle
   */
  void LogReceivedBundle (Ptr<Bundle> bundle);

  void RegisterBundleCallback(Callback<void, Ptr<Bundle>, NodeID> callback);

protected:
  // Inherited from Application
  void DoInitialize () override;
  void DoDispose () override;
  void StartApplication () override;
  void StopApplication () override;
  
private:
  NodeID m_nodeId;                                 //!< Node ID
  Ptr<BundleStore> m_store;                        //!< Bundle store
  Ptr<RoutingAlgorithm> m_routingAlgorithm;   
  Ptr<FragmentationManager> m_fragmentManager;        //!< Routing algorithm
  std::vector<Ptr<ConvergenceLayer>> m_convergenceLayers; //!< Convergence layers
 
  bool m_running;                                  //!< Whether the node is running
  Time m_cleanupInterval;                          //!< Interval for cleanup
  Time m_routingInterval;                          //!< Interval for routing
  EventId m_cleanupEvent;                          //!< Event for cleanup
  EventId m_routingEvent;                          //!< Event for routing
  
  uint64_t m_receivedBundles;                      //!< Number of received bundles
  uint64_t m_deliveredBundles;                     //!< Number of delivered bundles
  
  TracedCallback<Ptr<Bundle>> m_bundleReceivedTrace;  //!< Trace for received bundles
  TracedCallback<Ptr<Bundle>> m_bundleDeliveredTrace; //!< Trace for delivered bundles
  
  /**
   * \brief Handle a received bundle
   * \param bundle Received bundle
   * \param source Source node ID (传值而非引用，以匹配回调函数类型要求)
   */
  void HandleReceivedBundle (Ptr<Bundle> bundle, NodeID source);
  
  /**
   * \brief Periodic cleanup of expired bundles
   */
  void CleanupExpiredBundles ();
  
  /**
   * \brief Periodic routing task
   */
  void RoutingTask ();
  
  /**
   * \brief Update peer information
   * \param peer Peer information
   */
  void UpdatePeer (const PeerInfo& peer);
  
  /**
   * \brief Check if a bundle is deliverable
   * \param bundle Bundle to check
   * \return true if bundle is deliverable to this node
   */
  bool IsDeliverable (Ptr<Bundle> bundle) const;
};

} // namespace dtn7

} // namespace ns3

#endif /* DTN7_DTN_NODE_H */