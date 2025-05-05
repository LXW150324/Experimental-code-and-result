#ifndef DTN7_SPRAY_ROUTING_H
#define DTN7_SPRAY_ROUTING_H

#include "routing.h"
#include <unordered_map>
#include <mutex>

namespace ns3 {

namespace dtn7 {

/**
 * \ingroup dtn7
 * \brief Spray and Wait routing algorithm implementation
 */
class SprayAndWaitRouting : public RoutingAlgorithm
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
  SprayAndWaitRouting ();
  
  /**
   * \brief Destructor
   */
  virtual ~SprayAndWaitRouting ();
  
  // Inherited from RoutingAlgorithm
  void NotifyNewBundle (Ptr<Bundle> bundle, const NodeID& source) override;
  void NotifyPeerAppeared (const PeerInfo& peer) override;
  void NotifyPeerDisappeared (const NodeID& peer) override;
  std::string GetName () const override;
  std::string GetStats () const override;

protected:
  // Inherited from RoutingAlgorithm
  void DispatchBundles () override;
  
  /**
   * \brief Set copy count for a bundle
   * \param id Bundle ID
   * \param count Copy count
   */
  void SetCopyCount (const BundleID& id, uint32_t count);
  
  /**
   * \brief Get copy count for a bundle
   * \param id Bundle ID
   * \return Copy count
   */
  uint32_t GetCopyCount (const BundleID& id) const;
  
  /**
   * \brief Decrease copy count for a bundle
   * \param id Bundle ID
   * \return New copy count
   */
  uint32_t DecreaseCopyCount (const BundleID& id);
  
  std::unordered_map<BundleID, uint32_t> m_copies; //!< Bundle copy counts
  mutable std::mutex m_copiesMutex;                //!< Bundle copy counts mutex
  uint32_t m_maxCopies;                            //!< Maximum copies per bundle
};

} // namespace dtn7

} // namespace ns3

#endif /* DTN7_SPRAY_ROUTING_H */