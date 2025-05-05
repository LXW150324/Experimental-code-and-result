#ifndef DTN7_HELPER_H
#define DTN7_HELPER_H

#include "ns3/object-factory.h"
#include "ns3/node-container.h"
#include "ns3/application-container.h"
#include "ns3/ptr.h"

#include "../model/dtn-node.h"
#include "../model/routing.h"
#include "../model/bundle-store.h"
#include "../model/convergence-layer.h"

namespace ns3 {

namespace dtn7 {

/**
 * \ingroup dtn7
 * \brief Helper class to install DTN nodes
 */
class Dtn7Helper
{
public:
  /**
   * \brief Constructor
   */
  Dtn7Helper ();
  
  /**
   * \brief Set the routing algorithm
   * \param routingType Type of routing algorithm to use
   * \param n0 First attribute name
   * \param v0 First attribute value
   * \param n1 Second attribute name
   * \param v1 Second attribute value
   * \param n2 Third attribute name
   * \param v2 Third attribute value
   * \param n3 Fourth attribute name
   * \param v3 Fourth attribute value
   * \param n4 Fifth attribute name
   * \param v4 Fifth attribute value
   * \param n5 Sixth attribute name
   * \param v5 Sixth attribute value
   * \param n6 Seventh attribute name
   * \param v6 Seventh attribute value
   * \param n7 Eighth attribute name
   * \param v7 Eighth attribute value
   */
  void SetRoutingAlgorithm (std::string routingType,
                           std::string n0 = "", const AttributeValue &v0 = EmptyAttributeValue (),
                           std::string n1 = "", const AttributeValue &v1 = EmptyAttributeValue (),
                           std::string n2 = "", const AttributeValue &v2 = EmptyAttributeValue (),
                           std::string n3 = "", const AttributeValue &v3 = EmptyAttributeValue (),
                           std::string n4 = "", const AttributeValue &v4 = EmptyAttributeValue (),
                           std::string n5 = "", const AttributeValue &v5 = EmptyAttributeValue (),
                           std::string n6 = "", const AttributeValue &v6 = EmptyAttributeValue (),
                           std::string n7 = "", const AttributeValue &v7 = EmptyAttributeValue ());
  
  /**
   * \brief Set the bundle store
   * \param storeType Type of bundle store to use
   * \param n0 First attribute name
   * \param v0 First attribute value
   * \param n1 Second attribute name
   * \param v1 Second attribute value
   * \param n2 Third attribute name
   * \param v2 Third attribute value
   * \param n3 Fourth attribute name
   * \param v3 Fourth attribute value
   * \param n4 Fifth attribute name
   * \param v4 Fifth attribute value
   * \param n5 Sixth attribute name
   * \param v5 Sixth attribute value
   * \param n6 Seventh attribute name
   * \param v6 Seventh attribute value
   * \param n7 Eighth attribute name
   * \param v7 Eighth attribute value
   */
  void SetBundleStore (std::string storeType,
                      std::string n0 = "", const AttributeValue &v0 = EmptyAttributeValue (),
                      std::string n1 = "", const AttributeValue &v1 = EmptyAttributeValue (),
                      std::string n2 = "", const AttributeValue &v2 = EmptyAttributeValue (),
                      std::string n3 = "", const AttributeValue &v3 = EmptyAttributeValue (),
                      std::string n4 = "", const AttributeValue &v4 = EmptyAttributeValue (),
                      std::string n5 = "", const AttributeValue &v5 = EmptyAttributeValue (),
                      std::string n6 = "", const AttributeValue &v6 = EmptyAttributeValue (),
                      std::string n7 = "", const AttributeValue &v7 = EmptyAttributeValue ());
  
  /**
   * \brief Set the convergence layer
   * \param claType Type of convergence layer to use
   * \param n0 First attribute name
   * \param v0 First attribute value
   * \param n1 Second attribute name
   * \param v1 Second attribute value
   * \param n2 Third attribute name
   * \param v2 Third attribute value
   * \param n3 Fourth attribute name
   * \param v3 Fourth attribute value
   * \param n4 Fifth attribute name
   * \param v4 Fifth attribute value
   * \param n5 Sixth attribute name
   * \param v5 Sixth attribute value
   * \param n6 Seventh attribute name
   * \param v6 Seventh attribute value
   * \param n7 Eighth attribute name
   * \param v7 Eighth attribute value
   */
  void SetConvergenceLayer (std::string claType,
                           std::string n0 = "", const AttributeValue &v0 = EmptyAttributeValue (),
                           std::string n1 = "", const AttributeValue &v1 = EmptyAttributeValue (),
                           std::string n2 = "", const AttributeValue &v2 = EmptyAttributeValue (),
                           std::string n3 = "", const AttributeValue &v3 = EmptyAttributeValue (),
                           std::string n4 = "", const AttributeValue &v4 = EmptyAttributeValue (),
                           std::string n5 = "", const AttributeValue &v5 = EmptyAttributeValue (),
                           std::string n6 = "", const AttributeValue &v6 = EmptyAttributeValue (),
                           std::string n7 = "", const AttributeValue &v7 = EmptyAttributeValue ());
  
  /**
   * \brief Install DTN nodes on a container of nodes
   * \param c NodeContainer of nodes to install DTN on
   * \return Container of DTN node applications
   */
  ApplicationContainer Install (NodeContainer c);
  
  /**
   * \brief Install DTN node on a node
   * \param node Node to install DTN on
   * \return Container with the DTN node application
   */
  ApplicationContainer Install (Ptr<Node> node);
  
  /**
   * \brief Install DTN node on a node
   * \param nodeName Name of node to install DTN on
   * \return Container with the DTN node application
   */
  ApplicationContainer Install (std::string nodeName);

private:
  ObjectFactory m_routingFactory;    //!< Routing algorithm factory
  ObjectFactory m_storeFactory;      //!< Bundle store factory
  ObjectFactory m_claFactory;        //!< Convergence layer factory
  ObjectFactory m_nodeFactory;       //!< DTN node factory
  
  /**
   * \brief Install DTN node on a node
   * \param node Node to install DTN on
   * \return DTN node application
   */
  Ptr<Application> InstallPriv (Ptr<Node> node);
};

} // namespace dtn7

} // namespace ns3

#endif /* DTN7_HELPER_H */