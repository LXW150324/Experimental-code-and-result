#ifndef DTN7_PRIMARY_BLOCK_H
#define DTN7_PRIMARY_BLOCK_H

#include "ns3/nstime.h"
#include "ns3/buffer.h"

#include <cstdint>
#include <vector>
#include <optional>

#include "endpoint.h"
#include "dtn-time.h"
#include "block-type-codes.h"

namespace ns3 {

namespace dtn7 {

/**
 * \brief Bundle control flags as defined in RFC 9171
 */
enum class BundleControlFlags : uint64_t {
  NO_FLAGS = 0,
  BUNDLE_DELETION_STATUS_REPORTS_REQUESTED = 1 << 0,
  BUNDLE_DELIVERY_STATUS_REPORTS_REQUESTED = 1 << 1,
  BUNDLE_FORWARDING_STATUS_REPORTS_REQUESTED = 1 << 2,
  BUNDLE_RECEPTION_STATUS_REPORTS_REQUESTED = 1 << 3,
  BUNDLE_MUST_NOT_BE_FRAGMENTED = 1 << 4,
  PAYLOAD_IS_AN_ADMINISTRATIVE_RECORD = 1 << 5,
  BUNDLE_IS_A_FRAGMENT = 1 << 6
};

/**
 * \brief Bitwise OR operator for BundleControlFlags
 */
inline BundleControlFlags operator| (BundleControlFlags a, BundleControlFlags b)
{
  return static_cast<BundleControlFlags>(static_cast<uint64_t>(a) | static_cast<uint64_t>(b));
}

/**
 * \brief Bitwise AND operator for BundleControlFlags
 */
inline BundleControlFlags operator& (BundleControlFlags a, BundleControlFlags b)
{
  return static_cast<BundleControlFlags>(static_cast<uint64_t>(a) & static_cast<uint64_t>(b));
}

/**
 * \ingroup dtn7
 * \brief Primary block of a bundle as defined in RFC 9171
 */
class PrimaryBlock
{
public:
  static constexpr uint64_t DEFAULT_VERSION = 7;  //!< BP Version 7
  
  /**
   * \brief Default constructor
   */
  PrimaryBlock ();
  
  /**
   * \brief Constructor with parameters
   * \param version BP version
   * \param bundleControlFlags Control flags
   * \param crcType CRC type
   * \param destinationEID Destination endpoint ID
   * \param sourceNodeEID Source node endpoint ID
   * \param reportToEID Report-to endpoint ID
   * \param creationTimestamp Creation timestamp
   * \param sequenceNumber Sequence number
   * \param lifetime Bundle lifetime
   * \param fragmentOffset Fragment offset (if fragment)
   * \param totalApplicationDataUnitLength Total ADU length (if fragment)
   */
  PrimaryBlock (uint64_t version,
               BundleControlFlags bundleControlFlags,
               CRCType crcType,
               EndpointID destinationEID,
               EndpointID sourceNodeEID,
               EndpointID reportToEID,
               DtnTime creationTimestamp,
               uint64_t sequenceNumber,
               Time lifetime,
               uint64_t fragmentOffset = 0,
               uint64_t totalApplicationDataUnitLength = 0);
  
  // Getters
  uint64_t GetVersion () const { return m_version; }
  BundleControlFlags GetBundleControlFlags () const { return m_bundleControlFlags; }
  CRCType GetCRCType () const { return m_crcType; }
  const EndpointID& GetDestinationEID () const { return m_destinationEID; }
  const EndpointID& GetSourceNodeEID () const { return m_sourceNodeEID; }
  const EndpointID& GetReportToEID () const { return m_reportToEID; }
  DtnTime GetCreationTimestamp () const { return m_creationTimestamp; }
  uint64_t GetSequenceNumber () const { return m_sequenceNumber; }
  Time GetLifetime () const { return m_lifetime; }
  uint64_t GetFragmentOffset () const { return m_fragmentOffset; }
  uint64_t GetTotalApplicationDataUnitLength () const { return m_totalApplicationDataUnitLength; }
  
  // Setters
  void SetVersion (uint64_t version) { m_version = version; }
  void SetBundleControlFlags (BundleControlFlags flags) { m_bundleControlFlags = flags; }
  void SetCRCType (CRCType crcType) { m_crcType = crcType; }
  void SetDestinationEID (const EndpointID& eid) { m_destinationEID = eid; }
  void SetSourceNodeEID (const EndpointID& eid) { m_sourceNodeEID = eid; }
  void SetReportToEID (const EndpointID& eid) { m_reportToEID = eid; }
  void SetCreationTimestamp (DtnTime timestamp) { m_creationTimestamp = timestamp; }
  void SetSequenceNumber (uint64_t seqNum) { m_sequenceNumber = seqNum; }
  void SetLifetime (Time lifetime) { m_lifetime = lifetime; }
  void SetFragmentOffset (uint64_t offset) { m_fragmentOffset = offset; }
  void SetTotalApplicationDataUnitLength (uint64_t length) { m_totalApplicationDataUnitLength = length; }
  
  /**
   * \brief Check if bundle has fragmentation fields
   * \return true if has fragmentation fields
   */
  bool HasFragmentation () const;
  
  /**
   * \brief Check if bundle must not be fragmented
   * \return true if must not be fragmented
   */
  bool MustNotFragment () const;
  
  /**
   * \brief Check if bundle is a fragment
   * \return true if bundle is a fragment
   */
  bool IsFragment () const;
  
  /**
   * \brief Check if bundle is an administrative record
   * \return true if bundle is an administrative record
   */
  bool IsAdministrativeRecord () const;
  
  /**
   * \brief Check if bundle requests deletion status reports
   * \return true if deletion reports requested
   */
  bool RequestsBundleDeletionStatusReport () const;
  
  /**
   * \brief Check if bundle requests delivery status reports
   * \return true if delivery reports requested
   */
  bool RequestsBundleDeliveryStatusReport () const;
  
  /**
   * \brief Check if bundle requests forwarding status reports
   * \return true if forwarding reports requested
   */
  bool RequestsBundleForwardingStatusReport () const;
  
  /**
   * \brief Check if bundle requests reception status reports
   * \return true if reception reports requested
   */
  bool RequestsBundleReceptionStatusReport () const;
  
  /**
   * \brief Set bundle deletion status report flag
   * \param enable Enable or disable flag
   */
  void SetBundleDeletionStatusReport (bool enable);
  
  /**
   * \brief Set bundle delivery status report flag
   * \param enable Enable or disable flag
   */
  void SetBundleDeliveryStatusReport (bool enable);
  
  /**
   * \brief Set bundle forwarding status report flag
   * \param enable Enable or disable flag
   */
  void SetBundleForwardingStatusReport (bool enable);
  
  /**
   * \brief Set bundle reception status report flag
   * \param enable Enable or disable flag
   */
  void SetBundleReceptionStatusReport (bool enable);
  
  /**
   * \brief Set fragmentation flag
   * \param enable Enable or disable flag
   */
  void SetFragmentation (bool enable);
  
  /**
   * \brief Set administrative record flag
   * \param enable Enable or disable flag
   */
  void SetAdministrativeRecord (bool enable);
  
  /**
   * \brief Set must not fragment flag
   * \param enable Enable or disable flag
   */
  void SetMustNotFragment (bool enable);
  
  /**
   * \brief Calculate CRC
   */
  void CalculateCRC ();
  
  /**
   * \brief Check CRC
   * \return true if CRC is valid
   */
  bool CheckCRC () const;
  
  /**
   * \brief Serialize to CBOR format
   * \return CBOR encoded data
   */
  Buffer ToCbor () const;
  
  /**
   * \brief Deserialize from CBOR format
   * \param buffer CBOR encoded data
   * \return Deserialized primary block
   */
  static std::optional<PrimaryBlock> FromCbor (Buffer buffer);
  
  /**
   * \brief Get string representation
   * \return String representation of primary block
   */
  std::string ToString () const;

private:
  uint64_t m_version;                               //!< BP version number
  BundleControlFlags m_bundleControlFlags;          //!< Bundle control flags
  CRCType m_crcType;                                //!< CRC type
  EndpointID m_destinationEID;                      //!< Destination endpoint ID
  EndpointID m_sourceNodeEID;                       //!< Source node endpoint ID
  EndpointID m_reportToEID;                         //!< Report-to endpoint ID
  DtnTime m_creationTimestamp;                      //!< Creation timestamp
  uint64_t m_sequenceNumber;                        //!< Sequence number
  Time m_lifetime;                                  //!< Bundle lifetime
  uint64_t m_fragmentOffset;                        //!< Fragment offset
  uint64_t m_totalApplicationDataUnitLength;        //!< Total ADU length
  std::vector<uint8_t> m_crcValue;                  //!< CRC value
};

} // namespace dtn7

} // namespace ns3

#endif /* DTN7_PRIMARY_BLOCK_H */