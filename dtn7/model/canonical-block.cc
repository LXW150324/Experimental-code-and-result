#include "canonical-block.h"
#include "cbor.h"
#include "crc.h"
#include <string>
#include <sstream>
#include <vector>

namespace ns3 {

namespace dtn7 {

CanonicalBlock::CanonicalBlock()
  : m_blockType(BlockType::PAYLOAD_BLOCK),
    m_blockNumber(0),
    m_blockControlFlags(BlockControlFlags::NO_FLAGS),
    m_crcType(CRCType::NO_CRC)
{
}

CanonicalBlock::CanonicalBlock(BlockType blockType,
                               uint64_t blockNumber,
                               BlockControlFlags blockControlFlags,
                               CRCType crcType,
                               const std::vector<uint8_t>& data)
  : m_blockType(blockType),
    m_blockNumber(blockNumber),
    m_blockControlFlags(blockControlFlags),
    m_crcType(crcType),
    m_data(data)
{
}

Ptr<CanonicalBlock> 
CanonicalBlock::CreateBlock(BlockType blockType,
                            uint64_t blockNumber,
                            BlockControlFlags blockControlFlags,
                            CRCType crcType,
                            const std::vector<uint8_t>& data)
{
  switch (blockType)
    {
      case BlockType::PAYLOAD_BLOCK:
        return Create<PayloadBlock>(blockNumber, blockControlFlags, crcType, data);
      
      case BlockType::PREVIOUS_NODE_BLOCK:
        {
          Ptr<PreviousNodeBlock> block = Create<PreviousNodeBlock>();
          block->SetBlockNumber(blockNumber);
          block->SetBlockControlFlags(blockControlFlags);
          block->SetCRCType(crcType);
          block->SetData(data);
          return block;
        }
      
      case BlockType::BUNDLE_AGE_BLOCK:
        {
          Ptr<BundleAgeBlock> block = Create<BundleAgeBlock>();
          block->SetBlockNumber(blockNumber);
          block->SetBlockControlFlags(blockControlFlags);
          block->SetCRCType(crcType);
          block->SetData(data);
          return block;
        }
      
      case BlockType::HOP_COUNT_BLOCK:
        {
          Ptr<HopCountBlock> block = Create<HopCountBlock>();
          block->SetBlockNumber(blockNumber);
          block->SetBlockControlFlags(blockControlFlags);
          block->SetCRCType(crcType);
          block->SetData(data);
          return block;
        }
      
      default:
        {
          // Generic block for unknown types
          Ptr<CanonicalBlock> block = Create<CanonicalBlock>();
          block->SetBlockType(blockType);
          block->SetBlockNumber(blockNumber);
          block->SetBlockControlFlags(blockControlFlags);
          block->SetCRCType(crcType);
          block->SetData(data);
          return block;
        }
    }
}

bool 
CanonicalBlock::MustBeReplicated() const
{
  return (static_cast<uint64_t>(m_blockControlFlags) & 
          static_cast<uint64_t>(BlockControlFlags::BLOCK_MUST_BE_REPLICATED)) != 0;
}

bool 
CanonicalBlock::ReportIfUnprocessable() const
{
  return (static_cast<uint64_t>(m_blockControlFlags) & 
          static_cast<uint64_t>(BlockControlFlags::REPORT_BLOCK_IF_UNPROCESSABLE)) != 0;
}

bool 
CanonicalBlock::DeleteBundleIfUnprocessable() const
{
  return (static_cast<uint64_t>(m_blockControlFlags) & 
          static_cast<uint64_t>(BlockControlFlags::DELETE_BUNDLE_IF_BLOCK_UNPROCESSABLE)) != 0;
}

bool 
CanonicalBlock::RemoveBlockIfUnprocessable() const
{
  return (static_cast<uint64_t>(m_blockControlFlags) & 
          static_cast<uint64_t>(BlockControlFlags::REMOVE_BLOCK_IF_UNPROCESSABLE)) != 0;
}

bool 
CanonicalBlock::StatusReportRequested() const
{
  return (static_cast<uint64_t>(m_blockControlFlags) & 
          static_cast<uint64_t>(BlockControlFlags::STATUS_REPORT_REQUESTED)) != 0;
}

void 
CanonicalBlock::SetMustBeReplicated(bool enable)
{
  if (enable)
    {
      m_blockControlFlags = m_blockControlFlags | BlockControlFlags::BLOCK_MUST_BE_REPLICATED;
    }
  else
    {
      m_blockControlFlags = static_cast<BlockControlFlags>(
          static_cast<uint64_t>(m_blockControlFlags) & 
          ~static_cast<uint64_t>(BlockControlFlags::BLOCK_MUST_BE_REPLICATED));
    }
}

void 
CanonicalBlock::SetReportIfUnprocessable(bool enable)
{
  if (enable)
    {
      m_blockControlFlags = m_blockControlFlags | BlockControlFlags::REPORT_BLOCK_IF_UNPROCESSABLE;
    }
  else
    {
      m_blockControlFlags = static_cast<BlockControlFlags>(
          static_cast<uint64_t>(m_blockControlFlags) & 
          ~static_cast<uint64_t>(BlockControlFlags::REPORT_BLOCK_IF_UNPROCESSABLE));
    }
}

void 
CanonicalBlock::SetDeleteBundleIfUnprocessable(bool enable)
{
  if (enable)
    {
      m_blockControlFlags = m_blockControlFlags | BlockControlFlags::DELETE_BUNDLE_IF_BLOCK_UNPROCESSABLE;
    }
  else
    {
      m_blockControlFlags = static_cast<BlockControlFlags>(
          static_cast<uint64_t>(m_blockControlFlags) & 
          ~static_cast<uint64_t>(BlockControlFlags::DELETE_BUNDLE_IF_BLOCK_UNPROCESSABLE));
    }
}

void 
CanonicalBlock::SetRemoveBlockIfUnprocessable(bool enable)
{
  if (enable)
    {
      m_blockControlFlags = m_blockControlFlags | BlockControlFlags::REMOVE_BLOCK_IF_UNPROCESSABLE;
    }
  else
    {
      m_blockControlFlags = static_cast<BlockControlFlags>(
          static_cast<uint64_t>(m_blockControlFlags) & 
          ~static_cast<uint64_t>(BlockControlFlags::REMOVE_BLOCK_IF_UNPROCESSABLE));
    }
}

void 
CanonicalBlock::SetStatusReportRequested(bool enable)
{
  if (enable)
    {
      m_blockControlFlags = m_blockControlFlags | BlockControlFlags::STATUS_REPORT_REQUESTED;
    }
  else
    {
      m_blockControlFlags = static_cast<BlockControlFlags>(
          static_cast<uint64_t>(m_blockControlFlags) & 
          ~static_cast<uint64_t>(BlockControlFlags::STATUS_REPORT_REQUESTED));
    }
}

void 
CanonicalBlock::CalculateCRC()
{
  if (m_crcType == CRCType::NO_CRC)
    {
      m_crcValue.clear();
      return;
    }
  
  // Create CBOR representation of the block without CRC value
  std::vector<uint8_t> crcValue;
  m_crcValue = crcValue; // Temporarily clear CRC value
  
  Buffer buffer = ToCbor();
  
  // 修复代码：使用std::vector而不是VLA
  std::vector<uint8_t> data(buffer.GetSize());
  buffer.CopyData(data.data(), buffer.GetSize());
  
  // Calculate CRC
  if (m_crcType == CRCType::CRC_16)
  {
      uint16_t crc = CalculateCRC16(data);
      m_crcValue.clear();
      m_crcValue.push_back((crc >> 8) & 0xFF);  // 高字节
      m_crcValue.push_back(crc & 0xFF);         // 低字节
  }
  else if (m_crcType == CRCType::CRC_32)
  {
      uint32_t crc = CalculateCRC32(data);
      m_crcValue.clear();
      m_crcValue.push_back((crc >> 24) & 0xFF); // 最高字节
      m_crcValue.push_back((crc >> 16) & 0xFF);
      m_crcValue.push_back((crc >> 8) & 0xFF);
      m_crcValue.push_back(crc & 0xFF);         // 最低字节
  }
  else
  {
      // NO_CRC的情况
      m_crcValue.clear();
  }
}

bool 
CanonicalBlock::CheckCRC() const
{
  if (m_crcType == CRCType::NO_CRC)
    {
      return true;
    }
  
  // Create CBOR representation of the block without CRC value
  CanonicalBlock tempBlock = *this;
  tempBlock.m_crcValue.clear();
  
  Buffer buffer = tempBlock.ToCbor();
  
  // 修复代码：使用std::vector而不是VLA
  std::vector<uint8_t> data(buffer.GetSize());
  buffer.CopyData(data.data(), buffer.GetSize());
  
  // Verify CRC
  return VerifyCRC(m_crcType, data, m_crcValue);
}

Buffer 
CanonicalBlock::ToCbor() const
{
  // Create CBOR array for canonical block
  Cbor::CborArray array;
  
  // Add block elements according to BP specification
  array.push_back(Cbor::CborValue(static_cast<uint64_t>(m_blockType)));
  array.push_back(Cbor::CborValue(m_blockNumber));
  array.push_back(Cbor::CborValue(static_cast<uint64_t>(m_blockControlFlags)));
  array.push_back(Cbor::CborValue(static_cast<uint64_t>(m_crcType)));
  array.push_back(Cbor::CborValue(m_data));
  
  // Add CRC value if present
  if (m_crcType != CRCType::NO_CRC && !m_crcValue.empty())
    {
      array.push_back(Cbor::CborValue(m_crcValue));
    }
  
  return Cbor::Encode(Cbor::CborValue(array));
}

std::optional<Ptr<CanonicalBlock>> 
CanonicalBlock::FromCbor(Buffer buffer)
{
  auto cborOpt = Cbor::Decode(buffer);
  if (!cborOpt)
    {
      return std::nullopt;
    }
  
  const auto& cbor = *cborOpt;
  if (!cbor.IsArray())
    {
      return std::nullopt;
    }
  
  const auto& array = cbor.GetArray();
  if (array.size() < 5) // At least 5 elements required (without CRC)
    {
      return std::nullopt;
    }
  
  // Extract fields
  BlockType blockType = static_cast<BlockType>(array[0].GetUnsignedInteger());
  uint64_t blockNumber = array[1].GetUnsignedInteger();
  BlockControlFlags blockControlFlags = static_cast<BlockControlFlags>(array[2].GetUnsignedInteger());
  CRCType crcType = static_cast<CRCType>(array[3].GetUnsignedInteger());
  
  if (!array[4].IsByteString())
    {
      return std::nullopt;
    }
  
  std::vector<uint8_t> data = array[4].GetByteString();
  
  // Create block of the appropriate type
  Ptr<CanonicalBlock> block = CreateBlock(
      blockType,
      blockNumber,
      blockControlFlags,
      crcType,
      data);
  
  // Extract CRC if present
  if (crcType != CRCType::NO_CRC)
    {
      if (array.size() <= 5)
        {
          return std::nullopt;
        }
      
      if (!array[5].IsByteString())
        {
          return std::nullopt;
        }
      
      block->m_crcValue = array[5].GetByteString();
    }
  
  return block;
}

std::string 
CanonicalBlock::ToString() const
{
  // Convert all values to strings separately first
  std::string typeStr = BlockTypeToString(m_blockType);
  std::string numberStr = std::to_string(m_blockNumber);
  std::string flagsStr = std::to_string(static_cast<uint64_t>(m_blockControlFlags));
  std::string crcTypeStr = CRCTypeToString(m_crcType);
  std::string sizeStr = std::to_string(m_data.size());
  
  // Now concatenate them all using string addition
  std::string result = "CanonicalBlock(type=" + typeStr + 
                       ", number=" + numberStr + 
                       ", flags=" + flagsStr + 
                       ", crcType=" + crcTypeStr + 
                       ", dataLength=" + sizeStr + 
                       ")";
  
  return result;
}

// PayloadBlock implementation

PayloadBlock::PayloadBlock()
  : CanonicalBlock(BlockType::PAYLOAD_BLOCK, 1, BlockControlFlags::NO_FLAGS, CRCType::NO_CRC, {})
{
}

PayloadBlock::PayloadBlock(const std::vector<uint8_t>& payload)
  : CanonicalBlock(BlockType::PAYLOAD_BLOCK, 1, BlockControlFlags::NO_FLAGS, CRCType::NO_CRC, payload)
{
}

PayloadBlock::PayloadBlock(uint64_t blockNumber,
                         BlockControlFlags blockControlFlags,
                         CRCType crcType,
                         const std::vector<uint8_t>& payload)
  : CanonicalBlock(BlockType::PAYLOAD_BLOCK, blockNumber, blockControlFlags, crcType, payload)
{
}

std::vector<uint8_t> 
PayloadBlock::GetPayload() const
{
  return GetData();
}

void 
PayloadBlock::SetPayload(const std::vector<uint8_t>& payload)
{
  SetData(payload);
}

std::string 
PayloadBlock::ToString() const
{
  // Convert all values to strings separately first
  std::string numberStr = std::to_string(GetBlockNumber());
  std::string flagsStr = std::to_string(static_cast<uint64_t>(GetBlockControlFlags()));
  std::string crcTypeStr = CRCTypeToString(GetCRCType());
  std::string sizeStr = std::to_string(GetPayload().size());
  
  // Now concatenate them all using string addition
  std::string result = "PayloadBlock(number=" + numberStr + 
                       ", flags=" + flagsStr + 
                       ", crcType=" + crcTypeStr + 
                       ", payloadLength=" + sizeStr + 
                       ")";
  
  return result;
}

// PreviousNodeBlock implementation

PreviousNodeBlock::PreviousNodeBlock()
  : CanonicalBlock(BlockType::PREVIOUS_NODE_BLOCK, 2, BlockControlFlags::NO_FLAGS, CRCType::NO_CRC, {})
{
}

PreviousNodeBlock::PreviousNodeBlock(const EndpointID& previousNode)
  : CanonicalBlock(BlockType::PREVIOUS_NODE_BLOCK, 2, BlockControlFlags::NO_FLAGS, CRCType::NO_CRC, {})
{
  SetPreviousNode(previousNode);
}

PreviousNodeBlock::PreviousNodeBlock(uint64_t blockNumber,
                                   BlockControlFlags blockControlFlags,
                                   CRCType crcType,
                                   const EndpointID& previousNode)
  : CanonicalBlock(BlockType::PREVIOUS_NODE_BLOCK, blockNumber, blockControlFlags, crcType, {})
{
  SetPreviousNode(previousNode);
}

EndpointID 
PreviousNodeBlock::GetPreviousNode() const
{
  Buffer buffer;
  buffer.AddAtStart(GetData().size());
  Buffer::Iterator it = buffer.Begin();
  
  for (size_t i = 0; i < GetData().size(); ++i)
    {
      it.WriteU8(GetData()[i]);
    }
  
  auto eidOpt = EndpointID::FromCbor(buffer);
  if (eidOpt)
    {
      return *eidOpt;
    }
  
  return EndpointID("dtn:none");
}

void 
PreviousNodeBlock::SetPreviousNode(const EndpointID& previousNode)
{
  Buffer buffer = previousNode.ToCbor();
  
  // 修复代码：使用std::vector而不是VLA
  std::vector<uint8_t> data(buffer.GetSize());
  buffer.CopyData(data.data(), buffer.GetSize());
  
  SetData(data);
}

std::string 
PreviousNodeBlock::ToString() const
{
  // Convert all values to strings separately first
  std::string numberStr = std::to_string(GetBlockNumber());
  std::string flagsStr = std::to_string(static_cast<uint64_t>(GetBlockControlFlags()));
  std::string crcTypeStr = CRCTypeToString(GetCRCType());
  std::string previousNodeStr = GetPreviousNode().ToString();
  
  // Now concatenate them all using string addition
  std::string result = "PreviousNodeBlock(number=" + numberStr + 
                       ", flags=" + flagsStr + 
                       ", crcType=" + crcTypeStr + 
                       ", previousNode=" + previousNodeStr + 
                       ")";
  
  return result;
}

// BundleAgeBlock implementation

BundleAgeBlock::BundleAgeBlock()
  : CanonicalBlock(BlockType::BUNDLE_AGE_BLOCK, 3, BlockControlFlags::NO_FLAGS, CRCType::NO_CRC, {})
{
  SetAge(0);
}

BundleAgeBlock::BundleAgeBlock(uint64_t microseconds)
  : CanonicalBlock(BlockType::BUNDLE_AGE_BLOCK, 3, BlockControlFlags::NO_FLAGS, CRCType::NO_CRC, {})
{
  SetAge(microseconds);
}

BundleAgeBlock::BundleAgeBlock(uint64_t blockNumber,
                             BlockControlFlags blockControlFlags,
                             CRCType crcType,
                             uint64_t microseconds)
  : CanonicalBlock(BlockType::BUNDLE_AGE_BLOCK, blockNumber, blockControlFlags, crcType, {})
{
  SetAge(microseconds);
}

uint64_t 
BundleAgeBlock::GetAge() const
{
  if (GetData().empty())
    {
      return 0;
    }
  
  Buffer buffer;
  buffer.AddAtStart(GetData().size());
  Buffer::Iterator it = buffer.Begin();
  
  for (size_t i = 0; i < GetData().size(); ++i)
    {
      it.WriteU8(GetData()[i]);
    }
  
  auto cborOpt = Cbor::Decode(buffer);
  if (cborOpt && cborOpt->IsUnsignedInteger())
    {
      return cborOpt->GetUnsignedInteger();
    }
  
  return 0;
}

void 
BundleAgeBlock::SetAge(uint64_t microseconds)
{
  Cbor::CborValue value(microseconds);
  Buffer buffer = Cbor::Encode(value);
  
  // 修复代码：使用std::vector而不是VLA
  std::vector<uint8_t> data(buffer.GetSize());
  buffer.CopyData(data.data(), buffer.GetSize());
  
  SetData(data);
}

std::string 
BundleAgeBlock::ToString() const
{
  // Convert all values to strings separately first
  std::string numberStr = std::to_string(GetBlockNumber());
  std::string flagsStr = std::to_string(static_cast<uint64_t>(GetBlockControlFlags()));
  std::string crcTypeStr = CRCTypeToString(GetCRCType());
  std::string ageStr = std::to_string(GetAge()) + "μs";
  
  // Now concatenate them all using string addition
  std::string result = "BundleAgeBlock(number=" + numberStr + 
                       ", flags=" + flagsStr + 
                       ", crcType=" + crcTypeStr + 
                       ", age=" + ageStr + 
                       ")";
  
  return result;
}

// HopCountBlock implementation

HopCountBlock::HopCountBlock()
  : CanonicalBlock(BlockType::HOP_COUNT_BLOCK, 4, BlockControlFlags::NO_FLAGS, CRCType::NO_CRC, {})
{
  SetLimit(0);
  SetCount(0);
}

HopCountBlock::HopCountBlock(uint64_t limit, uint64_t count)
  : CanonicalBlock(BlockType::HOP_COUNT_BLOCK, 4, BlockControlFlags::NO_FLAGS, CRCType::NO_CRC, {})
{
  SetLimit(limit);
  SetCount(count);
}

HopCountBlock::HopCountBlock(uint64_t blockNumber,
                           BlockControlFlags blockControlFlags,
                           CRCType crcType,
                           uint64_t limit,
                           uint64_t count)
  : CanonicalBlock(BlockType::HOP_COUNT_BLOCK, blockNumber, blockControlFlags, crcType, {})
{
  SetLimit(limit);
  SetCount(count);
}

uint64_t 
HopCountBlock::GetLimit() const
{
  if (GetData().empty())
    {
      return 0;
    }
  
  Buffer buffer;
  buffer.AddAtStart(GetData().size());
  Buffer::Iterator it = buffer.Begin();
  
  for (size_t i = 0; i < GetData().size(); ++i)
    {
      it.WriteU8(GetData()[i]);
    }
  
  auto cborOpt = Cbor::Decode(buffer);
  if (cborOpt && cborOpt->IsArray() && cborOpt->GetArray().size() >= 2)
    {
      return cborOpt->GetArray()[0].GetUnsignedInteger();
    }
  
  return 0;
}

uint64_t 
HopCountBlock::GetCount() const
{
  if (GetData().empty())
    {
      return 0;
    }
  
  Buffer buffer;
  buffer.AddAtStart(GetData().size());
  Buffer::Iterator it = buffer.Begin();
  
  for (size_t i = 0; i < GetData().size(); ++i)
    {
      it.WriteU8(GetData()[i]);
    }
  
  auto cborOpt = Cbor::Decode(buffer);
  if (cborOpt && cborOpt->IsArray() && cborOpt->GetArray().size() >= 2)
    {
      return cborOpt->GetArray()[1].GetUnsignedInteger();
    }
  
  return 0;
}

void 
HopCountBlock::SetLimit(uint64_t limit)
{
  Cbor::CborArray array = {
    Cbor::CborValue(limit),
    Cbor::CborValue(GetCount())
  };
  
  Buffer buffer = Cbor::Encode(Cbor::CborValue(array));
  
  // 修复代码：使用std::vector而不是VLA
  std::vector<uint8_t> data(buffer.GetSize());
  buffer.CopyData(data.data(), buffer.GetSize());
  
  SetData(data);
}

void 
HopCountBlock::SetCount(uint64_t count)
{
  Cbor::CborArray array = {
    Cbor::CborValue(GetLimit()),
    Cbor::CborValue(count)
  };
  
  Buffer buffer = Cbor::Encode(Cbor::CborValue(array));
  
  // 修复代码：使用std::vector而不是VLA
  std::vector<uint8_t> data(buffer.GetSize());
  buffer.CopyData(data.data(), buffer.GetSize());
  
  SetData(data);
}

void 
HopCountBlock::Increment()
{
  SetCount(GetCount() + 1);
}

bool 
HopCountBlock::Exceeded() const
{
  return GetCount() >= GetLimit();
}

std::string 
HopCountBlock::ToString() const
{
  // Convert all values to strings separately first
  std::string numberStr = std::to_string(GetBlockNumber());
  std::string flagsStr = std::to_string(static_cast<uint64_t>(GetBlockControlFlags()));
  std::string crcTypeStr = CRCTypeToString(GetCRCType());
  std::string limitStr = std::to_string(GetLimit());
  std::string countStr = std::to_string(GetCount());
  
  // Now concatenate them all using string addition
  std::string result = "HopCountBlock(number=" + numberStr + 
                       ", flags=" + flagsStr + 
                       ", crcType=" + crcTypeStr + 
                       ", limit=" + limitStr + 
                       ", count=" + countStr + 
                       ")";
  
  return result;
}

} // namespace dtn7

} // namespace ns3