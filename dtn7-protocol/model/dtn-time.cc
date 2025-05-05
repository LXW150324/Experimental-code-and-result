#include "dtn-time.h"
#include "ns3/simulator.h"
#include <ctime>
#include <iomanip>
#include <sstream>

namespace ns3 {

namespace dtn7 {

DtnTime::DtnTime()
  : m_dtnTimeSeconds(0),
    m_dtnTimeNanoseconds(0)
{
  // Initialize with current DTN time
  *this = GetDtnNow();
}

DtnTime::DtnTime(const Time& time)
  : m_dtnTimeSeconds(0),
    m_dtnTimeNanoseconds(0)
{
  // Convert from NS3 time
  time_t unixTime = std::time(nullptr);
  // NS3 simulation time in seconds
  double simTime = Simulator::Now().GetSeconds();
  
  // Calculate DTN time (seconds since 2000-01-01)
  m_dtnTimeSeconds = static_cast<uint64_t>(unixTime - DTN_TIME_EPOCH + simTime);
  
  // Nanoseconds part from NS3 time
  m_dtnTimeNanoseconds = (time.GetNanoSeconds() % 1000000000);
}

DtnTime::DtnTime(uint64_t dtnTimeSeconds, uint64_t dtnTimeNanoseconds)
  : m_dtnTimeSeconds(dtnTimeSeconds),
    m_dtnTimeNanoseconds(dtnTimeNanoseconds)
{
}

Time 
DtnTime::ToTime() const
{
  // Convert to NS3 time
  return Seconds(m_dtnTimeSeconds) + NanoSeconds(m_dtnTimeNanoseconds);
}

DtnTime 
DtnTime::FromTime(const Time& time)
{
  uint64_t totalSeconds = time.GetSeconds();
  uint64_t nanoSeconds = time.GetNanoSeconds() % 1000000000;
  
  return DtnTime(totalSeconds, nanoSeconds);
}

bool 
DtnTime::operator==(const DtnTime& other) const
{
  return m_dtnTimeSeconds == other.m_dtnTimeSeconds &&
         m_dtnTimeNanoseconds == other.m_dtnTimeNanoseconds;
}

bool 
DtnTime::operator!=(const DtnTime& other) const
{
  return !(*this == other);
}

bool 
DtnTime::operator<(const DtnTime& other) const
{
  if (m_dtnTimeSeconds < other.m_dtnTimeSeconds)
    {
      return true;
    }
  else if (m_dtnTimeSeconds == other.m_dtnTimeSeconds)
    {
      return m_dtnTimeNanoseconds < other.m_dtnTimeNanoseconds;
    }
  return false;
}

bool 
DtnTime::operator<=(const DtnTime& other) const
{
  return *this < other || *this == other;
}

bool 
DtnTime::operator>(const DtnTime& other) const
{
  return !(*this <= other);
}

bool 
DtnTime::operator>=(const DtnTime& other) const
{
  return !(*this < other);
}

std::string 
DtnTime::ToString() const
{
  // Calculate Unix time from DTN time
  time_t unixTime = static_cast<time_t>(m_dtnTimeSeconds + DTN_TIME_EPOCH);
  
  // Format as ISO 8601
  struct tm timeinfo;
  gmtime_r(&unixTime, &timeinfo);
  
  std::stringstream ss;
  ss << std::put_time(&timeinfo, "%Y-%m-%dT%H:%M:%S");
  
  // Add nanoseconds if non-zero
  if (m_dtnTimeNanoseconds > 0)
    {
      ss << "." << std::setw(9) << std::setfill('0') << m_dtnTimeNanoseconds;
    }
  
  ss << "Z";
  
  return ss.str();
}

DtnTime 
GetDtnNow()
{
  // Get current time
  time_t unixTime = std::time(nullptr);
  
  // Add simulation time
  double simTime = Simulator::Now().GetSeconds();
  uint64_t totalSeconds = static_cast<uint64_t>(unixTime - DTN_TIME_EPOCH + simTime);
  
  // Nanoseconds from simulation time
  uint64_t nanoSeconds = (Simulator::Now().GetNanoSeconds() % 1000000000);
  
  return DtnTime(totalSeconds, nanoSeconds);
}

} // namespace dtn7

} // namespace ns3