#ifndef DTN7_CANONICAL_BLOCK_H
#define DTN7_CANONICAL_BLOCK_H

#include "ns3/simple-ref-count.h"
#include "ns3/buffer.h"

#include <cstdint>
#include <vector>
#include <string>
#include <memory>
#include <optional>

#include "block-type-codes.h"
#include "endpoint.h"
#include "ns3/ptr.h"
namespace ns3 {

namespace dtn7 {

/**
 * \brief Block control flags as defined in RFC 9171
 */
enum class BlockControlFlags : uint64_t {
  NO_FLAGS = 0,
  BLOCK_MUST_BE_REPLICATED = 1 << 0,
  REPORT_BLOCK_IF_UNPROCESSABLE = 1 << 1,
  DELETE_BUNDLE_IF_BLOCK_UNPROCESSABLE = 1 << 2,
  REMOVE_BLOCK_IF_UNPROCESSABLE = 1 << 3,
  STATUS_REPORT_REQUESTED = 1 << 4
};

/**
 * \brief Bitwise OR operator for BlockControlFlags
 */
inline BlockControlFlags operator| (BlockControlFlags a, BlockControlFlags b)
{
  return static_cast<BlockControlFlags>(static_cast<uint64_t>(a) | static_cast<uint64_t>(b));
}

/**
 * \brief Bitwise AND operator for BlockControlFlags
 */
inline BlockControlFlags operator& (BlockControlFlags a, BlockControlFlags b)
{
  return static_cast<BlockControlFlags>(static_cast<uint64_t>(a) & static_cast<uint64_t>(b));
}

/**
 * \ingroup dtn7
 * \brief Base class for canonical blocks in a bundle
 */
class CanonicalBlock : public SimpleRefCount<CanonicalBlock>
{
public:
  /**
   * \brief Default constructor
   */
  CanonicalBlock ();
  
  /**
   * \brief Constructor with parameters
   * \param blockType Block type
   * \param blockNumber Block number
   * \param blockControlFlags Block control flags
   * \param crcType CRC type
   * \param data Block data
   */
  CanonicalBlock (BlockType blockType,
                 uint64_t blockNumber,
                 BlockControlFlags blockControlFlags,
                 CRCType crcType,
                 const std::vector<uint8_t>& data);
  
  /**
   * \brief Virtual destructor
   */
  virtual ~CanonicalBlock () = default;
  
  /**
   * \brief Factory method for creating different block types
   * \param blockType Block type
   * \param blockNumber Block number
   * \param blockControlFlags Block control flags
   * \param crcType CRC type
   * \param data Block data
   * \return Pointer to created block
   */
  static Ptr<CanonicalBlock> CreateBlock (BlockType blockType,
                                         uint64_t blockNumber,
                                         BlockControlFlags blockControlFlags,
                                         CRCType crcType,
                                         const std::vector<uint8_t>& data);
  
  // Getters
  BlockType GetBlockType () const { return m_blockType; }
  uint64_t GetBlockNumber () const { return m_blockNumber; }
  BlockControlFlags GetBlockControlFlags () const { return m_blockControlFlags; }
  CRCType GetCRCType () const { return m_crcType; }
  const std::vector<uint8_t>& GetData () const { return m_data; }
  
  // Setters
  void SetBlockType (BlockType type) { m_blockType = type; }
  void SetBlockNumber (uint64_t number) { m_blockNumber = number; }
  void SetBlockControlFlags (BlockControlFlags flags) { m_blockControlFlags = flags; }
  void SetCRCType (CRCType type) { m_crcType = type; }
  void SetData (const std::vector<uint8_t>& data) { m_data = data; }
  
  /**
   * \brief Check if block must be replicated
   * \return true if block must be replicated
   */
  bool MustBeReplicated () const;
  
  /**
   * \brief Check if block should be reported if unprocessable
   * \return true if block should be reported
   */
  bool ReportIfUnprocessable () const;
  
  /**
   * \brief Check if bundle should be deleted if block is unprocessable
   * \return true if bundle should be deleted
   */
  bool DeleteBundleIfUnprocessable () const;
  
  /**
   * \brief Check if block should be removed if unprocessable
   * \return true if block should be removed
   */
  bool RemoveBlockIfUnprocessable () const;
  
  /**
   * \brief Check if status report is requested
   * \return true if status report is requested
   */
  bool StatusReportRequested () const;
  
  /**
   * \brief Set must be replicated flag
   * \param enable Enable or disable flag
   */
  void SetMustBeReplicated (bool enable);
  
  /**
   * \brief Set report if unprocessable flag
   * \param enable Enable or disable flag
   */
  void SetReportIfUnprocessable (bool enable);
  
  /**
   * \brief Set delete bundle if unprocessable flag
   * \param enable Enable or disable flag
   */
  void SetDeleteBundleIfUnprocessable (bool enable);
  
  /**
   * \brief Set remove block if unprocessable flag
   * \param enable Enable or disable flag
   */
  void SetRemoveBlockIfUnprocessable (bool enable);
  
  /**
   * \brief Set status report requested flag
   * \param enable Enable or disable flag
   */
  void SetStatusReportRequested (bool enable);
  
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
  virtual Buffer ToCbor () const;
  
  /**
   * \brief Deserialize from CBOR format
   * \param buffer CBOR encoded data
   * \return Deserialized block
   */
  static std::optional<Ptr<CanonicalBlock>> FromCbor (Buffer buffer);
  
  /**
   * \brief Get string representation
   * \return String representation of the block
   */
  virtual std::string ToString () const;

private:
  BlockType m_blockType;                 //!< Block type
  uint64_t m_blockNumber;                //!< Block number
  BlockControlFlags m_blockControlFlags; //!< Block control flags
  CRCType m_crcType;                     //!< CRC type
  std::vector<uint8_t> m_data;           //!< Block data
  std::vector<uint8_t> m_crcValue;       //!< CRC value
};

/**
 * \ingroup dtn7
 * \brief Payload block
 */
class PayloadBlock : public CanonicalBlock
{
public:
  /**
   * \brief Default constructor
   */
  PayloadBlock ();
  
  /**
   * \brief Constructor with payload
   * \param payload Payload data
   */
  explicit PayloadBlock (const std::vector<uint8_t>& payload);
  
  /**
   * \brief Constructor with parameters
   * \param blockNumber Block number
   * \param blockControlFlags Block control flags
   * \param crcType CRC type
   * \param payload Payload data
   */
  PayloadBlock (uint64_t blockNumber,
               BlockControlFlags blockControlFlags,
               CRCType crcType,
               const std::vector<uint8_t>& payload);
  
  /**
   * \brief Get payload data
   * \return Payload data
   */
  std::vector<uint8_t> GetPayload () const;
  
  /**
   * \brief Set payload data
   * \param payload Payload data
   */
  void SetPayload (const std::vector<uint8_t>& payload);
  
  /**
   * \brief Get string representation
   * \return String representation of the payload block
   */
  std::string ToString () const override;
};

/**
 * \ingroup dtn7
 * \brief Previous node block
 */
class PreviousNodeBlock : public CanonicalBlock
{
public:
  /**
   * \brief Default constructor
   */
  PreviousNodeBlock ();
  
  /**
   * \brief Constructor with previous node
   * \param previousNode Previous node endpoint ID
   */
  explicit PreviousNodeBlock (const EndpointID& previousNode);
  
  /**
   * \brief Constructor with parameters
   * \param blockNumber Block number
   * \param blockControlFlags Block control flags
   * \param crcType CRC type
   * \param previousNode Previous node endpoint ID
   */
  PreviousNodeBlock (uint64_t blockNumber,
                    BlockControlFlags blockControlFlags,
                    CRCType crcType,
                    const EndpointID& previousNode);
  
  /**
   * \brief Get previous node
   * \return Previous node endpoint ID
   */
  EndpointID GetPreviousNode () const;
  
  /**
   * \brief Set previous node
   * \param previousNode Previous node endpoint ID
   */
  void SetPreviousNode (const EndpointID& previousNode);
  
  /**
   * \brief Get string representation
   * \return String representation of the previous node block
   */
  std::string ToString () const override;
};

/**
 * \ingroup dtn7
 * \brief Bundle age block
 */
class BundleAgeBlock : public CanonicalBlock
{
public:
  /**
   * \brief Default constructor
   */
  BundleAgeBlock ();
  
  /**
   * \brief Constructor with age
   * \param microseconds Age in microseconds
   */
  explicit BundleAgeBlock (uint64_t microseconds);
  
  /**
   * \brief Constructor with parameters
   * \param blockNumber Block number
   * \param blockControlFlags Block control flags
   * \param crcType CRC type
   * \param microseconds Age in microseconds
   */
  BundleAgeBlock (uint64_t blockNumber,
                 BlockControlFlags blockControlFlags,
                 CRCType crcType,
                 uint64_t microseconds);
  
  /**
   * \brief Get age
   * \return Age in microseconds
   */
  uint64_t GetAge () const;
  
  /**
   * \brief Set age
   * \param microseconds Age in microseconds
   */
  void SetAge (uint64_t microseconds);
  
  /**
   * \brief Get string representation
   * \return String representation of the bundle age block
   */
  std::string ToString () const override;
};

/**
 * \ingroup dtn7
 * \brief Hop count block
 */
class HopCountBlock : public CanonicalBlock
{
public:
  /**
   * \brief Default constructor
   */
  HopCountBlock ();
  
  /**
   * \brief Constructor with limit and count
   * \param limit Hop limit
   * \param count Current hop count
   */
  HopCountBlock (uint64_t limit, uint64_t count);
  
  /**
   * \brief Constructor with parameters
   * \param blockNumber Block number
   * \param blockControlFlags Block control flags
   * \param crcType CRC type
   * \param limit Hop limit
   * \param count Current hop count
   */
  HopCountBlock (uint64_t blockNumber,
                BlockControlFlags blockControlFlags,
                CRCType crcType,
                uint64_t limit,
                uint64_t count);
  
  /**
   * \brief Get hop limit
   * \return Hop limit
   */
  uint64_t GetLimit () const;
  
  /**
   * \brief Get hop count
   * \return Current hop count
   */
  uint64_t GetCount () const;
  
  /**
   * \brief Set hop limit
   * \param limit Hop limit
   */
  void SetLimit (uint64_t limit);
  
  /**
   * \brief Set hop count
   * \param count Current hop count
   */
  void SetCount (uint64_t count);
  
  /**
   * \brief Increment hop count
   */
  void Increment ();
  
  /**
   * \brief Check if hop limit is exceeded
   * \return true if hop limit is exceeded
   */
  bool Exceeded () const;
  
  /**
   * \brief Get string representation
   * \return String representation of the hop count block
   */
  std::string ToString () const override;
};

} // namespace dtn7

} // namespace ns3

#endif /* DTN7_CANONICAL_BLOCK_H */