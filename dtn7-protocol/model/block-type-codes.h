#ifndef DTN7_BLOCK_TYPE_CODES_H
#define DTN7_BLOCK_TYPE_CODES_H

#include <cstdint>
#include <string>

namespace ns3 {

namespace dtn7 {

/**
 * \brief Block types as defined in RFC 9171
 */
enum class BlockType : uint64_t {
  PAYLOAD_BLOCK = 1,               //!< Bundle payload
  PREVIOUS_NODE_BLOCK = 6,         //!< Previous node endpoint
  BUNDLE_AGE_BLOCK = 7,            //!< Bundle age
  HOP_COUNT_BLOCK = 10             //!< Hop count
};

/**
 * \brief Convert block type to string
 * \param type Block type
 * \return String representation
 */
std::string BlockTypeToString(BlockType type);

/**
 * \brief Convert string to block type
 * \param typeStr String representation
 * \return Block type
 */
BlockType BlockTypeFromString(const std::string& typeStr);

/**
 * \brief CRC types as defined in RFC 9171
 */
enum class CRCType : uint64_t {
  NO_CRC = 0,                      //!< No CRC
  CRC_16 = 1,                      //!< 16-bit CRC
  CRC_32 = 2                       //!< 32-bit CRC
};

/**
 * \brief Convert CRC type to string
 * \param type CRC type
 * \return String representation
 */
std::string CRCTypeToString(CRCType type);

/**
 * \brief Convert string to CRC type
 * \param typeStr String representation
 * \return CRC type
 */
CRCType CRCTypeFromString(const std::string& typeStr);

} // namespace dtn7

} // namespace ns3

#endif /* DTN7_BLOCK_TYPE_CODES_H */