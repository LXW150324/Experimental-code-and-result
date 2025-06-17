#ifndef DTN7_ENDPOINT_H
#define DTN7_ENDPOINT_H

#include "ns3/buffer.h"

#include <string>
#include <optional>
#include <functional> // 添加这行来支持哈希函数

namespace ns3 {

namespace dtn7 {

/**
 * \ingroup dtn7
 * \brief Endpoint ID for DTN nodes
 */
class EndpointID
{
public:
  /**
   * \brief Default constructor
   */
  EndpointID ();
  
  /**
   * \brief Constructor with EID string
   * \param eid Endpoint ID string
   */
  explicit EndpointID (const std::string& eid);
  
  /**
   * \brief Equality operator
   * \param other Other endpoint ID
   * \return true if equal
   */
  bool operator== (const EndpointID& other) const;
  
  /**
   * \brief Inequality operator
   * \param other Other endpoint ID
   * \return true if not equal
   */
  bool operator!= (const EndpointID& other) const;
  
  /**
   * \brief Less-than operator for ordering
   * \param other Other endpoint ID
   * \return true if this < other
   */
  bool operator< (const EndpointID& other) const;
  
  /**
   * \brief Check if EID string is valid
   * \param eid Endpoint ID string
   * \return true if valid
   */
  static bool IsValid (const std::string& eid);
  
  /**
   * \brief Parse EID string
   * \param eid Endpoint ID string
   * \return Parsed endpoint ID
   */
  static std::optional<EndpointID> Parse (const std::string& eid);
  
  /**
   * \brief Get string representation
   * \return EID as string
   */
  std::string ToString () const;
  
  /**
   * \brief Check if EID is a singleton
   * \return true if singleton
   */
  bool IsSingleton () const;
  
  /**
   * \brief Check if EID is "dtn:none"
   * \return true if none
   */
  bool IsNone () const;
  
  /**
   * \brief Check if EID uses DTN scheme
   * \return true if DTN scheme
   */
  bool IsDtn () const;
  
  /**
   * \brief Check if EID uses IPN scheme
   * \return true if IPN scheme
   */
  bool IsIpn () const;
  
  /**
   * \brief Get DTN host part
   * \return Host part of DTN EID
   */
  std::string GetDtnHost () const;
  
  /**
   * \brief Get DTN service part
   * \return Service part of DTN EID
   */
  std::string GetDtnService () const;
  
  /**
   * \brief Get IPN node number
   * \return Node number of IPN EID
   */
  uint64_t GetIpnNode () const;
  
  /**
   * \brief Get IPN service number
   * \return Service number of IPN EID
   */
  uint64_t GetIpnService () const;
  
  /**
   * \brief Serialize to CBOR format
   * \return CBOR encoded data
   */
  Buffer ToCbor () const;
  
  /**
   * \brief Deserialize from CBOR format
   * \param buffer CBOR encoded data
   * \return Deserialized endpoint ID
   */
  static std::optional<EndpointID> FromCbor (Buffer buffer);

  /**
   * \brief Calculate hash for this endpoint ID
   * \return Hash value
   */
  size_t Hash() const {
    return std::hash<std::string>{}(m_uri);
  }

private:
  std::string m_scheme;     //!< Scheme part of EID (dtn or ipn)
  std::string m_ssp;        //!< Scheme-specific part
  std::string m_uri;        //!< Complete URI string
  
  /**
   * \brief Parse DTN scheme URI
   * \param uri URI string
   * \return true if successful
   */
  bool ParseDtn (const std::string& uri);
  
  /**
   * \brief Parse IPN scheme URI
   * \param uri URI string
   * \return true if successful
   */
  bool ParseIpn (const std::string& uri);
};

/**
 * \brief Node ID is an alias for EndpointID
 */
using NodeID = EndpointID;

} // namespace dtn7

} // namespace ns3

// 在命名空间外部定义哈希函数特化
namespace std {
  template<>
  struct hash<ns3::dtn7::EndpointID> {
    size_t operator()(const ns3::dtn7::EndpointID& eid) const {
      return eid.Hash();
    }
  };
}

#endif /* DTN7_ENDPOINT_H */