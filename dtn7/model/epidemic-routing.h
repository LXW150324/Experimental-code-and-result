#ifndef DTN7_EPIDEMIC_ROUTING_H
#define DTN7_EPIDEMIC_ROUTING_H

#include "routing.h"

namespace ns3 {

namespace dtn7 {

/**
 * \ingroup dtn7
 * \brief Epidemic routing algorithm implementation
 */
class EpidemicRouting : public RoutingAlgorithm
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
  EpidemicRouting ();
  
  /**
   * \brief Destructor
   */
  virtual ~EpidemicRouting ();
  
  // Inherited from RoutingAlgorithm
  void NotifyNewBundle (Ptr<Bundle> bundle, const NodeID& source) override;
  void NotifyPeerAppeared (const PeerInfo& peer) override;
  void NotifyPeerDisappeared (const NodeID& peer) override;
  std::string GetName () const override;
  std::string GetStats () const override;

protected:
  // Inherited from RoutingAlgorithm
  void DispatchBundles () override;
};

} // namespace dtn7

} // namespace ns3

#endif /* DTN7_EPIDEMIC_ROUTING_H */