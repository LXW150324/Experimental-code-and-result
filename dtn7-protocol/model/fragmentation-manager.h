#ifndef DTN7_FRAGMENTATION_MANAGER_H
#define DTN7_FRAGMENTATION_MANAGER_H

#include "ns3/object.h"
#include "ns3/ptr.h"
#include "ns3/simple-ref-count.h"
#include "ns3/log.h"

#include <unordered_map>
#include <vector>
#include <mutex>

#include "bundle.h"
#include "bundle-id.h"

namespace ns3 {

namespace dtn7 {

/**
 * \brief Fragment data structure to track fragment information
 */
struct FragmentInfo
{
  BundleID sourceId;                //!< Original bundle ID
  uint64_t totalLength;            //!< Total application data unit length
  std::vector<Ptr<Bundle>> fragments; //!< Received fragments
  Time expirationTime;             //!< When fragments expire
  bool complete;                   //!< Whether all fragments have been received
};

/**
 * \ingroup dtn7
 * \brief Manager for bundle fragmentation and reassembly
 */
class FragmentationManager : public SimpleRefCount<FragmentationManager>
{
public:
  /**
   * \brief Default constructor
   */
  FragmentationManager ();
  
  /**
   * \brief Destructor
   */
  virtual ~FragmentationManager ();

  /**
   * \brief Fragment a bundle into smaller bundles
   * \param bundle Bundle to fragment
   * \param maxFragmentSize Maximum size of each fragment
   * \return Vector of fragment bundles
   */
  std::vector<Ptr<Bundle>> FragmentBundle (Ptr<Bundle> bundle, size_t maxFragmentSize);
  
  /**
   * \brief Add a fragment to the reassembly process
   * \param fragment Fragment to add
   * \return Reassembled bundle if complete, nullptr otherwise
   */
  Ptr<Bundle> AddFragment (Ptr<Bundle> fragment);
  
  /**
   * \brief Cleanup expired fragments
   * \return Number of fragment sets cleaned up
   */
  size_t CleanupExpiredFragments ();
  
  /**
   * \brief Get statistics about fragmentation
   * \return Statistics string
   */
  std::string GetStats () const;
  
  /**
   * \brief Check if a bundle is a fragment
   * \param bundle Bundle to check
   * \return true if bundle is a fragment
   */
  static bool IsFragment (Ptr<Bundle> bundle);
  
  /**
   * \brief Get the original bundle ID for a fragment
   * \param fragment Fragment bundle
   * \return Original bundle ID
   */
  static BundleID GetOriginalBundleId (Ptr<Bundle> fragment);

private:
  std::unordered_map<BundleID, FragmentInfo> m_fragmentSets; //!< Fragment sets by original bundle ID
  mutable std::mutex m_mutex;                          //!< Mutex for thread safety
  uint64_t m_fragmentedBundles;                       //!< Number of bundles fragmented
  uint64_t m_createdFragments;                        //!< Number of fragments created
  uint64_t m_reassembledBundles;                      //!< Number of bundles reassembled
  uint64_t m_abandonedFragmentSets;                   //!< Number of abandoned fragment sets
  
  /**
   * \brief Try to reassemble fragments into a complete bundle
   * \param info Fragment info struct
   * \return Reassembled bundle if complete, nullptr otherwise
   */
  Ptr<Bundle> TryReassemble (FragmentInfo& info);
  
  /**
   * \brief Calculate fragment payload size
   * \param bundle Original bundle
   * \param maxFragmentSize Maximum size of each fragment
   * \param fragmentIndex Fragment index
   * \param totalFragments Total number of fragments
   * \return Size of fragment payload in bytes
   */
  size_t CalculateFragmentPayloadSize (Ptr<Bundle> bundle, size_t maxFragmentSize, 
                                     size_t fragmentIndex, size_t totalFragments);
};

} // namespace dtn7

} // namespace ns3

#endif /* DTN7_FRAGMENTATION_MANAGER_H */