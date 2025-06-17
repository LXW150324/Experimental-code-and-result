#include "fragmentation-manager.h"
#include "ns3/simulator.h"
#include <sstream>
#include <algorithm>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("FragmentationManager");

namespace dtn7 {

FragmentationManager::FragmentationManager ()
  : m_fragmentedBundles (0),
    m_createdFragments (0),
    m_reassembledBundles (0),
    m_abandonedFragmentSets (0)
{
  NS_LOG_FUNCTION (this);
}

FragmentationManager::~FragmentationManager ()
{
  NS_LOG_FUNCTION (this);
}

std::vector<Ptr<Bundle>> 
FragmentationManager::FragmentBundle (Ptr<Bundle> bundle, size_t maxFragmentSize)
{
  NS_LOG_FUNCTION (this << bundle << maxFragmentSize);
  
  std::vector<Ptr<Bundle>> fragments;
  
  // Don't fragment if bundle must not be fragmented
  if (bundle->GetPrimaryBlock().MustNotFragment())
    {
      NS_LOG_INFO ("Bundle must not be fragmented");
      return fragments;
    }
  
  // Don't fragment administrative records
  if (bundle->IsAdministrativeRecord())
    {
      NS_LOG_INFO ("Administrative record must not be fragmented");
      return fragments;
    }
  
  // Don't fragment bundles that are too small
  Buffer fullBuffer = bundle->ToCbor();
  if (fullBuffer.GetSize() <= maxFragmentSize)
    {
      NS_LOG_INFO ("Bundle size (" << fullBuffer.GetSize() << " bytes) <= max fragment size (" 
                  << maxFragmentSize << " bytes), no fragmentation needed");
      return fragments;
    }
  
  // Get payload block
  Ptr<CanonicalBlock> payloadBlock = bundle->GetPayloadBlock();
  if (!payloadBlock)
    {
      NS_LOG_ERROR ("Bundle has no payload block, cannot fragment");
      return fragments;
    }
  
  std::vector<uint8_t> fullPayload = payloadBlock->GetData();
  uint64_t totalLength = fullPayload.size();
  
  // Calculate number of fragments needed
  // Consider header overhead (primary block and other blocks that must be replicated)
  size_t headerOverhead = 0;
  Buffer primaryBuffer = bundle->GetPrimaryBlock().ToCbor();
  headerOverhead += primaryBuffer.GetSize();
  
  // Add size of other blocks that must be replicated
  for (const auto& block : bundle->GetCanonicalBlocks())
    {
      if (block->GetBlockType() != BlockType::PAYLOAD_BLOCK && block->MustBeReplicated())
        {
          Buffer blockBuffer = block->ToCbor();
          headerOverhead += blockBuffer.GetSize();
        }
    }
  
  // Adjust max payload per fragment considering header overhead
  size_t maxPayloadPerFragment = (maxFragmentSize > headerOverhead) ? 
                               maxFragmentSize - headerOverhead : maxFragmentSize / 2;
  
  // Calculate number of fragments
  size_t numFragments = (totalLength + maxPayloadPerFragment - 1) / maxPayloadPerFragment;
  
  NS_LOG_INFO ("Fragmenting bundle (" << fullBuffer.GetSize() << " bytes) into " 
              << numFragments << " fragments with max payload " << maxPayloadPerFragment << " bytes");
  
  // Create fragments
  for (size_t i = 0; i < numFragments; ++i)
    {
      // Calculate offset and size for this fragment
      size_t offset = i * maxPayloadPerFragment;
      size_t payloadSize = CalculateFragmentPayloadSize(bundle, maxFragmentSize, i, numFragments);
      
      // Check for boundary conditions
      if (offset >= totalLength)
        {
          NS_LOG_ERROR ("Fragment offset exceeds payload length, skipping fragment");
          continue;
        }
      
      if (offset + payloadSize > totalLength)
        {
          payloadSize = totalLength - offset;
        }
      
      // Create fragment primary block
      PrimaryBlock fragmentPrimary = bundle->GetPrimaryBlock();
      fragmentPrimary.SetFragmentation(true);
      fragmentPrimary.SetFragmentOffset(offset);
      fragmentPrimary.SetTotalApplicationDataUnitLength(totalLength);
      
      // Create fragment bundle object
      Ptr<Bundle> fragment = Create<Bundle>(fragmentPrimary);
      
      // Create payload block with fragment
      std::vector<uint8_t> fragmentPayload(fullPayload.begin() + offset, 
                                         fullPayload.begin() + offset + payloadSize);
      
      Ptr<PayloadBlock> fragmentPayloadBlock = Create<PayloadBlock>(fragmentPayload);
      fragmentPayloadBlock->SetCRCType(payloadBlock->GetCRCType());
      fragment->AddBlock(fragmentPayloadBlock);
      
      // Copy other blocks that must be replicated
      for (const auto& block : bundle->GetCanonicalBlocks())
        {
          if (block->GetBlockType() != BlockType::PAYLOAD_BLOCK && block->MustBeReplicated())
            {
              Ptr<CanonicalBlock> copyBlock = Create<CanonicalBlock>(
                  block->GetBlockType(),
                  block->GetBlockNumber(),
                  block->GetBlockControlFlags(),
                  block->GetCRCType(),
                  block->GetData());
              fragment->AddBlock(copyBlock);
            }
        }
      
      // Calculate CRCs
      fragment->CalculateCRC();
      
      fragments.push_back(fragment);
    }
  
  // Update statistics
  if (!fragments.empty())
    {
      std::lock_guard<std::mutex> lock(m_mutex);
      m_fragmentedBundles++;
      m_createdFragments += fragments.size();
    }
  
  return fragments;
}

Ptr<Bundle> 
FragmentationManager::AddFragment (Ptr<Bundle> fragment)
{
  NS_LOG_FUNCTION (this << fragment);
  
  // Check if bundle is a fragment
  if (!IsFragment(fragment))
    {
      NS_LOG_ERROR ("Bundle is not a fragment");
      return nullptr;
    }
  
  // Get original bundle ID
  BundleID originalId = GetOriginalBundleId(fragment);
  
  // Get fragment information
  const PrimaryBlock& primary = fragment->GetPrimaryBlock();
  uint64_t fragmentOffset = primary.GetFragmentOffset();
  uint64_t totalLength = primary.GetTotalApplicationDataUnitLength();
  
  // Calculate fragment expiration time
  Time creationTime = primary.GetCreationTimestamp().ToTime();
  Time lifetime = primary.GetLifetime();
  Time expirationTime = creationTime + lifetime;
  
  // Add to fragment set
  std::lock_guard<std::mutex> lock(m_mutex);
  
  FragmentInfo& info = m_fragmentSets[originalId];
  
  // Initialize fragment set if first fragment
  if (info.fragments.empty())
    {
      info.sourceId = originalId;
      info.totalLength = totalLength;
      info.expirationTime = expirationTime;
      info.complete = false;
    }
  
  // Check for duplicate fragments
  for (const auto& existingFragment : info.fragments)
    {
      if (existingFragment->GetPrimaryBlock().GetFragmentOffset() == fragmentOffset)
        {
          NS_LOG_INFO ("Duplicate fragment received, ignoring");
          return nullptr;
        }
    }
  
  // Add fragment
  info.fragments.push_back(fragment);
  NS_LOG_INFO ("Added fragment " << fragmentOffset << "/" << totalLength 
              << " (" << info.fragments.size() << " fragments total)");
  
  // Try to reassemble
  return TryReassemble(info);
}

Ptr<Bundle> 
FragmentationManager::TryReassemble (FragmentInfo& info)
{
  if (info.complete)
    {
      NS_LOG_ERROR ("Fragment set already complete");
      return nullptr;
    }
  
  // Sort fragments by offset
  std::sort(info.fragments.begin(), info.fragments.end(), 
           [](const Ptr<Bundle>& a, const Ptr<Bundle>& b) {
             return a->GetPrimaryBlock().GetFragmentOffset() < b->GetPrimaryBlock().GetFragmentOffset();
           });
  
  // Check if we have all fragments
  uint64_t coveredLength = 0;
  for (const auto& frag : info.fragments)
    {
      uint64_t offset = frag->GetPrimaryBlock().GetFragmentOffset();
      
      // Check for gap
      if (offset > coveredLength)
        {
          NS_LOG_INFO ("Gap detected in fragments, cannot reassemble yet");
          return nullptr;
        }
      
      // Get payload length
      std::vector<uint8_t> payload = frag->GetPayload();
      uint64_t length = payload.size();
      
      // Update covered length
      coveredLength = std::max(coveredLength, offset + length);
    }
  
  // Check if all data is covered
  if (coveredLength < info.totalLength)
    {
      NS_LOG_INFO ("Not all data covered (" << coveredLength << "/" << info.totalLength 
                  << "), cannot reassemble yet");
      return nullptr;
    }
  
  NS_LOG_INFO ("All fragments received, reassembling");
  
  // We have all fragments, reassemble the bundle
  
  // Start with the primary block from the first fragment
  PrimaryBlock reassembledPrimary = info.fragments[0]->GetPrimaryBlock();
  
  // Remove fragmentation flags and information
  reassembledPrimary.SetFragmentation(false);
  
  // Create the reassembled bundle
  Ptr<Bundle> reassembled = Create<Bundle>(reassembledPrimary);
  
  // Create payload by combining all fragments
  std::vector<uint8_t> reassembledPayload(info.totalLength);
  
  for (const auto& frag : info.fragments)
    {
      uint64_t offset = frag->GetPrimaryBlock().GetFragmentOffset();
      std::vector<uint8_t> fragPayload = frag->GetPayload();
      
      // Copy fragment payload to the correct position
      std::copy(fragPayload.begin(), fragPayload.end(), reassembledPayload.begin() + offset);
    }
  
  // Add payload block
  Ptr<PayloadBlock> payloadBlock = Create<PayloadBlock>(reassembledPayload);
  reassembled->AddBlock(payloadBlock);
  
  // Copy other blocks that should be in the reassembled bundle
  // (We assume these are the same in all fragments, as per BP spec)
  for (const auto& block : info.fragments[0]->GetCanonicalBlocks())
    {
      if (block->GetBlockType() != BlockType::PAYLOAD_BLOCK)
        {
          reassembled->AddBlock(block);
        }
    }
  
  // Calculate CRCs
  reassembled->CalculateCRC();
  
  // Mark as complete
  info.complete = true;
  m_reassembledBundles++;
  
  return reassembled;
}

size_t 
FragmentationManager::CleanupExpiredFragments ()
{
  NS_LOG_FUNCTION (this);
  
  std::lock_guard<std::mutex> lock(m_mutex);
  size_t removedCount = 0;
  
  Time now = Simulator::Now();
  
  for (auto it = m_fragmentSets.begin(); it != m_fragmentSets.end();)
    {
      if (now > it->second.expirationTime)
        {
          NS_LOG_INFO ("Removing expired fragment set for bundle " << it->first.ToString());
          it = m_fragmentSets.erase(it);
          removedCount++;
          m_abandonedFragmentSets++;
        }
      else
        {
          ++it;
        }
    }
  
  return removedCount;
}

std::string 
FragmentationManager::GetStats () const
{
  std::lock_guard<std::mutex> lock(m_mutex);
  
  std::stringstream ss;
  ss << "FragmentationManager(";
  ss << "fragmentSets=" << m_fragmentSets.size();
  ss << ", fragmentedBundles=" << m_fragmentedBundles;
  ss << ", createdFragments=" << m_createdFragments;
  ss << ", reassembledBundles=" << m_reassembledBundles;
  ss << ", abandonedSets=" << m_abandonedFragmentSets;
  ss << ")";
  
  return ss.str();
}

bool 
FragmentationManager::IsFragment (Ptr<Bundle> bundle)
{
  if (!bundle)
    {
      return false;
    }
  
  return bundle->GetPrimaryBlock().IsFragment();
}

BundleID 
FragmentationManager::GetOriginalBundleId (Ptr<Bundle> fragment)
{
  if (!fragment || !IsFragment(fragment))
    {
      return BundleID();
    }
  
  const PrimaryBlock& primary = fragment->GetPrimaryBlock();
  
  // Original Bundle ID is like the fragment's Bundle ID but without fragmentation flags
  return BundleID(
      primary.GetSourceNodeEID(),
      primary.GetCreationTimestamp(),
      primary.GetSequenceNumber(),
      false,  // Not a fragment
      0       // No fragment offset
  );
}

size_t 
FragmentationManager::CalculateFragmentPayloadSize (Ptr<Bundle> bundle, size_t maxFragmentSize, 
                                                 size_t fragmentIndex, size_t totalFragments)
{
  NS_LOG_FUNCTION (this << bundle << maxFragmentSize << fragmentIndex << totalFragments);
  
  std::vector<uint8_t> fullPayload = bundle->GetPayload();
  uint64_t totalLength = fullPayload.size();
  
  // Calculate header overhead
  size_t headerOverhead = 0;
  Buffer primaryBuffer = bundle->GetPrimaryBlock().ToCbor();
  headerOverhead += primaryBuffer.GetSize();
  
  // Add size of other blocks that must be replicated
  for (const auto& block : bundle->GetCanonicalBlocks())
    {
      if (block->GetBlockType() != BlockType::PAYLOAD_BLOCK && block->MustBeReplicated())
        {
          Buffer blockBuffer = block->ToCbor();
          headerOverhead += blockBuffer.GetSize();
        }
    }
  
  // Calculate max payload per fragment
  size_t effectiveMaxSize = (maxFragmentSize > headerOverhead) ? 
                          maxFragmentSize - headerOverhead : maxFragmentSize / 2;
  
  // For the last fragment, use remaining size
  if (fragmentIndex == totalFragments - 1)
    {
      size_t remainingSize = totalLength - (fragmentIndex * effectiveMaxSize);
      return remainingSize;
    }
  
  return effectiveMaxSize;
}

} // namespace dtn7

} // namespace ns3