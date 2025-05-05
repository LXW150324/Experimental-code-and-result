#include "crc.h"

namespace ns3 {

namespace dtn7 {

uint16_t 
CalculateCRC16(const std::vector<uint8_t>& data)
{
  // CCITT CRC-16 polynomial: x^16 + x^12 + x^5 + 1
  const uint16_t polynomial = 0x1021;
  uint16_t crc = 0xFFFF; // Initial value
  
  for (uint8_t byte : data)
    {
      crc ^= static_cast<uint16_t>(byte) << 8;
      for (int i = 0; i < 8; ++i)
        {
          if (crc & 0x8000)
            {
              crc = (crc << 1) ^ polynomial;
            }
          else
            {
              crc = crc << 1;
            }
        }
    }
  
  return crc;
}

uint32_t 
CalculateCRC32(const std::vector<uint8_t>& data)
{
  // CRC-32C (Castagnoli) polynomial: x^32 + x^28 + x^27 + x^26 + x^25 + x^23 + x^22 + x^20 + x^19 + x^18 + x^14 + x^13 + x^11 + x^10 + x^9 + x^8 + x^6 + 1
  const uint32_t polynomial = 0x1EDC6F41;
  uint32_t crc = 0xFFFFFFFF; // Initial value
  
  for (uint8_t byte : data)
    {
      crc ^= byte;
      for (int i = 0; i < 8; ++i)
        {
          if (crc & 1)
            {
              crc = (crc >> 1) ^ polynomial;
            }
          else
            {
              crc = crc >> 1;
            }
        }
    }
  
  return ~crc; // Final XOR
}

std::vector<uint8_t> 
CalculateCRC(CRCType type, const std::vector<uint8_t>& data)
{
  std::vector<uint8_t> crc;
  
  switch (type)
    {
      case CRCType::CRC_16:
        {
          uint16_t crc16 = CalculateCRC16(data);
          crc.push_back(static_cast<uint8_t>((crc16 >> 8) & 0xFF));
          crc.push_back(static_cast<uint8_t>(crc16 & 0xFF));
          break;
        }
      case CRCType::CRC_32:
        {
          uint32_t crc32 = CalculateCRC32(data);
          crc.push_back(static_cast<uint8_t>((crc32 >> 24) & 0xFF));
          crc.push_back(static_cast<uint8_t>((crc32 >> 16) & 0xFF));
          crc.push_back(static_cast<uint8_t>((crc32 >> 8) & 0xFF));
          crc.push_back(static_cast<uint8_t>(crc32 & 0xFF));
          break;
        }
      case CRCType::NO_CRC:
      default:
        // Return empty vector for NO_CRC
        break;
    }
  
  return crc;
}

bool 
VerifyCRC(CRCType type, const std::vector<uint8_t>& data, const std::vector<uint8_t>& crc)
{
  if (type == CRCType::NO_CRC)
    {
      return true; // No CRC to verify
    }
  
  std::vector<uint8_t> calculatedCrc = CalculateCRC(type, data);
  
  // Compare CRC values
  if (calculatedCrc.size() != crc.size())
    {
      return false;
    }
  
  for (size_t i = 0; i < crc.size(); ++i)
    {
      if (calculatedCrc[i] != crc[i])
        {
          return false;
        }
    }
  
  return true;
}

} // namespace dtn7

} // namespace ns3