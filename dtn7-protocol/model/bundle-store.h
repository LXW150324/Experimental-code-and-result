#ifndef DTN7_BUNDLE_STORE_H
#define DTN7_BUNDLE_STORE_H

#include "ns3/object.h"
#include "ns3/ptr.h"

#include <vector>
#include <unordered_map>
#include <functional>
#include <mutex>
#include <optional>

#include "bundle.h"
#include "bundle-id.h"

namespace ns3 {

namespace dtn7 {

/**
 * \ingroup dtn7
 * \brief Interface for bundle storage
 */
class BundleStore : public Object
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
  BundleStore ();
  
  /**
   * \brief Destructor
   */
  virtual ~BundleStore ();
  
  /**
   * \brief Store a bundle
   * \param bundle Bundle to store
   * \return true if stored successfully
   */
  virtual bool Push (Ptr<Bundle> bundle) = 0;
  
  /**
   * \brief Get a bundle by ID
   * \param id Bundle ID
   * \return Bundle or empty optional if not found
   */
  virtual std::optional<Ptr<Bundle>> Get (const BundleID& id) const = 0;
  
  /**
   * \brief Check if a bundle exists
   * \param id Bundle ID
   * \return true if bundle exists
   */
  virtual bool Has (const BundleID& id) const = 0;
  
  /**
   * \brief Remove a bundle
   * \param id Bundle ID
   * \return true if bundle was removed
   */
  virtual bool Remove (const BundleID& id) = 0;
  
  /**
   * \brief Get all stored bundles
   * \return Vector of all bundles
   */
  virtual std::vector<Ptr<Bundle>> GetAll () const = 0;
  
  /**
   * \brief Query bundles that match a predicate
   * \param predicate Function to filter bundles
   * \return Vector of matching bundles
   */
  virtual std::vector<Ptr<Bundle>> Query (std::function<bool(Ptr<Bundle>)> predicate) const = 0;
  
  /**
   * \brief Get the number of stored bundles
   * \return Number of bundles
   */
  virtual size_t Count () const = 0;
  
  /**
   * \brief Clean up expired bundles
   * \return Number of bundles cleaned up
   */
  virtual size_t Cleanup () = 0;
  
  /**
   * \brief Get statistics
   * \return Statistics string
   */
  virtual std::string GetStats () const = 0;
};

} // namespace dtn7

} // namespace ns3

#endif /* DTN7_BUNDLE_STORE_H */