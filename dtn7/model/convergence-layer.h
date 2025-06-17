#ifndef DTN7_CONVERGENCE_LAYER_H
#define DTN7_CONVERGENCE_LAYER_H

#include "ns3/object.h"
#include "ns3/callback.h"
#include "ns3/ptr.h"
#include "ns3/address.h"

#include <string>
#include <functional>
#include <vector>

#include "bundle.h"
#include "endpoint.h"

namespace ns3 {

namespace dtn7 {

/**
 * \ingroup dtn7
 * \brief Interface for bundle receivers
 */
class ConvergenceReceiver : public virtual Object
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
  ConvergenceReceiver ();
  
  /**
   * \brief Destructor
   */
  virtual ~ConvergenceReceiver ();
  
  /**
   * \brief Register a callback for received bundles
   * \param callback Function to call when a bundle is received
   */
  virtual void RegisterBundleCallback (Callback<void, Ptr<Bundle>, NodeID> callback) = 0;
  
  /**
   * \brief Start the receiver
   * \return true if started successfully
   */
  virtual bool Start () = 0;
  
  /**
   * \brief Stop the receiver
   * \return true if stopped successfully
   */
  virtual bool Stop () = 0;
  
  /**
   * \brief Get the endpoint address
   * \return Endpoint address string
   */
  virtual std::string GetEndpoint () const = 0;
};

/**
 * \ingroup dtn7
 * \brief Interface for bundle senders
 */
class ConvergenceSender : public virtual Object
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
  ConvergenceSender ();
  
  /**
   * \brief Destructor
   */
  virtual ~ConvergenceSender ();
  
  /**
   * \brief Send a bundle to a specific endpoint
   * \param bundle Bundle to send
   * \param endpoint Destination endpoint address
   * \return true if sent successfully
   */
  virtual bool Send (Ptr<Bundle> bundle, const std::string& endpoint) = 0;
  
  /**
   * \brief Check if an endpoint is currently reachable
   * \param endpoint Endpoint to check
   * \return true if endpoint is reachable
   */
  virtual bool IsEndpointReachable (const std::string& endpoint) const = 0;
  
  /**
   * \brief Start the sender
   * \return true if started successfully
   */
  virtual bool Start () = 0;
  
  /**
   * \brief Stop the sender
   * \return true if stopped successfully
   */
  virtual bool Stop () = 0;
};

/**
 * \ingroup dtn7
 * \brief Combined interface for bundle receivers and senders
 */
class ConvergenceLayer : public virtual ConvergenceReceiver, public virtual ConvergenceSender
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
  ConvergenceLayer ();
  
  /**
   * \brief Destructor
   */
  virtual ~ConvergenceLayer ();
  
  /**
   * \brief Get protocol specific node statistics
   * \return Statistics string
   */
  virtual std::string GetStats () const = 0;
  
  /**
   * \brief Get list of active connection endpoints
   * \return List of endpoint strings
   */
  virtual std::vector<std::string> GetActiveConnections () const = 0;
  
  /**
   * \brief Check if there is an active connection to an endpoint
   * \param endpoint Endpoint to check
   * \return true if there is an active connection
   */
  virtual bool HasActiveConnection (const std::string& endpoint) const = 0;
};

} // namespace dtn7

} // namespace ns3

#endif /* DTN7_CONVERGENCE_LAYER_H */