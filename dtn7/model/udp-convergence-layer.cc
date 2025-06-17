#include "udp-convergence-layer.h"
#include "ns3/log.h"
#include "ns3/socket-factory.h"
#include "ns3/udp-socket-factory.h"
#include "ns3/inet-socket-address.h"
#include "ns3/simulator.h"
#include "ns3/ipv4-address.h"
#include "ns3/uinteger.h"
#include "ns3/boolean.h"
#include "ns3/trace-source-accessor.h"
#include <sstream>
#include <algorithm>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("UdpConvergenceLayer");

namespace dtn7 {

// UDP连接实现

UdpConnection::UdpConnection()
  : active(true),
    lastSeen(Simulator::Now())
{
}

UdpConnection::UdpConnection(const std::string& endpoint)
  : endpoint(endpoint),
    active(true),
    lastSeen(Simulator::Now())
{
}

// UDP收敛层实现

NS_OBJECT_ENSURE_REGISTERED (UdpConvergenceLayer);

TypeId 
UdpConvergenceLayer::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::dtn7::UdpConvergenceLayer")
    .SetParent<ConvergenceLayer> ()
    .SetGroupName ("Dtn7")
    .AddConstructor<UdpConvergenceLayer> ()
    .AddAttribute ("LocalAddress",
                   "本地IP地址",
                   Ipv4AddressValue (Ipv4Address::GetAny ()),
                   MakeIpv4AddressAccessor (&UdpConvergenceLayer::m_address),
                   MakeIpv4AddressChecker ())
    .AddAttribute ("LocalPort",
                   "本地端口",
                   UintegerValue (4557),
                   MakeUintegerAccessor (&UdpConvergenceLayer::m_port),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("CleanupInterval",
                   "清理间隔",
                   TimeValue (Minutes (1)),
                   MakeTimeAccessor (&UdpConvergenceLayer::m_cleanupInterval),
                   MakeTimeChecker ())
    .AddTraceSource ("SentBundle",
                     "发送Bundle跟踪源",
                     MakeTraceSourceAccessor (&UdpConvergenceLayer::m_sentTrace),
                     "ns3::TracedCallback::PtrBundle_String")
    .AddTraceSource ("ReceivedBundle",
                     "接收Bundle跟踪源",
                     MakeTraceSourceAccessor (&UdpConvergenceLayer::m_receivedTrace),
                     "ns3::TracedCallback::PtrBundle_String");
  return tid;
}

UdpConvergenceLayer::UdpConvergenceLayer()
  : m_address(Ipv4Address::GetAny()),
    m_port(4557),
    m_running(false),
    m_nextBundleId(1),
    m_cleanupInterval(Minutes(1)),
    m_sentBundles(0),
    m_receivedBundles(0),
    m_failedSends(0)
{
  NS_LOG_FUNCTION (this);
}

UdpConvergenceLayer::UdpConvergenceLayer(Ptr<Node> node, Ipv4Address address, uint16_t port)
  : m_node(node),
    m_address(address),
    m_port(port),
    m_running(false),
    m_nextBundleId(1),
    m_cleanupInterval(Minutes(1)),
    m_sentBundles(0),
    m_receivedBundles(0),
    m_failedSends(0)
{
  NS_LOG_FUNCTION (this << node << address << port);
}

UdpConvergenceLayer::~UdpConvergenceLayer()
{
  NS_LOG_FUNCTION (this);
  Stop();
}

void 
UdpConvergenceLayer::RegisterBundleCallback(Callback<void, Ptr<Bundle>, NodeID> callback)
{
  NS_LOG_FUNCTION (this);
  m_bundleCallback = callback;
}

bool 
UdpConvergenceLayer::Start()
{
  NS_LOG_FUNCTION (this);
  
  if (m_running)
    {
      return true;
    }
  
  if (!m_node)
    {
      NS_LOG_ERROR ("未设置Node");
      return false;
    }
  
  // 创建UDP套接字
  m_socket = CreateSocket();
  if (!m_socket)
    {
      NS_LOG_ERROR ("无法创建UDP套接字");
      return false;
    }
  
  // 设置接收回调
  m_socket->SetRecvCallback(MakeCallback(&UdpConvergenceLayer::HandleReceive, this));
  
  // 安排清理任务
  m_cleanupEvent = Simulator::Schedule(m_cleanupInterval, 
                                     &UdpConvergenceLayer::CleanupExpired, 
                                     this);
  
  m_running = true;
  NS_LOG_INFO ("UDP收敛层已启动: " << m_address << ":" << m_port);
  
  return true;
}

bool 
UdpConvergenceLayer::Stop()
{
  NS_LOG_FUNCTION (this);
  
  if (!m_running)
    {
      return true;
    }
  
  // 取消清理事件
  if (m_cleanupEvent.IsPending())
    {
      Simulator::Cancel(m_cleanupEvent);
    }
  
  // 关闭套接字
  if (m_socket)
    {
      m_socket->Close();
      m_socket = nullptr;
    }
  
  m_running = false;
  NS_LOG_INFO ("UDP收敛层已停止");
  
  return true;
}

std::string 
UdpConvergenceLayer::GetEndpoint() const
{
  NS_LOG_FUNCTION (this);
  return FormatEndpoint(m_address, m_port);
}

bool 
UdpConvergenceLayer::Send(Ptr<Bundle> bundle, const std::string& endpoint)
{
  NS_LOG_FUNCTION (this << bundle << endpoint);
  
  if (!m_running)
    {
      NS_LOG_ERROR ("UDP收敛层未运行");
      m_failedSends++;
      return false;
    }
  
  // 解析端点地址
  Ipv4Address destAddress;
  uint16_t destPort;
  if (!ParseEndpoint(endpoint, destAddress, destPort))
    {
      NS_LOG_ERROR ("无效的端点格式: " << endpoint);
      m_failedSends++;
      return false;
    }
  
  // 发送Bundle
  bool success = SendBundle(bundle, destAddress, destPort);
  
  if (success)
    {
      m_sentBundles++;
      m_sentTrace(bundle, endpoint);
      NS_LOG_INFO ("已发送Bundle到 " << endpoint);
      
      // 更新连接状态
      std::lock_guard<std::mutex> lock(m_connectionsMutex);
      auto it = m_connections.find(endpoint);
      if (it == m_connections.end())
        {
          m_connections[endpoint] = Create<UdpConnection>(endpoint);
        }
      else
        {
          it->second->UpdateLastSeen();
        }
    }
  else
    {
      m_failedSends++;
      NS_LOG_ERROR ("无法发送Bundle到 " << endpoint);
    }
  
  return success;
}

bool 
UdpConvergenceLayer::IsEndpointReachable(const std::string& endpoint) const
{
  NS_LOG_FUNCTION (this << endpoint);
  
  // 在UDP中，我们不能确定端点是否可达，只能基于之前的通信
  std::lock_guard<std::mutex> lock(m_connectionsMutex);
  auto it = m_connections.find(endpoint);
  return it != m_connections.end() && it->second->IsActive();
}

std::string 
UdpConvergenceLayer::GetStats() const
{
  NS_LOG_FUNCTION (this);
  
  std::stringstream ss;
  
  ss << "UdpConvergenceLayer(";
  ss << "addr=" << m_address << ":" << m_port;
  ss << ", sent=" << m_sentBundles;
  ss << ", recv=" << m_receivedBundles;
  ss << ", failed=" << m_failedSends;
  ss << ", conn=" << m_connections.size();
  ss << ", pending=" << m_pendingBundles.size();
  ss << ")";
  
  return ss.str();
}

std::vector<std::string> 
UdpConvergenceLayer::GetActiveConnections() const
{
  NS_LOG_FUNCTION (this);
  
  std::vector<std::string> result;
  
  std::lock_guard<std::mutex> lock(m_connectionsMutex);
  for (const auto& pair : m_connections)
    {
      if (pair.second->IsActive())
        {
          result.push_back(pair.first);
        }
    }
  
  return result;
}

bool 
UdpConvergenceLayer::HasActiveConnection(const std::string& endpoint) const
{
  NS_LOG_FUNCTION (this << endpoint);
  
  std::lock_guard<std::mutex> lock(m_connectionsMutex);
  auto it = m_connections.find(endpoint);
  return it != m_connections.end() && it->second->IsActive();
}

void 
UdpConvergenceLayer::SetNode(Ptr<Node> node)
{
  NS_LOG_FUNCTION (this << node);
  m_node = node;
}

Ptr<Socket> 
UdpConvergenceLayer::CreateSocket()
{
  NS_LOG_FUNCTION (this);
  
  if (!m_node)
    {
      NS_LOG_ERROR ("未设置Node");
      return nullptr;
    }
  
  // 创建UDP套接字
  Ptr<SocketFactory> socketFactory = m_node->GetObject<UdpSocketFactory>();
  if (!socketFactory)
    {
      NS_LOG_ERROR ("未找到UDP套接字工厂");
      return nullptr;
    }
  
  Ptr<Socket> socket = socketFactory->CreateSocket();
  if (!socket)
    {
      NS_LOG_ERROR ("无法创建套接字");
      return nullptr;
    }
  
  // 绑定到本地地址和端口
  if (socket->Bind(InetSocketAddress(m_address, m_port)) != 0)
    {
      NS_LOG_ERROR ("无法绑定套接字到 " << m_address << ":" << m_port);
      return nullptr;
    }
  
  return socket;
}

void 
UdpConvergenceLayer::HandleReceive(Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  
  Address from;
  Ptr<Packet> packet;
  
  while ((packet = socket->RecvFrom(from)))
    {
      // 确保从地址是IP地址
      if (!InetSocketAddress::IsMatchingType(from))
        {
          NS_LOG_WARN ("收到非IP地址的消息");
          continue;
        }
      
      InetSocketAddress address = InetSocketAddress::ConvertFrom(from);
      NS_LOG_INFO ("收到数据包从 " << address.GetIpv4() << ":" << address.GetPort() 
                   << " (" << packet->GetSize() << " bytes)");
      
      // 读取数据
      uint32_t size = packet->GetSize();
      uint8_t *data = new uint8_t[size];
      packet->CopyData(data, size);
      
      // 处理分片
      HandleFragment(data, size, from);
      
      delete[] data;
    }
}

bool 
UdpConvergenceLayer::ParseEndpoint(const std::string& endpoint, Ipv4Address& address, uint16_t& port) const
{
  NS_LOG_FUNCTION (this << endpoint);
  
  // 端点格式：ip:port
  size_t pos = endpoint.find(':');
  if (pos == std::string::npos)
    {
      NS_LOG_ERROR ("无效的端点格式: " << endpoint);
      return false;
    }
  
  std::string ip = endpoint.substr(0, pos);
  std::string portStr = endpoint.substr(pos + 1);
  
  // 解析IP地址
  address.Set(ip.c_str());
  
  // 解析端口
  try
    {
      port = std::stoi(portStr);
    }
  catch (const std::exception& e)
    {
      NS_LOG_ERROR ("无效的端口: " << portStr);
      return false;
    }
  
  return true;
}

std::string 
UdpConvergenceLayer::FormatEndpoint(Ipv4Address address, uint16_t port) const
{
  NS_LOG_FUNCTION (this << address << port);
  std::stringstream ss;
  ss << address << ":" << port;
  return ss.str();
}

bool 
UdpConvergenceLayer::SendBundle(Ptr<Bundle> bundle, Ipv4Address destAddress, uint16_t destPort)
{
  NS_LOG_FUNCTION (this << bundle << destAddress << destPort);
  
  if (!m_socket)
    {
      NS_LOG_ERROR ("套接字未初始化");
      return false;
    }
  
  // 序列化Bundle为CBOR
  Buffer buffer = bundle->ToCbor();
  uint32_t totalSize = buffer.GetSize();
  
  // 如果大小超过最大UDP载荷，需要分片
  if (totalSize > MAX_FRAGMENT_SIZE)
    {
      NS_LOG_INFO ("Bundle大小超过UDP最大载荷，需要分片");
      
      // 分配Bundle ID
      uint32_t bundleId;
      {
        std::lock_guard<std::mutex> lock(m_pendingBundlesMutex);
        bundleId = m_nextBundleId++;
      }
      
      // 准备数据
      std::vector<uint8_t> data(totalSize);
      buffer.CopyData(data.data(), totalSize);
      
      // 计算分片数
      uint32_t numFragments = (totalSize + MAX_FRAGMENT_SIZE - 1) / MAX_FRAGMENT_SIZE;
      NS_LOG_INFO ("将Bundle分为 " << numFragments << " 个分片");
      
      // 为每个分片创建数据包
      for (uint32_t i = 0; i < numFragments; i++)
        {
          // 计算分片偏移量和大小
          uint32_t offset = i * MAX_FRAGMENT_SIZE;
          uint32_t fragmentSize = std::min(MAX_FRAGMENT_SIZE, totalSize - offset);
          
          // 创建分片头 (8 bytes)
          // | 0x1B (1 byte) | bundleId (4 bytes) | fragmentId (2 bytes) | numFragments (1 byte) |
          uint8_t header[8];
          header[0] = 0x1B; // 分片标记
          header[1] = (bundleId >> 24) & 0xFF;
          header[2] = (bundleId >> 16) & 0xFF;
          header[3] = (bundleId >> 8) & 0xFF;
          header[4] = bundleId & 0xFF;
          header[5] = (i >> 8) & 0xFF;
          header[6] = i & 0xFF;
          header[7] = numFragments;
          
          // 创建分片数据包
          Ptr<Packet> fragment = Create<Packet>(header, 8);
          fragment->AddAtEnd(Create<Packet>(data.data() + offset, fragmentSize));
          
          // 发送分片
          InetSocketAddress dest(destAddress, destPort);
          int sent = m_socket->SendTo(fragment, 0, dest);
          
          if (sent != static_cast<int>(fragment->GetSize()))
            {
              NS_LOG_ERROR ("分片发送失败: " << sent << "/" << fragment->GetSize());
              return false;
            }
        }
    }
  else
    {
      // 不需要分片，直接发送
      // 准备数据
      std::vector<uint8_t> data(totalSize);
      buffer.CopyData(data.data(), totalSize);
      
      // 创建数据包，添加0xBB标记表示完整Bundle
      uint8_t header = 0xBB;
      Ptr<Packet> packet = Create<Packet>(&header, 1);
      packet->AddAtEnd(Create<Packet>(data.data(), totalSize));
      
      // 发送Bundle
      InetSocketAddress dest(destAddress, destPort);
      int sent = m_socket->SendTo(packet, 0, dest);
      
      if (sent != static_cast<int>(packet->GetSize()))
        {
          NS_LOG_ERROR ("Bundle发送失败: " << sent << "/" << packet->GetSize());
          return false;
        }
    }
  
  return true;
}

void 
UdpConvergenceLayer::CleanupExpired()
{
  NS_LOG_FUNCTION (this);
  
  Time now = Simulator::Now();
  Time connectionTimeout = Seconds(60);
  
  // 清理过期连接
  {
    std::lock_guard<std::mutex> lock(m_connectionsMutex);
    
    for (auto it = m_connections.begin(); it != m_connections.end();)
      {
        if (now - it->second->lastSeen > connectionTimeout)
          {
            NS_LOG_INFO ("清理过期连接: " << it->first);
            it = m_connections.erase(it);
          }
        else
          {
            ++it;
          }
      }
  }
  
  // 清理过期的待接收Bundle
  {
    std::lock_guard<std::mutex> lock(m_pendingBundlesMutex);
    
    for (auto it = m_pendingBundles.begin(); it != m_pendingBundles.end();)
      {
        if (now > it->second.expiryTime)
          {
            NS_LOG_INFO ("清理过期待接收Bundle: " << it->first);
            it = m_pendingBundles.erase(it);
          }
        else
          {
            ++it;
          }
      }
  }
  
  // 安排下一次清理
  m_cleanupEvent = Simulator::Schedule(m_cleanupInterval, 
                                     &UdpConvergenceLayer::CleanupExpired, 
                                     this);
}

void 
UdpConvergenceLayer::HandleFragment(const uint8_t* data, uint32_t size, const Address& from)
{
  NS_LOG_FUNCTION (this << size);
  
  if (size < 1)
    {
      NS_LOG_WARN ("收到的数据太小，无法处理");
      return;
    }
  
  // 提取源地址信息
  InetSocketAddress address = InetSocketAddress::ConvertFrom(from);
  std::string endpoint = FormatEndpoint(address.GetIpv4(), address.GetPort());
  
  // 更新连接状态
  {
    std::lock_guard<std::mutex> lock(m_connectionsMutex);
    auto it = m_connections.find(endpoint);
    if (it == m_connections.end())
      {
        m_connections[endpoint] = Create<UdpConnection>(endpoint);
      }
    else
      {
        it->second->UpdateLastSeen();
      }
  }
  
  // 检查数据类型
  uint8_t type = data[0];
  
  if (type == 0xBB)
    {
      // 完整Bundle
      NS_LOG_INFO ("收到完整Bundle");
      
      // 跳过标记字节
      const uint8_t* bundleData = data + 1;
      uint32_t bundleSize = size - 1;
      
      // 创建Buffer
      Buffer buffer;
      buffer.AddAtStart(bundleSize);
      buffer.Begin().Write(bundleData, bundleSize);
      
      // 反序列化Bundle
      auto bundleOpt = Bundle::FromCbor(buffer);
      if (!bundleOpt)
        {
          NS_LOG_ERROR ("无法反序列化Bundle");
          return;
        }
      
      // 创建Bundle对象
      Ptr<Bundle> bundle = Create<Bundle>(*bundleOpt);
      
      m_receivedBundles++;
      m_receivedTrace(bundle, endpoint);
      
      // 通知Bundle回调
      if (!m_bundleCallback.IsNull())
        {
          // 使用主要区块中的源节点EID
          NodeID source = bundle->GetPrimaryBlock().GetSourceNodeEID();
          m_bundleCallback(bundle, source);
        }
    }
  else if (type == 0x1B && size >= 8)
    {
      // 分片Bundle
      // 解析分片头
      uint32_t bundleId = (data[1] << 24) | (data[2] << 16) | (data[3] << 8) | data[4];
      uint16_t fragmentId = (data[5] << 8) | data[6];
      uint8_t numFragments = data[7];
      
      // 跳过头部
      const uint8_t* fragmentData = data + 8;
      uint32_t fragmentSize = size - 8;
      
      NS_LOG_INFO ("收到Bundle分片: " << bundleId << ", fragmentId=" << fragmentId 
                   << ", numFragments=" << static_cast<uint32_t>(numFragments));
      
      // 查找或创建待接收Bundle
      std::lock_guard<std::mutex> lock(m_pendingBundlesMutex);
      
      auto it = m_pendingBundles.find(bundleId);
      if (it == m_pendingBundles.end())
        {
          // 创建新的待接收Bundle
          PendingBundle pending;
          pending.totalSize = 0; // 暂时未知总大小
          pending.nextFragmentId = 0;
          pending.receivedBytes = 0;
          pending.expiryTime = Simulator::Now() + Seconds(60); // 1分钟超时
          
          // 预分配分片向量
          pending.fragments.resize(numFragments);
          
          m_pendingBundles[bundleId] = pending;
          it = m_pendingBundles.find(bundleId);
        }
      
      // 检查分片ID是否有效
      if (fragmentId >= it->second.fragments.size())
        {
          NS_LOG_ERROR ("无效的分片ID: " << fragmentId);
          return;
        }
      
      // 存储分片
      Ptr<Packet> fragment = Create<Packet>(fragmentData, fragmentSize);
      it->second.fragments[fragmentId] = fragment;
      it->second.receivedBytes += fragmentSize;
      
      // 检查是否收到所有分片
      bool complete = true;
      for (uint32_t i = 0; i < it->second.fragments.size(); i++)
        {
          if (!it->second.fragments[i])
            {
              complete = false;
              break;
            }
        }
      
      if (complete)
        {
          NS_LOG_INFO ("收到所有分片，重组Bundle");
          
          // 计算总大小
          uint32_t totalSize = 0;
          for (const auto& frag : it->second.fragments)
            {
              totalSize += frag->GetSize();
            }
          
          // 重组数据
          std::vector<uint8_t> bundleData(totalSize);
          uint32_t offset = 0;
          
          for (const auto& frag : it->second.fragments)
            {
              uint32_t fragSize = frag->GetSize();
              uint8_t* buffer = new uint8_t[fragSize];
              frag->CopyData(buffer, fragSize);
              
              std::copy(buffer, buffer + fragSize, bundleData.begin() + offset);
              offset += fragSize;
              
              delete[] buffer;
            }
          
          // 创建Buffer
          Buffer buffer;
          buffer.AddAtStart(totalSize);
          buffer.Begin().Write(bundleData.data(), totalSize);
          
          // 反序列化Bundle
          auto bundleOpt = Bundle::FromCbor(buffer);
          if (!bundleOpt)
            {
              NS_LOG_ERROR ("无法反序列化重组的Bundle");
              m_pendingBundles.erase(it);
              return;
            }
          
          // 创建Bundle对象
          Ptr<Bundle> bundle = Create<Bundle>(*bundleOpt);
          
          m_receivedBundles++;
          m_receivedTrace(bundle, endpoint);
          
          // 通知Bundle回调
          if (!m_bundleCallback.IsNull())
            {
              // 使用主要区块中的源节点EID
              NodeID source = bundle->GetPrimaryBlock().GetSourceNodeEID();
              m_bundleCallback(bundle, source);
            }
          
          // 移除待接收Bundle
          m_pendingBundles.erase(it);
        }
    }
  else
    {
      NS_LOG_WARN ("未知数据类型: " << static_cast<uint32_t>(type));
    }
}

} // namespace dtn7

} // namespace ns3