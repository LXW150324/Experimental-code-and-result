#include "block-type-codes.h"

namespace ns3 {

namespace dtn7 {

std::string 
BlockTypeToString(BlockType type)
{
  switch (type)
    {
      case BlockType::PAYLOAD_BLOCK:
        return "PayloadBlock";
      case BlockType::PREVIOUS_NODE_BLOCK:
        return "PreviousNodeBlock";
      case BlockType::BUNDLE_AGE_BLOCK:
        return "BundleAgeBlock";
      case BlockType::HOP_COUNT_BLOCK:
        return "HopCountBlock";
      default:
        return "UnknownBlock_" + std::to_string(static_cast<uint64_t>(type));
    }
}

BlockType 
BlockTypeFromString(const std::string& typeStr)
{
  if (typeStr == "PayloadBlock")
    {
      return BlockType::PAYLOAD_BLOCK;
    }
  else if (typeStr == "PreviousNodeBlock")
    {
      return BlockType::PREVIOUS_NODE_BLOCK;
    }
  else if (typeStr == "BundleAgeBlock")
    {
      return BlockType::BUNDLE_AGE_BLOCK;
    }
  else if (typeStr == "HopCountBlock")
    {
      return BlockType::HOP_COUNT_BLOCK;
    }
  // Default to payload block for unknown types
  return BlockType::PAYLOAD_BLOCK;
}

std::string 
CRCTypeToString(CRCType type)
{
  switch (type)
    {
      case CRCType::NO_CRC:
        return "NoCRC";
      case CRCType::CRC_16:
        return "CRC16";
      case CRCType::CRC_32:
        return "CRC32";
      default:
        return "UnknownCRC_" + std::to_string(static_cast<uint64_t>(type));
    }
}

CRCType 
CRCTypeFromString(const std::string& typeStr)
{
  if (typeStr == "NoCRC")
    {
      return CRCType::NO_CRC;
    }
  else if (typeStr == "CRC16")
    {
      return CRCType::CRC_16;
    }
  else if (typeStr == "CRC32")
    {
      return CRCType::CRC_32;
    }
  // Default to no CRC for unknown types
  return CRCType::NO_CRC;
}

} // namespace dtn7

} // namespace ns3