#include "tcp-convergence-layer.h"
#include "ns3/log.h"
#include "ns3/socket-factory.h"
#include "ns3/tcp-socket-factory.h"
#include "ns3/inet-socket-address.h"
#include "ns3/simulator.h"
#include "ns3/ipv4-address.h"
#include "ns3/uinteger.h"
#include "ns3/boolean.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/pointer.h"
#include "ns3/object.h"
#include <sstream>
#include <mutex>
#include <vector>
#include <map>
#include <memory>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("TcpConvergenceLayer");

namespace dtn7 {

NS_OBJECT_ENSURE_REGISTERED (TcpConvergenceLayer);

TypeId 
TcpConvergenceLayer::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::dtn7::TcpConvergenceLayer")
    .SetParent<ConvergenceLayer> ()
    .SetGroupName ("Dtn7")
    .AddConstructor<TcpConvergenceLayer> ()
    .AddAttribute ("LocalAddress",
                   "Local IP address to bind to",
                   Ipv4AddressValue (Ipv4Address::GetAny ()),
                   MakeIpv4AddressAccessor (&TcpConvergenceLayer::m_address),
                   MakeIpv4AddressChecker ())
    .AddAttribute ("LocalPort",
                   "Local port to bind to",
                   UintegerValue (4556),
                   MakeUintegerAccessor (&TcpConvergenceLayer::m_port),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("PermanentConnections",
                   "Whether to keep connections open",
                   BooleanValue (false),
                   MakeBooleanAccessor (&TcpConvergenceLayer::m_permanent),
                   MakeBooleanChecker ())
    .AddTraceSource ("SentBundle",
                     "Trace source for sent bundles",
                     MakeTraceSourceAccessor (&TcpConvergenceLayer::m_sentTrace),
                     "ns3::TracedCallback::PtrBundle_String")
    .AddTraceSource ("ReceivedBundle",
                     "Trace source for received bundles",
                     MakeTraceSourceAccessor (&TcpConvergenceLayer::m_receivedTrace),
                     "ns3::TracedCallback::PtrBundle_String");
  return tid;
}

// 默认构造函数
TcpConvergenceLayer::TcpConvergenceLayer()
  : m_address(Ipv4Address::GetAny()),
    m_port(4556),
    m_permanent(false),
    m_running(false),
    m_listenerSocket(nullptr),
    m_sentBundles(0),
    m_receivedBundles(0),
    m_failedSends(0)
{
  NS_LOG_FUNCTION(this);
}

TcpConvergenceLayer::TcpConvergenceLayer(Ptr<Node> node, Ipv4Address address, uint16_t port, bool permanent)
  : m_node(node),
    m_address(address),
    m_port(port),
    m_permanent(permanent),
    m_running(false),
    m_listenerSocket(nullptr),
    m_sentBundles(0),
    m_receivedBundles(0),
    m_failedSends(0)
{
  NS_LOG_FUNCTION(this << node << address << port << permanent);
}

TcpConvergenceLayer::~TcpConvergenceLayer()
{
  NS_LOG_FUNCTION(this);
  Stop();
}

void 
TcpConvergenceLayer::RegisterBundleCallback(Callback<void, Ptr<Bundle>, NodeID> callback)
{
  NS_LOG_FUNCTION(this);
  m_bundleCallback = callback;
}
void 
TcpConvergenceLayer::SetNode(Ptr<Node> node)
{
  NS_LOG_FUNCTION(this << node);
  m_node = node;
}
bool 
TcpConvergenceLayer::Start()
{
  NS_LOG_FUNCTION(this);
  
  if (m_running)
    {
      return true;
    }
  
  if (!m_node) {
    NS_LOG_ERROR("No node set for TcpConvergenceLayer");
    return false;
  }
  
  // 创建监听socket
  m_listenerSocket = CreateListeningSocket();
  if (!m_listenerSocket)
    {
      NS_LOG_ERROR("Failed to create listener socket");
      return false;
    }
  
  m_running = true;
  NS_LOG_INFO("TCP convergence layer started on " << m_address << ":" << m_port);
  
  return true;
}

bool 
TcpConvergenceLayer::Stop()
{
  NS_LOG_FUNCTION(this);
  
  if (!m_running)
    {
      return true;
    }
  
  // 关闭监听socket
  if (m_listenerSocket)
    {
      m_listenerSocket->Close();
      m_listenerSocket = nullptr;
    }
  
  // 关闭所有连接
  std::lock_guard<std::mutex> lock(m_connectionsMutex);
  for (auto& pair : m_connections)
    {
      if (pair.second && pair.second->socket)
        {
          pair.second->socket->Close();
        }
    }
  
  m_connections.clear();
  m_running = false;
  
  NS_LOG_INFO("TCP convergence layer stopped");
  
  return true;
}

std::string 
TcpConvergenceLayer::GetEndpoint() const
{
  NS_LOG_FUNCTION(this);
  return FormatEndpoint(m_address, m_port);
}

bool 
TcpConvergenceLayer::Send(Ptr<Bundle> bundle, const std::string& endpoint)
{
  NS_LOG_FUNCTION(this << bundle << endpoint);
  
  if (!m_running)
    {
      NS_LOG_ERROR("TCP convergence layer not running");
      m_failedSends++;
      return false;
    }
  
  if (!bundle) {
    NS_LOG_ERROR("Trying to send null bundle");
    m_failedSends++;
    return false;
  }
  
  // 获取或创建到端点的连接
  Ptr<TcpConnection> conn = GetConnection(endpoint);
  if (!conn || !conn->active || !conn->socket)
    {
      NS_LOG_ERROR("Failed to connect to " << endpoint);
      m_failedSends++;
      return false;
    }
  
  // 通过连接发送bundle
  bool result = SendBundle(bundle, conn);
  
  // 如果非永久，则关闭连接
  if (!m_permanent)
    {
      CleanupConnection(endpoint);
    }
  
  if (result)
    {
      m_sentBundles++;
      m_sentTrace(bundle, endpoint);
      NS_LOG_INFO("Sent bundle to " << endpoint);
    }
  else
    {
      m_failedSends++;
      NS_LOG_ERROR("Failed to send bundle to " << endpoint);
    }
  
  return result;
}

bool 
TcpConvergenceLayer::IsEndpointReachable(const std::string& endpoint) const
{
  NS_LOG_FUNCTION(this << endpoint);
  
  if (!m_running)
    {
      return false;
    }
  
  // 检查连接是否已建立
  std::lock_guard<std::mutex> lock(m_connectionsMutex);
  auto it = m_connections.find(endpoint);
  if (it != m_connections.end() && it->second && it->second->active && it->second->socket)
    {
      return true;
    }
  
  // 解析端点以获取地址和端口
  Ipv4Address address;
  uint16_t port;
  if (!ParseEndpoint(endpoint, address, port))
    {
      return false;
    }
  
  // 在实际实现中，我们会检查地址是否可达
  // 现在，假设如果它是有效地址则可达
  return true;
}

std::string 
TcpConvergenceLayer::GetStats() const
{
  NS_LOG_FUNCTION(this);
  
  std::stringstream ss;
  
  ss << "TcpConvergenceLayer(";
  ss << "addr=" << m_address << ":" << m_port;
  ss << ", sent=" << m_sentBundles;
  ss << ", recv=" << m_receivedBundles;
  ss << ", failed=" << m_failedSends;
  ss << ", conn=" << m_connections.size();
  ss << ", perm=" << m_permanent;
  ss << ")";
  
  return ss.str();
}

std::vector<std::string> 
TcpConvergenceLayer::GetActiveConnections() const
{
  NS_LOG_FUNCTION(this);
  
  std::lock_guard<std::mutex> lock(m_connectionsMutex);
  std::vector<std::string> result;
  
  for (const auto& pair : m_connections)
    {
      if (pair.second && pair.second->active)
        {
          result.push_back(pair.first);
        }
    }
  
  return result;
}

bool 
TcpConvergenceLayer::HasActiveConnection(const std::string& endpoint) const
{
  NS_LOG_FUNCTION(this << endpoint);
  
  std::lock_guard<std::mutex> lock(m_connectionsMutex);
  auto it = m_connections.find(endpoint);
  return it != m_connections.end() && it->second && it->second->active;
}

Ptr<Socket> 
TcpConvergenceLayer::CreateListeningSocket()
{
  NS_LOG_FUNCTION(this);
  
  if (!m_node) {
    NS_LOG_ERROR("No node set for TcpConvergenceLayer");
    return nullptr;
  }
  
  // 创建TCP socket
  Ptr<SocketFactory> socketFactory = m_node->GetObject<TcpSocketFactory>();
  if (!socketFactory)
    {
      NS_LOG_ERROR("No TCP socket factory found");
      return nullptr;
    }
  
  Ptr<Socket> socket = socketFactory->CreateSocket();
  if (!socket) {
    NS_LOG_ERROR("Failed to create socket");
    return nullptr;
  }
  
  // 绑定到本地地址和端口
  InetSocketAddress local = InetSocketAddress(m_address, m_port);
  int result = socket->Bind(local);
  if (result < 0)
    {
      NS_LOG_ERROR("Failed to bind socket to " << m_address << ":" << m_port);
      return nullptr;
    }
  
  // 开始监听连接
  socket->Listen();
  
  // 设置socket回调
  socket->SetAcceptCallback(
      MakeNullCallback<bool, Ptr<Socket>, const Address&>(),
      MakeCallback(&TcpConvergenceLayer::HandleAccept, this));
  
  return socket;
}

Ptr<Socket> 
TcpConvergenceLayer::CreateConnectionSocket(Ipv4Address address, uint16_t port)
{
  NS_LOG_FUNCTION(this << address << port);
  
  if (!m_node) {
    NS_LOG_ERROR("No node set for TcpConvergenceLayer");
    return nullptr;
  }
  
  // 创建TCP socket
  Ptr<SocketFactory> socketFactory = m_node->GetObject<TcpSocketFactory>();
  if (!socketFactory)
    {
      NS_LOG_ERROR("No TCP socket factory found");
      return nullptr;
    }
  
  Ptr<Socket> socket = socketFactory->CreateSocket();
  if (!socket) {
    NS_LOG_ERROR("Failed to create socket");
    return nullptr;
  }
  
  // 设置socket回调
  socket->SetConnectCallback(
      MakeCallback(&TcpConvergenceLayer::HandleConnect, this),
      MakeCallback(&TcpConvergenceLayer::HandleClose, this));
  socket->SetRecvCallback(
      MakeCallback(&TcpConvergenceLayer::HandleRecv, this));
  socket->SetCloseCallbacks(
      MakeCallback(&TcpConvergenceLayer::HandleClose, this),
      MakeCallback(&TcpConvergenceLayer::HandleClose, this));
  
  // 连接到远程地址
  InetSocketAddress remote = InetSocketAddress(address, port);
  int result = socket->Connect(remote);
  if (result < 0)
    {
      NS_LOG_ERROR("Failed to connect to " << address << ":" << port);
      return nullptr;
    }
  
  return socket;
}

bool 
TcpConvergenceLayer::ParseEndpoint(const std::string& endpoint, Ipv4Address& address, uint16_t& port) const
{
  NS_LOG_FUNCTION(this << endpoint);
  
  // 端点格式：ip:port
  size_t pos = endpoint.find(':');
  if (pos == std::string::npos)
    {
      NS_LOG_ERROR("Invalid endpoint format: " << endpoint);
      return false;
    }
  
  std::string ip = endpoint.substr(0, pos);
  std::string portStr = endpoint.substr(pos + 1);
  
  // 解析IP地址
  if (ip.empty()) {
    NS_LOG_ERROR("Empty IP address in endpoint: " << endpoint);
    return false;
  }
  
  address.Set(ip.c_str());
  
  // 解析端口
  try
    {
      port = std::stoi(portStr);
    }
  catch (const std::exception& e)
    {
      NS_LOG_ERROR("Invalid port: " << portStr);
      return false;
    }
  
  return true;
}

std::string 
TcpConvergenceLayer::FormatEndpoint(Ipv4Address address, uint16_t port) const
{
  NS_LOG_FUNCTION(this << address << port);
  std::stringstream ss;
  ss << address << ":" << port;
  return ss.str();
}

void 
TcpConvergenceLayer::HandleAccept(Ptr<Socket> socket, const Address& from)
{
  NS_LOG_FUNCTION(this << socket << from);
  
  if (!socket) {
    NS_LOG_ERROR("Accepted null socket");
    return;
  }
  
  // 从远程地址提取地址和端口
  InetSocketAddress inetFrom = InetSocketAddress::ConvertFrom(from);
  Ipv4Address remoteAddress = inetFrom.GetIpv4();
  uint16_t remotePort = inetFrom.GetPort();
  std::string endpoint = FormatEndpoint(remoteAddress, remotePort);
  
  NS_LOG_INFO("Accepted connection from " << endpoint);
  
  // NS-3 Socket 没有 Accept 方法，接受的连接通过回调直接传入
  // 因此这里不需要调用 Accept，直接使用传入的 socket
  Ptr<Socket> connectionSocket = socket;
  
  // 设置socket回调
  connectionSocket->SetRecvCallback(
      MakeCallback(&TcpConvergenceLayer::HandleRecv, this));
  connectionSocket->SetCloseCallbacks(
      MakeCallback(&TcpConvergenceLayer::HandleClose, this),
      MakeCallback(&TcpConvergenceLayer::HandleClose, this));
  
  // 存储连接
  std::lock_guard<std::mutex> lock(m_connectionsMutex);
  Ptr<TcpConnection> conn = Create<TcpConnection>(connectionSocket, endpoint);
  if (conn) {
    m_connections[endpoint] = conn;
  } else {
    NS_LOG_ERROR("Failed to create TcpConnection");
  }
}

void 
TcpConvergenceLayer::HandleConnect(Ptr<Socket> socket)
{
  NS_LOG_FUNCTION(this << socket);
  
  if (!socket) {
    NS_LOG_ERROR("Connected with null socket");
    return;
  }
  
  // 连接已建立
  Address peerAddress;
  if (socket->GetPeerName(peerAddress) && InetSocketAddress::IsMatchingType(peerAddress))
    {
      InetSocketAddress inetAddr = InetSocketAddress::ConvertFrom(peerAddress);
      NS_LOG_INFO("Connected to " << inetAddr.GetIpv4() << ":" << inetAddr.GetPort());
    }
  else
    {
      NS_LOG_INFO("Connected to non-IP address");
    }
}

void 
TcpConvergenceLayer::HandleClose(Ptr<Socket> socket)
{
  NS_LOG_FUNCTION(this << socket);
  
  if (!socket) {
    NS_LOG_ERROR("Closed null socket");
    return;
  }
  
  // 找到此socket的连接
  std::string endpoint;
  {
    std::lock_guard<std::mutex> lock(m_connectionsMutex);
    for (const auto& pair : m_connections)
      {
        if (pair.second && pair.second->socket == socket)
          {
            endpoint = pair.first;
            break;
          }
      }
  }
  
  if (!endpoint.empty())
    {
      NS_LOG_INFO("Connection closed to " << endpoint);
      CleanupConnection(endpoint);
    }
}

void 
TcpConvergenceLayer::HandleRecv(Ptr<Socket> socket)
{
  NS_LOG_FUNCTION(this << socket);
  
  if (!socket) {
    NS_LOG_ERROR("Received data on null socket");
    return;
  }
  
  // 找到此socket的连接
  std::string endpoint;
  {
    std::lock_guard<std::mutex> lock(m_connectionsMutex);
    for (const auto& pair : m_connections)
      {
        if (pair.second && pair.second->socket == socket)
          {
            endpoint = pair.first;
            break;
          }
      }
  }
  
  if (endpoint.empty())
    {
      NS_LOG_ERROR("Received data on unknown socket");
      return;
    }
  
  // 接收bundle
  Ptr<Bundle> bundle = ReceiveBundle(socket);
  if (!bundle)
    {
      NS_LOG_ERROR("Failed to receive bundle from " << endpoint);
      return;
    }
  
  NS_LOG_INFO("Received bundle from " << endpoint);
  m_receivedBundles++;
  m_receivedTrace(bundle, endpoint);
  
  // 解析源节点从端点
  Ipv4Address address;
  uint16_t port;
  if (!ParseEndpoint(endpoint, address, port))
    {
      NS_LOG_ERROR("Invalid endpoint format: " << endpoint);
      return;
    }
  
  // 通知bundle回调
  if (!m_bundleCallback.IsNull())
    {
      // 使用主要区块中的源节点EID
      NodeID source = bundle->GetPrimaryBlock().GetSourceNodeEID();
      try {
        m_bundleCallback(bundle, source);
      } catch (const std::exception& e) {
        NS_LOG_ERROR("Exception in bundle callback: " << e.what());
      }
    }
}

void 
TcpConvergenceLayer::CleanupConnection(const std::string& endpoint)
{
  NS_LOG_FUNCTION(this << endpoint);
  
  std::lock_guard<std::mutex> lock(m_connectionsMutex);
  auto it = m_connections.find(endpoint);
  if (it != m_connections.end() && it->second)
    {
      if (it->second->socket)
        {
          it->second->socket->Close();
        }
      m_connections.erase(it);
      NS_LOG_INFO("Cleaned up connection to " << endpoint);
    }
}

Ptr<TcpConnection> 
TcpConvergenceLayer::GetConnection(const std::string& endpoint)
{
  NS_LOG_FUNCTION(this << endpoint);
  
  // 检查连接是否已存在
  {
    std::lock_guard<std::mutex> lock(m_connectionsMutex);
    auto it = m_connections.find(endpoint);
    if (it != m_connections.end() && it->second && it->second->active && it->second->socket)
      {
        return it->second;
      }
  }
  
  // 解析端点以获取地址和端口
  Ipv4Address address;
  uint16_t port;
  if (!ParseEndpoint(endpoint, address, port))
    {
      NS_LOG_ERROR("Invalid endpoint format: " << endpoint);
      return nullptr;
    }
  
  // 创建新连接socket
  Ptr<Socket> socket = CreateConnectionSocket(address, port);
  if (!socket)
    {
      NS_LOG_ERROR("Failed to create connection socket to " << endpoint);
      return nullptr;
    }
  
  // 存储连接
  Ptr<TcpConnection> conn = Create<TcpConnection>(socket, endpoint);
  if (!conn) {
    NS_LOG_ERROR("Failed to create TcpConnection object");
    return nullptr;
  }
  
  {
    std::lock_guard<std::mutex> lock(m_connectionsMutex);
    m_connections[endpoint] = conn;
  }
  
  NS_LOG_INFO("Created connection to " << endpoint);
  
  return conn;
}

bool 
TcpConvergenceLayer::SendBundle(Ptr<Bundle> bundle, Ptr<TcpConnection> conn)
{
  NS_LOG_FUNCTION(this << bundle << conn);
  
  if (!bundle) {
    NS_LOG_ERROR("Trying to send null bundle");
    return false;
  }
  
  if (!conn || !conn->active || !conn->socket)
    {
      NS_LOG_ERROR("Invalid connection");
      return false;
    }
  
  // 将bundle序列化为CBOR
  Buffer buffer = bundle->ToCbor();
  uint32_t size = buffer.GetSize();
  
  // 先发送bundle大小(4字节，网络字节序)
  uint8_t sizeBytes[4];
  sizeBytes[0] = (size >> 24) & 0xFF;
  sizeBytes[1] = (size >> 16) & 0xFF;
  sizeBytes[2] = (size >> 8) & 0xFF;
  sizeBytes[3] = size & 0xFF;
  
  int sent = conn->socket->Send(sizeBytes, 4, 0);
  if (sent != 4)
    {
      NS_LOG_ERROR("Failed to send bundle size: " << sent);
      return false;
    }
  
  // 为缓冲区数据创建向量
  std::vector<uint8_t> data(size);
  buffer.CopyData(data.data(), size);
  
  // 发送bundle数据
  sent = conn->socket->Send(data.data(), size, 0);
  if (sent != static_cast<int>(size))
    {
      NS_LOG_ERROR("Failed to send bundle data: " << sent << "/" << size);
      return false;
    }
  
  return true;
}

Ptr<Bundle> 
TcpConvergenceLayer::ReceiveBundle(Ptr<Socket> socket)
{
  NS_LOG_FUNCTION(this << socket);
  
  if (!socket) {
    NS_LOG_ERROR("Receiving from null socket");
    return nullptr;
  }
  
  // 先接收bundle大小(4字节，网络字节序)
  uint8_t sizeBytes[4];
  int received = socket->Recv(sizeBytes, 4, 0);
  if (received != 4)
    {
      NS_LOG_ERROR("Failed to receive bundle size: " << received);
      return nullptr;
    }
  
  uint32_t size = (sizeBytes[0] << 24) | (sizeBytes[1] << 16) | (sizeBytes[2] << 8) | sizeBytes[3];
  
  // 为bundle数据分配向量
  std::vector<uint8_t> data(size);
  
  // 接收bundle数据
  received = socket->Recv(data.data(), size, 0);
  if (received != static_cast<int>(size))
    {
      NS_LOG_ERROR("Failed to receive bundle data: " << received << "/" << size);
      return nullptr;
    }
  
  // 从接收的数据创建缓冲区
  Buffer buffer;
  buffer.AddAtStart(size);
  buffer.Begin().Write(data.data(), size);
  
  // 从CBOR反序列化bundle
  auto bundleOpt = Bundle::FromCbor(buffer);
  if (!bundleOpt)
    {
      NS_LOG_ERROR("Failed to deserialize bundle");
      return nullptr;
    }
  
  Ptr<Bundle> bundle = Create<Bundle>(*bundleOpt);
  if (!bundle) {
    NS_LOG_ERROR("Failed to create Bundle object");
    return nullptr;
  }
  
  return bundle;
}

} // namespace dtn7

} // namespace ns3