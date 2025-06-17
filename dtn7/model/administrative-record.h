#ifndef DTN7_ADMINISTRATIVE_RECORD_H
#define DTN7_ADMINISTRATIVE_RECORD_H

#include "ns3/buffer.h"
#include "ns3/simple-ref-count.h"
#include "ns3/ptr.h"

#include <vector>
#include <string>
#include <optional>

#include "bundle-id.h"
#include "endpoint.h"
#include "dtn-time.h"
#include "bundle.h"

namespace ns3 {

namespace dtn7 {

/**
 * \brief 管理记录类型
 */
enum class AdminRecordType : uint64_t {
  BUNDLE_STATUS_REPORT = 1,  //!< Bundle状态报告
  CUSTODY_SIGNAL = 2         //!< 管理信号
};

/**
 * \brief Bundle状态标志
 */
enum class BundleStatusFlag : uint64_t {
  NO_FLAGS = 0,
  BUNDLE_RECEIVED = 1 << 0,           //!< Bundle已接收
  BUNDLE_FORWARDED = 1 << 1,          //!< Bundle已转发
  BUNDLE_DELIVERED = 1 << 2,          //!< Bundle已投递
  BUNDLE_DELETED = 1 << 3,            //!< Bundle已删除
  ACKNOWLEDGED_BY_CUSTODIAN = 1 << 4  //!< Bundle已被管理者确认
};

/**
 * \brief 位操作符实现
 */
inline BundleStatusFlag operator| (BundleStatusFlag a, BundleStatusFlag b) {
  return static_cast<BundleStatusFlag>(static_cast<uint64_t>(a) | static_cast<uint64_t>(b));
}

inline BundleStatusFlag operator& (BundleStatusFlag a, BundleStatusFlag b) {
  return static_cast<BundleStatusFlag>(static_cast<uint64_t>(a) & static_cast<uint64_t>(b));
}

/**
 * \ingroup dtn7
 * \brief 管理记录的基类
 */
class AdministrativeRecord : public SimpleRefCount<AdministrativeRecord> {
public:
  /**
   * \brief 获取管理记录类型
   * \return 管理记录类型
   */
  virtual AdminRecordType GetAdministrativeRecordType() const = 0;
  
  /**
   * \brief 序列化为CBOR格式
   * \return CBOR编码的数据
   */
  virtual Buffer ToCbor() const = 0;
  
  /**
   * \brief 从CBOR反序列化
   * \param buffer CBOR编码的数据
   * \return 反序列化的管理记录，失败返回空
   */
  static std::optional<Ptr<AdministrativeRecord>> FromCbor(Buffer buffer);
  
  /**
   * \brief 获取文本表示
   * \return 文本表示
   */
  virtual std::string ToString() const = 0;
  
  /**
   * \brief 虚析构函数
   */
  virtual ~AdministrativeRecord() = default;
};

/**
 * \ingroup dtn7
 * \brief Bundle状态报告
 */
class BundleStatusReport : public AdministrativeRecord {
public:
  /**
   * \brief 默认构造函数
   */
  BundleStatusReport();
  
  /**
   * \brief 使用参数构造状态报告
   * \param statusFlags 状态标志
   * \param reasonCode 原因代码
   * \param refBundle 引用的Bundle ID
   * \param sourceNode 源节点
   */
  BundleStatusReport(BundleStatusFlag statusFlags,
                    uint64_t reasonCode,
                    const BundleID& refBundle,
                    const EndpointID& sourceNode);
  
  // 继承自AdministrativeRecord
  AdminRecordType GetAdministrativeRecordType() const override;
  Buffer ToCbor() const override;
  std::string ToString() const override;
  
  /**
   * \brief 从CBOR反序列化
   * \param buffer CBOR编码的数据
   * \return 反序列化的状态报告，失败返回空
   */
  static std::optional<BundleStatusReport> FromCbor(Buffer buffer);
  
  /**
   * \brief 创建Bundle成功接收状态报告
   * \param bundle 源Bundle
   * \param reportingNode 报告节点
   * \return 状态报告
   */
  static Ptr<BundleStatusReport> CreateReceivedReport(Ptr<Bundle> bundle, const EndpointID& reportingNode);
  
  /**
   * \brief 创建Bundle成功转发状态报告
   * \param bundle 源Bundle
   * \param reportingNode 报告节点
   * \return 状态报告
   */
  static Ptr<BundleStatusReport> CreateForwardedReport(Ptr<Bundle> bundle, const EndpointID& reportingNode);
  
  /**
   * \brief 创建Bundle成功投递状态报告
   * \param bundle 源Bundle
   * \param reportingNode 报告节点
   * \return 状态报告
   */
  static Ptr<BundleStatusReport> CreateDeliveredReport(Ptr<Bundle> bundle, const EndpointID& reportingNode);
  
  /**
   * \brief 创建Bundle删除状态报告
   * \param bundle 源Bundle
   * \param reasonCode 原因代码
   * \param reportingNode 报告节点
   * \return 状态报告
   */
  static Ptr<BundleStatusReport> CreateDeletedReport(Ptr<Bundle> bundle, uint64_t reasonCode, const EndpointID& reportingNode);

  /**
   * \brief 检查是否包含指定状态标志
   * \param flag 要检查的状态标志
   * \return 如果包含返回true
   */
  bool HasStatusFlag(BundleStatusFlag flag) const;
  
  /**
   * \brief 获取状态标志
   * \return 状态标志
   */
  BundleStatusFlag GetStatusFlags() const { return m_statusFlags; }
  
  /**
   * \brief 获取原因代码
   * \return 原因代码
   */
  uint64_t GetReasonCode() const { return m_reasonCode; }
  
  /**
   * \brief 获取引用的Bundle ID
   * \return Bundle ID
   */
  const BundleID& GetRefBundle() const { return m_refBundle; }
  
  /**
   * \brief 获取源节点
   * \return 源节点
   */
  const EndpointID& GetSourceNode() const { return m_sourceNode; }
  
  /**
   * \brief 获取接收时间
   * \return 接收时间
   */
  std::optional<DtnTime> GetReceiveTime() const { return m_receiveTime; }
  
  /**
   * \brief 获取转发时间
   * \return 转发时间
   */
  std::optional<DtnTime> GetForwardTime() const { return m_forwardTime; }
  
  /**
   * \brief 获取投递时间
   * \return 投递时间
   */
  std::optional<DtnTime> GetDeliveryTime() const { return m_deliveryTime; }
  
  /**
   * \brief 获取删除时间
   * \return 删除时间
   */
  std::optional<DtnTime> GetDeletionTime() const { return m_deletionTime; }
  
  /**
   * \brief 设置状态标志
   * \param flags 状态标志
   */
  void SetStatusFlags(BundleStatusFlag flags) { m_statusFlags = flags; }
  
  /**
   * \brief 设置原因代码
   * \param code 原因代码
   */
  void SetReasonCode(uint64_t code) { m_reasonCode = code; }
  
  /**
   * \brief 设置引用的Bundle ID
   * \param bundleId Bundle ID
   */
  void SetRefBundle(const BundleID& bundleId) { m_refBundle = bundleId; }
  
  /**
   * \brief 设置源节点
   * \param node 源节点
   */
  void SetSourceNode(const EndpointID& node) { m_sourceNode = node; }
  
  /**
   * \brief 设置接收时间
   * \param time 接收时间
   */
  void SetReceiveTime(const DtnTime& time) { m_receiveTime = time; }
  
  /**
   * \brief 设置转发时间
   * \param time 转发时间
   */
  void SetForwardTime(const DtnTime& time) { m_forwardTime = time; }
  
  /**
   * \brief 设置投递时间
   * \param time 投递时间
   */
  void SetDeliveryTime(const DtnTime& time) { m_deliveryTime = time; }
  
  /**
   * \brief 设置删除时间
   * \param time 删除时间
   */
  void SetDeletionTime(const DtnTime& time) { m_deletionTime = time; }

private:
  BundleStatusFlag m_statusFlags;          //!< 状态标志
  uint64_t m_reasonCode;                   //!< 原因代码
  BundleID m_refBundle;                    //!< 引用的Bundle ID
  EndpointID m_sourceNode;                 //!< 源节点
  std::optional<DtnTime> m_receiveTime;    //!< 接收时间
  std::optional<DtnTime> m_forwardTime;    //!< 转发时间
  std::optional<DtnTime> m_deliveryTime;   //!< 投递时间
  std::optional<DtnTime> m_deletionTime;   //!< 删除时间
};

/**
 * \brief 原因代码
 */
enum class ReasonCode : uint64_t {
  NO_INFORMATION = 0,                    //!< 没有信息
  LIFETIME_EXPIRED = 1,                  //!< 生命周期过期
  FORWARDED_OVER_UNIDIRECTIONAL_LINK = 2, //!< 通过单向链路转发
  TRANSMISSION_CANCELED = 3,             //!< 传输被取消
  DEPLETED_STORAGE = 4,                  //!< 存储耗尽
  DESTINATION_ENDPOINT_ID_UNINTELLIGIBLE = 5, //!< 目标端点ID无法解析
  NO_ROUTE_TO_DESTINATION_FROM_HERE = 6, //!< 从这里无法路由到目的地
  NO_TIMELY_CONTACT_WITH_NEXT_NODE_ON_ROUTE = 7, //!< 与路由上的下一节点没有及时联系
  BLOCK_UNINTELLIGIBLE = 8,              //!< 区块无法解析
  HOP_LIMIT_EXCEEDED = 9,                //!< 超过跳数限制
  TRAFFIC_PARED = 10,                    //!< 流量被削减
  BLOCK_UNSUPPORTED = 11                 //!< 不支持的区块
};

} // namespace dtn7

} // namespace ns3

#endif /* DTN7_ADMINISTRATIVE_RECORD_H */