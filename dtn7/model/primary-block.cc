#include "primary-block.h"
#include "cbor.h"
#include "crc.h"
#include <sstream>
#include <vector>

namespace ns3 {

namespace dtn7 {

PrimaryBlock::PrimaryBlock()
  : m_version(DEFAULT_VERSION),
    m_bundleControlFlags(BundleControlFlags::NO_FLAGS),
    m_crcType(CRCType::NO_CRC),
    m_destinationEID("dtn:none"),
    m_sourceNodeEID("dtn:none"),
    m_reportToEID("dtn:none"),
    m_sequenceNumber(0),
    m_lifetime(Seconds(3600)), // Default lifetime: 1 hour
    m_fragmentOffset(0),
    m_totalApplicationDataUnitLength(0)
{
}

PrimaryBlock::PrimaryBlock(uint64_t version,
                           BundleControlFlags bundleControlFlags,
                           CRCType crcType,
                           EndpointID destinationEID,
                           EndpointID sourceNodeEID,
                           EndpointID reportToEID,
                           DtnTime creationTimestamp,
                           uint64_t sequenceNumber,
                           Time lifetime,
                           uint64_t fragmentOffset,
                           uint64_t totalApplicationDataUnitLength)
  : m_version(version),
    m_bundleControlFlags(bundleControlFlags),
    m_crcType(crcType),
    m_destinationEID(destinationEID),
    m_sourceNodeEID(sourceNodeEID),
    m_reportToEID(reportToEID),
    m_creationTimestamp(creationTimestamp),
    m_sequenceNumber(sequenceNumber),
    m_lifetime(lifetime),
    m_fragmentOffset(fragmentOffset),
    m_totalApplicationDataUnitLength(totalApplicationDataUnitLength)
{
}

bool 
PrimaryBlock::HasFragmentation() const
{
  return (static_cast<uint64_t>(m_bundleControlFlags) & 
          static_cast<uint64_t>(BundleControlFlags::BUNDLE_IS_A_FRAGMENT)) != 0;
}

bool 
PrimaryBlock::MustNotFragment() const
{
  return (static_cast<uint64_t>(m_bundleControlFlags) & 
          static_cast<uint64_t>(BundleControlFlags::BUNDLE_MUST_NOT_BE_FRAGMENTED)) != 0;
}

bool 
PrimaryBlock::IsFragment() const
{
  return HasFragmentation();
}

bool 
PrimaryBlock::IsAdministrativeRecord() const
{
  return (static_cast<uint64_t>(m_bundleControlFlags) & 
          static_cast<uint64_t>(BundleControlFlags::PAYLOAD_IS_AN_ADMINISTRATIVE_RECORD)) != 0;
}

bool 
PrimaryBlock::RequestsBundleDeletionStatusReport() const
{
  return (static_cast<uint64_t>(m_bundleControlFlags) & 
          static_cast<uint64_t>(BundleControlFlags::BUNDLE_DELETION_STATUS_REPORTS_REQUESTED)) != 0;
}

bool 
PrimaryBlock::RequestsBundleDeliveryStatusReport() const
{
  return (static_cast<uint64_t>(m_bundleControlFlags) & 
          static_cast<uint64_t>(BundleControlFlags::BUNDLE_DELIVERY_STATUS_REPORTS_REQUESTED)) != 0;
}

bool 
PrimaryBlock::RequestsBundleForwardingStatusReport() const
{
  return (static_cast<uint64_t>(m_bundleControlFlags) & 
          static_cast<uint64_t>(BundleControlFlags::BUNDLE_FORWARDING_STATUS_REPORTS_REQUESTED)) != 0;
}

bool 
PrimaryBlock::RequestsBundleReceptionStatusReport() const
{
  return (static_cast<uint64_t>(m_bundleControlFlags) & 
          static_cast<uint64_t>(BundleControlFlags::BUNDLE_RECEPTION_STATUS_REPORTS_REQUESTED)) != 0;
}

void 
PrimaryBlock::SetBundleDeletionStatusReport(bool enable)
{
  if (enable)
    {
      m_bundleControlFlags = m_bundleControlFlags | BundleControlFlags::BUNDLE_DELETION_STATUS_REPORTS_REQUESTED;
    }
  else
    {
      m_bundleControlFlags = static_cast<BundleControlFlags>(
          static_cast<uint64_t>(m_bundleControlFlags) & 
          ~static_cast<uint64_t>(BundleControlFlags::BUNDLE_DELETION_STATUS_REPORTS_REQUESTED));
    }
}

void 
PrimaryBlock::SetBundleDeliveryStatusReport(bool enable)
{
  if (enable)
    {
      m_bundleControlFlags = m_bundleControlFlags | BundleControlFlags::BUNDLE_DELIVERY_STATUS_REPORTS_REQUESTED;
    }
  else
    {
      m_bundleControlFlags = static_cast<BundleControlFlags>(
          static_cast<uint64_t>(m_bundleControlFlags) & 
          ~static_cast<uint64_t>(BundleControlFlags::BUNDLE_DELIVERY_STATUS_REPORTS_REQUESTED));
    }
}

void 
PrimaryBlock::SetBundleForwardingStatusReport(bool enable)
{
  if (enable)
    {
      m_bundleControlFlags = m_bundleControlFlags | BundleControlFlags::BUNDLE_FORWARDING_STATUS_REPORTS_REQUESTED;
    }
  else
    {
      m_bundleControlFlags = static_cast<BundleControlFlags>(
          static_cast<uint64_t>(m_bundleControlFlags) & 
          ~static_cast<uint64_t>(BundleControlFlags::BUNDLE_FORWARDING_STATUS_REPORTS_REQUESTED));
    }
}

void 
PrimaryBlock::SetBundleReceptionStatusReport(bool enable)
{
  if (enable)
    {
      m_bundleControlFlags = m_bundleControlFlags | BundleControlFlags::BUNDLE_RECEPTION_STATUS_REPORTS_REQUESTED;
    }
  else
    {
      m_bundleControlFlags = static_cast<BundleControlFlags>(
          static_cast<uint64_t>(m_bundleControlFlags) & 
          ~static_cast<uint64_t>(BundleControlFlags::BUNDLE_RECEPTION_STATUS_REPORTS_REQUESTED));
    }
}

void 
PrimaryBlock::SetFragmentation(bool enable)
{
  if (enable)
    {
      m_bundleControlFlags = m_bundleControlFlags | BundleControlFlags::BUNDLE_IS_A_FRAGMENT;
    }
  else
    {
      m_bundleControlFlags = static_cast<BundleControlFlags>(
          static_cast<uint64_t>(m_bundleControlFlags) & 
          ~static_cast<uint64_t>(BundleControlFlags::BUNDLE_IS_A_FRAGMENT));
    }
}

void 
PrimaryBlock::SetAdministrativeRecord(bool enable)
{
  if (enable)
    {
      m_bundleControlFlags = m_bundleControlFlags | BundleControlFlags::PAYLOAD_IS_AN_ADMINISTRATIVE_RECORD;
    }
  else
    {
      m_bundleControlFlags = static_cast<BundleControlFlags>(
          static_cast<uint64_t>(m_bundleControlFlags) & 
          ~static_cast<uint64_t>(BundleControlFlags::PAYLOAD_IS_AN_ADMINISTRATIVE_RECORD));
    }
}

void 
PrimaryBlock::SetMustNotFragment(bool enable)
{
  if (enable)
    {
      m_bundleControlFlags = m_bundleControlFlags | BundleControlFlags::BUNDLE_MUST_NOT_BE_FRAGMENTED;
    }
  else
    {
      m_bundleControlFlags = static_cast<BundleControlFlags>(
          static_cast<uint64_t>(m_bundleControlFlags) & 
          ~static_cast<uint64_t>(BundleControlFlags::BUNDLE_MUST_NOT_BE_FRAGMENTED));
    }
}

void 
PrimaryBlock::CalculateCRC()
{
  if (m_crcType == CRCType::NO_CRC)
    {
      m_crcValue.clear();
      return;
    }
  
  // 创建不带CRC值的区块CBOR表示
  std::vector<uint8_t> crcValue;
  m_crcValue = crcValue; // 临时清除CRC值
  
  Buffer buffer = ToCbor();
  
  // 修复代码：使用std::vector而不是VLA
  std::vector<uint8_t> data(buffer.GetSize());
  buffer.CopyData(data.data(), buffer.GetSize());
  
  // 计算CRC
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
PrimaryBlock::CheckCRC() const
{
  if (m_crcType == CRCType::NO_CRC)
    {
      return true;
    }
  
  // 创建不带CRC值的区块CBOR表示
  PrimaryBlock tempBlock = *this;
  tempBlock.m_crcValue.clear();
  
  Buffer buffer = tempBlock.ToCbor();
  
  // 修复代码：使用std::vector而不是VLA
  std::vector<uint8_t> data(buffer.GetSize());
  buffer.CopyData(data.data(), buffer.GetSize());
  
  // 验证CRC
  return VerifyCRC(m_crcType, data, m_crcValue);
}

Buffer 
PrimaryBlock::ToCbor() const
{
  // 创建CBOR数组
  Cbor::CborArray array;
  
  // 添加主要区块元素
  array.push_back(Cbor::CborValue(m_version));
  array.push_back(Cbor::CborValue(static_cast<uint64_t>(m_bundleControlFlags)));
  array.push_back(Cbor::CborValue(static_cast<uint64_t>(m_crcType)));
  array.push_back(Cbor::CborValue(m_destinationEID.ToString()));
  array.push_back(Cbor::CborValue(m_sourceNodeEID.ToString()));
  array.push_back(Cbor::CborValue(m_reportToEID.ToString()));
  
  // 时间戳数组
  Cbor::CborArray timestampArray = {
    Cbor::CborValue(m_creationTimestamp.GetSeconds()),
    Cbor::CborValue(m_sequenceNumber)
  };
  array.push_back(Cbor::CborValue(timestampArray));
  
  // 生存时间（毫秒）
  array.push_back(Cbor::CborValue(m_lifetime.GetMilliSeconds()));
  
  // 如果是分片则添加分片字段
  if (IsFragment())
    {
      array.push_back(Cbor::CborValue(m_fragmentOffset));
      array.push_back(Cbor::CborValue(m_totalApplicationDataUnitLength));
    }
  
  // 添加CRC值（如果存在）
  if (m_crcType != CRCType::NO_CRC && !m_crcValue.empty())
    {
      array.push_back(Cbor::CborValue(m_crcValue));
    }
  
  return Cbor::Encode(Cbor::CborValue(array));
}

std::optional<PrimaryBlock> 
PrimaryBlock::FromCbor(Buffer buffer)
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
  if (array.size() < 8) // 至少需要8个元素（不带CRC的非分片）
    {
      return std::nullopt;
    }
  
  // 提取字段
  uint64_t version = array[0].GetUnsignedInteger();
  BundleControlFlags bundleControlFlags = static_cast<BundleControlFlags>(array[1].GetUnsignedInteger());
  CRCType crcType = static_cast<CRCType>(array[2].GetUnsignedInteger());
  EndpointID destinationEID(array[3].GetTextString());
  EndpointID sourceNodeEID(array[4].GetTextString());
  EndpointID reportToEID(array[5].GetTextString());
  
  // 提取时间戳
  if (!array[6].IsArray() || array[6].GetArray().size() != 2)
    {
      return std::nullopt;
    }
  
  DtnTime creationTimestamp(array[6].GetArray()[0].GetUnsignedInteger(), 0);
  uint64_t sequenceNumber = array[6].GetArray()[1].GetUnsignedInteger();
  
  // 提取生存时间
  Time lifetime = MilliSeconds(array[7].GetUnsignedInteger());
  
  // 创建主要区块
  PrimaryBlock block(
      version,
      bundleControlFlags,
      crcType,
      destinationEID,
      sourceNodeEID,
      reportToEID,
      creationTimestamp,
      sequenceNumber,
      lifetime);
  
  // 如果存在分片字段则提取
  bool isFragment = (static_cast<uint64_t>(bundleControlFlags) & 
                     static_cast<uint64_t>(BundleControlFlags::BUNDLE_IS_A_FRAGMENT)) != 0;
  
  if (isFragment)
    {
      if (array.size() < 10) // 分片至少需要10个元素
        {
          return std::nullopt;
        }
      
      block.SetFragmentOffset(array[8].GetUnsignedInteger());
      block.SetTotalApplicationDataUnitLength(array[9].GetUnsignedInteger());
    }
  
  // 如果存在CRC则提取
  if (crcType != CRCType::NO_CRC)
    {
      size_t crcIndex = isFragment ? 10 : 8;
      if (array.size() <= crcIndex)
        {
          return std::nullopt;
        }
      
      if (!array[crcIndex].IsByteString())
        {
          return std::nullopt;
        }
      
      block.m_crcValue = array[crcIndex].GetByteString();
    }
  
  return block;
}

std::string 
PrimaryBlock::ToString() const
{
  std::stringstream ss;
  
  ss << "PrimaryBlock(version=" << m_version
     << ", flags=" << static_cast<uint64_t>(m_bundleControlFlags)
     << ", crcType=" << CRCTypeToString(m_crcType)
     << ", dst=" << m_destinationEID.ToString()
     << ", src=" << m_sourceNodeEID.ToString()
     << ", report=" << m_reportToEID.ToString()
     << ", created=" << m_creationTimestamp.ToString()
     << ", seq=" << m_sequenceNumber
     << ", lifetime=" << m_lifetime.GetSeconds() << "s";
  
  if (IsFragment())
    {
      ss << ", fragOffset=" << m_fragmentOffset
         << ", totalLength=" << m_totalApplicationDataUnitLength;
    }
  
  ss << ")";
  
  return ss.str();
}

} // namespace dtn7

} // namespace ns3