#ifndef DTN7_TCP_CONVERGENCE_LAYER_H
#define DTN7_TCP_CONVERGENCE_LAYER_H

#include "convergence-layer.h"
#include "bundle.h"
#include "ns3/ipv4-address.h"
#include "ns3/socket.h"
#include "ns3/callback.h"
#include "ns3/node.h"
#include "ns3/traced-callback.h"

#include <string>
#include <mutex>
#include <map>
#include <vector>

namespace ns3 {

namespace dtn7 {

// 定义TcpConnection类
class TcpConnection : public SimpleRefCount<TcpConnection>
{
public:
  Ptr<Socket> socket;
  std::string endpoint;
  bool active;

  TcpConnection(Ptr<Socket> s, const std::string& ep)
    : socket(s), endpoint(ep), active(true) {}
};

/**
 * \brief TCP Convergence Layer implementation
 */
class TcpConvergenceLayer : public ConvergenceLayer
{
public:
  static TypeId GetTypeId (void);

  /**
   * \brief Default constructor (needed for TypeId system)
   */
  TcpConvergenceLayer ();

  /**
   * \brief Constructor with parameters
   */
  TcpConvergenceLayer (Ptr<Node> node, 
                        Ipv4Address address = Ipv4Address::GetAny(), 
                        uint16_t port = 4556, 
                        bool permanent = false);

  /**
   * \brief Destructor
   */
  virtual ~TcpConvergenceLayer ();

  /**
   * \brief Set the node this convergence layer is attached to
   * \param node The node
   */
  void SetNode (Ptr<Node> node);

  // 从ConvergenceReceiver继承
  void RegisterBundleCallback (Callback<void, Ptr<Bundle>, NodeID> callback) override;
  bool Start () override;
  bool Stop () override;
  std::string GetEndpoint () const override;

  // 从ConvergenceSender继承
  bool Send (Ptr<Bundle> bundle, const std::string& endpoint) override;
  bool IsEndpointReachable (const std::string& endpoint) const override;

  // 从ConvergenceLayer继承
  std::string GetStats () const override;
  std::vector<std::string> GetActiveConnections () const override;
  bool HasActiveConnection (const std::string& endpoint) const override;

protected:
  Ptr<Socket> CreateListeningSocket ();
  Ptr<Socket> CreateConnectionSocket (Ipv4Address address, uint16_t port);
  bool ParseEndpoint (const std::string& endpoint, Ipv4Address& address, uint16_t& port) const;
  std::string FormatEndpoint (Ipv4Address address, uint16_t port) const;
  void HandleAccept (Ptr<Socket> socket, const Address& from);
  void HandleConnect (Ptr<Socket> socket);
  void HandleClose (Ptr<Socket> socket);
  void HandleRecv (Ptr<Socket> socket);
  void CleanupConnection (const std::string& endpoint);
  Ptr<TcpConnection> GetConnection (const std::string& endpoint);
  bool SendBundle (Ptr<Bundle> bundle, Ptr<TcpConnection> conn);
  Ptr<Bundle> ReceiveBundle (Ptr<Socket> socket);

private:
  Ptr<Node> m_node;                            //!< 节点
  Ipv4Address m_address;                       //!< 本地地址
  uint16_t m_port;                             //!< 本地端口
  bool m_permanent;                            //!< 保持连接开启
  bool m_running;                              //!< 运行标志
  Ptr<Socket> m_listenerSocket;                //!< 监听套接字
  std::map<std::string, Ptr<TcpConnection>> m_connections; //!< 活跃连接
  mutable std::mutex m_connectionsMutex;       //!< 连接互斥锁
  Callback<void, Ptr<Bundle>, NodeID> m_bundleCallback; //!< Bundle回调
  uint32_t m_sentBundles;                      //!< 已发送Bundle计数器
  uint32_t m_receivedBundles;                  //!< 已接收Bundle计数器
  uint32_t m_failedSends;                      //!< 发送失败计数器
  TracedCallback<Ptr<Bundle>, std::string> m_sentTrace;        //!< 发送Bundle追踪源
  TracedCallback<Ptr<Bundle>, std::string> m_receivedTrace;    //!< 接收Bundle追踪源
};

} // namespace dtn7

} // namespace ns3

#endif /* DTN7_TCP_CONVERGENCE_LAYER_H */