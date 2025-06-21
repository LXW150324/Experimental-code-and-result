// 文件: ndn-simulation.cc
// 用于NS-3.44的NDN Interest包机制实现 - 增强版 (修改版)
// 修改: 增加了订阅者ID和发布者ID，改进了路径选择，加入物理距离感知功能
// 修复: 增强了移动速度对实验结果的影响

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/wifi-module.h"
#include "ns3/netanim-module.h"
#include "ns3/flow-monitor-module.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <string>
#include <algorithm>
#include <memory>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <set>  // Use set instead of unordered_set for Address
#include <cmath>  // 确保包含cmath以使用数学函数

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("NdnInterestSimulation");

// 全局参数
uint32_t numNodes = 50;       // 总节点数
uint32_t gridSize = 7;        // 网格大小
double simulationTime = 500.0; // 仿真时间(秒)
double nodeMobility = 5.0;    // 移动速度(m/s)
uint32_t packetSize = 1024;   // 数据包大小(字节)
bool enableDetailedLogging = true; // 启用详细日志记录，方便调试
double communicationRange = 250.0; // 通信范围(m) - 新增参数，影响物理距离判断
double routingUpdateInterval = 2.0; // 路由更新间隔(秒) - 新增参数，随移动速度调整

// NDN兴趣包类型
enum NdnPacketType {
  INTEREST = 1,
  DATA = 2,
  NOTIFICATION = 3 // 代理迁移通知
};

// 修改后的NDN头: 增加订阅者ID和发布者ID
class NdnHeader : public Header 
{
public:
  NdnHeader ();
  virtual ~NdnHeader ();

  // 内容名字/前缀
  void SetContentName (std::string contentName);
  std::string GetContentName (void) const;

  // 包类型: Interest, Data, Notification
  void SetPacketType (uint8_t type);
  uint8_t GetPacketType (void) const;

  // Nonce值
  void SetNonce (uint32_t nonce);
  uint32_t GetNonce (void) const;

  // 新增: 订阅者ID
  void SetSubscriberId (uint32_t id);
  uint32_t GetSubscriberId (void) const;

  // 新增: 发布者ID
  void SetPublisherId (uint32_t id);
  uint32_t GetPublisherId (void) const;

  // 新代理ID (用于通知包)
  void SetNewBrokerId (uint32_t id);
  uint32_t GetNewBrokerId (void) const;
  
  // 迁移时间戳 (用于计算通知延迟)
  void SetMigrationTime (Time time);
  Time GetMigrationTime (void) const;

  // 必须的Header接口方法
  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;
  virtual void Print (std::ostream &os) const;
  virtual void Serialize (Buffer::Iterator start) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);
  virtual uint32_t GetSerializedSize (void) const;

private:
  std::string m_contentName;
  uint8_t m_packetType;
  uint32_t m_nonce;
  uint32_t m_subscriberId;  // 新增: 订阅者ID
  uint32_t m_publisherId;   // 新增: 发布者ID
  uint32_t m_newBrokerId;
  Time m_migrationTime; // 添加迁移时间戳
};

NdnHeader::NdnHeader ()
  : m_contentName ("/"),
    m_packetType (INTEREST),
    m_nonce (0),
    m_subscriberId (0),  // 初始化订阅者ID
    m_publisherId (0),   // 初始化发布者ID
    m_newBrokerId (0),
    m_migrationTime (Seconds(0)) // 初始化迁移时间
{
}

NdnHeader::~NdnHeader ()
{
}

TypeId
NdnHeader::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::NdnHeader")
    .SetParent<Header> ()
    .AddConstructor<NdnHeader> ()
  ;
  return tid;
}

TypeId
NdnHeader::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

void
NdnHeader::Print (std::ostream &os) const
{
  os << "ContentName=" << m_contentName;
  os << ", Type=" << (uint32_t) m_packetType;
  os << ", Nonce=" << m_nonce;
  
  // 添加新字段到输出
  if (m_packetType == INTEREST) {
    os << ", SubscriberId=" << m_subscriberId;
    os << ", PublisherId=" << m_publisherId;
  }
  
  if (m_packetType == NOTIFICATION)
    {
      os << ", NewBrokerId=" << m_newBrokerId;
      os << ", MigrationTime=" << m_migrationTime.GetSeconds() << "s";
    }
}

uint32_t
NdnHeader::GetSerializedSize (void) const
{
  // 更新序列化大小，包括订阅者ID和发布者ID
  return m_contentName.size() + 1 + sizeof(uint8_t) + sizeof(uint32_t) + 
         sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint32_t) + sizeof(double);
}

void
NdnHeader::Serialize (Buffer::Iterator start) const
{
  // 序列化内容名字
  start.WriteU8 (m_contentName.size());
  for (uint32_t i = 0; i < m_contentName.size(); i++)
    {
      start.WriteU8 (m_contentName[i]);
    }
  
  // 序列化包类型
  start.WriteU8 (m_packetType);
  
  // 序列化Nonce
  start.WriteHtonU32 (m_nonce);
  
  // 序列化订阅者ID
  start.WriteHtonU32 (m_subscriberId);
  
  // 序列化发布者ID
  start.WriteHtonU32 (m_publisherId);
  
  // 序列化新代理ID
  start.WriteHtonU32 (m_newBrokerId);
  
  // 序列化迁移时间戳 (以秒为单位的双精度值)
  start.WriteHtonU64 (m_migrationTime.GetNanoSeconds());
}

uint32_t
NdnHeader::Deserialize (Buffer::Iterator start)
{
  // 反序列化内容名字
  uint8_t nameSize = start.ReadU8 ();
  m_contentName.clear ();
  for (uint8_t i = 0; i < nameSize; i++)
    {
      m_contentName.push_back (start.ReadU8 ());
    }
  
  // 反序列化包类型
  m_packetType = start.ReadU8 ();
  
  // 反序列化Nonce
  m_nonce = start.ReadNtohU32 ();
  
  // 反序列化订阅者ID
  m_subscriberId = start.ReadNtohU32 ();
  
  // 反序列化发布者ID
  m_publisherId = start.ReadNtohU32 ();
  
  // 反序列化新代理ID
  m_newBrokerId = start.ReadNtohU32 ();
  
  // 反序列化迁移时间戳
  uint64_t timeNs = start.ReadNtohU64 ();
  m_migrationTime = NanoSeconds (timeNs);
  
  return GetSerializedSize ();
}

void
NdnHeader::SetContentName (std::string contentName)
{
  m_contentName = contentName;
}

std::string
NdnHeader::GetContentName (void) const
{
  return m_contentName;
}

void
NdnHeader::SetPacketType (uint8_t type)
{
  m_packetType = type;
}

uint8_t
NdnHeader::GetPacketType (void) const
{
  return m_packetType;
}

void
NdnHeader::SetNonce (uint32_t nonce)
{
  m_nonce = nonce;
}

uint32_t
NdnHeader::GetNonce (void) const
{
  return m_nonce;
}

// 新增: 设置订阅者ID
void
NdnHeader::SetSubscriberId (uint32_t id)
{
  m_subscriberId = id;
}

// 新增: 获取订阅者ID
uint32_t
NdnHeader::GetSubscriberId (void) const
{
  return m_subscriberId;
}

// 新增: 设置发布者ID
void
NdnHeader::SetPublisherId (uint32_t id)
{
  m_publisherId = id;
}

// 新增: 获取发布者ID
uint32_t
NdnHeader::GetPublisherId (void) const
{
  return m_publisherId;
}

void
NdnHeader::SetNewBrokerId (uint32_t id)
{
  m_newBrokerId = id;
}

uint32_t
NdnHeader::GetNewBrokerId (void) const
{
  return m_newBrokerId;
}

void
NdnHeader::SetMigrationTime (Time time)
{
  m_migrationTime = time;
}

Time
NdnHeader::GetMigrationTime (void) const
{
  return m_migrationTime;
}

// 修改: PIT (Pending Interest Table) 条目 - 增加订阅者ID和发布者ID
struct PitEntry {
  std::string contentName;
  uint32_t nonce;
  uint32_t subscriberId;  // 新增: 订阅者ID
  uint32_t publisherId;   // 新增: 发布者ID
  InetSocketAddress sourceAddress; // 使用InetSocketAddress替代Address
  Time expiryTime;
  
  // 添加显式构造函数
  PitEntry() : contentName(""), nonce(0), subscriberId(0), publisherId(0), 
               sourceAddress(Ipv4Address::GetAny(), 0), expiryTime(Seconds(0)) {}
  
  // 添加带参数的构造函数
  PitEntry(std::string name, uint32_t n, uint32_t sid, uint32_t pid, 
           InetSocketAddress addr, Time expiry) 
    : contentName(name), nonce(n), subscriberId(sid), publisherId(pid), 
      sourceAddress(addr), expiryTime(expiry) {}
};

// FIB (Forwarding Information Base) 条目 - 增加对订阅者路径的记录和距离信息
struct FibEntry {
  std::string prefix;
  std::vector<InetSocketAddress> nextHops; // 修改为明确的InetSocketAddress类型
  
  // 新增: 订阅者路径记录 <订阅者ID, 最佳下一跳>
  std::map<uint32_t, InetSocketAddress> subscriberPaths;
  
  // 新增: 节点距离信息 <节点ID, 距离>
  std::map<uint32_t, double> nodeDistances;
  
  // 新增: 节点速度和稳定性评分 <节点ID, 稳定性评分>
  std::map<uint32_t, double> nodeStability;
  
  // 新增: 最后更新时间
  Time lastUpdateTime;
  
  // 添加显式构造函数
  FibEntry() : prefix(""), lastUpdateTime(Seconds(0)) {}
  
  // 添加带参数的构造函数
  FibEntry(std::string p) : prefix(p), lastUpdateTime(Seconds(0)) {}
};

// 内容存储条目
struct ContentStoreEntry {
  std::string contentName;
  Ptr<Packet> data;
  Time expiryTime;
};

// 新增: 物理位置信息
struct NodeLocationInfo {
  Vector position;     // 节点位置
  Vector velocity;     // 节点速度向量 (新增)
  Time lastUpdateTime; // 上次更新时间
  double speed;        // 节点移动速度
  double acceleration; // 加速度 (新增)
  
  NodeLocationInfo() : position(Vector(0,0,0)), velocity(Vector(0,0,0)), 
                      lastUpdateTime(Seconds(0)), speed(0), acceleration(0) {}
  
  NodeLocationInfo(Vector pos, Vector vel, Time time, double s, double a) 
    : position(pos), velocity(vel), lastUpdateTime(time), speed(s), acceleration(a) {}
};

// NDN节点应用
class NdnApp : public Application
{
public:
  static TypeId GetTypeId (void);
  NdnApp ();
  virtual ~NdnApp ();

  // 设置节点角色 (发布者/订阅者/代理/普通节点)
  enum NodeRole { PUBLISHER, SUBSCRIBER, BROKER, REGULAR };
  void SetNodeRole (NodeRole role);
  NodeRole GetNodeRole (void) const;

  // 通知类型枚举
  enum NotificationType {
    UNICAST_NOTIFICATION = 1,   // 发布者单播通知
    INTEREST_NOTIFICATION = 2,  // 基于NDN兴趣的通知
    BROADCAST_NOTIFICATION = 3  // 大规模广播通知
  };

  // 设置/获取当前代理ID
  void SetCurrentBrokerId (uint32_t id);
  uint32_t GetCurrentBrokerId (void) const;

  // 开始发布内容 (发布者使用)
  void StartPublishing (std::string contentPrefix);
  
  // 开始订阅内容 (订阅者使用)
  void StartSubscribing (std::string contentPrefix);
  
  // 迁移代理功能 (旧代理调用)
  void MigrateBroker (uint32_t newBrokerId);
  
  // 记录通知延迟 (测量从代理迁移到订阅者收到通知的时间)
  void RecordNotificationDelay (Time delay);
  
  // 记录消息成功率
  void RecordMessageSuccess (bool success);

  // 应用程序初始化（延迟执行）
  void DoInitialize (void);

  // 获取通知延迟统计
  static Time GetAverageNotificationDelay (void);
  static Time GetUnicastNotificationDelay (void);
  static Time GetInterestNotificationDelay (void);
  static Time GetBroadcastNotificationDelay (void);
  
  // 计算两点之间的距离
  static double CalculateDistance(Vector a, Vector b);
  
  // 获取消息成功率统计
  static double GetMessageSuccessRate (void);

  // 手动触发代理迁移测试
  void TriggerBrokerMigration (uint32_t newBrokerId);
  
  // 新增: 更新节点位置信息
  void UpdateNodeLocation();
  
  // 新增: 周期性更新路由拓扑
  void UpdateRoutingTopology();
  
  // 新增: 获取最近节点ID
  uint32_t GetNearestNodeId(Vector position, std::vector<uint32_t> excludeIds = {});
  
  // 新增: 判断是否在通信范围内
  bool IsInCommunicationRange(uint32_t nodeId);
  
  // 新增: 计算节点稳定性评分
  double CalculateNodeStability(uint32_t nodeId);
  
  // 新增: 预测节点未来位置
  Vector PredictFuturePosition(uint32_t nodeId, double timeOffset);

  // 声明为公有以便在主函数中访问
  static uint32_t s_outOfRangeFailures;

protected:
  virtual void DoDispose (void);

private:
  virtual void StartApplication (void);
  virtual void StopApplication (void);

  // 发送Interest包
  void SendInterest (std::string contentName);
  
  // 发送Data包 - 修改为支持订阅者路径选择
  void SendData (std::string contentName, InetSocketAddress destination, uint32_t subscriberId);
  
  // 发送代理迁移通知 - 三种通知方法
  void SendBrokerNotification (uint32_t newBrokerId);
  void SendUnicastNotification (uint32_t newBrokerId);
  void SendInterestBasedNotification (uint32_t newBrokerId);
  void SendBroadcastNotification (uint32_t newBrokerId, bool forceBroadcast = false);
  
  // 处理收到的包
  void ReceivePacket (Ptr<Socket> socket);
  
  // 处理Interest包
  void HandleInterest (Ptr<Packet> packet, Address from, NdnHeader &header);
  
  // 处理Data包
  void HandleData (Ptr<Packet> packet, Address from, NdnHeader &header);
  
  // 处理代理迁移通知包
  void HandleBrokerNotification (Ptr<Packet> packet, Address from, NdnHeader &header);
  
  // 检查内容存储
  bool CheckContentStore (std::string contentName, Ptr<Packet> &outPacket);
  
  // 添加到内容存储
  void AddToContentStore (std::string contentName, Ptr<Packet> packet);
  
  // 修改: 添加到PIT，包含订阅者ID和发布者ID
  void AddToPit (std::string contentName, uint32_t nonce, uint32_t subscriberId, uint32_t publisherId, Address source);
  
  // 修改: 查找匹配PIT条目并移除，返回订阅者ID
  bool FindAndRemovePitEntry (std::string contentName, std::vector<InetSocketAddress> &outSources, std::vector<uint32_t> &outSubscriberIds);
  
  // 查找FIB条目
  bool FindFibEntry (std::string contentName, std::vector<InetSocketAddress> &outNextHops);
  
  // 修改: 添加/更新FIB条目，包括订阅者路径
  void UpdateFib (std::string prefix, Address nextHop, uint32_t subscriberId = 0, double distance = -1);
  
  // 新增: 获取到特定订阅者的最佳路径
  InetSocketAddress GetBestPathToSubscriber(uint32_t subscriberId);
  
  // 创建和初始化Socket
  void CreateSocket (void);
  
  // 周期性清理过期PIT条目
  void CleanupExpiredPitEntries ();
  
  // 订阅者定期发送内容请求
  void ScheduleNextRequest ();
  
  // 生成内容请求 (订阅者使用)
  void RequestContent ();

  // 估计订阅者数量
  uint32_t EstimateSubscriberCount ();
  
  // 新增: 从NodeId获取IP地址
  Ipv4Address GetIpv4FromNodeId(uint32_t nodeId);
  
  // 新增: 获取节点当前位置
  Vector GetCurrentPosition();
  
  // 新增: 获取节点当前速度向量
  Vector GetCurrentVelocity();

  // 节点角色
  NodeRole m_nodeRole;
  
  // 当前代理ID
  uint32_t m_currentBrokerId;
  
  // 内容前缀
  std::string m_contentPrefix;
  
  // 请求序号
  uint32_t m_requestSequence;
  
  // 上一个知道的代理ID
  uint32_t m_lastKnownBrokerId;
  
  // 迁移前的时间戳
  Time m_migrationStartTime;
  
  // 最后一次兴趣通知时间
  Time m_lastInterestNotificationTime;
  
  // 最后一次广播时间
  Time m_lastBroadcastTime;
  
  // 当前通知类型
  NotificationType m_notificationType;
  
  // 迁移事件ID
  EventId m_migrationEvent;
  
  // 请求事件ID
  EventId m_requestEvent;
  
  // 清理事件ID
  EventId m_cleanupEvent;
  
  // 新增: 位置更新事件ID
  EventId m_locationUpdateEvent;
  
  // 新增: 路由拓扑更新事件ID
  EventId m_routingUpdateEvent;
  
  // UDP socket
  Ptr<Socket> m_socket;
  
  // PIT表
  std::vector<PitEntry> m_pitTable;
  
  // FIB表
  std::vector<FibEntry> m_fibTable;
  
  // 内容存储
  std::vector<ContentStoreEntry> m_contentStore;
  
  // 通知延迟累计 - 总体
  static Time s_totalNotificationDelay;
  static uint32_t s_notificationCount;
  
  // 通知延迟累计 - 按类型
  static Time s_unicastNotificationDelay;
  static uint32_t s_unicastNotificationCount;
  static Time s_interestNotificationDelay;
  static uint32_t s_interestNotificationCount;
  static Time s_broadcastNotificationDelay;
  static uint32_t s_broadcastNotificationCount;
  
  // 消息成功率统计
  static uint32_t s_messagesSent;
  static uint32_t s_messagesReceived;
  
  // 已经处理过的Nonce和订阅者ID组合集合 (避免处理重复兴趣包)
  std::set<std::pair<uint32_t, uint32_t>> m_processedInterests;

  // 随机数生成器
  Ptr<UniformRandomVariable> m_rand;
  
  // 新增: 已知的发布者ID (初始为0，表示未知)
  uint32_t m_knownPublisherId;
  
  // 新增: 节点位置信息缓存 <节点ID, 位置信息>
  static std::map<uint32_t, NodeLocationInfo> s_nodeLocations;
  
  // 新增: 上次位置更新时保存的位置
  Vector m_lastPosition;
  
  // 新增: 连续位置更新计数器
  uint32_t m_positionUpdateCount;
};

// 初始化静态成员
std::map<uint32_t, NodeLocationInfo> NdnApp::s_nodeLocations;
uint32_t NdnApp::s_outOfRangeFailures = 0;

// 静态成员初始化 - 总体统计
Time NdnApp::s_totalNotificationDelay = Time (0);
uint32_t NdnApp::s_notificationCount = 0;
uint32_t NdnApp::s_messagesSent = 0;
uint32_t NdnApp::s_messagesReceived = 0;

// 静态成员初始化 - 按通知类型统计
Time NdnApp::s_unicastNotificationDelay = Time (0);
uint32_t NdnApp::s_unicastNotificationCount = 0;
Time NdnApp::s_interestNotificationDelay = Time (0);
uint32_t NdnApp::s_interestNotificationCount = 0;
Time NdnApp::s_broadcastNotificationDelay = Time (0);
uint32_t NdnApp::s_broadcastNotificationCount = 0;

TypeId
NdnApp::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::NdnApp")
    .SetParent<Application> ()
    .AddConstructor<NdnApp> ()
  ;
  return tid;
}

NdnApp::NdnApp ()
  : m_nodeRole (REGULAR),
    m_currentBrokerId (0),
    m_contentPrefix ("/content"),
    m_requestSequence (0),
    m_lastKnownBrokerId (0),
    m_lastInterestNotificationTime (Seconds(0)),
    m_lastBroadcastTime (Seconds(0)),
    m_notificationType (UNICAST_NOTIFICATION),
    m_socket (nullptr),
    m_knownPublisherId (0),  // 初始化发布者ID为0
    m_lastPosition (Vector(0,0,0)),
    m_positionUpdateCount (0)
{
  m_rand = CreateObject<UniformRandomVariable> ();
}

NdnApp::~NdnApp ()
{
}

void
NdnApp::DoDispose (void)
{
  Application::DoDispose ();
}

void
NdnApp::StartApplication (void)
{
  // 确保节点已经正确初始化
  NS_ASSERT (GetNode() != nullptr);
  
  // 创建并初始化Socket
  NS_LOG_INFO("正在为节点 " << GetNode()->GetId() << " 初始化Socket");
  CreateSocket();
  
  // 延迟初始化以确保所有组件都已正确设置
  Simulator::Schedule (MilliSeconds(100), &NdnApp::DoInitialize, this);
  
  // 开始定期清理过期PIT条目（也稍微延迟启动）
  m_cleanupEvent = Simulator::Schedule (Seconds (10.0), &NdnApp::CleanupExpiredPitEntries, this);
  
  // 新增: 节点位置更新
  m_locationUpdateEvent = Simulator::Schedule (Seconds (0.5), &NdnApp::UpdateNodeLocation, this);
  
  // 新增: 路由拓扑更新
  // 将更新频率与节点移动速度关联 - 速度越快，更新越频繁
  double initialRoutingInterval = std::max(1.0, 5.0 / (nodeMobility + 0.1));
  m_routingUpdateEvent = Simulator::Schedule (Seconds (initialRoutingInterval), 
                                             &NdnApp::UpdateRoutingTopology, this);
}

void 
NdnApp::CreateSocket (void)
{
  // 确保节点已经正确初始化
  if (GetNode() == nullptr)
    {
      NS_LOG_ERROR("无法创建Socket：节点为空");
      return;
    }
  
  // 创建UDP socket
  if (m_socket == nullptr)
    {
      TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
      m_socket = Socket::CreateSocket (GetNode (), tid);
      
      // 绑定到任意端口
      InetSocketAddress local = InetSocketAddress (Ipv4Address::GetAny (), 9);
      
      // 尝试绑定，如果失败则输出错误
      int result = m_socket->Bind (local);
      if (result == -1)
        {
          NS_LOG_ERROR("Socket绑定失败，节点ID: " << GetNode()->GetId());
        }
      else
        {
          NS_LOG_INFO("Socket绑定成功，节点ID: " << GetNode()->GetId());
          m_socket->SetRecvCallback (MakeCallback (&NdnApp::ReceivePacket, this));
          m_socket->SetAllowBroadcast (true);
        }
    }
}

void
NdnApp::StopApplication (void)
{
  // 取消所有定时事件 - 使用IsPending()替代IsRunning()
  if (m_migrationEvent.IsPending())
    {
      Simulator::Cancel (m_migrationEvent);
    }
  
  if (m_requestEvent.IsPending())
    {
      Simulator::Cancel (m_requestEvent);
    }
  
  if (m_cleanupEvent.IsPending())
    {
      Simulator::Cancel (m_cleanupEvent);
    }
  
  // 新增: 取消位置更新和路由更新事件
  if (m_locationUpdateEvent.IsPending())
    {
      Simulator::Cancel (m_locationUpdateEvent);
    }
  
  if (m_routingUpdateEvent.IsPending())
    {
      Simulator::Cancel (m_routingUpdateEvent);
    }
  
  // 关闭socket
  if (m_socket)
    {
      m_socket->Close ();
      m_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
      m_socket = 0;
    }
}

void
NdnApp::SetNodeRole (NodeRole role)
{
  m_nodeRole = role;
}

NdnApp::NodeRole
NdnApp::GetNodeRole (void) const
{
  return m_nodeRole;
}

void
NdnApp::SetCurrentBrokerId (uint32_t id)
{
  m_currentBrokerId = id;
}

uint32_t
NdnApp::GetCurrentBrokerId (void) const
{
  return m_currentBrokerId;
}

void
NdnApp::StartPublishing (std::string contentPrefix)
{
  m_contentPrefix = contentPrefix;
  
  // 发布者知道代理节点但不直接发送内容
  // 内容由订阅者请求驱动
}

void
NdnApp::StartSubscribing (std::string contentPrefix)
{
  m_contentPrefix = contentPrefix;
  
  // 开始周期性请求内容
  ScheduleNextRequest ();
}

void
NdnApp::ScheduleNextRequest ()
{
  // 基于节点移动速度调整请求频率
  // 高速移动时，更频繁地发送请求以适应拓扑变化
  double requestInterval = std::max(0.3, 1.0 / (1.0 + nodeMobility / 10.0));
  m_requestEvent = Simulator::Schedule (Seconds (requestInterval), &NdnApp::RequestContent, this);
}

void
NdnApp::RequestContent ()
{
  if (enableDetailedLogging)
    NS_LOG_INFO ("节点 " << GetNode()->GetId() << " 在时间 " << Simulator::Now().GetSeconds() << " 请求内容");
  else
    NS_LOG_INFO ("Subscriber requesting content at time " << Simulator::Now().GetSeconds());
  
  // 如果Socket为空，尝试重新创建
  if (m_socket == nullptr)
    {
      NS_LOG_WARN("Socket为空，尝试重新创建");
      CreateSocket();
      
      // 如果仍然为空，记录错误并退出
      if (m_socket == nullptr)
        {
          NS_LOG_ERROR("无法创建Socket，跳过本次请求");
          // 继续安排下一次请求
          ScheduleNextRequest();
          return;
        }
    }
  
  // 请求特定内容
  std::string contentName = m_contentPrefix + "/" + std::to_string(m_requestSequence % 100 + 1);
  SendInterest (contentName);
  
  // 统计已发送消息
  s_messagesSent++;
  
  // 增加请求序号
  m_requestSequence++;
  
  // 安排下一次请求
  ScheduleNextRequest ();
}

void
NdnApp::SendInterest (std::string contentName)
{
  NS_LOG_FUNCTION (this << contentName);
  
  // 确保socket已经初始化
  if (m_socket == nullptr)
    {
      NS_LOG_ERROR("Socket is null while trying to send interest");
      return;
    }
  
  // 创建兴趣包
  Ptr<Packet> packet = Create<Packet> ();
  
  // 添加NDN头，现在包含订阅者ID和发布者ID
  NdnHeader ndnHeader;
  ndnHeader.SetContentName (contentName);
  ndnHeader.SetPacketType (INTEREST);
  ndnHeader.SetNonce (m_rand->GetInteger (1, UINT32_MAX)); // 生成随机Nonce
  ndnHeader.SetSubscriberId (GetNode()->GetId()); // 设置订阅者ID为当前节点ID
  ndnHeader.SetPublisherId (m_knownPublisherId);  // 设置已知的发布者ID，初始为0
  
  packet->AddHeader (ndnHeader);
  
  // 获取目标地址
  // 新增: 如果知道代理节点ID，则先检查是否在通信范围内
  bool directToBroker = false;
  if (m_currentBrokerId > 0)
    {
      if (IsInCommunicationRange(m_currentBrokerId))
        {
          // 代理节点在通信范围内，直接发送
          Ipv4Address brokerAddr = GetIpv4FromNodeId(m_currentBrokerId);
          if (brokerAddr != Ipv4Address::GetAny())
            {
              InetSocketAddress brokerDest = InetSocketAddress(brokerAddr, 9);
              m_socket->SendTo (packet, 0, brokerDest);
              NS_LOG_INFO("直接向代理 " << m_currentBrokerId << " 发送兴趣包: " << contentName);
              directToBroker = true;
            }
        }
    }
  
  if (!directToBroker)
    {
      // 使用广播地址作为默认策略
      Ipv4Address bcastAddr = Ipv4Address ("255.255.255.255");
      uint16_t port = 9;
      InetSocketAddress dest (bcastAddr, port);
      
      // 如果FIB有已知的下一跳，则检查是否在通信范围内
      std::vector<InetSocketAddress> nextHops;
      if (FindFibEntry (contentName, nextHops) && !nextHops.empty())
        {
          // 检查所有下一跳，找到一个在通信范围内的
          bool foundValidHop = false;
          for (auto &hop : nextHops)
            {
              // 从IP地址查找节点ID
              for (uint32_t i = 0; i < NodeList::GetNNodes(); i++)
                {
                  Ptr<Node> node = NodeList::GetNode(i);
                  if (node)
                    {
                      Ptr<Ipv4> ipv4 = node->GetObject<Ipv4>();
                      if (ipv4 && ipv4->GetNInterfaces() > 1)
                        {
                          Ipv4Address addr = ipv4->GetAddress(1, 0).GetLocal();
                          if (addr == hop.GetIpv4())
                            {
                              // 找到节点ID，检查是否在通信范围内和稳定性
                              if (IsInCommunicationRange(i))
                                {
                                  // 高速移动环境下，考虑节点稳定性
                                  double stability = CalculateNodeStability(i);
                                  if (stability > 0.3 || nodeMobility < 5.0) // 低稳定性节点在高速移动时被忽略
                                    {
                                      // 找到可达的下一跳
                                      dest = hop;
                                      foundValidHop = true;
                                      if (enableDetailedLogging)
                                        NS_LOG_INFO ("使用FIB下一跳路由: " << contentName << " -> " << dest.GetIpv4() << ", 稳定性=" << stability);
                                      else
                                        NS_LOG_INFO ("Using FIB next hop for " << contentName);
                                      break;
                                    }
                                  else
                                    {
                                      NS_LOG_INFO("下一跳节点 " << i << " 稳定性太低 (" << stability << "), 跳过");
                                    }
                                }
                            }
                        }
                    }
                }
              
              if (foundValidHop) break;
            }
          
          if (!foundValidHop)
            {
              // 所有已知下一跳都不在通信范围内，尝试找最近的节点
              NS_LOG_INFO("所有FIB下一跳都不在通信范围内，查找最近节点");
              uint32_t nearestId = GetNearestNodeId(GetCurrentPosition());
              if (nearestId != GetNode()->GetId() && IsInCommunicationRange(nearestId))
                {
                  // 检查节点稳定性
                  double stability = CalculateNodeStability(nearestId);
                  if (stability > 0.3 || nodeMobility < 5.0)
                    {
                      Ipv4Address nearestAddr = GetIpv4FromNodeId(nearestId);
                      if (nearestAddr != Ipv4Address::GetAny())
                        {
                          dest = InetSocketAddress(nearestAddr, 9);
                          NS_LOG_INFO("转发到最近节点 " << nearestId << ", 稳定性=" << stability);
                        }
                    }
                  else
                    {
                      NS_LOG_INFO("最近节点 " << nearestId << " 稳定性太低 (" << stability << "), 使用广播");
                    }
                }
            }
        }
        else
        {
          if (enableDetailedLogging)
            NS_LOG_INFO ("广播兴趣包: " << contentName);
          else
            NS_LOG_INFO ("Broadcasting interest for " << contentName);
        }
      
      // 发送兴趣包
      m_socket->SendTo (packet, 0, dest);
    }

  // 安全地添加到PIT - 现在包含订阅者ID和发布者ID
  Ptr<Node> node = GetNode();
  if (node)
    {
      Ptr<Ipv4> ipv4 = node->GetObject<Ipv4>();
      if (ipv4 && ipv4->GetNInterfaces() > 1)
        {
          // 创建明确的InetSocketAddress
          Ipv4Address myAddr = ipv4->GetAddress(1, 0).GetLocal();
          InetSocketAddress sourceAddr = InetSocketAddress(myAddr, 9);
          
          // 传递订阅者ID和发布者ID
          AddToPit (contentName, ndnHeader.GetNonce(), node->GetId(), m_knownPublisherId, sourceAddr);
          
          if (enableDetailedLogging)
            NS_LOG_INFO ("节点 " << node->GetId() << " 发送兴趣包: " << contentName 
                        << ", 订阅者ID=" << node->GetId() << ", 发布者ID=" << m_knownPublisherId);
          else
            NS_LOG_INFO ("Node " << node->GetId() << " sent interest for " << contentName);
        }
      else
        {
          NS_LOG_WARN("节点没有有效的IPv4接口");
        }
    }
  else
    {
      NS_LOG_ERROR("SendInterest中节点指针无效");
    }
}

void
NdnApp::SendData (std::string contentName, InetSocketAddress destination, uint32_t subscriberId = 0)
{
  NS_LOG_FUNCTION (this << contentName);
  
  // 查找内容是否在本地存储
  Ptr<Packet> contentPacket;
  bool found = CheckContentStore (contentName, contentPacket);
  
  if (!found)
    {
      NS_LOG_INFO ("存储中未找到内容: " << contentName << "，尝试生成");
      
      // 如果是代理节点，可以生成内容
      if (m_nodeRole == BROKER && contentName.find(m_contentPrefix) == 0)
        {
          // 生成内容
          contentPacket = Create<Packet> (packetSize);
          // 添加到内容存储
          AddToContentStore (contentName, contentPacket);
          NS_LOG_INFO ("代理节点生成了内容: " << contentName);
        }
      else
        {
          NS_LOG_INFO ("非代理节点无法生成内容: " << contentName);
          return;
        }
    }
  
  // 新增: 检查目标是否在通信范围内，并考虑未来位置
  bool inRange = true;
  uint32_t targetNodeId = 0;
  
  // 从destination IP获取节点ID
  for (uint32_t i = 0; i < NodeList::GetNNodes(); i++)
    {
      Ptr<Node> node = NodeList::GetNode(i);
      if (node)
        {
          Ptr<Ipv4> ipv4 = node->GetObject<Ipv4>();
          if (ipv4 && ipv4->GetNInterfaces() > 1)
            {
              Ipv4Address addr = ipv4->GetAddress(1, 0).GetLocal();
              if (addr == destination.GetIpv4())
                {
                  targetNodeId = i;
                  
                  // 考虑节点的未来位置
                  double predictionTime = 0.5; // 预测半秒后的位置
                  if (nodeMobility > 5.0)
                    {
                      Vector futurePosition = PredictFuturePosition(i, predictionTime);
                      Vector myFuturePosition = PredictFuturePosition(GetNode()->GetId(), predictionTime);
                      
                      double futureDistance = CalculateDistance(myFuturePosition, futurePosition);
                      inRange = futureDistance <= communicationRange * 0.9; // 预留10%安全边界
                      
                      if (!inRange)
                        {
                          NS_LOG_INFO("预测目标节点 " << targetNodeId << " 即将离开通信范围, 距离=" << futureDistance);
                        }
                    }
                  else
                    {
                      // 低速移动时使用当前位置判断
                      inRange = IsInCommunicationRange(i);
                    }
                  break;
                }
            }
        }
    }
  
  // 如果目标是广播地址，则向所有在通信范围内的节点发送
  if (destination.GetIpv4() == Ipv4Address("255.255.255.255"))
    {
      // 对于广播，我们继续，但只发送到实际在范围内的节点
      inRange = true;
    }
  
  if (!inRange)
    {
      NS_LOG_INFO("目标节点 " << targetNodeId << " 不在通信范围内，无法发送数据");
      s_outOfRangeFailures++;
      return;
    }
  
  // 创建数据包的副本
  Ptr<Packet> packet = Create<Packet> (packetSize);
  
  // 添加NDN头
  NdnHeader ndnHeader;
  ndnHeader.SetContentName (contentName);
  ndnHeader.SetPacketType (DATA);
  // 数据包中也设置订阅者ID以便追踪
  ndnHeader.SetSubscriberId (subscriberId);
  
  packet->AddHeader (ndnHeader);
  
  // 如果有订阅者ID并且不是广播地址，尝试使用优化的路径
  if (subscriberId > 0 && destination.GetIpv4() != Ipv4Address("255.255.255.255"))
    {
      // 尝试查找到该订阅者的最佳路径
      InetSocketAddress bestPath = GetBestPathToSubscriber(subscriberId);
      
      // 如果找到更好的路径，并且在通信范围内，使用它
      if (bestPath.GetIpv4() != Ipv4Address::GetAny())
        {
          // 同样需要检查这个最佳路径是否在通信范围内
          bool bestPathInRange = false;
          for (uint32_t i = 0; i < NodeList::GetNNodes(); i++)
            {
              Ptr<Node> node = NodeList::GetNode(i);
              if (node)
                {
                  Ptr<Ipv4> ipv4 = node->GetObject<Ipv4>();
                  if (ipv4 && ipv4->GetNInterfaces() > 1)
                    {
                      Ipv4Address addr = ipv4->GetAddress(1, 0).GetLocal();
                      if (addr == bestPath.GetIpv4())
                        {
                          // 考虑高速移动场景
                          if (nodeMobility > 5.0)
                            {
                              // 检查节点稳定性
                              double stability = CalculateNodeStability(i);
                              if (stability > 0.4) // 要求更高的稳定性
                                {
                                  bestPathInRange = IsInCommunicationRange(i);
                                }
                              else
                                {
                                  NS_LOG_INFO("最佳路径节点 " << i << " 稳定性太低 (" << stability << "), 不使用");
                                  bestPathInRange = false;
                                }
                            }
                          else
                            {
                              bestPathInRange = IsInCommunicationRange(i);
                            }
                          break;
                        }
                    }
                }
            }
          
          if (bestPathInRange)
            {
              destination = bestPath;
              NS_LOG_INFO("使用优化路径向订阅者 " << subscriberId << " 发送数据: " << destination.GetIpv4());
            }
        }
    }
  
  // 如果是广播，则向所有在通信范围内的节点发送
  if (destination.GetIpv4() == Ipv4Address("255.255.255.255"))
    {
      // 向所有在通信范围内的节点发送
      for (uint32_t i = 0; i < NodeList::GetNNodes(); i++)
        {
          if (i != GetNode()->GetId() && IsInCommunicationRange(i))
            {
              // 高速移动时过滤低稳定性节点
              if (nodeMobility > 5.0)
                {
                  double stability = CalculateNodeStability(i);
                  if (stability < 0.3)
                    {
                      NS_LOG_INFO("跳过低稳定性节点 " << i << " (稳定性=" << stability << ")");
                      continue;
                    }
                }
              
              Ipv4Address targetAddr = GetIpv4FromNodeId(i);
              if (targetAddr != Ipv4Address::GetAny())
                {
                  InetSocketAddress targetDest = InetSocketAddress(targetAddr, 9);
                  m_socket->SendTo (packet->Copy(), 0, targetDest);
                  
                  if (enableDetailedLogging)
                    NS_LOG_INFO("向节点 " << i << " 发送广播数据包: " << contentName);
                }
            }
        }
    }
  else
    {
      // 发送数据包
      m_socket->SendTo (packet, 0, destination);
      
      // 日志输出
      if (enableDetailedLogging)
        NS_LOG_INFO ("节点 " << GetNode()->GetId() << " 发送数据: " << contentName 
                    << " -> " << destination.GetIpv4() << " (订阅者ID: " << subscriberId << ")");
      else
        NS_LOG_INFO ("Node " << GetNode()->GetId() << " sent data for " << contentName << " to " << destination.GetIpv4());
    }
}

// 新增: 获取到特定订阅者的最佳路径
InetSocketAddress
NdnApp::GetBestPathToSubscriber(uint32_t subscriberId)
{
  // 遍历FIB表寻找该订阅者的最佳路径
  for (auto &entry : m_fibTable)
    {
      // 在订阅者路径映射中查找
      auto it = entry.subscriberPaths.find(subscriberId);
      if (it != entry.subscriberPaths.end())
        {
          return it->second; // 返回找到的路径
        }
    }
  
  // 未找到路径，返回默认值（使用显式构造函数）
  return InetSocketAddress(Ipv4Address::GetAny(), 0);
}

// 发送代理迁移通知 - 结合三种策略
void
NdnApp::SendBrokerNotification (uint32_t newBrokerId)
{
  NS_LOG_FUNCTION (this << newBrokerId);
  
  // 安全检查
  if (m_socket == nullptr)
    {
      NS_LOG_ERROR("Socket为空，无法发送代理通知");
      return;
    }
  
  // 设置迁移开始时间为当前时间
  m_migrationStartTime = Simulator::Now();
  
  NS_LOG_INFO("迁移开始时间: " << m_migrationStartTime.GetSeconds() << "秒");
  
  // 1. 发布者单播通知
  try
    {
      NS_LOG_INFO("发送单播通知");
      SendUnicastNotification(newBrokerId);
    }
  catch (const std::exception& e)
    {
      NS_LOG_ERROR("发送单播通知时出错: " << e.what());
    }
  
  // 2. 基于NDN兴趣的通知
  try
    {
      NS_LOG_INFO("发送基于兴趣的通知");
      SendInterestBasedNotification(newBrokerId);
    }
  catch (const std::exception& e)
    {
      NS_LOG_ERROR("发送基于兴趣的通知时出错: " << e.what());
    }
  
  // 3. 根据移动速度决定是否启用大规模广播通知
  try
    {
      // 移动速度越快，越需要广播
      bool needForceBroadcast = nodeMobility > 3.0;
      NS_LOG_INFO("发送广播通知 (强制广播=" << needForceBroadcast << ")");
      SendBroadcastNotification(newBrokerId, needForceBroadcast);
    }
  catch (const std::exception& e)
    {
      NS_LOG_ERROR("发送广播通知时出错: " << e.what());
    }
}

// 发布者单播通知
void
NdnApp::SendUnicastNotification (uint32_t newBrokerId)
{
  NS_LOG_FUNCTION (this << newBrokerId);
  
  // 创建通知包
  Ptr<Packet> packet = Create<Packet> ();
  
  // 添加NDN头，并设置迁移时间戳
  NdnHeader ndnHeader;
  ndnHeader.SetContentName (m_contentPrefix + "/unicastBrokerChange");
  ndnHeader.SetPacketType (NOTIFICATION);
  ndnHeader.SetNewBrokerId (newBrokerId);
  ndnHeader.SetNonce (m_rand->GetInteger (1, UINT32_MAX));
  ndnHeader.SetMigrationTime (m_migrationStartTime);
  
  packet->AddHeader (ndnHeader);
  
  // 设置当前通知类型
  m_notificationType = UNICAST_NOTIFICATION;
  
  // 对发布者单播通知
  std::vector<InetSocketAddress> publisherAddresses;
  if (FindFibEntry (m_contentPrefix + "/publisher", publisherAddresses) && !publisherAddresses.empty())
    {
      // 向发布者发送通知
      InetSocketAddress publisherDest = publisherAddresses.front();
      
      // 新增: 确认发布者在通信范围内，并检查稳定性
      bool publisherInRange = false;
      uint32_t publisherId = 0;
      
      for (uint32_t i = 0; i < NodeList::GetNNodes(); i++)
        {
          Ptr<Node> node = NodeList::GetNode(i);
          if (node)
            {
              Ptr<Ipv4> ipv4 = node->GetObject<Ipv4>();
              if (ipv4 && ipv4->GetNInterfaces() > 1)
                {
                  Ipv4Address addr = ipv4->GetAddress(1, 0).GetLocal();
                  if (addr == publisherDest.GetIpv4())
                    {
                      publisherId = i;
                      publisherInRange = IsInCommunicationRange(i);
                      
                      // 高速场景检查稳定性
                      if (publisherInRange && nodeMobility > 5.0)
                        {
                          double stability = CalculateNodeStability(i);
                          publisherInRange = stability > 0.3;
                          NS_LOG_INFO("发布者稳定性检查: ID=" << i << ", 稳定性=" << stability << 
                                     ", 结果=" << (publisherInRange ? "可用" : "不可用"));
                        }
                      break;
                    }
                }
            }
        }
      
      if (publisherInRange)
        {
          m_socket->SendTo (packet, 0, publisherDest);
          NS_LOG_INFO ("已向发布者发送单播代理迁移通知，发布者在通信范围内");
        }
      else
        {
          NS_LOG_INFO ("发布者不在通信范围内或稳定性太低，无法发送单播通知");
        }
    }
  else
    {
      // 找不到发布者，尝试找到任何对该内容感兴趣的发布者
      bool foundPublisher = false;
      for (uint32_t i = 0; i < numNodes; ++i)
        {
          if (i != GetNode()->GetId() && i < NodeList::GetNNodes())
            {
              Ptr<Node> node = NodeList::GetNode(i);
              if (node && node->GetNApplications() > 0)
                {
                  Ptr<NdnApp> app = DynamicCast<NdnApp>(node->GetApplication(0));
                  if (app && app->GetNodeRole() == PUBLISHER)
                    {
                      // 检查通信范围和稳定性
                      bool publisherUsable = IsInCommunicationRange(i);
                      
                      // 高速场景检查稳定性
                      if (publisherUsable && nodeMobility > 5.0)
                        {
                          double stability = CalculateNodeStability(i);
                          publisherUsable = stability > 0.3;
                          NS_LOG_INFO("发布者稳定性检查: ID=" << i << ", 稳定性=" << stability << 
                                     ", 结果=" << (publisherUsable ? "可用" : "不可用"));
                        }
                      
                      if (publisherUsable)
                        {
                          // 找到发布者节点，记住它的ID
                          m_knownPublisherId = i;
                          
                          // 向发布者发送通知
                          Ipv4Address publisherAddr = GetIpv4FromNodeId(i);
                          if (publisherAddr != Ipv4Address::GetAny())
                            {
                              InetSocketAddress publisherDest = InetSocketAddress(publisherAddr, 9);
                              m_socket->SendTo (packet->Copy(), 0, publisherDest);
                              NS_LOG_INFO ("已向节点 " << i << " 发送单播通知（发布者）");
                              foundPublisher = true;
                              
                              // 添加到FIB，以便之后可以找到
                              UpdateFib(m_contentPrefix + "/publisher", publisherDest);
                              break;
                            }
                        }
                    }
                }
            }
        }
      
      if (!foundPublisher)
        {
          NS_LOG_WARN("找不到发布者节点或发布者不在通信范围内，单播通知失败");
        }
    }
  
  // 向订阅者也发送单播通知
  bool foundSubscriber = false;
  for (uint32_t i = 0; i < numNodes; ++i)
    {
      if (i != GetNode()->GetId() && i < NodeList::GetNNodes())
        {
          Ptr<Node> node = NodeList::GetNode(i);
          if (node && node->GetNApplications() > 0)
            {
              Ptr<NdnApp> app = DynamicCast<NdnApp>(node->GetApplication(0));
              if (app && app->GetNodeRole() == SUBSCRIBER)
                {
                  // 检查通信范围和稳定性
                  bool subscriberUsable = IsInCommunicationRange(i);
                  
                  // 高速场景检查稳定性
                  if (subscriberUsable && nodeMobility > 5.0)
                    {
                      double stability = CalculateNodeStability(i);
                      subscriberUsable = stability > 0.3;
                      NS_LOG_INFO("订阅者稳定性检查: ID=" << i << ", 稳定性=" << stability << 
                                 ", 结果=" << (subscriberUsable ? "可用" : "不可用"));
                    }
                  
                  if (subscriberUsable)
                    {
                      // 找到订阅者节点
                      Ipv4Address subscriberAddr = GetIpv4FromNodeId(i);
                      if (subscriberAddr != Ipv4Address::GetAny())
                        {
                          InetSocketAddress subscriberDest = InetSocketAddress(subscriberAddr, 9);
                          m_socket->SendTo (packet->Copy(), 0, subscriberDest);
                          NS_LOG_INFO ("已向节点 " << i << " 发送单播通知(订阅者)");
                          foundSubscriber = true;
                          
                          // 添加到FIB，以便之后可以找到，同时记录这是订阅者路径
                          UpdateFib(m_contentPrefix + "/subscriber", subscriberDest, i);
                        }
                    }
                }
            }
        }
    }
  
  if (!foundSubscriber)
    {
      NS_LOG_WARN("找不到订阅者节点或订阅者不在通信范围内，单播通知可能不完整");
    }
}

// 基于NDN兴趣的通知
void
NdnApp::SendInterestBasedNotification (uint32_t newBrokerId)
{
  NS_LOG_FUNCTION (this << newBrokerId);
  
  // 检查socket是否有效
  if (m_socket == nullptr)
    {
      NS_LOG_ERROR("Socket is null while trying to send interest-based notification");
      return;
    }
  
  // 检查节点是否有效
  Ptr<Node> thisNode = GetNode();
  if (thisNode == nullptr)
    {
      NS_LOG_ERROR("Node is null in SendInterestBasedNotification");
      return;
    }
  
  // 创建通知包
  Ptr<Packet> packet = Create<Packet> ();
  
  // 添加NDN头，包括迁移时间戳
  NdnHeader ndnHeader;
  ndnHeader.SetContentName (m_contentPrefix + "/interestBrokerChange");
  ndnHeader.SetPacketType (NOTIFICATION);
  ndnHeader.SetNewBrokerId (newBrokerId);
  ndnHeader.SetNonce (m_rand->GetInteger (1, UINT32_MAX));
  ndnHeader.SetMigrationTime (m_migrationStartTime);
  
  packet->AddHeader (ndnHeader);
  
  // 设置当前通知类型
  m_notificationType = INTEREST_NOTIFICATION;
  m_lastInterestNotificationTime = Simulator::Now();
  
  // 查找之前有哪些节点对内容感兴趣（从PIT可以获取）
  std::vector<InetSocketAddress> interestedNodes;
  std::vector<uint32_t> subscriberIds;
  
  // 记录每个订阅者ID对应的源地址 - 使用insert_or_assign而不是operator[]
  std::map<uint32_t, InetSocketAddress> subscriberSourceMap;
  
  for (auto &pitEntry : m_pitTable)
    {
      // 检查是否对相关内容感兴趣
      if (pitEntry.contentName.find(m_contentPrefix) == 0)
        {
          // 新增: 确认这个PIT条目对应的节点在通信范围内和稳定
          bool nodeUsable = false;
          uint32_t nodeId = 0;
          
          // 从IP地址查找节点ID
          for (uint32_t i = 0; i < NodeList::GetNNodes(); i++)
            {
              Ptr<Node> node = NodeList::GetNode(i);
              if (node)
                {
                  Ptr<Ipv4> ipv4 = node->GetObject<Ipv4>();
                  if (ipv4 && ipv4->GetNInterfaces() > 1)
                    {
                      Ipv4Address addr = ipv4->GetAddress(1, 0).GetLocal();
                      if (addr == pitEntry.sourceAddress.GetIpv4())
                        {
                          nodeId = i;
                          nodeUsable = IsInCommunicationRange(i);
                          
                          // 高速场景检查稳定性
                          if (nodeUsable && nodeMobility > 5.0)
                            {
                              double stability = CalculateNodeStability(i);
                              nodeUsable = stability > 0.3;
                              if (!nodeUsable)
                                {
                                  NS_LOG_INFO("PIT条目节点 " << i << " 稳定性太低 (" << stability << "), 跳过");
                                }
                            }
                          break;
                        }
                    }
                }
            }
          
          if (nodeUsable)
            {
              interestedNodes.push_back(pitEntry.sourceAddress);
              subscriberIds.push_back(pitEntry.subscriberId);
              
              // 记录订阅者路径 - 使用insert_or_assign而不是operator[]
              subscriberSourceMap.insert_or_assign(pitEntry.subscriberId, pitEntry.sourceAddress);
            }
          else
            {
              NS_LOG_INFO("PIT条目节点 " << nodeId << " 不在通信范围内或稳定性太低，跳过");
            }
        }
    }
  
  // 如果找到了感兴趣的节点，沿着兴趣反向路径发送通知
  if (!interestedNodes.empty())
    {
      NS_LOG_INFO ("找到 " << interestedNodes.size() << " 个对该内容感兴趣且在通信范围内的节点");
      
      for (size_t i = 0; i < interestedNodes.size(); i++)
        {
          // 向每个感兴趣的节点发送通知
          m_socket->SendTo (packet->Copy(), 0, interestedNodes[i]);
          
          // 更新FIB，记录订阅者路径
          if (i < subscriberIds.size() && subscriberIds[i] > 0)
            {
              UpdateFib(m_contentPrefix, interestedNodes[i], subscriberIds[i]);
              NS_LOG_INFO("更新了订阅者 " << subscriberIds[i] << " 的路径: " << interestedNodes[i].GetIpv4());
            }
        }
    }
    else
    {
      // 如果没有找到感兴趣的节点，强制进行邻近节点通知
      NS_LOG_INFO ("PIT中没有找到感兴趣的节点或节点不在通信范围内，使用邻近通知代替");
      
      // 遍历所有节点，向距离较近且稳定的节点发送
      for (uint32_t i = 0; i < numNodes && i < NodeList::GetNNodes(); ++i)
        {
          if (i != thisNode->GetId() && IsInCommunicationRange(i))
            {
              Ptr<Node> node = NodeList::GetNode(i);
              if (node == nullptr)
                {
                  NS_LOG_WARN("Node " << i << " is null");
                  continue;
                }
              
              // 高速场景检查稳定性
              bool nodeUsable = true;
              if (nodeMobility > 5.0)
                {
                  double stability = CalculateNodeStability(i);
                  nodeUsable = stability > 0.3;
                  if (!nodeUsable)
                    {
                      NS_LOG_INFO("邻近节点 " << i << " 稳定性太低 (" << stability << "), 跳过");
                      continue;
                    }
                }
              
              if (nodeUsable)
                {
                  // 获取节点IP地址
                  Ipv4Address addr = GetIpv4FromNodeId(i);
                  if (addr != Ipv4Address::GetAny())
                    {
                      // 查看节点角色，优先向订阅者和邻近节点发送
                      InetSocketAddress nodeDest = InetSocketAddress(addr, 9);
                      m_socket->SendTo(packet->Copy(), 0, nodeDest);
                      
                      // 记录订阅者节点路径
                      Ptr<NdnApp> app = DynamicCast<NdnApp>(node->GetApplication(0));
                      if (app && app->GetNodeRole() == SUBSCRIBER)
                        {
                          UpdateFib(m_contentPrefix, nodeDest, i);
                          NS_LOG_INFO("记录订阅者 " << i << " 的路径: " << addr);
                        }
                      
                      if (enableDetailedLogging)
                        NS_LOG_INFO("向节点 " << i << " 发送兴趣通知");
                    }
                }
            }
        }
    }
}

// 大规模广播通知
void
NdnApp::SendBroadcastNotification (uint32_t newBrokerId, bool forceBroadcast)
{
  NS_LOG_FUNCTION (this << newBrokerId << forceBroadcast);
  
  // 新增: 根据移动速度决定是否需要广播
  // 移动速度越快，越倾向于广播
  double broadcastThreshold = 2.0; // 默认阈值
  bool needBroadcast = nodeMobility > broadcastThreshold || forceBroadcast;
  
  if (needBroadcast)
    {
      // 创建通知包
      Ptr<Packet> packet = Create<Packet> ();
      
      // 添加NDN头，标记为广播通知，并加入迁移时间戳
      NdnHeader ndnHeader;
      ndnHeader.SetContentName (m_contentPrefix + "/broadcastBrokerChange");
      ndnHeader.SetPacketType (NOTIFICATION);
      ndnHeader.SetNewBrokerId (newBrokerId);
      ndnHeader.SetNonce (m_rand->GetInteger (1, UINT32_MAX));
      ndnHeader.SetMigrationTime (m_migrationStartTime);
      
      packet->AddHeader (ndnHeader);
      
      // 设置当前通知类型
      m_notificationType = BROADCAST_NOTIFICATION;
      m_lastBroadcastTime = Simulator::Now();
      
      // 执行选择性广播 - 只向在通信范围内且相对稳定的节点广播
      for (uint32_t i = 0; i < NodeList::GetNNodes(); i++)
        {
          if (i != GetNode()->GetId() && IsInCommunicationRange(i))
            {
              // 高速场景下筛选稳定节点
              bool nodeUsable = true;
              if (nodeMobility > 5.0 && !forceBroadcast) // 强制广播时不检查稳定性
                {
                  double stability = CalculateNodeStability(i);
                  nodeUsable = stability > 0.3;
                  if (!nodeUsable)
                    {
                      NS_LOG_INFO("广播目标节点 " << i << " 稳定性太低 (" << stability << "), 跳过");
                      continue;
                    }
                }
              
              Ipv4Address targetAddr = GetIpv4FromNodeId(i);
              if (targetAddr != Ipv4Address::GetAny())
                {
                  InetSocketAddress dest = InetSocketAddress(targetAddr, 9);
                  m_socket->SendTo (packet->Copy(), 0, dest);
                  
                  if (enableDetailedLogging)
                    NS_LOG_INFO("向节点 " << i << " 发送广播通知");
                }
            }
        }
      
      NS_LOG_INFO ("选择性广播通知已发送: 时间 " << Simulator::Now().GetSeconds() << "秒, 新代理ID: " << newBrokerId);
    }
  else
    {
      NS_LOG_INFO ("移动速度不高，不需要广播通知");
    }
}

// 估计订阅者数量的辅助函数
uint32_t 
NdnApp::EstimateSubscriberCount()
{
  // 安全检查
  if (m_pitTable.empty())
    {
      NS_LOG_INFO("PIT表为空，估计订阅者数量为0");
      return 0;
    }
  
  // 使用std::set存储唯一的订阅者ID
  std::set<uint32_t> uniqueSubscribers;
  
  try
    {
      for (auto &pitEntry : m_pitTable)
        {
          if (pitEntry.contentName.find(m_contentPrefix) == 0 && pitEntry.subscriberId > 0)
            {
              // 使用订阅者ID进行去重
              uniqueSubscribers.insert(pitEntry.subscriberId);
            }
        }
    }
  catch (const std::exception& e)
    {
      NS_LOG_ERROR("估计订阅者数量时出错: " << e.what());
      return 0;
    }
  
  NS_LOG_INFO("估计到 " << uniqueSubscribers.size() << " 个订阅者");
  return uniqueSubscribers.size();
}

// 计算两点之间的距离
double
NdnApp::CalculateDistance(Vector a, Vector b)
{
  // 计算各维度差值
  double dx = a.x - b.x;
  double dy = a.y - b.y;
  double dz = a.z - b.z;
  
  // 计算平方和
  double distSquared = dx*dx + dy*dy + dz*dz;
  
  // 计算平方根
  return std::sqrt(distSquared);
}

void
NdnApp::DoInitialize (void)
{
  // 这个函数会在应用程序启动后延迟执行，确保所有组件都已正确初始化
  NS_LOG_INFO("DoInitialize for node " << GetNode()->GetId());
  
  // 初始化节点位置信息
  UpdateNodeLocation();
  
  // 根据节点角色执行不同的初始化
  if (m_nodeRole == PUBLISHER)
    {
      NS_LOG_INFO ("Starting publisher node " << GetNode()->GetId());
      StartPublishing (m_contentPrefix);
      
      // 记住自己是发布者
      m_knownPublisherId = GetNode()->GetId();
      
      // 为发布者添加预先配置的代理路由
      if (m_currentBrokerId > 0)
        {
          Ptr<Node> brokerNode = NodeList::GetNode(m_currentBrokerId);
          if (brokerNode && brokerNode->GetObject<Ipv4>())
            {
              Ipv4Address brokerAddr = brokerNode->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal();
              InetSocketAddress brokerSocketAddr = InetSocketAddress(brokerAddr, 9);
              
              // 添加到FIB
              UpdateFib(m_contentPrefix, brokerSocketAddr);
              NS_LOG_INFO("发布者预配置代理路由: " << m_contentPrefix << " -> " << brokerAddr);
            }
        }
    }
  else if (m_nodeRole == SUBSCRIBER)
    {
      NS_LOG_INFO ("Starting subscriber node " << GetNode()->GetId());
      StartSubscribing (m_contentPrefix);
      
      // 为订阅者添加预先配置的代理路由
      if (m_currentBrokerId > 0)
        {
          Ptr<Node> brokerNode = NodeList::GetNode(m_currentBrokerId);
          if (brokerNode && brokerNode->GetObject<Ipv4>())
            {
              Ipv4Address brokerAddr = brokerNode->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal();
              InetSocketAddress brokerSocketAddr = InetSocketAddress(brokerAddr, 9);
              
              // 添加到FIB
              UpdateFib(m_contentPrefix, brokerSocketAddr);
              NS_LOG_INFO("订阅者预配置代理路由: " << m_contentPrefix << " -> " << brokerAddr);
            }
        }
    }
  else if (m_nodeRole == BROKER)
    {
      NS_LOG_INFO ("Starting broker node " << GetNode()->GetId());
      // 代理节点初始拥有内容
      for (int i = 1; i <= 100; i++)
        {
          std::string contentName = m_contentPrefix + "/" + std::to_string(i);
          Ptr<Packet> content = Create<Packet> (packetSize);
          AddToContentStore (contentName, content);
        }

      // 设置为当前代理
      m_currentBrokerId = GetNode()->GetId();
      NS_LOG_INFO("代理节点已初始化内容存储: " << GetNode()->GetId());
    }
}

void
NdnApp::MigrateBroker (uint32_t newBrokerId)
{
  NS_LOG_FUNCTION (this << newBrokerId);
  
  // 安全检查
  if (m_socket == nullptr)
    {
      NS_LOG_ERROR("Socket为空，无法执行代理迁移");
      return;
    }
  
  if (GetNode() == nullptr)
    {
      NS_LOG_ERROR("节点为空，无法执行代理迁移");
      return;
    }
  
  try 
    {
      // 发送代理迁移通知 - 使用三种机制的组合
      NS_LOG_INFO("开始发送代理迁移通知: 从节点 " << GetNode()->GetId() << " 到节点 " << newBrokerId);
      SendBrokerNotification (newBrokerId);
      
      // 更新当前代理ID
      m_lastKnownBrokerId = m_currentBrokerId;
      m_currentBrokerId = newBrokerId;
      
      // 为新代理迁移内容
      Ptr<Node> newBrokerNode = NodeList::GetNode(newBrokerId);
      if (newBrokerNode && newBrokerNode->GetNApplications() > 0)
        {
          Ptr<NdnApp> newBrokerApp = DynamicCast<NdnApp>(newBrokerNode->GetApplication(0));
          if (newBrokerApp)
            {
              // 设置新代理角色为BROKER
              newBrokerApp->SetNodeRole(BROKER);
              newBrokerApp->SetCurrentBrokerId(newBrokerId);
              
              NS_LOG_INFO("已将节点 " << newBrokerId << " 设置为新的代理");
            }
        }
      
      NS_LOG_INFO ("代理已迁移: 从节点 " << m_lastKnownBrokerId << " 到节点 " << m_currentBrokerId);
    }
  catch (const std::exception& e)
    {
      NS_LOG_ERROR("代理迁移时发生异常: " << e.what());
    }
  catch (...)
    {
      NS_LOG_ERROR("代理迁移时发生未知异常");
    }
}

void 
NdnApp::TriggerBrokerMigration (uint32_t newBrokerId)
{
  NS_LOG_FUNCTION (this << newBrokerId);
  
  // 只有当前代理才能触发迁移
  if (m_nodeRole == BROKER)
    {
      NS_LOG_INFO ("手动触发代理迁移: 从节点 " << GetNode()->GetId() << " 到节点 " << newBrokerId << " 当前时间: " << Simulator::Now().GetSeconds() << "秒");
      
      // 确保Socket已初始化
      if (m_socket == nullptr)
        {
          NS_LOG_WARN("Socket为空，尝试重新创建");
          CreateSocket();
          
          if (m_socket == nullptr)
            {
              NS_LOG_ERROR("无法创建Socket，取消代理迁移");
              return;
            }
        }
      
      // 检查节点指针
      if (GetNode() == nullptr)
        {
          NS_LOG_ERROR("节点为空，取消代理迁移");
          return;
        }
      
      // 确保目标节点存在
      Ptr<Node> targetNode = NodeList::GetNode(newBrokerId);
      if (!targetNode)
        {
          NS_LOG_ERROR("无法找到目标节点 " << newBrokerId << "，取消代理迁移");
          return;
        }
      
      // 确保目标节点有应用程序并且可以作为代理
      if (targetNode->GetNApplications() == 0)
        {
          NS_LOG_ERROR("目标节点没有应用程序，无法作为代理");
          return;
        }
      
      // 新增: 检查目标节点是否在通信范围内，同时考虑移动速度
      bool targetInRange = IsInCommunicationRange(newBrokerId);
      double stability = CalculateNodeStability(newBrokerId);
      
      if (!targetInRange)
        {
          NS_LOG_ERROR("目标节点 " << newBrokerId << " 不在通信范围内，无法迁移代理");
          return;
        }
      
      // 高速移动场景下检查稳定性
      if (nodeMobility > 5.0 && stability < 0.4)
        {
          NS_LOG_ERROR("目标节点 " << newBrokerId << " 稳定性太低 (" << stability << "), 不适合作为代理");
          return;
        }
      
      // 调用迁移函数
      MigrateBroker (newBrokerId);
      NS_LOG_INFO("代理迁移已完成");
    }
    else
    {
      NS_LOG_WARN("非代理节点 " << GetNode()->GetId() << " 尝试触发迁移");
    }
}

void
NdnApp::ReceivePacket (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this);
  
  Address from;
  Ptr<Packet> packet;
  
  // 接收数据包
  while ((packet = socket->RecvFrom (from)))
    {
      try
        {
          // 保存原始packet大小用于日志
          uint32_t packetSize = packet->GetSize();
          
          // 解析NDN头
          NdnHeader ndnHeader;
          packet->RemoveHeader (ndnHeader);
          
          // 更新FIB表，将发送者作为来源内容的下一跳，如果有订阅者ID则一并记录
          if (ndnHeader.GetPacketType() == INTEREST && ndnHeader.GetSubscriberId() > 0)
            {
              UpdateFib(ndnHeader.GetContentName(), from, ndnHeader.GetSubscriberId());
            }
          else
            {
              UpdateFib(ndnHeader.GetContentName(), from);
            }
          
          // 如果是发布者，记录发布者ID
          if (ndnHeader.GetPacketType() == INTEREST && ndnHeader.GetPublisherId() > 0)
            {
              m_knownPublisherId = ndnHeader.GetPublisherId();
              NS_LOG_INFO("记录发布者ID: " << m_knownPublisherId);
            }
          
          if (enableDetailedLogging)
            {
              InetSocketAddress sourceAddr = InetSocketAddress::ConvertFrom(from);
              NS_LOG_INFO("节点 " << GetNode()->GetId() << " 收到包: 类型=" << 
                          (int)ndnHeader.GetPacketType() << ", 大小=" << packetSize << 
                          ", 来源=" << sourceAddr.GetIpv4() << ", 内容=" << ndnHeader.GetContentName());
                          
              if (ndnHeader.GetPacketType() == INTEREST) {
                NS_LOG_INFO("收到兴趣包: 订阅者ID=" << ndnHeader.GetSubscriberId() << 
                           ", 发布者ID=" << ndnHeader.GetPublisherId());
              }
            }
          
          // 根据包类型分别处理
          switch (ndnHeader.GetPacketType())
            {
              case INTEREST:
                HandleInterest (packet, from, ndnHeader);
                break;
              case DATA:
                HandleData (packet, from, ndnHeader);
                break;
              case NOTIFICATION:
                HandleBrokerNotification (packet, from, ndnHeader);
                break;
              default:
                NS_LOG_WARN ("Unknown packet type received");
                break;
            }
        }
      catch (const std::exception& e)
        {
          NS_LOG_ERROR("处理数据包时发生异常: " << e.what());
        }
      catch (...)
        {
          NS_LOG_ERROR("处理数据包时发生未知异常");
        }
    }
}

void
NdnApp::HandleInterest (Ptr<Packet> packet, Address from, NdnHeader &header)
{
  NS_LOG_FUNCTION (this << from << header.GetContentName());
  
  // 获取内容名字、Nonce和订阅者ID
  std::string contentName = header.GetContentName();
  uint32_t nonce = header.GetNonce();
  uint32_t subscriberId = header.GetSubscriberId();
  uint32_t publisherId = header.GetPublisherId();
  
  // 确保发送方在通信范围内
  bool senderInRange = false;
  uint32_t senderId = 0;
  
  // 从IP地址查找节点ID
  if (InetSocketAddress::IsMatchingType(from))
    {
      InetSocketAddress inetFrom = InetSocketAddress::ConvertFrom(from);
      for (uint32_t i = 0; i < NodeList::GetNNodes(); i++)
        {
          Ptr<Node> node = NodeList::GetNode(i);
          if (node)
            {
              Ptr<Ipv4> ipv4 = node->GetObject<Ipv4>();
              if (ipv4 && ipv4->GetNInterfaces() > 1)
                {
                  Ipv4Address addr = ipv4->GetAddress(1, 0).GetLocal();
                  if (addr == inetFrom.GetIpv4())
                    {
                      senderId = i;
                      senderInRange = IsInCommunicationRange(i);
                      
                      // 高速移动场景检查稳定性
                      if (senderInRange && nodeMobility > 5.0)
                        {
                          double stability = CalculateNodeStability(i);
                          if (stability < 0.3)
                            {
                              NS_LOG_INFO("发送方节点 " << i << " 稳定性太低 (" << stability << "), 处理兴趣包但不会回复");
                            }
                        }
                      break;
                    }
                }
            }
        }
    }
  
  if (!senderInRange)
    {
      NS_LOG_INFO("发送方节点 " << senderId << " 不在通信范围内，忽略兴趣包");
      return;
    }
  
  // 更新已知的发布者ID
  if (publisherId > 0) {
    m_knownPublisherId = publisherId;
  }
  
  // 检查是否处理过相同的兴趣包(使用Nonce和订阅者ID组合来避免循环)
  std::pair<uint32_t, uint32_t> interestKey(nonce, subscriberId);
  if (m_processedInterests.find(interestKey) != m_processedInterests.end())
    {
      if (enableDetailedLogging)
        NS_LOG_INFO ("忽略重复兴趣包: nonce=" << nonce << ", 订阅者ID=" << subscriberId);
      return;
    }
  
  // 添加到已处理兴趣集合
  m_processedInterests.insert(interestKey);
  
  // 限制已处理兴趣集合大小
  if (m_processedInterests.size() > 1000)
    {
      m_processedInterests.clear();
    }
  
  // 检查内容存储
  Ptr<Packet> contentPacket;
  bool found = CheckContentStore (contentName, contentPacket);
  
  if (found)
    {
      // 内容在本地存储，直接回复
      NS_LOG_INFO("在本地内容存储中找到内容: " << contentName);
      
      // 确保from是InetSocketAddress类型
      if (InetSocketAddress::IsMatchingType(from))
        {
          InetSocketAddress sourceDest = InetSocketAddress::ConvertFrom(from);
          // 传递订阅者ID以便优化路径选择
          SendData (contentName, sourceDest, subscriberId);
        }
      else
        {
          NS_LOG_WARN("收到不兼容地址类型的兴趣包，无法回复数据");
        }
    }
  else
    {
      // 内容不在本地，代理节点会转发兴趣
      if (m_nodeRole == BROKER)
        {
          // 代理节点可以生成内容
          if (contentName.find(m_contentPrefix) == 0)
            {
              // 生成内容
              contentPacket = Create<Packet> (packetSize);
              // 添加到内容存储
              AddToContentStore (contentName, contentPacket);
              
              // 确保from是InetSocketAddress类型
              if (InetSocketAddress::IsMatchingType(from))
                {
                  InetSocketAddress sourceDest = InetSocketAddress::ConvertFrom(from);
                  // 传递订阅者ID以便优化路径选择
                  SendData (contentName, sourceDest, subscriberId);
                  NS_LOG_INFO("代理节点生成内容并回复: " << contentName);
                }
              else
                {
                  NS_LOG_WARN("收到不兼容地址类型的兴趣包，无法回复数据");
                }
            }
        }
      else
        {
          // 普通节点，添加到PIT并转发兴趣
          AddToPit (contentName, nonce, subscriberId, publisherId, from);
          
          // 如果知道下一跳，就转发兴趣
          std::vector<InetSocketAddress> nextHops;
          if (FindFibEntry (contentName, nextHops) && !nextHops.empty())
            {
              NS_LOG_INFO("使用FIB转发兴趣包: " << contentName);
              // 重新创建Interest包并转发
              Ptr<Packet> interestPacket = Create<Packet> ();
              interestPacket->AddHeader (header);
              
              // 向所有在通信范围内的下一跳转发
              for (auto &nextHop : nextHops)
                {
                  // 查找节点ID并检查是否在通信范围内
                  for (uint32_t i = 0; i < NodeList::GetNNodes(); i++)
                    {
                      Ptr<Node> node = NodeList::GetNode(i);
                      if (node)
                        {
                          Ptr<Ipv4> ipv4 = node->GetObject<Ipv4>();
                          if (ipv4 && ipv4->GetNInterfaces() > 1)
                            {
                              Ipv4Address addr = ipv4->GetAddress(1, 0).GetLocal();
                              if (addr == nextHop.GetIpv4())
                                {
                                  bool nodeUsable = IsInCommunicationRange(i);
                                  
                                  // 高速移动场景检查稳定性
                                  if (nodeUsable && nodeMobility > 5.0)
                                    {
                                      double stability = CalculateNodeStability(i);
                                      nodeUsable = stability > 0.3;
                                      if (!nodeUsable)
                                        {
                                          NS_LOG_INFO("下一跳节点 " << i << " 稳定性太低 (" << stability << "), 跳过");
                                        }
                                    }
                                  
                                  if (nodeUsable)
                                    {
                                      m_socket->SendTo (interestPacket->Copy(), 0, nextHop);
                                      if (enableDetailedLogging)
                                        NS_LOG_INFO("转发兴趣包: " << contentName << " -> " << nextHop.GetIpv4() << 
                                                  ", 订阅者ID=" << subscriberId << ", 发布者ID=" << publisherId);
                                    }
                                  break;
                                }
                            }
                        }
                    }
                }
            }
          else
            {
              NS_LOG_INFO("无法找到FIB下一跳，尝试向周围节点发送兴趣包: " << contentName);
              
              // 没有已知下一跳，向周围节点发送
              Ptr<Packet> interestPacket = Create<Packet> ();
              interestPacket->AddHeader (header);
              
              // 向所有在通信范围内的节点发送
              for (uint32_t i = 0; i < NodeList::GetNNodes(); i++)
                {
                  if (i != GetNode()->GetId() && IsInCommunicationRange(i))
                    {
                      // 高速移动场景检查稳定性
                      bool nodeUsable = true;
                      if (nodeMobility > 5.0)
                        {
                          double stability = CalculateNodeStability(i);
                          nodeUsable = stability > 0.3;
                          if (!nodeUsable)
                            {
                              NS_LOG_INFO("邻近节点 " << i << " 稳定性太低 (" << stability << "), 跳过");
                              continue;
                            }
                        }
                      
                      if (nodeUsable)
                        {
                          Ipv4Address targetAddr = GetIpv4FromNodeId(i);
                          if (targetAddr != Ipv4Address::GetAny())
                            {
                              InetSocketAddress dest = InetSocketAddress(targetAddr, 9);
                              m_socket->SendTo (interestPacket->Copy(), 0, dest);
                              NS_LOG_INFO("向邻近节点 " << i << " 发送兴趣包");
                            }
                        }
                    }
                }
            }
        }
    }
}

void
NdnApp::HandleData (Ptr<Packet> packet, Address from, NdnHeader &header)
{
  NS_LOG_FUNCTION (this << from << header.GetContentName());
  
  // 获取内容名字和订阅者ID (已修复未使用变量警告)
  std::string contentName = header.GetContentName();
  // 添加到内容存储
  AddToContentStore (contentName, packet);
  
  // 查找匹配的PIT条目
  std::vector<InetSocketAddress> sources;
  std::vector<uint32_t> outSubscriberIds;
  bool found = FindAndRemovePitEntry (contentName, sources, outSubscriberIds);
  
  if (found)
    {
      NS_LOG_INFO("找到PIT条目，转发数据至 " << sources.size() << " 个请求来源");
      // 转发数据包到所有在通信范围内的兴趣源
      for (size_t i = 0; i < sources.size(); i++)
        {
          uint32_t subId = (i < outSubscriberIds.size()) ? outSubscriberIds[i] : 0;
          
          // 查找节点ID并检查是否在通信范围内
          for (uint32_t j = 0; j < NodeList::GetNNodes(); j++)
            {
              Ptr<Node> node = NodeList::GetNode(j);
              if (node)
                {
                  Ptr<Ipv4> ipv4 = node->GetObject<Ipv4>();
                  if (ipv4 && ipv4->GetNInterfaces() > 1)
                    {
                      Ipv4Address addr = ipv4->GetAddress(1, 0).GetLocal();
                      if (addr == sources[i].GetIpv4())
                        {
                          bool nodeUsable = IsInCommunicationRange(j);
                          
                          // 高速移动场景检查稳定性
                          if (nodeUsable && nodeMobility > 5.0)
                            {
                              double stability = CalculateNodeStability(j);
                              if (stability < 0.3)
                                {
                                  NS_LOG_INFO("PIT来源节点 " << j << " 稳定性太低 (" << stability << "), 尝试发送但可能失败");
                                }
                            }
                          
                          if (nodeUsable)
                            {
                              // 传递订阅者ID以便优化路径选择
                              SendData (contentName, sources[i], subId);
                            }
                          else
                            {
                              NS_LOG_INFO("PIT来源节点 " << j << " 不在通信范围内，跳过");
                            }
                          break;
                        }
                    }
                }
            }
        }
    }
  
  // 如果是订阅者，记录消息接收成功
  if (m_nodeRole == SUBSCRIBER)
    {
      s_messagesReceived++;
      RecordMessageSuccess (true);
      NS_LOG_INFO ("订阅者收到数据: " << contentName << ", 总计已接收消息: " << s_messagesReceived);
    }
}

void
NdnApp::HandleBrokerNotification (Ptr<Packet> packet, Address from, NdnHeader &header)
{
  InetSocketAddress sourceAddr = InetSocketAddress::ConvertFrom(from);
  NS_LOG_INFO ("节点 " << GetNode()->GetId() << " 收到代理迁移通知, 来源: " << sourceAddr.GetIpv4());
  
  // 获取新代理ID
  uint32_t newBrokerId = header.GetNewBrokerId();
  
  // 获取迁移时间戳
  Time migrationTime = header.GetMigrationTime();
  
  // 判断通知类型
  std::string contentName = header.GetContentName();
  if (contentName.find("/unicastBrokerChange") != std::string::npos)
    {
      m_notificationType = UNICAST_NOTIFICATION;
      NS_LOG_INFO ("收到单播通知");
    }
  else if (contentName.find("/interestBrokerChange") != std::string::npos)
    {
      m_notificationType = INTEREST_NOTIFICATION;
      NS_LOG_INFO ("收到基于兴趣的通知");
    }
  else if (contentName.find("/broadcastBrokerChange") != std::string::npos)
    {
      m_notificationType = BROADCAST_NOTIFICATION;
      NS_LOG_INFO ("收到广播通知");
    }
  
  // 更新对代理的认知
  m_lastKnownBrokerId = m_currentBrokerId;
  m_currentBrokerId = newBrokerId;
  
  // 更新FIB，指向新代理，并确保新代理在通信范围内
  if (IsInCommunicationRange(newBrokerId))
    {
      // 在高速移动场景下，检查新代理稳定性
      bool brokerUsable = true;
      if (nodeMobility > 5.0)
        {
          double stability = CalculateNodeStability(newBrokerId);
          brokerUsable = stability > 0.3;
          NS_LOG_INFO("新代理稳定性检查: " << stability << ", 结果=" << (brokerUsable ? "可用" : "不可用"));
        }
      
      if (brokerUsable)
        {
          Ipv4Address newBrokerAddr = GetIpv4FromNodeId(newBrokerId);
          if (newBrokerAddr != Ipv4Address::GetAny())
            {
              InetSocketAddress newBrokerSocketAddr = InetSocketAddress(newBrokerAddr, 9);
              
              // 更新FIB条目
              UpdateFib(m_contentPrefix, newBrokerSocketAddr);
              NS_LOG_INFO("更新FIB: " << m_contentPrefix << " -> " << newBrokerAddr << "（新代理在通信范围内）");
            }
        }
      else
        {
          NS_LOG_INFO("新代理 " << newBrokerId << " 稳定性太低，需要找到中继节点");
          // 寻找更稳定的中继节点
          uint32_t relayNodeId = GetNearestNodeId(GetCurrentPosition(), {(uint32_t)GetNode()->GetId(), newBrokerId});
          if (relayNodeId != GetNode()->GetId() && IsInCommunicationRange(relayNodeId))
            {
              double stability = CalculateNodeStability(relayNodeId);
              if (stability > 0.4) // 要求中继节点更高的稳定性
                {
                  Ipv4Address relayAddr = GetIpv4FromNodeId(relayNodeId);
                  if (relayAddr != Ipv4Address::GetAny())
                    {
                      InetSocketAddress relaySocketAddr = InetSocketAddress(relayAddr, 9);
                      UpdateFib(m_contentPrefix, relaySocketAddr);
                      NS_LOG_INFO("使用高稳定性中继节点 " << relayNodeId << " (稳定性=" << stability << ") 到达新代理");
                    }
                }
            }
        }
    }
    else
    {
      NS_LOG_INFO("新代理 " << newBrokerId << " 不在通信范围内，需要找到中继节点");
      
      // 找到最近的可作为中继的节点
      uint32_t relayNodeId = GetNearestNodeId(GetCurrentPosition(), {(uint32_t)GetNode()->GetId()});
      if (relayNodeId != GetNode()->GetId() && IsInCommunicationRange(relayNodeId))
        {
          // 高速移动场景下检查中继节点稳定性
          bool relayUsable = true;
          if (nodeMobility > 5.0)
            {
              double stability = CalculateNodeStability(relayNodeId);
              relayUsable = stability > 0.4; // 要求中继节点更高的稳定性
              if (!relayUsable)
                {
                  NS_LOG_INFO("最近中继节点 " << relayNodeId << " 稳定性太低 (" << stability << "), 寻找替代");
                  // 尝试找到稳定性更高的节点
                  for (uint32_t i = 0; i < NodeList::GetNNodes(); i++)
                    {
                      if (i != GetNode()->GetId() && i != relayNodeId && IsInCommunicationRange(i))
                        {
                          double newStability = CalculateNodeStability(i);
                          if (newStability > 0.4)
                            {
                              relayNodeId = i;
                              relayUsable = true;
                              NS_LOG_INFO("找到替代中继节点 " << i << " 稳定性=" << newStability);
                              break;
                            }
                        }
                    }
                }
            }
          
          if (relayUsable)
            {
              Ipv4Address relayAddr = GetIpv4FromNodeId(relayNodeId);
              if (relayAddr != Ipv4Address::GetAny())
                {
                  InetSocketAddress relaySocketAddr = InetSocketAddress(relayAddr, 9);
                  
                  // 更新FIB，使用中继节点作为下一跳
                  UpdateFib(m_contentPrefix, relaySocketAddr);
                  NS_LOG_INFO("使用中继节点 " << relayNodeId << " 到达新代理 " << newBrokerId);
                }
            }
          else
            {
              NS_LOG_INFO("无法找到合适的中继节点，将使用广播方式通信");
            }
        }
    }
  
  NS_LOG_INFO ("节点 " << GetNode()->GetId() << " 收到代理迁移通知，新代理: " << newBrokerId);
  
  // 如果是发布者或订阅者，记录通知延迟
  if (m_nodeRole == PUBLISHER || m_nodeRole == SUBSCRIBER)
    {
      // 计算通知延迟，使用通知包中的时间戳
      Time notificationDelay = Simulator::Now() - migrationTime;
      RecordNotificationDelay (notificationDelay);
      
      NS_LOG_INFO ("通知延迟: " << notificationDelay.GetSeconds() << " 秒, 迁移时间: " 
                  << migrationTime.GetSeconds() << ", 当前时间: " << Simulator::Now().GetSeconds());
      
      // 继续向其他节点转发通知，但仅转发给在通信范围内的节点
      if (newBrokerId != GetNode()->GetId()) // 避免自己转发给自己
        {
          // 广播通知，确保其他节点也收到
          Ptr<Packet> notificationPacket = Create<Packet> ();
          NdnHeader newHeader;
          newHeader.SetContentName (contentName);
          newHeader.SetPacketType (NOTIFICATION);
          newHeader.SetNewBrokerId (newBrokerId);
          newHeader.SetNonce (header.GetNonce());
          newHeader.SetMigrationTime (migrationTime);
          
          notificationPacket->AddHeader (newHeader);
          
          // 发送到已知的在通信范围内的邻居节点
          for (auto &fibEntry : m_fibTable)
            {
              for (auto &nextHop : fibEntry.nextHops)
                {
                  if (InetSocketAddress::ConvertFrom(from).GetIpv4() != nextHop.GetIpv4()) // 避免发回给发送者
                    {
                      // 从IP地址查找节点ID并检查是否在通信范围内
                      for (uint32_t i = 0; i < NodeList::GetNNodes(); i++)
                        {
                          Ptr<Node> node = NodeList::GetNode(i);
                          if (node)
                            {
                              Ptr<Ipv4> ipv4 = node->GetObject<Ipv4>();
                              if (ipv4 && ipv4->GetNInterfaces() > 1)
                                {
                                  Ipv4Address addr = ipv4->GetAddress(1, 0).GetLocal();
                                  if (addr == nextHop.GetIpv4())
                                    {
                                      bool nodeUsable = IsInCommunicationRange(i);
                                      
                                      // 高速移动场景检查稳定性
                                      if (nodeUsable && nodeMobility > 5.0)
                                        {
                                          double stability = CalculateNodeStability(i);
                                          nodeUsable = stability > 0.3;
                                          if (!nodeUsable)
                                            {
                                              NS_LOG_INFO("邻居节点 " << i << " 稳定性太低 (" << stability << "), 跳过转发");
                                            }
                                        }
                                      
                                      if (nodeUsable)
                                        {
                                          m_socket->SendTo (notificationPacket->Copy(), 0, nextHop);
                                          NS_LOG_INFO("转发通知到邻居节点 " << i);
                                        }
                                      break;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

void
NdnApp::RecordNotificationDelay (Time delay)
{
  // 排除负值和异常大的延迟
  if (delay < Seconds(0) || delay > Seconds(300))
    {
      NS_LOG_WARN("忽略异常延迟值: " << delay.GetSeconds() << "秒");
      return;
    }
  
  // 总体延迟
  s_totalNotificationDelay += delay;
  s_notificationCount++;
  
  // 按通知类型分别记录
  switch (m_notificationType)
    {
      case UNICAST_NOTIFICATION:
        s_unicastNotificationDelay += delay;
        s_unicastNotificationCount++;
        NS_LOG_INFO ("记录单播通知延迟: " << delay.GetSeconds() << "秒");
        break;
        
      case INTEREST_NOTIFICATION:
        s_interestNotificationDelay += delay;
        s_interestNotificationCount++;
        NS_LOG_INFO ("记录基于兴趣的通知延迟: " << delay.GetSeconds() << "秒");
        break;
        
      case BROADCAST_NOTIFICATION:
        s_broadcastNotificationDelay += delay;
        s_broadcastNotificationCount++;
        NS_LOG_INFO ("记录广播通知延迟: " << delay.GetSeconds() << "秒");
        break;
        
      default:
        NS_LOG_WARN ("未知通知类型");
        break;
    }
}

void
NdnApp::RecordMessageSuccess (bool success)
{
  // 这里简单统计收到的消息数量，success总是true
  if (success)
    {
      NS_LOG_INFO("消息接收成功，总计已接收: " << s_messagesReceived << " / 已发送: " << s_messagesSent);
    }
}

Time
NdnApp::GetAverageNotificationDelay (void)
{
  if (s_notificationCount > 0)
    {
      return s_totalNotificationDelay / s_notificationCount;
    }
  return Time (0);
}

Time
NdnApp::GetUnicastNotificationDelay (void)
{
  if (s_unicastNotificationCount > 0)
    {
      return s_unicastNotificationDelay / s_unicastNotificationCount;
    }
  return Time (0);
}

Time
NdnApp::GetInterestNotificationDelay (void)
{
  if (s_interestNotificationCount > 0)
    {
      return s_interestNotificationDelay / s_interestNotificationCount;
    }
  return Time (0);
}

Time
NdnApp::GetBroadcastNotificationDelay (void)
{
  if (s_broadcastNotificationCount > 0)
    {
      return s_broadcastNotificationDelay / s_broadcastNotificationCount;
    }
  return Time (0);
}

double
NdnApp::GetMessageSuccessRate (void)
{
  if (s_messagesSent > 0)
    {
      return (double) s_messagesReceived / s_messagesSent;
    }
  return 0.0;
}

bool
NdnApp::CheckContentStore (std::string contentName, Ptr<Packet> &outPacket)
{
  // 查找内容存储
  for (auto &entry : m_contentStore)
    {
      if (entry.contentName == contentName && Simulator::Now() < entry.expiryTime)
        {
          outPacket = entry.data;
          return true;
        }
    }
  return false;
}

void
NdnApp::AddToContentStore (std::string contentName, Ptr<Packet> packet)
{
  // 先检查是否已存在
  for (auto &entry : m_contentStore)
    {
      if (entry.contentName == contentName)
        {
          // 更新现有条目
          entry.data = packet;
          entry.expiryTime = Simulator::Now() + Seconds (300); // 缓存5分钟
          return;
        }
    }
  
  // 限制内容存储大小
  if (m_contentStore.size() >= 200)
    {
      // 移除最老的条目
      m_contentStore.erase (m_contentStore.begin());
    }
  
  // 添加新条目
  ContentStoreEntry entry;
  entry.contentName = contentName;
  entry.data = packet;
  entry.expiryTime = Simulator::Now() + Seconds (300); // 缓存5分钟
  
  m_contentStore.push_back (entry);
}

void
NdnApp::AddToPit (std::string contentName, uint32_t nonce, uint32_t subscriberId, uint32_t publisherId, Address source)
{
  // 确保地址是InetSocketAddress类型并进行转换
  if (!InetSocketAddress::IsMatchingType(source))
    {
      NS_LOG_WARN("添加PIT条目时遇到不兼容的地址类型，尝试转换");
      
      // 尝试获取当前节点的有效地址
      Ptr<Node> node = GetNode();
      if (node && node->GetObject<Ipv4>() && node->GetObject<Ipv4>()->GetNInterfaces() > 1)
        {
          Ipv4Address fallbackAddr = node->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal();
          InetSocketAddress fallbackSource = InetSocketAddress(fallbackAddr, 9);
          
          // 检查是否已存在相同内容和Nonce的条目
          for (auto &entry : m_pitTable)
            {
              if (entry.contentName == contentName && entry.nonce == nonce && 
                  entry.subscriberId == subscriberId)
                {
                  // 更新过期时间
                  entry.expiryTime = Simulator::Now() + Seconds (4);
                  return;
                }
            }
          
          // 使用带参数的构造函数创建新条目，使用备用地址
          PitEntry entry(contentName, nonce, subscriberId, publisherId, 
                         fallbackSource, Simulator::Now() + Seconds(4));
          m_pitTable.push_back (entry);
          NS_LOG_INFO("使用备用地址添加PIT条目: " << contentName);
        }
      else
        {
          NS_LOG_ERROR("无法创建备用地址，放弃添加PIT条目");
        }
      
      return;
    }
  
  // 转换为InetSocketAddress
  InetSocketAddress inetSource = InetSocketAddress::ConvertFrom(source);
  
  // 检查是否已存在相同内容和Nonce的条目
  for (auto &entry : m_pitTable)
    {
      if (entry.contentName == contentName && entry.nonce == nonce && 
          entry.subscriberId == subscriberId)
        {
          // 更新过期时间
          entry.expiryTime = Simulator::Now() + Seconds (4);
          return;
        }
    }
  
  // 使用带参数的构造函数创建新条目
  PitEntry entry(contentName, nonce, subscriberId, publisherId, 
                 inetSource, Simulator::Now() + Seconds(4));
  m_pitTable.push_back (entry);
  NS_LOG_INFO("添加PIT条目: " << contentName << " 来源: " << inetSource.GetIpv4() << 
             ", 订阅者ID=" << subscriberId << ", 发布者ID=" << publisherId);
}

bool
NdnApp::FindAndRemovePitEntry (std::string contentName, std::vector<InetSocketAddress> &outSources, std::vector<uint32_t> &outSubscriberIds)
{
  bool found = false;
  
  // 找到所有匹配contentName的PIT条目
  for (auto it = m_pitTable.begin(); it != m_pitTable.end(); )
    {
      if (it->contentName == contentName && Simulator::Now() < it->expiryTime)
        {
          outSources.push_back (it->sourceAddress);
          outSubscriberIds.push_back (it->subscriberId);
          it = m_pitTable.erase (it);
          found = true;
        }
      else
        {
          ++it;
        }
    }
  
  return found;
}

bool
NdnApp::FindFibEntry (std::string contentName, std::vector<InetSocketAddress> &outNextHops)
{
  // 查找最长前缀匹配
  std::string bestPrefix = "";
  
  for (auto &entry : m_fibTable)
    {
      // 检查是否是前缀
      if (contentName.find(entry.prefix) == 0 && entry.prefix.length() > bestPrefix.length())
        {
          bestPrefix = entry.prefix;
          outNextHops = entry.nextHops;
        }
    }
  
  return !bestPrefix.empty();
}

void
NdnApp::UpdateFib (std::string prefix, Address nextHop, uint32_t subscriberId, double distance)
{
  // 确保地址是InetSocketAddress类型
  if (!InetSocketAddress::IsMatchingType(nextHop))
    {
      NS_LOG_WARN("更新FIB时遇到不兼容的地址类型，跳过更新");
      return;
    }
  
  // 转换为InetSocketAddress
  InetSocketAddress inetNextHop = InetSocketAddress::ConvertFrom(nextHop);
  
  // 查找已有条目
  for (auto &entry : m_fibTable)
    {
      if (entry.prefix == prefix)
        {
          // 检查是否已存在该nextHop
          bool found = false;
          for (auto &hop : entry.nextHops)
            {
              if (hop.GetIpv4() == inetNextHop.GetIpv4() && hop.GetPort() == inetNextHop.GetPort())
                {
                  found = true;
                  break;
                }
            }
          
          if (!found)
            {
              // 添加新的nextHop
              entry.nextHops.push_back (inetNextHop);
            }
          
          // 如果有订阅者ID，更新订阅者路径
          if (subscriberId > 0)
            {
              // 使用insert_or_assign代替operator[]
              entry.subscriberPaths.insert_or_assign(subscriberId, inetNextHop);
              NS_LOG_INFO("更新订阅者 " << subscriberId << " 的路径: " << inetNextHop.GetIpv4());
            }
          
          // 更新节点距离信息 - 如果提供了距离值
          if (distance >= 0)
            {
              // 从IP地址查找节点ID
              for (uint32_t i = 0; i < NodeList::GetNNodes(); i++)
                {
                  Ptr<Node> node = NodeList::GetNode(i);
                  if (node)
                    {
                      Ptr<Ipv4> ipv4 = node->GetObject<Ipv4>();
                      if (ipv4 && ipv4->GetNInterfaces() > 1)
                        {
                          Ipv4Address addr = ipv4->GetAddress(1, 0).GetLocal();
                          if (addr == inetNextHop.GetIpv4())
                            {
                              entry.nodeDistances[i] = distance;
                              
                              // 新增: 更新节点稳定性评分
                              entry.nodeStability[i] = CalculateNodeStability(i);
                              
                              NS_LOG_INFO("更新节点 " << i << " 的距离: " << distance 
                                         << ", 稳定性: " << entry.nodeStability[i]);
                              break;
                            }
                        }
                    }
                }
            }
          
          // 更新上次更新时间
          entry.lastUpdateTime = Simulator::Now();
          
          return;
        }
    }
  
  // 使用带参数的构造函数创建新条目
  FibEntry entry(prefix);
  entry.nextHops.push_back(inetNextHop);
  entry.lastUpdateTime = Simulator::Now();
  
  // 如果有订阅者ID，记录订阅者路径
  if (subscriberId > 0)
    {
      // 使用insert代替operator[]
      entry.subscriberPaths.insert(std::make_pair(subscriberId, inetNextHop));
      NS_LOG_INFO("记录订阅者 " << subscriberId << " 的路径: " << inetNextHop.GetIpv4());
    }
  
  // 记录节点距离信息
  if (distance >= 0)
    {
      // 从IP地址查找节点ID
      for (uint32_t i = 0; i < NodeList::GetNNodes(); i++)
        {
          Ptr<Node> node = NodeList::GetNode(i);
          if (node)
            {
              Ptr<Ipv4> ipv4 = node->GetObject<Ipv4>();
              if (ipv4 && ipv4->GetNInterfaces() > 1)
                {
                  Ipv4Address addr = ipv4->GetAddress(1, 0).GetLocal();
                  if (addr == inetNextHop.GetIpv4())
                    {
                      entry.nodeDistances[i] = distance;
                      
                      // 新增: 更新节点稳定性评分
                      entry.nodeStability[i] = CalculateNodeStability(i);
                      
                      NS_LOG_INFO("记录节点 " << i << " 的距离: " << distance 
                                 << ", 稳定性: " << entry.nodeStability[i]);
                      break;
                    }
                }
            }
        }
    }
  
  m_fibTable.push_back(entry);
}

void
NdnApp::CleanupExpiredPitEntries ()
{
  // 移除过期PIT条目
  for (auto it = m_pitTable.begin(); it != m_pitTable.end(); )
    {
      if (Simulator::Now() > it->expiryTime)
        {
          it = m_pitTable.erase (it);
        }
      else
        {
          ++it;
        }
    }
  
  // 定期清理
  m_cleanupEvent = Simulator::Schedule (Seconds (10.0), &NdnApp::CleanupExpiredPitEntries, this);
}

// 新增: 更新节点位置信息
void
NdnApp::UpdateNodeLocation()
{
  Ptr<Node> node = GetNode();
  if (node)
    {
      Ptr<MobilityModel> mobility = node->GetObject<MobilityModel>();
      if (mobility)
        {
          Vector position = mobility->GetPosition();
          Vector velocity = mobility->GetVelocity();
          double speed = std::sqrt(velocity.x * velocity.x + velocity.y * velocity.y + velocity.z * velocity.z);
          uint32_t nodeId = node->GetId();
          
          // 计算加速度和记录位置变化
          double acceleration = 0.0;
          if (m_positionUpdateCount > 0)
            {
              // 计算与上次位置的时间差
              double timeDiff = 0.5; // 默认位置更新间隔
              
              // 查找上次记录的位置信息
              auto it = s_nodeLocations.find(nodeId);
              if (it != s_nodeLocations.end())
                {
                  // 如果有上次位置记录，计算实际时间差
                  Time lastUpdateTime = it->second.lastUpdateTime;
                  timeDiff = (Simulator::Now() - lastUpdateTime).GetSeconds();
                  
                  // 过短的时间差可能导致不稳定的计算，设置最小值
                  if (timeDiff < 0.1) timeDiff = 0.1;
                  
                  // 计算加速度（速度变化率）
                  double lastSpeed = it->second.speed;
                  double speedDiff = speed - lastSpeed;
                  acceleration = speedDiff / timeDiff;
                }
            }
          
          // 记录当前位置和速度等信息
          NodeLocationInfo locationInfo;
          locationInfo.position = position;
          locationInfo.velocity = velocity;
          locationInfo.lastUpdateTime = Simulator::Now();
          locationInfo.speed = speed;
          locationInfo.acceleration = acceleration;
          
          // 更新到全局位置信息缓存
          s_nodeLocations[nodeId] = locationInfo;
          
          // 更新上次位置和计数器
          m_lastPosition = position;
          m_positionUpdateCount++;
          
          if (enableDetailedLogging && (m_positionUpdateCount % 10 == 0 || speed > nodeMobility * 1.2))
            {
              NS_LOG_INFO("节点 " << nodeId << " 位置: (" << position.x << "," << position.y << "," 
                         << position.z << "), 速度: " << speed << " m/s, 加速度: " << acceleration);
            }
        }
    }
  
  // 调度下一次位置更新，更新频率与节点速度相关
  // 速度越快，更新越频繁
  double updateInterval = std::max(0.1, 0.5 / (1.0 + (nodeMobility / 5.0)));
  m_locationUpdateEvent = Simulator::Schedule (Seconds (updateInterval), &NdnApp::UpdateNodeLocation, this);
}

// 新增: 周期性更新路由拓扑
void
NdnApp::UpdateRoutingTopology()
{
  // 更新路由表中的节点距离信息
  Vector myPosition = GetCurrentPosition();
  
  // 更新到所有已知节点的距离
  for (uint32_t i = 0; i < NodeList::GetNNodes(); i++)
    {
      if (i != GetNode()->GetId())
        {
          // 获取节点位置
          auto it = s_nodeLocations.find(i);
          if (it != s_nodeLocations.end())
            {
              Vector nodePosition = it->second.position;
              double distance = CalculateDistance(myPosition, nodePosition);
              
              // 计算可能的通信可靠性 (基于距离和相对速度)
              double reliability = 1.0 - (distance / (communicationRange * 1.2));
              
              // 考虑相对速度的影响 - 相对速度越大，可靠性越低
              Vector relativeVelocity;
              Vector myVelocity = GetCurrentVelocity();
              relativeVelocity.x = it->second.velocity.x - myVelocity.x;
              relativeVelocity.y = it->second.velocity.y - myVelocity.y;
              relativeVelocity.z = it->second.velocity.z - myVelocity.z;
              
              double relativeSpeed = std::sqrt(
                  relativeVelocity.x * relativeVelocity.x + 
                  relativeVelocity.y * relativeVelocity.y + 
                  relativeVelocity.z * relativeVelocity.z);
              
              // 相对速度越大，可靠性系数越低
              double speedFactor = std::max(0.3, 1.0 - (relativeSpeed / 20.0));
              reliability *= speedFactor;
              
              // 计算稳定性评分
              double stability = CalculateNodeStability(i);
              
              // 综合可靠性和稳定性
              reliability *= stability;
              reliability = std::max(0.0, std::min(1.0, reliability));
              
              if (reliability > 0.1) // 只考虑最低通信可靠性为10%的节点
                {
                  // 获取节点IP地址
                  Ipv4Address nodeAddr = GetIpv4FromNodeId(i);
                  if (nodeAddr != Ipv4Address::GetAny())
                    {
                      // 更新FIB中的距离信息
                      InetSocketAddress nodeDest = InetSocketAddress(nodeAddr, 9);
                      
                      // 为所有前缀更新该节点的距离信息
                      for (auto &entry : m_fibTable)
                        {
                          bool found = false;
                          for (auto &hop : entry.nextHops)
                            {
                              if (hop.GetIpv4() == nodeAddr)
                                {
                                  // 已存在该节点的下一跳，更新距离
                                  entry.nodeDistances[i] = distance;
                                  entry.nodeStability[i] = stability;
                                  found = true;
                                  break;
                                }
                            }
                          
                          // 如果可靠性高且不在下一跳列表中，考虑添加
                          if (!found && reliability > 0.5)
                            {
                              entry.nextHops.push_back(nodeDest);
                              entry.nodeDistances[i] = distance;
                              entry.nodeStability[i] = stability;
                              
                              if (enableDetailedLogging)
                                NS_LOG_INFO("添加高可靠性节点 " << i << " 作为下一跳: 距离=" 
                                           << distance << ", 可靠性=" << reliability 
                                           << ", 稳定性=" << stability);
                            }
                        }
                    }
                }
            }
        }
    }
  
  // 根据节点移动速度设置更新频率 - 速度越快，更新越频繁
  double updateFreq = std::max(0.5, routingUpdateInterval / (1.0 + (nodeMobility / 5.0)));
  m_routingUpdateEvent = Simulator::Schedule (Seconds (updateFreq), &NdnApp::UpdateRoutingTopology, this);
}

// 新增: 获取最近节点ID
uint32_t
NdnApp::GetNearestNodeId(Vector position, std::vector<uint32_t> excludeIds)
{
  double minDistance = std::numeric_limits<double>::max();
  uint32_t nearestId = GetNode()->GetId(); // 默认为自己
  
  for (uint32_t i = 0; i < NodeList::GetNNodes(); i++)
    {
      // 排除指定节点
      if (std::find(excludeIds.begin(), excludeIds.end(), i) != excludeIds.end())
        continue;
      
      // 获取节点位置
      auto it = s_nodeLocations.find(i);
      if (it != s_nodeLocations.end())
        {
          Vector nodePosition = it->second.position;
          double distance = CalculateDistance(position, nodePosition);
          
          // 考虑速度因素 - 速度越高的节点距离权重越大
          double speedWeight = 1.0 + (it->second.speed / 10.0);
          double weightedDistance = distance * speedWeight;
          
          // 如果节点移动速度很高，增加距离权重
          if (nodeMobility > 5.0)
            {
              // 计算稳定性
              double stability = CalculateNodeStability(i);
              // 稳定性低的节点距离权重增加
              weightedDistance /= std::max(0.2, stability);
            }
          
          if (weightedDistance < minDistance)
            {
              minDistance = weightedDistance;
              nearestId = i;
            }
        }
    }
  
  return nearestId;
}

// 新增: 判断是否在通信范围内
bool
NdnApp::IsInCommunicationRange(uint32_t nodeId)
{
  // 获取当前节点位置
  Vector myPosition = GetCurrentPosition();
  
  // 获取目标节点位置
  auto it = s_nodeLocations.find(nodeId);
  if (it != s_nodeLocations.end())
    {
      Vector nodePosition = it->second.position;
      double distance = CalculateDistance(myPosition, nodePosition);
      
      // 考虑移动速度的影响 - 速度越快，有效通信范围越小
      double effectiveRange = communicationRange;
      
      // 计算相对速度
      Vector myVelocity = GetCurrentVelocity();
      Vector relativeVelocity;
      relativeVelocity.x = it->second.velocity.x - myVelocity.x;
      relativeVelocity.y = it->second.velocity.y - myVelocity.y;
      relativeVelocity.z = it->second.velocity.z - myVelocity.z;
      
      double relativeSpeed = std::sqrt(
          relativeVelocity.x * relativeVelocity.x + 
          relativeVelocity.y * relativeVelocity.y + 
          relativeVelocity.z * relativeVelocity.z);
      
      // 相对速度越高，有效通信范围越小
      if (relativeSpeed > 2.0)
        {
          // 每增加1m/s的相对速度，减少2%的有效通信范围
          double reductionFactor = std::min(0.5, relativeSpeed * 0.02);
          effectiveRange *= (1.0 - reductionFactor);
        }
      
      // 应用有效通信范围限制
      return distance <= effectiveRange;
    }
  
  // 默认情况下假设不在范围内
  return false;
}

// 新增: 计算节点稳定性评分
double
NdnApp::CalculateNodeStability(uint32_t nodeId)
{
  // 稳定性评分范围 0.0-1.0，1.0表示最稳定
  double stabilityScore = 1.0;
  
  auto it = s_nodeLocations.find(nodeId);
  if (it != s_nodeLocations.end())
    {
      // 考虑以下因素影响稳定性:
      // 1. 速度 - 速度越高稳定性越低
      double speedFactor = std::max(0.0, 1.0 - (it->second.speed / 20.0));
      
      // 2. 加速度 - 加速度越高稳定性越低
      double accelerationFactor = std::max(0.0, 1.0 - (std::abs(it->second.acceleration) / 5.0));
      
      // 3. 历史稳定性 - 根据节点历史轨迹评估
      double historicalFactor = 1.0;
      
      // 4. 当前移动模式对全局移动速度影响
      double globalSpeedFactor = 1.0;
      if (nodeMobility > 0)
        {
          // 当节点速度远高于全局设定速度时，稳定性降低
          if (it->second.speed > nodeMobility * 1.5)
            {
              globalSpeedFactor = std::max(0.3, nodeMobility / it->second.speed);
            }
          // 当节点速度远低于全局设定速度时，稳定性提高
          else if (it->second.speed < nodeMobility * 0.5 && it->second.speed < 1.0)
            {
              globalSpeedFactor = std::min(1.5, 1.0 + (nodeMobility - it->second.speed) / nodeMobility);
            }
        }
      
      // 综合考虑各因素，计算最终稳定性评分
      stabilityScore = 0.4 * speedFactor + 
                       0.3 * accelerationFactor + 
                       0.1 * historicalFactor + 
                       0.2 * globalSpeedFactor;
      
      // 限制范围在 0.1 到 1.0 之间
      stabilityScore = std::max(0.1, std::min(1.0, stabilityScore));
    }
  
  return stabilityScore;
}

// 新增: 预测节点未来位置
Vector
NdnApp::PredictFuturePosition(uint32_t nodeId, double timeOffset)
{
  Vector futurePosition;
  
  auto it = s_nodeLocations.find(nodeId);
  if (it != s_nodeLocations.end())
    {
      // 使用当前位置、速度和加速度预测未来位置
      futurePosition.x = it->second.position.x + it->second.velocity.x * timeOffset + 
                         0.5 * it->second.acceleration * timeOffset * timeOffset;
      futurePosition.y = it->second.position.y + it->second.velocity.y * timeOffset +
                         0.5 * it->second.acceleration * timeOffset * timeOffset;
      futurePosition.z = it->second.position.z + it->second.velocity.z * timeOffset +
                         0.5 * it->second.acceleration * timeOffset * timeOffset;
    }
  else
    {
      // 如果没有位置信息，返回原点
      futurePosition = Vector(0, 0, 0);
    }
  
  return futurePosition;
}

// 新增: 从NodeId获取IP地址
Ipv4Address 
NdnApp::GetIpv4FromNodeId(uint32_t nodeId)
{
  if (nodeId < NodeList::GetNNodes())
    {
      Ptr<Node> node = NodeList::GetNode(nodeId);
      if (node)
        {
          Ptr<Ipv4> ipv4 = node->GetObject<Ipv4>();
          if (ipv4 && ipv4->GetNInterfaces() > 1)
            {
              return ipv4->GetAddress(1, 0).GetLocal();
            }
        }
    }
  
  return Ipv4Address::GetAny();
}

// 新增: 获取节点当前位置
Vector
NdnApp::GetCurrentPosition()
{
  Ptr<Node> node = GetNode();
  if (node)
    {
      Ptr<MobilityModel> mobility = node->GetObject<MobilityModel>();
      if (mobility)
        {
          return mobility->GetPosition();
        }
    }
  
  // 默认位置
  return Vector(0, 0, 0);
}

// 新增: 获取节点当前速度向量
Vector
NdnApp::GetCurrentVelocity()
{
  Ptr<Node> node = GetNode();
  if (node)
    {
      Ptr<MobilityModel> mobility = node->GetObject<MobilityModel>();
      if (mobility)
        {
          return mobility->GetVelocity();
        }
    }
  
  // 默认速度
  return Vector(0, 0, 0);
}

int main (int argc, char *argv[])
{
  // 配置日志组件
  LogComponentEnable ("NdnInterestSimulation", LOG_LEVEL_INFO);
  
  // 允许命令行参数修改配置
  CommandLine cmd;
  cmd.AddValue ("numNodes", "Number of nodes", numNodes);
  cmd.AddValue ("simTime", "Simulation time in seconds", simulationTime);
  cmd.AddValue ("mobility", "Node mobility speed in m/s", nodeMobility);
  cmd.AddValue ("detailLog", "Enable detailed logging", enableDetailedLogging);
  cmd.AddValue ("commRange", "Communication range in meters", communicationRange);
  cmd.Parse (argc, argv);
  
  // 确保至少有50个节点
  if (numNodes < 50)
    {
      numNodes = 50;
    }
  
  // 根据移动速度动态调整路由更新间隔
  routingUpdateInterval = std::max(1.0, 5.0 / (nodeMobility + 0.1));
  NS_LOG_INFO("节点移动速度: " << nodeMobility << " m/s, 路由更新间隔: " << routingUpdateInterval << " 秒");
  
  // 创建节点
  NodeContainer nodes;
  nodes.Create (numNodes);
  
  // 配置WiFi，增强Wi-Fi传输参数以确保连接稳定性
  WifiHelper wifi;
  wifi.SetStandard (WIFI_STANDARD_80211b);
  
  YansWifiPhyHelper wifiPhy;
  YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default ();
  wifiPhy.SetChannel (wifiChannel.Create ());
  
  // 增强传输功率和接收增益
  wifiPhy.Set("TxPowerStart", DoubleValue(16.0));
  wifiPhy.Set("TxPowerEnd", DoubleValue(16.0));
  wifiPhy.Set("TxGain", DoubleValue(12.0));
  wifiPhy.Set("RxGain", DoubleValue(12.0));
  
  // 动态调整接收灵敏度来适应通信范围，使之与移动速度有关
  double adaptedRange = communicationRange * (1.0 - 0.05 * std::min(6.0, nodeMobility / 2.0));
  wifiPhy.Set("RxSensitivity", DoubleValue(-95.0 - (adaptedRange / 10.0)));
  
  NS_LOG_INFO("移动场景调整: 适应通信范围=" << adaptedRange << " m (原始=" << communicationRange << " m)");
  
  WifiMacHelper wifiMac;
  wifiMac.SetType ("ns3::AdhocWifiMac");
  
  NetDeviceContainer devices = wifi.Install (wifiPhy, wifiMac, nodes);
  
  // 配置网络协议栈
  InternetStackHelper internet;
  internet.Install (nodes);
  
  Ipv4AddressHelper ipv4;
  ipv4.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer interfaces = ipv4.Assign (devices);
  
  // ========== 修改：配置移动模型 ==========
  // 选择发布者、订阅者和代理节点
  uint32_t publisherId = 0;  // 网格左上角
  uint32_t subscriberId = 48; // 网格右下角
  uint32_t brokerId = 24;    // 网格中央附近
  uint32_t newBrokerId = 25; // 选择一个新的代理节点，靠近原代理以确保连接性
  
  // 创建位置分配器 - 7x7网格
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();
  
  double spacing = 50.0; // 网格节点间距
  for (uint32_t i = 0; i < 7; ++i)
    {
      for (uint32_t j = 0; j < 7; ++j)
        {
          if (i * 7 + j < 49) // 确保不超过49个网格节点
            {
              positionAlloc->Add(Vector(i * spacing, j * spacing, 0.0));
            }
        }
    }
  
  // 第50个节点放在中间位置
  double maxPos = 3 * spacing; // 放在网格中间
  positionAlloc->Add(Vector(maxPos, maxPos, 0.0));
  
  // 为所有节点配置固定位置的移动模型
  MobilityHelper staticMobility;
  staticMobility.SetPositionAllocator(positionAlloc);
  staticMobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  
  // 为除了subscriber外的所有节点应用静态移动模型
  for (uint32_t i = 0; i < numNodes; i++)
    {
      if (i != subscriberId) // 不是订阅者节点
        {
          staticMobility.Install(nodes.Get(i));
        }
    }
  
  // 为subscriber节点配置随机游走移动模型
  MobilityHelper subscriberMobility;
  subscriberMobility.SetPositionAllocator(positionAlloc);
  
  // 设置移动模型参数
  std::ostringstream speedConfigStream;
  speedConfigStream << "ns3::UniformRandomVariable[Min=" << std::max(0.5, nodeMobility * 0.8) 
                   << "|Max=" << (nodeMobility * 1.2) << "]";
  StringValue speedConfig = StringValue(speedConfigStream.str());
  
  // 为订阅者设置随机游走移动模型
  subscriberMobility.SetMobilityModel("ns3::RandomWalk2dMobilityModel",
                                     "Mode", StringValue("Time"),
                                     "Time", StringValue("1s"),
                                     "Speed", speedConfig,
                                     "Bounds", RectangleValue(Rectangle(0.0, 7*spacing, 0.0, 7*spacing)));
  
  // 仅为订阅者节点安装移动模型
  subscriberMobility.Install(nodes.Get(subscriberId));
  
  NS_LOG_INFO("移动模型配置: publisher(ID=" << publisherId << ")静止, broker(ID=" << brokerId 
             << ")静止, subscriber(ID=" << subscriberId << ")移动(速度="
             << nodeMobility << "m/s)");
  
  // 验证订阅者节点的速度设置
  Ptr<MobilityModel> subMobility = nodes.Get(subscriberId)->GetObject<MobilityModel>();
  if (subMobility)
    {
      Vector vel = subMobility->GetVelocity();
      double speed = std::sqrt(vel.x*vel.x + vel.y*vel.y + vel.z*vel.z);
      NS_LOG_INFO("订阅者节点初始速度: " << speed << " m/s");
    }
  
  // 创建NDN应用
  ApplicationContainer apps;
  
  for (uint32_t i = 0; i < numNodes; ++i)
    {
      Ptr<NdnApp> app = CreateObject<NdnApp> ();
      
      if (i == publisherId)
        {
          app->SetNodeRole (NdnApp::PUBLISHER);
          app->SetCurrentBrokerId(brokerId); // 使发布者知道初始代理
          app->StartPublishing ("/content");
          NS_LOG_INFO ("Node " << i << " is publisher");
        }
      else if (i == subscriberId)
        {
          app->SetNodeRole (NdnApp::SUBSCRIBER);
          app->SetCurrentBrokerId(brokerId); // 使订阅者知道初始代理
          app->StartSubscribing ("/content");
          NS_LOG_INFO ("Node " << i << " is subscriber");
        }
      else if (i == brokerId)
        {
          app->SetNodeRole (NdnApp::BROKER);
          app->SetCurrentBrokerId (i);
          NS_LOG_INFO ("Node " << i << " is broker");
        }
      else if (i == newBrokerId)
        {
          // 预先将目标节点配置为普通节点，但会在迁移时转变为代理
          app->SetNodeRole (NdnApp::REGULAR);
          NS_LOG_INFO ("Node " << i << " is future broker");
        }
      else
        {
          app->SetNodeRole (NdnApp::REGULAR);
        }
      
      nodes.Get (i)->AddApplication (app);
      app->SetStartTime (Seconds (1.0));
      app->SetStopTime (Seconds (simulationTime + 1.0));
      apps.Add (app);
    }
  
  // 在模拟中间触发一次代理迁移
  // 延迟触发，确保应用已经完全初始化且系统稳定
  Simulator::Schedule (Seconds (150.0), [&] {
    NS_LOG_INFO("调度的代理迁移即将开始: 从节点 " << brokerId << " 到节点 " << newBrokerId);
    
    Ptr<NdnApp> brokerApp = DynamicCast<NdnApp> (nodes.Get (brokerId)->GetApplication (0));
    if (brokerApp) {
      NS_LOG_INFO("开始执行代理迁移");
      brokerApp->TriggerBrokerMigration(newBrokerId);
      NS_LOG_INFO("代理迁移已调度");
    } else {
      NS_LOG_ERROR("无法获取代理应用程序");
    }
  });
  
  // 启用流监控器
  FlowMonitorHelper flowmon;
  Ptr<FlowMonitor> monitor = flowmon.InstallAll ();
  
  // 启用动画
  AnimationInterface anim ("ndn-interest-simulation.xml");
  
  // 运行仿真
  NS_LOG_INFO ("Running simulation for " << simulationTime << " seconds with subscriber mobility speed " << nodeMobility << " m/s");
  Simulator::Stop (Seconds (simulationTime));
  
  try
    {
      Simulator::Run ();
    }
  catch (const std::exception& e)
    {
      NS_LOG_ERROR("模拟运行时发生异常: " << e.what());
    }
  
  // 输出统计信息
  try
    {
      monitor->CheckForLostPackets ();
      Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmon.GetClassifier ());
      if (classifier == nullptr)
        {
          NS_LOG_ERROR("无法获取流分类器");
        }
      else
        {
          std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats ();
          
          NS_LOG_INFO ("Flow statistics:");
          for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin (); i != stats.end (); ++i)
            {
              Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (i->first);
              NS_LOG_INFO ("Flow " << i->first << " (" << t.sourceAddress << " -> " << t.destinationAddress << ")");
              NS_LOG_INFO ("  Tx Packets: " << i->second.txPackets);
              NS_LOG_INFO ("  Rx Packets: " << i->second.rxPackets);
              NS_LOG_INFO ("  Packet Loss: " << 100.0 * (i->second.txPackets - i->second.rxPackets) / i->second.txPackets << "%");
              
              // 防止除零错误
              double duration = i->second.timeLastRxPacket.GetSeconds() - i->second.timeFirstTxPacket.GetSeconds();
              if (duration > 0)
                {
                  NS_LOG_INFO ("  Throughput: " << i->second.rxBytes * 8.0 / duration / 1024 << " Kbps");
                }
              else
                {
                  NS_LOG_INFO ("  Throughput: N/A (duration is zero)");
                }
            }
        }
    }
  catch (const std::exception& e)
    {
      NS_LOG_ERROR("处理流统计时发生异常: " << e.what());
    }
  
  // 输出NDN特定统计信息
  try
    {
      Time avgNotificationDelay = NdnApp::GetAverageNotificationDelay ();
      Time unicastDelay = NdnApp::GetUnicastNotificationDelay ();
      Time interestDelay = NdnApp::GetInterestNotificationDelay ();
      Time broadcastDelay = NdnApp::GetBroadcastNotificationDelay ();
      double messageSuccessRate = NdnApp::GetMessageSuccessRate ();
      
      NS_LOG_INFO ("=========== NDN Interest Notification Mechanism Results ===========");
      NS_LOG_INFO ("订阅者移动速度: " << nodeMobility << " m/s, 通信范围: " << communicationRange << " m");
      NS_LOG_INFO ("  Average Notification Delay: " << avgNotificationDelay.GetSeconds() << " seconds");
      NS_LOG_INFO ("  Unicast Notification Delay: " << unicastDelay.GetSeconds() << " seconds");
      NS_LOG_INFO ("  Interest-based Notification Delay: " << interestDelay.GetSeconds() << " seconds");
      NS_LOG_INFO ("  Broadcast Notification Delay: " << broadcastDelay.GetSeconds() << " seconds");
      NS_LOG_INFO ("  Message Success Rate: " << messageSuccessRate * 100.0 << "%");
      NS_LOG_INFO ("  Out of Range Failures: " << NdnApp::s_outOfRangeFailures << " times");
      
      // 写入结果到CSV文件 - 包含移动速度信息
      std::string filename = "ndn-results-mobility-" + std::to_string(int(nodeMobility)) + ".csv";
      std::ofstream resultFile;
      resultFile.open (filename.c_str());
      resultFile << "Metric,Value\n";
      resultFile << "SubscriberMobilitySpeed," << nodeMobility << "\n";
      resultFile << "CommunicationRange," << communicationRange << "\n";
      resultFile << "AverageNotificationDelay," << avgNotificationDelay.GetSeconds() << "\n";
      resultFile << "UnicastNotificationDelay," << unicastDelay.GetSeconds() << "\n";
      resultFile << "InterestNotificationDelay," << interestDelay.GetSeconds() << "\n";
      resultFile << "BroadcastNotificationDelay," << broadcastDelay.GetSeconds() << "\n";
      resultFile << "MessageSuccessRate," << messageSuccessRate * 100.0 << "\n";
      resultFile << "OutOfRangeFailures," << NdnApp::s_outOfRangeFailures << "\n";
      resultFile.close();
      
      NS_LOG_INFO ("Results have been saved to " << filename);
    }
  catch (const std::exception& e)
    {
      NS_LOG_ERROR("处理NDN统计时发生异常: " << e.what());
    }
  
  // 清理
  Simulator::Destroy ();
  
  return 0;
}