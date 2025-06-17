#ifndef DTN7_DTN_TIME_H
#define DTN7_DTN_TIME_H

#include "ns3/nstime.h"

#include <cstdint>
#include <string>

namespace ns3 {

namespace dtn7 {

/**
 * \ingroup dtn7
 * \brief DTN time as specified in the Bundle Protocol
 */
class DtnTime
{
public:
  /**
   * \brief Default constructor - current time
   */
  DtnTime ();
  
  /**
   * \brief Constructor from ns3 time
   * \param time NS3 time
   */
  explicit DtnTime (const Time& time);
  
  /**
   * \brief Constructor from DTN time values
   * \param dtnTimeSeconds DTN time seconds
   * \param dtnTimeNanoseconds DTN time nanoseconds
   */
  DtnTime (uint64_t dtnTimeSeconds, uint64_t dtnTimeNanoseconds = 0);
  
  /**
   * \brief Convert to NS3 time
   * \return NS3 time
   */
  Time ToTime () const;
  
  /**
   * \brief Convert from NS3 time
   * \param time NS3 time
   * \return DTN time
   */
  static DtnTime FromTime (const Time& time);
  
  /**
   * \brief Get DTN time seconds
   * \return DTN time seconds
   */
  uint64_t GetSeconds () const { return m_dtnTimeSeconds; }
  
  /**
   * \brief Get DTN time nanoseconds
   * \return DTN time nanoseconds
   */
  uint64_t GetNanoseconds () const { return m_dtnTimeNanoseconds; }
  
  /**
   * \brief Equality operator
   * \param other Other DTN time
   * \return true if equal
   */
  bool operator== (const DtnTime& other) const;
  
  /**
   * \brief Inequality operator
   * \param other Other DTN time
   * \return true if not equal
   */
  bool operator!= (const DtnTime& other) const;
  
  /**
   * \brief Less-than operator
   * \param other Other DTN time
   * \return true if this < other
   */
  bool operator< (const DtnTime& other) const;
  
  /**
   * \brief Less-than-or-equal operator
   * \param other Other DTN time
   * \return true if this <= other
   */
  bool operator<= (const DtnTime& other) const;
  
  /**
   * \brief Greater-than operator
   * \param other Other DTN time
   * \return true if this > other
   */
  bool operator> (const DtnTime& other) const;
  
  /**
   * \brief Greater-than-or-equal operator
   * \param other Other DTN time
   * \return true if this >= other
   */
  bool operator>= (const DtnTime& other) const;
  
  /**
   * \brief Get string representation (ISO 8601 format)
   * \return Time string
   */
  std::string ToString () const;

private:
  uint64_t m_dtnTimeSeconds;      //!< DTN time seconds since DTN epoch
  uint64_t m_dtnTimeNanoseconds;  //!< DTN time nanoseconds
};

/**
 * \brief Get the current DTN time
 * \return Current DTN time
 */
DtnTime GetDtnNow ();

/**
 * \brief DTN time epoch (2000-01-01T00:00:00Z in Unix time)
 */
constexpr uint64_t DTN_TIME_EPOCH = 946684800;

} // namespace dtn7

} // namespace ns3

#endif /* DTN7_DTN_TIME_H */