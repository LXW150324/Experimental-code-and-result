#ifndef DTN7_BUNDLE_H
#define DTN7_BUNDLE_H

#include "ns3/object.h"
#include "ns3/ptr.h"
#include "ns3/buffer.h"
#include "ns3/packet.h"
#include "ns3/simple-ref-count.h"

#include <vector>
#include <memory>
#include <string>
#include <optional>

#include "endpoint.h"
#include "primary-block.h"
#include "canonical-block.h"
#include "bundle-id.h"

namespace ns3 {

namespace dtn7 {

/**
 * \ingroup dtn7
 * \brief A Bundle Protocol 7 bundle as defined in RFC 9171
 */
class Bundle : public SimpleRefCount<Bundle>
{
public:
  /**
   * \brief Default constructor
   */
  Bundle ();

  /**
   * \brief Constructor with primary block
   * \param primaryBlock The primary block of the bundle
   */
  explicit Bundle (const PrimaryBlock& primaryBlock);
  
  /**
   * \brief Create a new bundle
   * \param primaryBlock The primary block
   * \param canonicalBlocks The canonical blocks
   * \return A new bundle object
   */
  static Bundle NewBundle (const PrimaryBlock& primaryBlock, 
                          const std::vector<Ptr<CanonicalBlock>>& canonicalBlocks);
  
  /**
   * \brief Create a new bundle with validation
   * \param source Source endpoint
   * \param destination Destination endpoint
   * \param creationTimestamp Creation timestamp
   * \param lifetime Bundle lifetime
   * \param payload Payload data
   * \return A new bundle object
   */
  static Bundle MustNewBundle (const std::string& source,
                              const std::string& destination,
                              const DtnTime& creationTimestamp,
                              const Time& lifetime,
                              const std::vector<uint8_t>& payload);
  
  /**
   * \brief Get the primary block
   * \return Reference to primary block
   */
  PrimaryBlock& GetPrimaryBlock () { return m_primaryBlock; }
  
  /**
   * \brief Get the primary block (const)
   * \return Const reference to primary block
   */
  const PrimaryBlock& GetPrimaryBlock () const { return m_primaryBlock; }
  
  /**
   * \brief Get the canonical blocks
   * \return Reference to vector of canonical blocks
   */
  std::vector<Ptr<CanonicalBlock>>& GetCanonicalBlocks () { return m_canonicalBlocks; }
  
  /**
   * \brief Get the canonical blocks (const)
   * \return Const reference to vector of canonical blocks
   */
  const std::vector<Ptr<CanonicalBlock>>& GetCanonicalBlocks () const { return m_canonicalBlocks; }
  
  /**
   * \brief Get the bundle ID
   * \return Bundle ID
   */
  BundleID GetId () const;
  
  /**
   * \brief Get string representation
   * \return String representation of the bundle
   */
  std::string ToString () const;
  
  /**
   * \brief Check if bundle is an administrative record
   * \return true if bundle is an administrative record
   */
  bool IsAdministrativeRecord () const;
  
  /**
   * \brief Check if bundle is a fragment
   * \return true if bundle is a fragment
   */
  bool IsFragment () const;
  
  /**
   * \brief Add a block to the bundle
   * \param block Block to add
   */
  void AddBlock (Ptr<CanonicalBlock> block);
  
  /**
   * \brief Get block by type
   * \param blockType Block type to find
   * \return Pointer to block or nullptr if not found
   */
  Ptr<CanonicalBlock> GetBlockByType (BlockType blockType) const;
  
  /**
   * \brief Get all blocks of a specific type
   * \param blockType Block type to find
   * \return Vector of blocks matching the type
   */
  std::vector<Ptr<CanonicalBlock>> GetBlocksByType (BlockType blockType) const;
  
  /**
   * \brief Remove a block by type
   * \param blockType Block type to remove
   * \return true if block was removed
   */
  bool RemoveBlockByType (BlockType blockType);
  
  /**
   * \brief Get the payload block
   * \return Pointer to payload block or nullptr if not found
   */
  Ptr<CanonicalBlock> GetPayloadBlock () const;
  
  /**
   * \brief Get the payload data
   * \return Payload data
   */
  std::vector<uint8_t> GetPayload () const;
  
  /**
   * \brief Set the payload data
   * \param payload Payload data
   */
  void SetPayload (const std::vector<uint8_t>& payload);
  
  /**
   * \brief Calculate CRC for all blocks
   */
  void CalculateCRC ();
  
  /**
   * \brief Check CRC for all blocks
   * \return true if all CRCs are valid
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
   * \return Deserialized bundle
   */
  static std::optional<Bundle> FromCbor (Buffer buffer);
  
  /**
   * \brief Fragment a bundle into smaller bundles
   * \param maxFragmentSize Maximum size of each fragment
   * \return Vector of fragment bundles
   */
  std::vector<Bundle> Fragment (size_t maxFragmentSize) const;

private:
  PrimaryBlock m_primaryBlock;                      //!< Primary block
  std::vector<Ptr<CanonicalBlock>> m_canonicalBlocks;  //!< Canonical blocks
};

} // namespace dtn7

} // namespace ns3

#endif /* DTN7_BUNDLE_H */