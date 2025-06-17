#include "administrative-record.h"
#include "cbor.h"
#include "bundle.h"
#include <sstream>

namespace ns3 {

namespace dtn7 {

std::optional<Ptr<AdministrativeRecord>> 
AdministrativeRecord::FromCbor(Buffer buffer)
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
  if (array.size() < 2)
    {
      return std::nullopt;
    }

  uint64_t recordType = array[0].GetUnsignedInteger();

  // 根据记录类型创建特定的管理记录
  switch (static_cast<AdminRecordType>(recordType))
    {
      case AdminRecordType::BUNDLE_STATUS_REPORT:
        {
          auto reportOpt = BundleStatusReport::FromCbor(buffer);
          if (!reportOpt)
            {
              return std::nullopt;
            }
          Ptr<BundleStatusReport> report = Create<BundleStatusReport>(*reportOpt);
          return report;
        }
      case AdminRecordType::CUSTODY_SIGNAL:
        // 暂不支持管理信号
        // TODO: 实现管理信号
        return std::nullopt;
      default:
        return std::nullopt;
    }
}

// BundleStatusReport 实现

BundleStatusReport::BundleStatusReport()
  : m_statusFlags(BundleStatusFlag::NO_FLAGS),
    m_reasonCode(0)
{
}

BundleStatusReport::BundleStatusReport(BundleStatusFlag statusFlags,
                                       uint64_t reasonCode,
                                       const BundleID& refBundle,
                                       const EndpointID& sourceNode)
  : m_statusFlags(statusFlags),
    m_reasonCode(reasonCode),
    m_refBundle(refBundle),
    m_sourceNode(sourceNode)
{
}

AdminRecordType 
BundleStatusReport::GetAdministrativeRecordType() const
{
  return AdminRecordType::BUNDLE_STATUS_REPORT;
}

Buffer 
BundleStatusReport::ToCbor() const
{
  // 创建CBOR数组来表示状态报告
  Cbor::CborArray array;
  
  // 添加记录类型
  array.push_back(Cbor::CborValue(static_cast<uint64_t>(AdminRecordType::BUNDLE_STATUS_REPORT)));
  
  // 创建状态报告数组
  Cbor::CborArray reportArray;
  
  // 添加状态标志
  reportArray.push_back(Cbor::CborValue(static_cast<uint64_t>(m_statusFlags)));
  
  // 添加原因代码
  reportArray.push_back(Cbor::CborValue(m_reasonCode));
  
  // 添加源Bundle ID
  // 创建Bundle ID数组
  Cbor::CborArray bundleIdArray;
  bundleIdArray.push_back(Cbor::CborValue(m_sourceNode.ToString()));
  bundleIdArray.push_back(Cbor::CborValue(m_refBundle.GetTimestamp().GetSeconds()));
  bundleIdArray.push_back(Cbor::CborValue(m_refBundle.GetSequenceNumber()));
  
  if (m_refBundle.IsFragment())
    {
      bundleIdArray.push_back(Cbor::CborValue(static_cast<uint64_t>(1))); // 是分片的标志
      bundleIdArray.push_back(Cbor::CborValue(m_refBundle.GetFragmentOffset()));
    }
  else
    {
      bundleIdArray.push_back(Cbor::CborValue(static_cast<uint64_t>(0))); // 不是分片的标志
    }
  
  reportArray.push_back(Cbor::CborValue(bundleIdArray));
  
  // 添加各种时间戳（如果存在）
  if (HasStatusFlag(BundleStatusFlag::BUNDLE_RECEIVED) && m_receiveTime)
    {
      reportArray.push_back(Cbor::CborValue(m_receiveTime->GetSeconds()));
    }
  else
    {
      reportArray.push_back(Cbor::CborValue(static_cast<uint64_t>(0)));
    }
  
  if (HasStatusFlag(BundleStatusFlag::BUNDLE_FORWARDED) && m_forwardTime)
    {
      reportArray.push_back(Cbor::CborValue(m_forwardTime->GetSeconds()));
    }
  else
    {
      reportArray.push_back(Cbor::CborValue(static_cast<uint64_t>(0)));
    }
  
  if (HasStatusFlag(BundleStatusFlag::BUNDLE_DELIVERED) && m_deliveryTime)
    {
      reportArray.push_back(Cbor::CborValue(m_deliveryTime->GetSeconds()));
    }
  else
    {
      reportArray.push_back(Cbor::CborValue(static_cast<uint64_t>(0)));
    }
  
  if (HasStatusFlag(BundleStatusFlag::BUNDLE_DELETED) && m_deletionTime)
    {
      reportArray.push_back(Cbor::CborValue(m_deletionTime->GetSeconds()));
    }
  else
    {
      reportArray.push_back(Cbor::CborValue(static_cast<uint64_t>(0)));
    }
  
  // 添加状态报告数组到主数组
  array.push_back(Cbor::CborValue(reportArray));
  
  return Cbor::Encode(Cbor::CborValue(array));
}

std::string 
BundleStatusReport::ToString() const
{
  std::stringstream ss;
  
  ss << "BundleStatusReport(";
  
  ss << "flags=";
  if (HasStatusFlag(BundleStatusFlag::BUNDLE_RECEIVED))
    ss << "RECEIVED,";
  if (HasStatusFlag(BundleStatusFlag::BUNDLE_FORWARDED))
    ss << "FORWARDED,";
  if (HasStatusFlag(BundleStatusFlag::BUNDLE_DELIVERED))
    ss << "DELIVERED,";
  if (HasStatusFlag(BundleStatusFlag::BUNDLE_DELETED))
    ss << "DELETED,";
  if (HasStatusFlag(BundleStatusFlag::ACKNOWLEDGED_BY_CUSTODIAN))
    ss << "ACKNOWLEDGED,";
  
  ss << "reason=" << m_reasonCode;
  ss << ", refBundle=" << m_refBundle.ToString();
  ss << ", source=" << m_sourceNode.ToString();
  
  if (m_receiveTime)
    ss << ", receiveTime=" << m_receiveTime->ToString();
  if (m_forwardTime)
    ss << ", forwardTime=" << m_forwardTime->ToString();
  if (m_deliveryTime)
    ss << ", deliveryTime=" << m_deliveryTime->ToString();
  if (m_deletionTime)
    ss << ", deletionTime=" << m_deletionTime->ToString();
  
  ss << ")";
  
  return ss.str();
}

std::optional<BundleStatusReport> 
BundleStatusReport::FromCbor(Buffer buffer)
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
  if (array.size() < 2)
    {
      return std::nullopt;
    }

  // 检查记录类型
  uint64_t recordType = array[0].GetUnsignedInteger();
  if (recordType != static_cast<uint64_t>(AdminRecordType::BUNDLE_STATUS_REPORT))
    {
      return std::nullopt;
    }

  const auto& reportArray = array[1].GetArray();
  if (reportArray.size() < 7)  // 至少需要7个元素
    {
      return std::nullopt;
    }

  // 解析状态标志
  BundleStatusFlag statusFlags = static_cast<BundleStatusFlag>(reportArray[0].GetUnsignedInteger());
  
  // 解析原因代码
  uint64_t reasonCode = reportArray[1].GetUnsignedInteger();
  
  // 解析源Bundle ID
  const auto& bundleIdArray = reportArray[2].GetArray();
  if (bundleIdArray.size() < 4)
    {
      return std::nullopt;
    }
  
  EndpointID sourceNode(bundleIdArray[0].GetTextString());
  DtnTime timestamp(bundleIdArray[1].GetUnsignedInteger());
  uint64_t sequenceNumber = bundleIdArray[2].GetUnsignedInteger();
  
  bool isFragment = bundleIdArray[3].GetUnsignedInteger() != 0;
  uint64_t fragmentOffset = 0;
  
  if (isFragment && bundleIdArray.size() >= 5)
    {
      fragmentOffset = bundleIdArray[4].GetUnsignedInteger();
    }
  
  // 创建Bundle ID
  BundleID refBundle(sourceNode, timestamp, sequenceNumber, isFragment, fragmentOffset);
  
  // 创建状态报告
  BundleStatusReport report(statusFlags, reasonCode, refBundle, sourceNode);
  
  // 解析各种时间戳
  if (static_cast<uint64_t>(statusFlags) & static_cast<uint64_t>(BundleStatusFlag::BUNDLE_RECEIVED))
    {
      uint64_t receiveTime = reportArray[3].GetUnsignedInteger();
      if (receiveTime > 0)
        {
          report.SetReceiveTime(DtnTime(receiveTime));
        }
    }
  
  if (static_cast<uint64_t>(statusFlags) & static_cast<uint64_t>(BundleStatusFlag::BUNDLE_FORWARDED))
    {
      uint64_t forwardTime = reportArray[4].GetUnsignedInteger();
      if (forwardTime > 0)
        {
          report.SetForwardTime(DtnTime(forwardTime));
        }
    }
  
  if (static_cast<uint64_t>(statusFlags) & static_cast<uint64_t>(BundleStatusFlag::BUNDLE_DELIVERED))
    {
      uint64_t deliveryTime = reportArray[5].GetUnsignedInteger();
      if (deliveryTime > 0)
        {
          report.SetDeliveryTime(DtnTime(deliveryTime));
        }
    }
  
  if (static_cast<uint64_t>(statusFlags) & static_cast<uint64_t>(BundleStatusFlag::BUNDLE_DELETED))
    {
      uint64_t deletionTime = reportArray[6].GetUnsignedInteger();
      if (deletionTime > 0)
        {
          report.SetDeletionTime(DtnTime(deletionTime));
        }
    }
  
  return report;
}

Ptr<BundleStatusReport> 
BundleStatusReport::CreateReceivedReport(Ptr<Bundle> bundle, const EndpointID& reportingNode)
{
  BundleStatusFlag flags = BundleStatusFlag::BUNDLE_RECEIVED;
  uint64_t reasonCode = static_cast<uint64_t>(ReasonCode::NO_INFORMATION);
  BundleID bundleId = bundle->GetId();
  EndpointID sourceNode = bundle->GetPrimaryBlock().GetSourceNodeEID();
  
  Ptr<BundleStatusReport> report = Create<BundleStatusReport>(flags, reasonCode, bundleId, sourceNode);
  report->SetReceiveTime(GetDtnNow());
  
  return report;
}

Ptr<BundleStatusReport> 
BundleStatusReport::CreateForwardedReport(Ptr<Bundle> bundle, const EndpointID& reportingNode)
{
  BundleStatusFlag flags = BundleStatusFlag::BUNDLE_FORWARDED;
  uint64_t reasonCode = static_cast<uint64_t>(ReasonCode::NO_INFORMATION);
  BundleID bundleId = bundle->GetId();
  EndpointID sourceNode = bundle->GetPrimaryBlock().GetSourceNodeEID();
  
  Ptr<BundleStatusReport> report = Create<BundleStatusReport>(flags, reasonCode, bundleId, sourceNode);
  report->SetForwardTime(GetDtnNow());
  
  return report;
}

Ptr<BundleStatusReport> 
BundleStatusReport::CreateDeliveredReport(Ptr<Bundle> bundle, const EndpointID& reportingNode)
{
  BundleStatusFlag flags = BundleStatusFlag::BUNDLE_DELIVERED;
  uint64_t reasonCode = static_cast<uint64_t>(ReasonCode::NO_INFORMATION);
  BundleID bundleId = bundle->GetId();
  EndpointID sourceNode = bundle->GetPrimaryBlock().GetSourceNodeEID();
  
  Ptr<BundleStatusReport> report = Create<BundleStatusReport>(flags, reasonCode, bundleId, sourceNode);
  report->SetDeliveryTime(GetDtnNow());
  
  return report;
}

Ptr<BundleStatusReport> 
BundleStatusReport::CreateDeletedReport(Ptr<Bundle> bundle, uint64_t reasonCode, const EndpointID& reportingNode)
{
  BundleStatusFlag flags = BundleStatusFlag::BUNDLE_DELETED;
  BundleID bundleId = bundle->GetId();
  EndpointID sourceNode = bundle->GetPrimaryBlock().GetSourceNodeEID();
  
  Ptr<BundleStatusReport> report = Create<BundleStatusReport>(flags, reasonCode, bundleId, sourceNode);
  report->SetDeletionTime(GetDtnNow());
  
  return report;
}

bool 
BundleStatusReport::HasStatusFlag(BundleStatusFlag flag) const
{
  return (static_cast<uint64_t>(m_statusFlags) & static_cast<uint64_t>(flag)) != 0;
}

} // namespace dtn7

} // namespace ns3