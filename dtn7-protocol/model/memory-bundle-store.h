#ifndef DTN7_MEMORY_BUNDLE_STORE_H
#define DTN7_MEMORY_BUNDLE_STORE_H

#include "bundle-store.h"

#include <unordered_map>
#include <mutex>
#include <chrono>

namespace ns3 {

namespace dtn7 {

/**
 * \ingroup dtn7
 * \brief In-memory implementation of bundle storage
 */
class MemoryBundleStore : public BundleStore
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
  MemoryBundleStore ();
  
  /**
   * \brief Destructor
   */
  virtual ~MemoryBundleStore ();
  
  // Inherited from BundleStore
  bool Push (Ptr<Bundle> bundle) override;
  std::optional<Ptr<Bundle>> Get (const BundleID& id) const override;
  bool Has (const BundleID& id) const override;
  bool Remove (const BundleID& id) override;
  std::vector<Ptr<Bundle>> GetAll () const override;
  std::vector<Ptr<Bundle>> Query (std::function<bool(Ptr<Bundle>)> predicate) const override;
  size_t Count () const override;
  size_t Cleanup () override;
  std::string GetStats () const override;

private:
  std::unordered_map<BundleID, Ptr<Bundle>> m_bundles;  //!< Stored bundles
  mutable std::mutex m_mutex;                           //!< Mutex for thread safety
  size_t m_pushCount;                                   //!< Number of pushed bundles
  size_t m_getCount;                                    //!< Number of retrieved bundles
  size_t m_removeCount;                                 //!< Number of removed bundles
  Time m_creationTime;                                  //!< Store creation time
  
  /**
   * \brief Check if a bundle is expired
   * \param bundle Bundle to check
   * \return true if bundle is expired
   */
  bool IsExpired (Ptr<Bundle> bundle) const;
};

} // namespace dtn7

} // namespace ns3

#endif /* DTN7_MEMORY_BUNDLE_STORE_H */