#ifndef DTN7_UDP_CONVERGENCE_LAYER_H
#define DTN7_UDP_CONVERGENCE_LAYER_H

#include "convergence-layer.h"

#include "ns3/socket.h"
#include "ns3/ipv4-address.h"
#include "ns3/traced-callback.h"
#include "ns3/node.h"
#include "ns3/timer.h"

#include <queue>
#include <map>
#include <mutex>

namespace ns3 {

namespace dtn7 {

/**
 * \ingroup dtn7
 * \brief 与UDP传输相关的连接类
 */
class UdpConnection : public SimpleRefCount<UdpConnection>
{
public:
  /**
   * \brief 默认构造函数
   */
  UdpConnection();
  
  /**
   * \brief 构造函数
   * \param endpoint 端点地址
   */
  UdpConnection(const std::string& endpoint);
  
  /**
   * \brief 检查连接是否活跃
   * \return 如果活跃返回true
   */
  bool IsActive() const { return active; }
  
  /**
   * \brief 更新活跃时间
   */
  void UpdateLastSeen() { lastSeen = Simulator::Now(); }

  std::string endpoint;  //!< 端点地址
  bool active;           //!< 活跃状态
  Time lastSeen;         //!< 最后活跃时间
  std::queue<Ptr<Packet>> pendingPackets; //!< 等待发送的数据包队列
};

/**
 * \brief 正在传输中的bundle与其分片
 */
struct PendingBundle
{
  Ptr<Bundle> bundle;         //!< Bundle
  std::vector<Ptr<Packet>> fragments; //!< 分片
  uint32_t totalSize;         //!< 总大小
  uint32_t nextFragmentId;    //!< 下一个分片ID
  uint32_t receivedBytes;     //!< 已接收字节数
  Time expiryTime;            //!< 过期时间
  std::vector<uint8_t> data;  //!< 完整数据
};

/**
 * \ingroup dtn7
 * \brief UDP收敛层实现
 */
class UdpConvergenceLayer : public ConvergenceLayer
{
public:
  /**
   * \brief 获取类型ID
   * \return 类型ID
   */
  static TypeId GetTypeId ();
  
  /**
   * \brief 默认构造函数
   */
  UdpConvergenceLayer ();
  
  /**
   * \brief 带参数构造函数
   * \param node NS3节点
   * \param address 本地地址
   * \param port 本地端口
   */
  UdpConvergenceLayer (Ptr<Node> node, Ipv4Address address, uint16_t port);
  
  /**
   * \brief 析构函数
   */
  virtual ~UdpConvergenceLayer ();
  
  // 从ConvergenceReceiver继承的方法
  void RegisterBundleCallback (Callback<void, Ptr<Bundle>, NodeID> callback) override;
  bool Start () override;
  bool Stop () override;
  std::string GetEndpoint () const override;
  
  // 从ConvergenceSender继承的方法
  bool Send (Ptr<Bundle> bundle, const std::string& endpoint) override;
  bool IsEndpointReachable (const std::string& endpoint) const override;
  
  // 从ConvergenceLayer继承的方法
  std::string GetStats () const override;
  std::vector<std::string> GetActiveConnections () const override;
  bool HasActiveConnection (const std::string& endpoint) const override;
  
  /**
   * \brief 设置节点对象
   * \param node NS3节点
   */
  void SetNode (Ptr<Node> node);

private:
  static const uint32_t MAX_FRAGMENT_SIZE = 65507; //!< 最大UDP载荷大小

  Ptr<Node> m_node;                        //!< NS3节点
  Ipv4Address m_address;                   //!< 本地地址
  uint16_t m_port;                         //!< 本地端口
  bool m_running;                          //!< 运行状态
  
  Ptr<Socket> m_socket;                    //!< UDP套接字
  Callback<void, Ptr<Bundle>, NodeID> m_bundleCallback; //!< Bundle接收回调
  
  std::map<std::string, Ptr<UdpConnection>> m_connections; //!< 连接映射
  mutable std::mutex m_connectionsMutex;   //!< 连接映射互斥锁
  
  std::map<uint32_t, PendingBundle> m_pendingBundles; //!< 待接收Bundle映射
  uint32_t m_nextBundleId;                 //!< 下一个Bundle ID
  mutable std::mutex m_pendingBundlesMutex; //!< 待接收Bundle映射互斥锁
  
  Time m_cleanupInterval;                  //!< 清理间隔
  EventId m_cleanupEvent;                  //!< 清理事件
  
  uint32_t m_sentBundles;                  //!< 已发送Bundle数
  uint32_t m_receivedBundles;              //!< 已接收Bundle数
  uint32_t m_failedSends;                  //!< 发送失败数
  
  TracedCallback<Ptr<Bundle>, std::string> m_sentTrace; //!< 发送跟踪
  TracedCallback<Ptr<Bundle>, std::string> m_receivedTrace; //!< 接收跟踪
  
  /**
   * \brief 创建UDP套接字
   * \return 创建的套接字
   */
  Ptr<Socket> CreateSocket();
  
  /**
   * \brief 处理接收到的数据
   * \param socket 接收套接字
   */
  void HandleReceive(Ptr<Socket> socket);
  
  /**
   * \brief 解析端点字符串
   * \param endpoint 端点字符串
   * \param address 解析出的地址
   * \param port 解析出的端口
   * \return 解析成功返回true
   */
  bool ParseEndpoint(const std::string& endpoint, Ipv4Address& address, uint16_t& port) const;
  
  /**
   * \brief 格式化端点字符串
   * \param address 地址
   * \param port 端口
   * \return 格式化的端点字符串
   */
  std::string FormatEndpoint(Ipv4Address address, uint16_t port) const;
  
  /**
   * \brief 发送Bundle
   * \param bundle Bundle对象
   * \param destAddress 目标地址
   * \param destPort 目标端口
   * \return 发送成功返回true
   */
  bool SendBundle(Ptr<Bundle> bundle, Ipv4Address destAddress, uint16_t destPort);
  
  /**
   * \brief 清理过期的连接和Bundle
   */
  void CleanupExpired();
  
  /**
   * \brief 处理接收到的分片
   * \param data 数据
   * \param size 数据大小
   * \param from 源地址
   */
  void HandleFragment(const uint8_t* data, uint32_t size, const Address& from);
};

} // namespace dtn7

} // namespace ns3

#endif /* DTN7_UDP_CONVERGENCE_LAYER_H */