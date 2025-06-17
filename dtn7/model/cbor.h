#ifndef DTN7_CBOR_H
#define DTN7_CBOR_H

#include "ns3/buffer.h"

#include <vector>
#include <string>
#include <map>
#include <memory>
#include <variant>
#include <optional>

namespace ns3 {

namespace dtn7 {

/**
 * \brief CBOR data types
 */
enum class CborType {
  UNSIGNED_INTEGER,  //!< Unsigned integer (0-23, 24-255, 256-65535, 65536-4294967295, 4294967296-18446744073709551615)
  NEGATIVE_INTEGER,  //!< Negative integer (-1-(-24), -25-(-256), -257-(-65536), -65537-(-4294967296), -4294967297-(-18446744073709551616))
  BYTE_STRING,       //!< Byte string (0-23 bytes, 24-255 bytes, 256-65535 bytes, 65536-4294967295 bytes, indefinite)
  TEXT_STRING,       //!< Text string (0-23 bytes, 24-255 bytes, 256-65535 bytes, 65536-4294967295 bytes, indefinite)
  ARRAY,             //!< Array (0-23 items, 24-255 items, 256-65535 items, 65536-4294967295 items, indefinite)
  MAP,               //!< Map (0-23 pairs, 24-255 pairs, 256-65535 pairs, 65536-4294967295 pairs, indefinite)
  TAG,               //!< Tag (0-23, 24-255, 256-65535, 65536-4294967295, 4294967296-18446744073709551615)
  SIMPLE,            //!< Simple value (0-23, 24-255) including false (20), true (21), null (22), undefined (23), float16 (25), float32 (26), float64 (27), break (31)
  INVALID            //!< Invalid data
};

/**
 * \brief CBOR simple values
 */
enum class CborSimpleValue : uint8_t {
  FALSE = 20,         //!< Boolean false
  TRUE = 21,          //!< Boolean true
  NULL_VALUE = 22,    //!< Null value
  UNDEFINED = 23,     //!< Undefined value
  FLOAT16 = 25,       //!< 16-bit floating point
  FLOAT32 = 26,       //!< 32-bit floating point
  FLOAT64 = 27,       //!< 64-bit floating point
  BREAK = 31          //!< Stop code for indefinite-length items
};

/**
 * \ingroup dtn7
 * \brief CBOR encoder/decoder
 */
class Cbor
{
public:
  // Forward declaration for map value type
  class CborValue;
  
  // Define map and array types to avoid cycles
  using CborArray = std::vector<CborValue>;
  using CborMap = std::map<CborValue, CborValue>;
  
  /**
   * \brief CBOR value class
   */
  class CborValue
  {
  public:
    // Variant type for different CBOR values - 关键修改：使用std::shared_ptr<CborValue>替代直接使用CborValue
    using ValueType = std::variant<
        uint64_t,                                   // UNSIGNED_INTEGER
        int64_t,                                    // NEGATIVE_INTEGER
        std::vector<uint8_t>,                       // BYTE_STRING
        std::string,                                // TEXT_STRING
        std::shared_ptr<CborArray>,                 // ARRAY
        std::shared_ptr<CborMap>,                   // MAP
        std::pair<uint64_t, std::shared_ptr<CborValue>>, // TAG - 修改这里使用std::shared_ptr
        CborSimpleValue,                            // SIMPLE
        std::monostate                              // INVALID
    >;
    
    /**
     * \brief Default constructor (invalid value)
     */
    CborValue();
    
    /**
     * \brief Constructor for unsigned integer
     * \param value Integer value
     */
    CborValue(uint64_t value);
    
    /**
     * \brief Constructor for negative integer
     * \param value Integer value
     */
    CborValue(int64_t value);
    
    /**
     * \brief Constructor for byte string
     * \param value Byte string
     */
    CborValue(const std::vector<uint8_t>& value);
    
    /**
     * \brief Constructor for text string
     * \param value Text string
     */
    CborValue(const std::string& value);
    
    /**
     * \brief Constructor for array
     * \param value Array
     */
    CborValue(const CborArray& value);
    
    /**
     * \brief Constructor for map
     * \param value Map
     */
    CborValue(const CborMap& value);
    
    /**
     * \brief Factory method for tag - 替换原始的构造函数
     * \param tag Tag number
     * \param value Tagged value
     * \return CborValue with tag
     */
    static CborValue CreateTaggedValue(uint64_t tag, const CborValue& value);
    
    /**
     * \brief Constructor for simple value
     * \param value Simple value
     */
    CborValue(CborSimpleValue value);
    
    /**
     * \brief Get the CBOR type
     * \return CBOR type
     */
    CborType GetType() const;
    
    /**
     * \brief Check if value is unsigned integer
     * \return true if value is unsigned integer
     */
    bool IsUnsignedInteger() const;
    
    /**
     * \brief Check if value is negative integer
     * \return true if value is negative integer
     */
    bool IsNegativeInteger() const;
    
    /**
     * \brief Check if value is any integer (unsigned or negative)
     * \return true if value is integer
     */
    bool IsInteger() const;
    
    /**
     * \brief Check if value is byte string
     * \return true if value is byte string
     */
    bool IsByteString() const;
    
    /**
     * \brief Check if value is text string
     * \return true if value is text string
     */
    bool IsTextString() const;
    
    /**
     * \brief Check if value is array
     * \return true if value is array
     */
    bool IsArray() const;
    
    /**
     * \brief Check if value is map
     * \return true if value is map
     */
    bool IsMap() const;
    
    /**
     * \brief Check if value is tag
     * \return true if value is tag
     */
    bool IsTag() const;
    
    /**
     * \brief Check if value is simple value
     * \return true if value is simple value
     */
    bool IsSimple() const;
    
    /**
     * \brief Check if value is null
     * \return true if value is null
     */
    bool IsNull() const;
    
    /**
     * \brief Check if value is undefined
     * \return true if value is undefined
     */
    bool IsUndefined() const;
    
    /**
     * \brief Check if value is boolean
     * \return true if value is boolean
     */
    bool IsBoolean() const;
    
    /**
     * \brief Check if value is floating point
     * \return true if value is floating point
     */
    bool IsFloat() const;
    
    /**
     * \brief Get unsigned integer value
     * \return Unsigned integer value
     */
    uint64_t GetUnsignedInteger() const;
    
    /**
     * \brief Get negative integer value
     * \return Negative integer value
     */
    int64_t GetNegativeInteger() const;
    
    /**
     * \brief Get integer value (positive or negative)
     * \return Integer value
     */
    int64_t GetInteger() const;
    
    /**
     * \brief Get byte string value
     * \return Byte string value
     */
    const std::vector<uint8_t>& GetByteString() const;
    
    /**
     * \brief Get text string value
     * \return Text string value
     */
    const std::string& GetTextString() const;
    
    /**
     * \brief Get array value
     * \return Array value
     */
    const CborArray& GetArray() const;
    
    /**
     * \brief Get map value
     * \return Map value
     */
    const CborMap& GetMap() const;
    
    /**
     * \brief Get tag value - 修改返回类型
     * \return Tag value (tag number and tagged value)
     */
    std::pair<uint64_t, std::shared_ptr<const CborValue>> GetTag() const;
    
    /**
     * \brief Get simple value
     * \return Simple value
     */
    CborSimpleValue GetSimple() const;
    
    /**
     * \brief Get boolean value
     * \return Boolean value
     */
    bool GetBoolean() const;
    
    /**
     * \brief Get double value (from floating point)
     * \return Double value
     */
    double GetFloat() const;
    
    /**
     * \brief Comparison operators for maps
     * \param other Other CBOR value
     * \return Comparison result
     */
    bool operator==(const CborValue& other) const;
    bool operator!=(const CborValue& other) const;
    bool operator<(const CborValue& other) const;
    
    /**
     * \brief Get string representation
     * \return String representation
     */
    std::string ToString() const;
    
    /**
     * \brief Get the underlying variant value
     * \return Variant value
     */
    const ValueType& GetValue() const { return m_value; }
    
  private:
    ValueType m_value;  //!< CBOR value
  };
  
  /**
   * \brief Encode CBOR value to binary data
   * \param value CBOR value
   * \return Encoded data
   */
  static Buffer Encode(const CborValue& value);
  
  /**
   * \brief Decode binary data to CBOR value
   * \param buffer Binary data
   * \return Decoded CBOR value or empty optional if error
   */
  static std::optional<CborValue> Decode(Buffer buffer);
  
private:
  /**
   * \brief Encode header byte for a major type and additional info
   * \param majorType Major type (0-7)
   * \param additionalInfo Additional info (0-31)
   * \return Header byte
   */
  static uint8_t EncodeHeader(uint8_t majorType, uint8_t additionalInfo);
  
  /**
   * \brief Decode header byte to major type and additional info
   * \param header Header byte
   * \param majorType Output parameter for major type
   * \param additionalInfo Output parameter for additional info
   */
  static void DecodeHeader(uint8_t header, uint8_t& majorType, uint8_t& additionalInfo);
  
  /**
   * \brief Encode unsigned integer
   * \param buffer Output buffer
   * \param majorType Major type (0-7)
   * \param value Integer value
   */
  static void EncodeUnsigned(Buffer& buffer, uint8_t majorType, uint64_t value);
  
  /**
   * \brief Decode unsigned integer
   * \param buffer Input buffer
   * \param majorType Major type (0-7)
   * \param bytesRead Output parameter for number of bytes read
   * \return Decoded value or empty optional if error
   */
  static std::optional<uint64_t> DecodeUnsigned(Buffer& buffer, uint8_t majorType, size_t& bytesRead);
};

} // namespace dtn7

} // namespace ns3

#endif /* DTN7_CBOR_H */