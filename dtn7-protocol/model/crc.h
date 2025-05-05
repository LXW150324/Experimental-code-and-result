#ifndef DTN7_CRC_H
#define DTN7_CRC_H

#include <cstdint>
#include <vector>
#include "block-type-codes.h"

namespace ns3 {

namespace dtn7 {

/**
 * \brief Calculate CRC-16-CCITT
 * \param data Data to calculate CRC for
 * \return 16-bit CRC value
 */
uint16_t CalculateCRC16(const std::vector<uint8_t>& data);

/**
 * \brief Calculate CRC-32C (Castagnoli)
 * \param data Data to calculate CRC for
 * \return 32-bit CRC value
 */
uint32_t CalculateCRC32(const std::vector<uint8_t>& data);

/**
 * \brief Calculate CRC based on type
 * \param type CRC type
 * \param data Data to calculate CRC for
 * \return CRC value as byte vector
 */
std::vector<uint8_t> CalculateCRC(CRCType type, const std::vector<uint8_t>& data);

/**
 * \brief Verify CRC
 * \param type CRC type
 * \param data Data to verify CRC for
 * \param crc CRC value to check against
 * \return true if CRC is valid
 */
bool VerifyCRC(CRCType type, const std::vector<uint8_t>& data, const std::vector<uint8_t>& crc);

} // namespace dtn7

} // namespace ns3

#endif /* DTN7_CRC_H */