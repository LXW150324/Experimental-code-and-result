#include "bundle.h"
#include "cbor.h"
#include <sstream>
#include <vector>

namespace ns3 {

namespace dtn7 {

Bundle::Bundle()
{
}

Bundle::Bundle(const PrimaryBlock& primaryBlock)
  : m_primaryBlock(primaryBlock)
{
}

Bundle 
Bundle::NewBundle(const PrimaryBlock& primaryBlock, 
                  const std::vector<Ptr<CanonicalBlock>>& canonicalBlocks)
{
  Bundle bundle(primaryBlock);
  bundle.m_canonicalBlocks = canonicalBlocks;
  return bundle;
}

Bundle 
Bundle::MustNewBundle(const std::string& source,
                      const std::string& destination,
                      const DtnTime& creationTimestamp,
                      const Time& lifetime,
                      const std::vector<uint8_t>& payload)
{
  // Create primary block
  PrimaryBlock primaryBlock(
      PrimaryBlock::DEFAULT_VERSION,
      BundleControlFlags::NO_FLAGS,
      CRCType::CRC_32,
      EndpointID(destination),
      EndpointID(source),
      EndpointID("dtn:none"),  // Report-to: none
      creationTimestamp,
      0,  // Sequence number will be set by application
      lifetime);
  
  // Create payload block
  Ptr<PayloadBlock> payloadBlock = Create<PayloadBlock>(payload);
  payloadBlock->SetCRCType(CRCType::CRC_32);
  
  // Create bundle
  Bundle bundle(primaryBlock);
  bundle.AddBlock(payloadBlock);
  
  // Calculate CRCs
  bundle.CalculateCRC();
  
  return bundle;
}

BundleID 
Bundle::GetId() const
{
  return BundleID(
      m_primaryBlock.GetSourceNodeEID(),
      m_primaryBlock.GetCreationTimestamp(),
      m_primaryBlock.GetSequenceNumber(),
      m_primaryBlock.IsFragment(),
      m_primaryBlock.GetFragmentOffset());
}

std::string 
Bundle::ToString() const
{
  std::stringstream ss;
  
  ss << "Bundle(src=" << m_primaryBlock.GetSourceNodeEID().ToString()
     << ", dst=" << m_primaryBlock.GetDestinationEID().ToString()
     << ", created=" << m_primaryBlock.GetCreationTimestamp().ToString()
     << ", seq=" << m_primaryBlock.GetSequenceNumber()
     << ", blocks=" << m_canonicalBlocks.size();
  
  if (IsFragment())
    {
      ss << ", fragment=" << m_primaryBlock.GetFragmentOffset() 
         << "/" << m_primaryBlock.GetTotalApplicationDataUnitLength();
    }
  
  ss << ")";
  
  return ss.str();
}

bool 
Bundle::IsAdministrativeRecord() const
{
  return m_primaryBlock.IsAdministrativeRecord();
}

bool 
Bundle::IsFragment() const
{
  return m_primaryBlock.IsFragment();
}

void 
Bundle::AddBlock(Ptr<CanonicalBlock> block)
{
  // Assign block number if not specified
  if (block->GetBlockNumber() == 0)
    {
      uint64_t maxNumber = 0;
      for (const auto& existingBlock : m_canonicalBlocks)
        {
          maxNumber = std::max(maxNumber, existingBlock->GetBlockNumber());
        }
      block->SetBlockNumber(maxNumber + 1);
    }
  
  m_canonicalBlocks.push_back(block);
}

Ptr<CanonicalBlock> 
Bundle::GetBlockByType(BlockType blockType) const
{
  for (const auto& block : m_canonicalBlocks)
    {
      if (block->GetBlockType() == blockType)
        {
          return block;
        }
    }
  
  return nullptr;
}

std::vector<Ptr<CanonicalBlock>> 
Bundle::GetBlocksByType(BlockType blockType) const
{
  std::vector<Ptr<CanonicalBlock>> result;
  
  for (const auto& block : m_canonicalBlocks)
    {
      if (block->GetBlockType() == blockType)
        {
          result.push_back(block);
        }
    }
  
  return result;
}

bool 
Bundle::RemoveBlockByType(BlockType blockType)
{
  for (auto it = m_canonicalBlocks.begin(); it != m_canonicalBlocks.end(); ++it)
    {
      if ((*it)->GetBlockType() == blockType)
        {
          m_canonicalBlocks.erase(it);
          return true;
        }
    }
  
  return false;
}

Ptr<CanonicalBlock> 
Bundle::GetPayloadBlock() const
{
  return GetBlockByType(BlockType::PAYLOAD_BLOCK);
}

std::vector<uint8_t> 
Bundle::GetPayload() const
{
  Ptr<CanonicalBlock> payloadBlock = GetPayloadBlock();
  if (payloadBlock)
    {
      return payloadBlock->GetData();
    }
  
  return std::vector<uint8_t>();
}

void 
Bundle::SetPayload(const std::vector<uint8_t>& payload)
{
  Ptr<CanonicalBlock> payloadBlock = GetPayloadBlock();
  if (payloadBlock)
    {
      payloadBlock->SetData(payload);
    }
  else
    {
      Ptr<PayloadBlock> newPayloadBlock = Create<PayloadBlock>(payload);
      newPayloadBlock->SetCRCType(CRCType::CRC_32);
      AddBlock(newPayloadBlock);
    }
}

void 
Bundle::CalculateCRC()
{
  // Calculate CRC for primary block
  m_primaryBlock.CalculateCRC();
  
  // Calculate CRC for canonical blocks
  for (auto& block : m_canonicalBlocks)
    {
      block->CalculateCRC();
    }
}

bool 
Bundle::CheckCRC() const
{
  // Check CRC for primary block
  if (!m_primaryBlock.CheckCRC())
    {
      return false;
    }
  
  // Check CRC for canonical blocks
  for (const auto& block : m_canonicalBlocks)
    {
      if (!block->CheckCRC())
        {
          return false;
        }
    }
  
  return true;
}

Buffer 
Bundle::ToCbor() const
{
  // Create CBOR array for bundle
  Cbor::CborArray array;
  
  // Add primary block
  Buffer primaryBuffer = m_primaryBlock.ToCbor();
  
  // 修复代码：使用std::vector而不是VLA
  std::vector<uint8_t> primaryData(primaryBuffer.GetSize());
  primaryBuffer.CopyData(primaryData.data(), primaryBuffer.GetSize());
  
  auto primaryCbor = Cbor::Decode(primaryBuffer);
  if (primaryCbor)
    {
      array.push_back(*primaryCbor);
    }
  
  // Add canonical blocks
  for (const auto& block : m_canonicalBlocks)
    {
      Buffer blockBuffer = block->ToCbor();
      auto blockCbor = Cbor::Decode(blockBuffer);
      if (blockCbor)
        {
          array.push_back(*blockCbor);
        }
    }
  
  return Cbor::Encode(Cbor::CborValue(array));
}

std::optional<Bundle> 
Bundle::FromCbor(Buffer buffer)
{
  auto cborOpt = Cbor::Decode(buffer);
  if (!cborOpt)
    {
      return std::nullopt;
    }
  
  const auto& cbor = *cborOpt;
  if (!cbor.IsArray() || cbor.GetArray().empty())
    {
      return std::nullopt;
    }
  
  const auto& array = cbor.GetArray();
  
  // Extract primary block
  if (!array[0].IsArray())
    {
      return std::nullopt;
    }
  
  Buffer primaryBuffer = Cbor::Encode(array[0]);
  auto primaryBlockOpt = PrimaryBlock::FromCbor(primaryBuffer);
  if (!primaryBlockOpt)
    {
      return std::nullopt;
    }
  
  Bundle bundle(*primaryBlockOpt);
  
  // Extract canonical blocks
  for (size_t i = 1; i < array.size(); ++i)
    {
      if (!array[i].IsArray())
        {
          continue;
        }
      
      Buffer blockBuffer = Cbor::Encode(array[i]);
      auto blockOpt = CanonicalBlock::FromCbor(blockBuffer);
      if (blockOpt)
        {
          bundle.AddBlock(*blockOpt);
        }
    }
  
  return bundle;
}

std::vector<Bundle> 
Bundle::Fragment(size_t maxFragmentSize) const
{
  std::vector<Bundle> fragments;
  
  // Don't fragment if bundle must not be fragmented
  if (m_primaryBlock.MustNotFragment())
    {
      return fragments;
    }
  
  // Don't fragment bundles that are too small
  Buffer fullBuffer = ToCbor();
  if (fullBuffer.GetSize() <= maxFragmentSize)
    {
      return fragments;
    }
  
  // Get payload block
  Ptr<CanonicalBlock> payloadBlock = GetPayloadBlock();
  if (!payloadBlock)
    {
      return fragments;
    }
  
  std::vector<uint8_t> fullPayload = payloadBlock->GetData();
  uint64_t totalLength = fullPayload.size();
  
  // Calculate payload size per fragment
  // (This is a simplified approach - in a real implementation we would need to account for
  // the overhead of the primary block and other blocks in each fragment)
  size_t payloadPerFragment = maxFragmentSize / 2; // Conservative estimate
  
  // Create fragments
  for (size_t offset = 0; offset < totalLength; offset += payloadPerFragment)
    {
      // Create fragment primary block
      PrimaryBlock fragmentPrimary = m_primaryBlock;
      fragmentPrimary.SetFragmentation(true);
      fragmentPrimary.SetFragmentOffset(offset);
      fragmentPrimary.SetTotalApplicationDataUnitLength(totalLength);
      
      // Create fragment
      Bundle fragment(fragmentPrimary);
      
      // Calculate fragment payload size
      size_t payloadSize = std::min(payloadPerFragment, static_cast<size_t>(totalLength - offset));
      
      // Create payload block with fragment
      std::vector<uint8_t> fragmentPayload(fullPayload.begin() + offset, 
                                         fullPayload.begin() + offset + payloadSize);
      
      Ptr<PayloadBlock> fragmentPayloadBlock = Create<PayloadBlock>(fragmentPayload);
      fragmentPayloadBlock->SetCRCType(payloadBlock->GetCRCType());
      fragment.AddBlock(fragmentPayloadBlock);
      
      // Copy other blocks that must be replicated
      for (const auto& block : m_canonicalBlocks)
        {
          if (block->GetBlockType() != BlockType::PAYLOAD_BLOCK && block->MustBeReplicated())
            {
              fragment.AddBlock(block);
            }
        }
      
      // Calculate CRCs
      fragment.CalculateCRC();
      
      fragments.push_back(fragment);
    }
  
  return fragments;
}

} // namespace dtn7

} // namespace ns3