#include "discovery.h"
#include "ns3/log.h"
#include "ns3/socket-factory.h"
#include "ns3/udp-socket-factory.h"
#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/simulator.h"
#include "ns3/nstime.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/string.h"
#include "ns3/ipv4.h"
#include "ns3/ipv4-address.h"
#include "ns3/ipv4-interface-address.h"
// 添加缺少的头文件
#include "ns3/uinteger.h"
#include "ns3/boolean.h"
#include "ns3/pointer.h"
#include "ns3/double.h"
#include "ns3/net-device.h"
#include "ns3/event-id.h"

#include <sstream>
#include <algorithm>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("DiscoveryAgent");

namespace dtn7 {

// BeaconMessage实现

std::string 
BeaconMessage::Serialize() const
{
  std::stringstream ss;
  ss << "DTN7|" << nodeId << "|" << endpoint << "|" << services << "|" 
     << timestamp.GetSeconds();
  return ss.str();
}

bool 
BeaconMessage::Deserialize(const std::string& data)
{
  std::stringstream ss(data);
  std::string prefix;
  
  // 检查协议前缀
  std::getline(ss, prefix, '|');
  if (prefix != "DTN7")
    {
      return false;
    }
  
  // 解析各字段
  std::getline(ss, nodeId, '|');
  std::getline(ss, endpoint, '|');
  std::getline(ss, services, '|');
  
  // 解析时间戳
  double timestampSec;
  ss >> timestampSec;
  timestamp = Seconds(timestampSec);
  
  return true;
}

// DiscoveryAgent实现

NS_OBJECT_ENSURE_REGISTERED (DiscoveryAgent);

TypeId 
DiscoveryAgent::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::dtn7::DiscoveryAgent")
    .SetParent<Object> ()
    .SetGroupName ("Dtn7")
  ;
  return tid;
}

DiscoveryAgent::DiscoveryAgent()
{
  NS_LOG_FUNCTION (this);
}

DiscoveryAgent::~DiscoveryAgent()
{
  NS_LOG_FUNCTION (this);
}

// IpDiscoveryAgent实现

NS_OBJECT_ENSURE_REGISTERED (IpDiscoveryAgent);

TypeId 
IpDiscoveryAgent::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::dtn7::IpDiscoveryAgent")
    .SetParent<DiscoveryAgent> ()
    .SetGroupName ("Dtn7")
    .AddConstructor<IpDiscoveryAgent> ()
    .AddAttribute ("LocalAddress",
                   "本地IP地址",
                   Ipv4AddressValue (Ipv4Address::GetAny ()),
                   MakeIpv4AddressAccessor (&IpDiscoveryAgent::m_address),
                   MakeIpv4AddressChecker ())
    .AddAttribute ("LocalPort",
                   "本地端口",
                   UintegerValue (3835),
                   MakeUintegerAccessor (&IpDiscoveryAgent::m_port),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("MulticastAddress",
                   "组播地址",
                   Ipv4AddressValue ("224.0.0.26"),
                   MakeIpv4AddressAccessor (&IpDiscoveryAgent::m_multicastAddress),
                   MakeIpv4AddressChecker ())
    .AddAttribute ("AnnounceInterval",
                   "通告间隔",
                   TimeValue (Seconds (10)),
                   MakeTimeAccessor (&IpDiscoveryAgent::m_announceInterval),
                   MakeTimeChecker ())
  ;
  return tid;
}

IpDiscoveryAgent::IpDiscoveryAgent()
  : m_address(Ipv4Address::GetAny()),
    m_port(3835),
    m_multicastAddress("224.0.0.26"),
    m_announceInterval(Seconds(10)),
    m_running(false),
    m_nodeId("dtn://local/"),
    m_announcementsSent(0),
    m_announcementsReceived(0)
{
  NS_LOG_FUNCTION (this);
}

IpDiscoveryAgent::IpDiscoveryAgent(Ptr<Node> node,
                                 Ipv4Address address,
                                 uint16_t port,
                                 Ipv4Address multicastAddress,
                                 Time announceInterval)
  : m_node(node),
    m_address(address),
    m_port(port),
    m_multicastAddress(multicastAddress),
    m_announceInterval(announceInterval),
    m_running(false),
    m_nodeId("dtn://local/"),
    m_announcementsSent(0),
    m_announcementsReceived(0)
{
  NS_LOG_FUNCTION (this << node << address << port << multicastAddress
                  << announceInterval);
}

IpDiscoveryAgent::~IpDiscoveryAgent()
{
  NS_LOG_FUNCTION (this);
  Stop();
}

void 
IpDiscoveryAgent::RegisterDiscoveryCallback(DiscoveryCallback callback)
{
  NS_LOG_FUNCTION (this);
  m_discoveryCallback = callback;
}

void 
IpDiscoveryAgent::SetNodeId(const std::string& nodeId)
{
  NS_LOG_FUNCTION (this << nodeId);
  m_nodeId = nodeId;
}

void 
IpDiscoveryAgent::AddService(const std::string& service, const std::string& endpoint)
{
  NS_LOG_FUNCTION (this << service << endpoint);
  
  std::lock_guard<std::mutex> lock(m_servicesMutex);
  m_services[service] = endpoint;
  
  // 如果已运行，立即发送更新的通告
  if (m_running)
    {
      SendAnnouncement();
    }
}

void 
IpDiscoveryAgent::RemoveService(const std::string& service)
{
  NS_LOG_FUNCTION (this << service);
  
  std::lock_guard<std::mutex> lock(m_servicesMutex);
  m_services.erase(service);
  
  // 如果已运行，立即发送更新的通告
  if (m_running)
    {
      SendAnnouncement();
    }
}

bool 
IpDiscoveryAgent::Start()
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
  
  // 创建套接字
  m_socket = CreateSocket();
  if (!m_socket)
    {
      NS_LOG_ERROR ("无法创建套接字");
      return false;
    }
  
  // 设置接收回调
  m_socket->SetRecvCallback(MakeCallback(&IpDiscoveryAgent::HandleReceive, this));
  
  // 发送初始通告
  SendAnnouncement();
  
  m_running = true;
  NS_LOG_INFO ("发现代理已启动: " << m_address << ":" << m_port);
  
  return true;
}

bool 
IpDiscoveryAgent::Stop()
{
  NS_LOG_FUNCTION (this);
  
  if (!m_running)
    {
      return true;
    }
  
  // 取消通告事件
  if (m_announceEvent.IsPending())
    {
      Simulator::Cancel(m_announceEvent);
    }
  
  // 关闭套接字
  if (m_socket)
    {
      m_socket->Close();
      m_socket = nullptr;
    }
  
  m_running = false;
  NS_LOG_INFO ("发现代理已停止");
  
  return true;
}

std::string 
IpDiscoveryAgent::GetStats() const
{
  NS_LOG_FUNCTION (this);
  
  std::stringstream ss;
  
  ss << "IpDiscoveryAgent(";
  ss << "addr=" << m_address << ":" << m_port;
  ss << ", mcast=" << m_multicastAddress;
  ss << ", interval=" << m_announceInterval.GetSeconds() << "s";
  ss << ", sent=" << m_announcementsSent;
  ss << ", recv=" << m_announcementsReceived;
  ss << ", services=" << m_services.size();
  ss << ")";
  
  return ss.str();
}

void 
IpDiscoveryAgent::SetNode(Ptr<Node> node)
{
  NS_LOG_FUNCTION (this << node);
  m_node = node;
}

Ptr<Socket> 
IpDiscoveryAgent::CreateSocket()
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
  
  // 配置套接字选项
  socket->SetAllowBroadcast(true);
  // 多播TTL设置
  socket->SetAttribute("IpTtl", UintegerValue(1));
  
  // 绑定到本地地址和端口
  if (socket->Bind(InetSocketAddress(m_address, m_port)) != 0)
    {
      NS_LOG_ERROR ("无法绑定套接字到 " << m_address << ":" << m_port);
      return nullptr;
    }
  
  // 对于NS-3.44，我们使用不同的方式接收多播数据
  // 1. 确保套接字接收广播和多播数据包
  socket->SetAttribute("RecvBroadcast", BooleanValue(true));
  
  // 2. 绑定到多播地址以接收多播数据
  InetSocketAddress mcastAddr(m_multicastAddress, m_port);
  if (socket->Bind(mcastAddr) != 0)
    {
      NS_LOG_ERROR ("无法绑定套接字到多播地址 " << m_multicastAddress);
      return nullptr;
    }
  
  NS_LOG_INFO ("套接字已配置为接收来自多播组 " << m_multicastAddress << " 的数据包");
  
  return socket;
}

void 
IpDiscoveryAgent::SendAnnouncement()
{
  NS_LOG_FUNCTION (this);
  
  if (!m_socket)
    {
      NS_LOG_ERROR ("套接字未初始化");
      return;
    }
  
  // 创建通告消息
  BeaconMessage beacon;
  beacon.nodeId = m_nodeId;
  
  // 使用第一个非回环接口的地址作为端点
  Ptr<Ipv4> ipv4 = m_node->GetObject<Ipv4>();
  if (ipv4 && ipv4->GetNInterfaces() > 0)
    {
      for (uint32_t i = 0; i < ipv4->GetNInterfaces(); ++i)
        {
          if (ipv4->GetAddress(i, 0).GetLocal() != Ipv4Address::GetLoopback())
            {
              std::stringstream ss;
              ss << ipv4->GetAddress(i, 0).GetLocal() << ":" << m_port;
              beacon.endpoint = ss.str();
              break;
            }
        }
    }
  
  // 如果未找到合适的接口，使用配置的地址
  if (beacon.endpoint.empty())
    {
      std::stringstream ss;
      ss << m_address << ":" << m_port;
      beacon.endpoint = ss.str();
    }
  
  // 获取服务列表
  beacon.services = GetServicesString();
  
  // 设置时间戳
  beacon.timestamp = Simulator::Now();
  
  // 序列化消息
  std::string message = beacon.Serialize();
  
  // 发送到多播组
  InetSocketAddress dest(m_multicastAddress, m_port);
  m_socket->SendTo(reinterpret_cast<const uint8_t*>(message.c_str()),
                  message.size(), 0, dest);
  
  m_announcementsSent++;
  NS_LOG_INFO ("已发送通告到 " << m_multicastAddress << ":" << m_port);
  
  // 安排下一次通告
  ScheduleNextAnnouncement();
}

void 
IpDiscoveryAgent::HandleReceive(Ptr<Socket> socket)
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
      
      // 跳过自己发送的消息
      if (address.GetIpv4() == m_address && address.GetPort() == m_port)
        {
          continue;
        }
      
      // 读取数据
      uint32_t size = packet->GetSize();
      uint8_t *data = new uint8_t[size + 1];
      packet->CopyData(data, size);
      data[size] = '\0';  // 确保字符串以null结尾
      
      std::string message(reinterpret_cast<char*>(data));
      delete[] data;
      
      // 解析通告消息
      BeaconMessage beacon;
      if (!beacon.Deserialize(message))
        {
          NS_LOG_WARN ("无法解析通告消息: " << message);
          continue;
        }
      
      m_announcementsReceived++;
      NS_LOG_INFO ("收到通告来自 " << address.GetIpv4() << ":" << address.GetPort() 
                   << " (nodeId=" << beacon.nodeId << ")");
      
      // 解析服务列表
      std::stringstream ss(beacon.services);
      std::string service, endpoint;
      
      while (std::getline(ss, service, ';'))
        {
          size_t sep = service.find('=');
          if (sep != std::string::npos)
            {
              std::string serviceName = service.substr(0, sep);
              std::string serviceEndpoint = service.substr(sep + 1);
              
              // 调用发现回调
              if (!m_discoveryCallback.IsNull())
                {
                  m_discoveryCallback(beacon.nodeId, serviceName, serviceEndpoint);
                }
            }
        }
    }
}

void 
IpDiscoveryAgent::ScheduleNextAnnouncement()
{
  NS_LOG_FUNCTION (this);
  
  // 取消已有事件
  if (m_announceEvent.IsPending())
    {
      Simulator::Cancel(m_announceEvent);
    }
  
  // 安排下一次通告
  m_announceEvent = Simulator::Schedule(m_announceInterval,
                                      &IpDiscoveryAgent::SendAnnouncement,
                                      this);
}

std::string 
IpDiscoveryAgent::GetServicesString() const
{
  NS_LOG_FUNCTION (this);
  
  std::stringstream ss;
  
  std::lock_guard<std::mutex> lock(m_servicesMutex);
  
  bool first = true;
  for (const auto& pair : m_services)
    {
      if (!first)
        {
          ss << ";";
        }
      first = false;
      
      ss << pair.first << "=" << pair.second;
    }
  
  return ss.str();
}

} // namespace dtn7

} // namespace ns3