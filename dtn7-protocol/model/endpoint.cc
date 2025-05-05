#include "endpoint.h"
#include "cbor.h"
#include <regex>
#include <sstream>

namespace ns3 {

namespace dtn7 {

EndpointID::EndpointID()
  : m_scheme("dtn"),
    m_ssp("none"),
    m_uri("dtn:none")
{
}

EndpointID::EndpointID(const std::string& eid)
  : m_uri(eid)
{
  if (eid.empty())
    {
      // Default to dtn:none
      m_scheme = "dtn";
      m_ssp = "none";
      m_uri = "dtn:none";
      return;
    }
  
  size_t schemeEnd = eid.find(':');
  if (schemeEnd == std::string::npos)
    {
      // No scheme, assume malformed and default to dtn:none
      m_scheme = "dtn";
      m_ssp = "none";
      m_uri = "dtn:none";
      return;
    }
  
  m_scheme = eid.substr(0, schemeEnd);
  m_ssp = eid.substr(schemeEnd + 1);
  
  if (m_scheme == "dtn")
    {
      if (!ParseDtn(eid))
        {
          // Reset to dtn:none on parsing failure
          m_scheme = "dtn";
          m_ssp = "none";
          m_uri = "dtn:none";
        }
    }
  else if (m_scheme == "ipn")
    {
      if (!ParseIpn(eid))
        {
          // Reset to dtn:none on parsing failure
          m_scheme = "dtn";
          m_ssp = "none";
          m_uri = "dtn:none";
        }
    }
  else
    {
      // Unknown scheme, default to dtn:none
      m_scheme = "dtn";
      m_ssp = "none";
      m_uri = "dtn:none";
    }
}

bool 
EndpointID::operator==(const EndpointID& other) const
{
  return m_uri == other.m_uri;
}

bool 
EndpointID::operator!=(const EndpointID& other) const
{
  return !(*this == other);
}

bool 
EndpointID::operator<(const EndpointID& other) const
{
  return m_uri < other.m_uri;
}

bool 
EndpointID::IsValid(const std::string& eid)
{
  if (eid.empty())
    {
      return false;
    }
  
  size_t schemeEnd = eid.find(':');
  if (schemeEnd == std::string::npos)
    {
      return false;
    }
  
  std::string scheme = eid.substr(0, schemeEnd);
  
  if (scheme == "dtn")
    {
      // DTN scheme format: dtn:host/service
      std::regex dtnRegex("^dtn:(//)?([^/]+)(/.*)?$");
      return std::regex_match(eid, dtnRegex);
    }
  else if (scheme == "ipn")
    {
      // IPN scheme format: ipn:node.service
      std::regex ipnRegex("^ipn:(\\d+)\\.(\\d+)$");
      return std::regex_match(eid, ipnRegex);
    }
  
  return false;
}

std::optional<EndpointID> 
EndpointID::Parse(const std::string& eid)
{
  if (IsValid(eid))
    {
      return EndpointID(eid);
    }
  return std::nullopt;
}

std::string 
EndpointID::ToString() const
{
  return m_uri;
}

bool 
EndpointID::IsSingleton() const
{
  if (m_scheme == "dtn")
    {
      // DTN singleton: not a group EID (no '*' wildcard)
      return m_ssp.find('*') == std::string::npos;
    }
  else if (m_scheme == "ipn")
    {
      // IPN EIDs are always singletons
      return true;
    }
  
  return false;
}

bool 
EndpointID::IsNone() const
{
  return m_scheme == "dtn" && m_ssp == "none";
}

bool 
EndpointID::IsDtn() const
{
  return m_scheme == "dtn";
}

bool 
EndpointID::IsIpn() const
{
  return m_scheme == "ipn";
}

std::string 
EndpointID::GetDtnHost() const
{
  if (!IsDtn())
    {
      return "";
    }
  
  // Extract host part from DTN SSP
  std::string host = m_ssp;
  if (host.substr(0, 2) == "//")
    {
      host = host.substr(2);
    }
  
  size_t serviceStart = host.find('/');
  if (serviceStart != std::string::npos)
    {
      host = host.substr(0, serviceStart);
    }
  
  return host;
}

std::string 
EndpointID::GetDtnService() const
{
  if (!IsDtn())
    {
      return "";
    }
  
  // Extract service part from DTN SSP
  std::string ssp = m_ssp;
  if (ssp.substr(0, 2) == "//")
    {
      ssp = ssp.substr(2);
    }
  
  size_t serviceStart = ssp.find('/');
  if (serviceStart != std::string::npos)
    {
      return ssp.substr(serviceStart);
    }
  
  return "";
}

uint64_t 
EndpointID::GetIpnNode() const
{
  if (!IsIpn())
    {
      return 0;
    }
  
  // Extract node number from IPN SSP
  size_t dotPos = m_ssp.find('.');
  if (dotPos != std::string::npos)
    {
      try
        {
          return std::stoull(m_ssp.substr(0, dotPos));
        }
      catch (const std::exception&)
        {
          return 0;
        }
    }
  
  return 0;
}

uint64_t 
EndpointID::GetIpnService() const
{
  if (!IsIpn())
    {
      return 0;
    }
  
  // Extract service number from IPN SSP
  size_t dotPos = m_ssp.find('.');
  if (dotPos != std::string::npos)
    {
      try
        {
          return std::stoull(m_ssp.substr(dotPos + 1));
        }
      catch (const std::exception&)
        {
          return 0;
        }
    }
  
  return 0;
}

Buffer 
EndpointID::ToCbor() const
{
  // Create a CBOR array with two elements: scheme and SSP
  Cbor::CborArray array = {
    Cbor::CborValue(m_scheme),
    Cbor::CborValue(m_ssp)
  };
  
  return Cbor::Encode(Cbor::CborValue(array));
}

std::optional<EndpointID> 
EndpointID::FromCbor(Buffer buffer)
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
  if (array.size() != 2 || !array[0].IsTextString() || !array[1].IsTextString())
    {
      return std::nullopt;
    }
  
  std::string scheme = array[0].GetTextString();
  std::string ssp = array[1].GetTextString();
  
  std::string uri = scheme + ":" + ssp;
  return EndpointID(uri);
}

bool 
EndpointID::ParseDtn(const std::string& uri)
{
  // Check DTN scheme format
  std::regex dtnRegex("^dtn:(//)?([^/]+)(/.*)?$");
  std::smatch match;
  
  if (!std::regex_match(uri, match, dtnRegex))
    {
      return false;
    }
  
  // Valid DTN URI
  return true;
}

bool 
EndpointID::ParseIpn(const std::string& uri)
{
  // Check IPN scheme format
  std::regex ipnRegex("^ipn:(\\d+)\\.(\\d+)$");
  std::smatch match;
  
  if (!std::regex_match(uri, match, ipnRegex))
    {
      return false;
    }
  
  // Valid IPN URI
  return true;
}

} // namespace dtn7

} // namespace ns3