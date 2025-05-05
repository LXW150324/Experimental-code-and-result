#include "bundle-id.h"
#include <sstream>
#include <functional>

namespace ns3 {

namespace dtn7 {

BundleID::BundleID()
  : m_sequenceNumber(0),
    m_isFragment(false),
    m_fragmentOffset(0)
{
}

BundleID::BundleID(const EndpointID& source, 
                  DtnTime timestamp, 
                  uint64_t sequenceNumber, 
                  bool isFragment, 
                  uint64_t fragmentOffset)
  : m_source(source),
    m_timestamp(timestamp),
    m_sequenceNumber(sequenceNumber),
    m_isFragment(isFragment),
    m_fragmentOffset(fragmentOffset)
{
}

bool 
BundleID::operator==(const BundleID& other) const
{
  if (m_isFragment != other.m_isFragment)
    {
      return false;
    }
  
  if (m_source != other.m_source)
    {
      return false;
    }
  
  if (m_timestamp != other.m_timestamp)
    {
      return false;
    }
  
  if (m_sequenceNumber != other.m_sequenceNumber)
    {
      return false;
    }
  
  // Only check fragment offset if both are fragments
  if (m_isFragment && other.m_isFragment && m_fragmentOffset != other.m_fragmentOffset)
    {
      return false;
    }
  
  return true;
}

bool 
BundleID::operator!=(const BundleID& other) const
{
  return !(*this == other);
}

bool 
BundleID::operator<(const BundleID& other) const
{
  // First compare source
  if (m_source != other.m_source)
    {
      return m_source < other.m_source;
    }
  
  // Then timestamp
  if (m_timestamp != other.m_timestamp)
    {
      return m_timestamp < other.m_timestamp;
    }
  
  // Then sequence number
  if (m_sequenceNumber != other.m_sequenceNumber)
    {
      return m_sequenceNumber < other.m_sequenceNumber;
    }
  
  // Then fragment status
  if (m_isFragment != other.m_isFragment)
    {
      return !m_isFragment && other.m_isFragment;
    }
  
  // If both are fragments, compare offset
  if (m_isFragment && other.m_isFragment)
    {
      return m_fragmentOffset < other.m_fragmentOffset;
    }
  
  return false;
}

std::string 
BundleID::ToString() const
{
  std::stringstream ss;
  ss << m_source.ToString() << "@" 
     << m_timestamp.ToString() << "#" 
     << m_sequenceNumber;
  
  if (m_isFragment)
    {
      ss << ":" << m_fragmentOffset;
    }
  
  return ss.str();
}

size_t 
BundleID::Hash() const
{
  size_t hash = 0;
  std::string sourceStr = m_source.ToString();
  
  // Combine hash values from all fields
  hash ^= std::hash<std::string>{}(sourceStr);
  hash ^= std::hash<uint64_t>{}(m_timestamp.GetSeconds()) << 1;
  hash ^= std::hash<uint64_t>{}(m_sequenceNumber) << 1;
  
  if (m_isFragment)
    {
      hash ^= std::hash<uint64_t>{}(m_fragmentOffset) << 1;
    }
  
  return hash;
}

} // namespace dtn7

} // namespace ns3