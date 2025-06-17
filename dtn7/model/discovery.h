#ifndef DTN7_DISCOVERY_H
#define DTN7_DISCOVERY_H

#include "ns3/object.h"
#include "ns3/callback.h"
#include "ns3/timer.h"
#include "ns3/socket.h"
#include "ns3/ipv4-address.h"
#include "ns3/traced-callback.h"
#include "ns3/ptr.h"
#include "ns3/node.h"

#include <vector>
#include <string>
#include <map>
#include <mutex>

#include "endpoint.h"

namespace ns3 {

namespace dtn7 {

/**
 * \brief 节点通告消息结构
 */
struct BeaconMessage {
  std::string nodeId;      //!< 节点ID
  std::string endpoint;    //!< 端点地址
  std::string services;    //!< 支持的服务列表
  Time timestamp;          //!< 时间戳
  
  /**
   * \brief 序列化为字符串
   * \return 序列化后的字符串
   */
  std::string Serialize() const;
  
  /**
   * \brief 从字符串反序列化
   * \param data 序列化数据
   * \return 解析成功返回true
   */
  bool Deserialize(const std::string& data);
};

/**
 * \brief 节点发现回调函数类型
 */
typedef Callback<void, const std::string&, const std::string&, const std::string&> DiscoveryCallback;

/**
 * \ingroup dtn7
 * \brief 节点发现接口
 */
class DiscoveryAgent : public Object
{
public:
  /**
   * \brief 获取类型ID
   * \return 类型ID
   */
  static TypeId GetTypeId();
  
  /**
   * \brief 默认构造函数
   */
  DiscoveryAgent();
  
  /**
   * \brief 析构函数
   */
  virtual ~DiscoveryAgent();
  
  /**
   * \brief 注册节点发现回调
   * \param callback 发现节点时调用的回调函数
   */
  virtual void RegisterDiscoveryCallback(DiscoveryCallback callback) = 0;
  
  /**
   * \brief 设置节点ID
   * \param nodeId 节点ID
   */
  virtual void SetNodeId(const std::string& nodeId) = 0;
  
  /**
   * \brief 添加服务
   * \param service 服务名称
   * \param endpoint 端点地址
   */
  virtual void AddService(const std::string& service, const std::string& endpoint) = 0;
  
  /**
   * \brief 移除服务
   * \param service 服务名称
   */
  virtual void RemoveService(const std::string& service) = 0;
  
  /**
   * \brief 启动发现代理
   * \return 启动成功返回true
   */
  virtual bool Start() = 0;
  
  /**
   * \brief 停止发现代理
   * \return 停止成功返回true
   */
  virtual bool Stop() = 0;
  
  /**
   * \brief 获取状态统计
   * \return 状态统计字符串
   */
  virtual std::string GetStats() const = 0;
};

/**
 * \ingroup dtn7
 * \brief 基于IP多播的节点发现实现
 */
class IpDiscoveryAgent : public DiscoveryAgent
{
public:
  /**
   * \brief 获取类型ID
   * \return 类型ID
   */
  static TypeId GetTypeId();
  
  /**
   * \brief 默认构造函数
   */
  IpDiscoveryAgent();
  
  /**
   * \brief 带参数构造函数
   * \param node NS3节点
   * \param address 本地地址
   * \param port 本地端口
   * \param multicastAddress 组播地址
   * \param announceInterval 通告间隔
   */
  IpDiscoveryAgent(Ptr<Node> node,
                  Ipv4Address address,
                  uint16_t port,
                  Ipv4Address multicastAddress,
                  Time announceInterval);
  
  /**
   * \brief 析构函数
   */
  virtual ~IpDiscoveryAgent();
  
  // 继承自DiscoveryAgent的方法
  void RegisterDiscoveryCallback(DiscoveryCallback callback) override;
  void SetNodeId(const std::string& nodeId) override;
  void AddService(const std::string& service, const std::string& endpoint) override;
  void RemoveService(const std::string& service) override;
  bool Start() override;
  bool Stop() override;
  std::string GetStats() const override;
  
  /**
   * \brief 设置节点对象
   * \param node NS3节点
   */
  void SetNode(Ptr<Node> node);

private:
  Ptr<Node> m_node;                    //!< NS3节点
  Ipv4Address m_address;               //!< 本地地址
  uint16_t m_port;                     //!< 本地端口
  Ipv4Address m_multicastAddress;      //!< 组播地址
  Time m_announceInterval;             //!< 通告间隔
  
  bool m_running;                      //!< 运行状态
  std::string m_nodeId;                //!< 节点ID
  std::map<std::string, std::string> m_services; //!< 服务映射
  mutable std::mutex m_servicesMutex;  //!< 服务映射互斥锁
  
  DiscoveryCallback m_discoveryCallback; //!< 发现回调
  EventId m_announceEvent;               //!< 通告定时器
  Ptr<Socket> m_socket;                //!< UDP套接字
  
  uint64_t m_announcementsSent;        //!< 已发送通告数
  uint64_t m_announcementsReceived;    //!< 已接收通告数
  
  /**
   * \brief 创建套接字
   * \return 创建的套接字
   */
  Ptr<Socket> CreateSocket();
  
  /**
   * \brief 发送通告消息
   */
  void SendAnnouncement();
  
  /**
   * \brief 处理接收到的消息
   * \param socket 接收套接字
   */
  void HandleReceive(Ptr<Socket> socket);
  
  /**
   * \brief 安排下一次通告
   */
  void ScheduleNextAnnouncement();
  
  /**
   * \brief 生成服务列表字符串
   * \return 服务列表字符串
   */
  std::string GetServicesString() const;
};

} // namespace dtn7

} // namespace ns3

#endif /* DTN7_DISCOVERY_H */