#ifndef DTN7_BUNDLE_ID_H
#define DTN7_BUNDLE_ID_H

#include <cstdint>
#include <string>
#include <functional> // 添加这行来支持哈希函数
#include "endpoint.h"
#include "dtn-time.h"

namespace ns3 {

namespace dtn7 {

/**
 * \ingroup dtn7
 * \brief BundleID uniquely identifies a bundle
 * 
 * Identification is based on source endpoint, creation timestamp
 * sequence number, and fragment information if applicable.
 */
class BundleID
{
public:
  /**
   * \brief Default constructor
   */
  BundleID ();
  
  /**
   * \brief Constructor with parameters
   * \param source Source endpoint
   * \param timestamp Creation timestamp
   * \param sequenceNumber Sequence number
   * \param isFragment Whether bundle is a fragment
   * \param fragmentOffset Fragment offset
   */
  BundleID (const EndpointID& source, 
           DtnTime timestamp, 
           uint64_t sequenceNumber, 
           bool isFragment = false, 
           uint64_t fragmentOffset = 0);
  
  /**
   * \brief Equality operator
   * \param other Other bundle ID
   * \return true if IDs are equal
   */
  bool operator== (const BundleID& other) const;
  
  /**
   * \brief Inequality operator
   * \param other Other bundle ID
   * \return true if IDs are not equal
   */
  bool operator!= (const BundleID& other) const;
  
  /**
   * \brief Less-than operator for sorting
   * \param other Other bundle ID
   * \return true if this bundle ID is less than the other
   */
  bool operator< (const BundleID& other) const;
  
  /**
   * \brief Get the source endpoint
   * \return Source endpoint
   */
  const EndpointID& GetSource () const { return m_source; }
  
  /**
   * \brief Get the creation timestamp
   * \return Creation timestamp
   */
  DtnTime GetTimestamp () const { return m_timestamp; }
  
  /**
   * \brief Get the sequence number
   * \return Sequence number
   */
  uint64_t GetSequenceNumber () const { return m_sequenceNumber; }
  
  /**
   * \brief Check if bundle is a fragment
   * \return true if bundle is a fragment
   */
  bool IsFragment () const { return m_isFragment; }
  
  /**
   * \brief Get the fragment offset
   * \return Fragment offset
   */
  uint64_t GetFragmentOffset () const { return m_fragmentOffset; }
  
  /**
   * \brief Get string representation
   * \return String representation
   */
  std::string ToString () const;
  
  /**
   * \brief Calculate hash for use in hash maps
   * \return Hash value
   */
  size_t Hash () const;

private:
  EndpointID m_source;             //!< Source endpoint
  DtnTime m_timestamp;             //!< Creation timestamp
  uint64_t m_sequenceNumber;       //!< Sequence number
  bool m_isFragment;               //!< Whether bundle is a fragment
  uint64_t m_fragmentOffset;       //!< Fragment offset
};

} // namespace dtn7

} // namespace ns3

// Hash function for ns3::dtn7::BundleID in std hash maps
namespace std {
  template<>
  struct hash<ns3::dtn7::BundleID> {
    size_t operator() (const ns3::dtn7::BundleID& id) const {
      return id.Hash();
    }
  };
}

#endif /* DTN7_BUNDLE_ID_H */