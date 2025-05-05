#include "cbor.h"
#include <limits>
#include <sstream>
#include <iomanip>

namespace ns3 {

namespace dtn7 {

// CborValue implementation

Cbor::CborValue::CborValue()
  : m_value(std::monostate{})
{
}

Cbor::CborValue::CborValue(uint64_t value)
  : m_value(value)
{
}

Cbor::CborValue::CborValue(int64_t value)
{
  if (value >= 0)
    {
      m_value = static_cast<uint64_t>(value);
    }
  else
    {
      m_value = value;
    }
}

Cbor::CborValue::CborValue(const std::vector<uint8_t>& value)
  : m_value(value)
{
}

Cbor::CborValue::CborValue(const std::string& value)
  : m_value(value)
{
}

Cbor::CborValue::CborValue(const CborArray& value)
  : m_value(std::make_shared<CborArray>(value))
{
}

Cbor::CborValue::CborValue(const CborMap& value)
  : m_value(std::make_shared<CborMap>(value))
{
}

// 工厂方法替代了原来的构造函数
Cbor::CborValue 
Cbor::CborValue::CreateTaggedValue(uint64_t tag, const CborValue& value)
{
  CborValue result;
  // 创建shared_ptr并存储在pair中
  result.m_value = std::pair<uint64_t, std::shared_ptr<CborValue>>(
      tag, std::make_shared<CborValue>(value));
  return result;
}

Cbor::CborValue::CborValue(CborSimpleValue value)
  : m_value(value)
{
}

CborType 
Cbor::CborValue::GetType() const
{
  if (std::holds_alternative<uint64_t>(m_value))
    {
      return CborType::UNSIGNED_INTEGER;
    }
  else if (std::holds_alternative<int64_t>(m_value))
    {
      return CborType::NEGATIVE_INTEGER;
    }
  else if (std::holds_alternative<std::vector<uint8_t>>(m_value))
    {
      return CborType::BYTE_STRING;
    }
  else if (std::holds_alternative<std::string>(m_value))
    {
      return CborType::TEXT_STRING;
    }
  else if (std::holds_alternative<std::shared_ptr<CborArray>>(m_value))
    {
      return CborType::ARRAY;
    }
  else if (std::holds_alternative<std::shared_ptr<CborMap>>(m_value))
    {
      return CborType::MAP;
    }
  else if (std::holds_alternative<std::pair<uint64_t, std::shared_ptr<CborValue>>>(m_value))
    {
      return CborType::TAG;
    }
  else if (std::holds_alternative<CborSimpleValue>(m_value))
    {
      return CborType::SIMPLE;
    }
  
  return CborType::INVALID;
}

bool 
Cbor::CborValue::IsUnsignedInteger() const
{
  return std::holds_alternative<uint64_t>(m_value);
}

bool 
Cbor::CborValue::IsNegativeInteger() const
{
  return std::holds_alternative<int64_t>(m_value);
}

bool 
Cbor::CborValue::IsInteger() const
{
  return IsUnsignedInteger() || IsNegativeInteger();
}

bool 
Cbor::CborValue::IsByteString() const
{
  return std::holds_alternative<std::vector<uint8_t>>(m_value);
}

bool 
Cbor::CborValue::IsTextString() const
{
  return std::holds_alternative<std::string>(m_value);
}

bool 
Cbor::CborValue::IsArray() const
{
  return std::holds_alternative<std::shared_ptr<CborArray>>(m_value);
}

bool 
Cbor::CborValue::IsMap() const
{
  return std::holds_alternative<std::shared_ptr<CborMap>>(m_value);
}

bool 
Cbor::CborValue::IsTag() const
{
  return std::holds_alternative<std::pair<uint64_t, std::shared_ptr<CborValue>>>(m_value);
}

bool 
Cbor::CborValue::IsSimple() const
{
  return std::holds_alternative<CborSimpleValue>(m_value);
}

bool 
Cbor::CborValue::IsNull() const
{
  return IsSimple() && GetSimple() == CborSimpleValue::NULL_VALUE;
}

bool 
Cbor::CborValue::IsUndefined() const
{
  return IsSimple() && GetSimple() == CborSimpleValue::UNDEFINED;
}

bool 
Cbor::CborValue::IsBoolean() const
{
  return IsSimple() && (GetSimple() == CborSimpleValue::TRUE || GetSimple() == CborSimpleValue::FALSE);
}

bool 
Cbor::CborValue::IsFloat() const
{
  return IsSimple() && (GetSimple() == CborSimpleValue::FLOAT16 || 
                       GetSimple() == CborSimpleValue::FLOAT32 || 
                       GetSimple() == CborSimpleValue::FLOAT64);
}

uint64_t 
Cbor::CborValue::GetUnsignedInteger() const
{
  if (IsUnsignedInteger())
    {
      return std::get<uint64_t>(m_value);
    }
  return 0;
}

int64_t 
Cbor::CborValue::GetNegativeInteger() const
{
  if (IsNegativeInteger())
    {
      return std::get<int64_t>(m_value);
    }
  return 0;
}

int64_t 
Cbor::CborValue::GetInteger() const
{
  if (IsUnsignedInteger())
    {
      uint64_t val = GetUnsignedInteger();
      if (val <= static_cast<uint64_t>(std::numeric_limits<int64_t>::max()))
        {
          return static_cast<int64_t>(val);
        }
    }
  else if (IsNegativeInteger())
    {
      return GetNegativeInteger();
    }
  return 0;
}

const std::vector<uint8_t>& 
Cbor::CborValue::GetByteString() const
{
  static const std::vector<uint8_t> empty;
  if (IsByteString())
    {
      return std::get<std::vector<uint8_t>>(m_value);
    }
  return empty;
}

const std::string& 
Cbor::CborValue::GetTextString() const
{
  static const std::string empty;
  if (IsTextString())
    {
      return std::get<std::string>(m_value);
    }
  return empty;
}

const Cbor::CborArray& 
Cbor::CborValue::GetArray() const
{
  static const CborArray empty;
  if (IsArray())
    {
      return *std::get<std::shared_ptr<CborArray>>(m_value);
    }
  return empty;
}

const Cbor::CborMap& 
Cbor::CborValue::GetMap() const
{
  static const CborMap empty;
  if (IsMap())
    {
      return *std::get<std::shared_ptr<CborMap>>(m_value);
    }
  return empty;
}

// 修改返回类型和实现
std::pair<uint64_t, std::shared_ptr<const Cbor::CborValue>> 
Cbor::CborValue::GetTag() const
{
  static const auto emptyValue = std::make_shared<CborValue>();
  if (IsTag())
    {
      const auto& tag = std::get<std::pair<uint64_t, std::shared_ptr<CborValue>>>(m_value);
      return std::make_pair(tag.first, std::const_pointer_cast<const CborValue>(tag.second));
    }
  return std::make_pair(0, emptyValue);
}

CborSimpleValue 
Cbor::CborValue::GetSimple() const
{
  if (IsSimple())
    {
      return std::get<CborSimpleValue>(m_value);
    }
  return CborSimpleValue::UNDEFINED;
}

bool 
Cbor::CborValue::GetBoolean() const
{
  if (IsBoolean())
    {
      return GetSimple() == CborSimpleValue::TRUE;
    }
  return false;
}

double 
Cbor::CborValue::GetFloat() const
{
  // In a real implementation, we would extract the float value
  // This is simplified for now
  return 0.0;
}

bool 
Cbor::CborValue::operator==(const CborValue& other) const
{
  // Types must match
  if (GetType() != other.GetType())
    {
      return false;
    }
  
  // Compare based on type
  switch (GetType())
    {
      case CborType::UNSIGNED_INTEGER:
        return GetUnsignedInteger() == other.GetUnsignedInteger();
      case CborType::NEGATIVE_INTEGER:
        return GetNegativeInteger() == other.GetNegativeInteger();
      case CborType::BYTE_STRING:
        return GetByteString() == other.GetByteString();
      case CborType::TEXT_STRING:
        return GetTextString() == other.GetTextString();
      case CborType::ARRAY:
        return GetArray() == other.GetArray();
      case CborType::MAP:
        return GetMap() == other.GetMap();
      case CborType::TAG:
        {
          auto thisTag = GetTag();
          auto otherTag = other.GetTag();
          return thisTag.first == otherTag.first && *thisTag.second == *otherTag.second;
        }
      case CborType::SIMPLE:
        return GetSimple() == other.GetSimple();
      case CborType::INVALID:
        return true; // Both are invalid
    }
  
  return false;
}

bool 
Cbor::CborValue::operator!=(const CborValue& other) const
{
  return !(*this == other);
}

bool 
Cbor::CborValue::operator<(const CborValue& other) const
{
  // First compare by type
  if (GetType() != other.GetType())
    {
      return static_cast<int>(GetType()) < static_cast<int>(other.GetType());
    }
  
  // Then compare by value
  switch (GetType())
    {
      case CborType::UNSIGNED_INTEGER:
        return GetUnsignedInteger() < other.GetUnsignedInteger();
      case CborType::NEGATIVE_INTEGER:
        return GetNegativeInteger() < other.GetNegativeInteger();
      case CborType::BYTE_STRING:
        return GetByteString() < other.GetByteString();
      case CborType::TEXT_STRING:
        return GetTextString() < other.GetTextString();
      case CborType::ARRAY:
        return GetArray() < other.GetArray();
      case CborType::MAP:
        return GetMap() < other.GetMap();
      case CborType::TAG:
        {
          auto thisTag = GetTag();
          auto otherTag = other.GetTag();
          if (thisTag.first != otherTag.first)
            {
              return thisTag.first < otherTag.first;
            }
          return *thisTag.second < *otherTag.second;
        }
      case CborType::SIMPLE:
        return static_cast<int>(GetSimple()) < static_cast<int>(other.GetSimple());
      case CborType::INVALID:
        return false; // Both are invalid
    }
  
  return false;
}

std::string 
Cbor::CborValue::ToString() const
{
  std::stringstream ss;
  
  switch (GetType())
    {
      case CborType::UNSIGNED_INTEGER:
        {
          // 使用str()将数值转换为字符串
          uint64_t val = GetUnsignedInteger();
          ss.str(ss.str() + std::to_string(val));
        }
        break;
      case CborType::NEGATIVE_INTEGER:
        {
          int64_t val = GetNegativeInteger();
          ss.str(ss.str() + std::to_string(val));
        }
        break;
      case CborType::BYTE_STRING:
        {
          const auto& bytes = GetByteString();
          ss.str(ss.str() + "h'");
          for (uint8_t byte : bytes)
            {
              char buf[3];
              std::snprintf(buf, sizeof(buf), "%02x", static_cast<int>(byte));
              ss.str(ss.str() + buf);
            }
          ss.str(ss.str() + "'");
        }
        break;
      case CborType::TEXT_STRING:
        {
          ss.str(ss.str() + "\"" + GetTextString() + "\"");
        }
        break;
      case CborType::ARRAY:
        {
          const auto& array = GetArray();
          ss.str(ss.str() + "[");
          for (size_t i = 0; i < array.size(); ++i)
            {
              if (i > 0)
                {
                  ss.str(ss.str() + ", ");
                }
              ss.str(ss.str() + array[i].ToString());
            }
          ss.str(ss.str() + "]");
        }
        break;
      case CborType::MAP:
        {
          const auto& map = GetMap();
          ss.str(ss.str() + "{");
          bool first = true;
          for (const auto& pair : map)
            {
              if (!first)
                {
                  ss.str(ss.str() + ", ");
                }
              first = false;
              ss.str(ss.str() + pair.first.ToString() + ": " + pair.second.ToString());
            }
          ss.str(ss.str() + "}");
        }
        break;
      case CborType::TAG:
        {
          auto tag = GetTag();
          ss.str(ss.str() + std::to_string(tag.first) + "(" + tag.second->ToString() + ")");
        }
        break;
      case CborType::SIMPLE:
        {
          auto simple = GetSimple();
          switch (simple)
            {
              case CborSimpleValue::FALSE:
                ss.str(ss.str() + "false");
                break;
              case CborSimpleValue::TRUE:
                ss.str(ss.str() + "true");
                break;
              case CborSimpleValue::NULL_VALUE:
                ss.str(ss.str() + "null");
                break;
              case CborSimpleValue::UNDEFINED:
                ss.str(ss.str() + "undefined");
                break;
              default:
                ss.str(ss.str() + "simple(" + std::to_string(static_cast<int>(simple)) + ")");
                break;
            }
        }
        break;
      case CborType::INVALID:
        ss.str(ss.str() + "invalid");
        break;
    }
  
  return ss.str();
}

// Cbor implementation

Buffer 
Cbor::Encode(const CborValue& value)
{
  Buffer buffer;
  buffer.AddAtStart(1024); // Start with reasonable size, will be resized as needed
  Buffer::Iterator it = buffer.Begin();
  
  // Encode based on type
  switch (value.GetType())
    {
      case CborType::UNSIGNED_INTEGER:
        {
          uint64_t val = value.GetUnsignedInteger();
          EncodeUnsigned(buffer, 0, val);
        }
        break;
      case CborType::NEGATIVE_INTEGER:
        {
          int64_t val = value.GetNegativeInteger();
          // In CBOR, negative integers are encoded as -1-n
          EncodeUnsigned(buffer, 1, static_cast<uint64_t>(-1 - val));
        }
        break;
      case CborType::BYTE_STRING:
        {
          const auto& bytes = value.GetByteString();
          EncodeUnsigned(buffer, 2, bytes.size());
          it.Write(bytes.data(), bytes.size());
        }
        break;
      case CborType::TEXT_STRING:
        {
          const auto& text = value.GetTextString();
          EncodeUnsigned(buffer, 3, text.size());
          it.Write(reinterpret_cast<const uint8_t*>(text.data()), text.size());
        }
        break;
      case CborType::ARRAY:
        {
          const auto& array = value.GetArray();
          EncodeUnsigned(buffer, 4, array.size());
          for (const auto& item : array)
            {
              Buffer itemBuffer = Encode(item);
              // Append item to buffer
              // Note: In a real implementation, this needs proper buffer management
            }
        }
        break;
      case CborType::MAP:
        {
          const auto& map = value.GetMap();
          EncodeUnsigned(buffer, 5, map.size());
          for (const auto& pair : map)
            {
              Buffer keyBuffer = Encode(pair.first);
              Buffer valueBuffer = Encode(pair.second);
              // Append key and value to buffer
              // Note: In a real implementation, this needs proper buffer management
            }
        }
        break;
      case CborType::TAG:
        {
          auto tag = value.GetTag();
          EncodeUnsigned(buffer, 6, tag.first);
          Buffer valueBuffer = Encode(*tag.second);
          // Append tagged value to buffer
          // Note: In a real implementation, this needs proper buffer management
        }
        break;
      case CborType::SIMPLE:
        {
          auto simple = value.GetSimple();
          uint8_t val = static_cast<uint8_t>(simple);
          if (val <= 23)
            {
              it.WriteU8(EncodeHeader(7, val));
            }
          else
            {
              it.WriteU8(EncodeHeader(7, 24));
              it.WriteU8(val);
            }
        }
        break;
      case CborType::INVALID:
        // Don't encode invalid values
        break;
    }
  
  return buffer;
}

std::optional<Cbor::CborValue> 
Cbor::Decode(Buffer buffer)
{
  // In a real implementation, this would parse CBOR data from the buffer
  // This is a simplified placeholder
  
  if (buffer.GetSize() == 0)
    {
      return std::nullopt;
    }
  
  Buffer::Iterator it = buffer.Begin();
  uint8_t header = it.ReadU8();
  
  uint8_t majorType, additionalInfo;
  DecodeHeader(header, majorType, additionalInfo);
  
  size_t bytesRead = 1;
  
  switch (majorType)
    {
      case 0: // Unsigned integer
        {
          auto unsignedOpt = DecodeUnsigned(buffer, majorType, bytesRead);
          if (!unsignedOpt)
            {
              return std::nullopt;
            }
          return CborValue(*unsignedOpt);
        }
      case 1: // Negative integer
        {
          auto unsignedOpt = DecodeUnsigned(buffer, majorType, bytesRead);
          if (!unsignedOpt)
            {
              return std::nullopt;
            }
          return CborValue(static_cast<int64_t>(-1 - static_cast<int64_t>(*unsignedOpt)));
        }
      // Additional cases for other types would be implemented similarly
    }
  
  return std::nullopt;
}

uint8_t 
Cbor::EncodeHeader(uint8_t majorType, uint8_t additionalInfo)
{
  return (majorType << 5) | (additionalInfo & 0x1F);
}

void 
Cbor::DecodeHeader(uint8_t header, uint8_t& majorType, uint8_t& additionalInfo)
{
  majorType = (header >> 5) & 0x07;
  additionalInfo = header & 0x1F;
}

void 
Cbor::EncodeUnsigned(Buffer& buffer, uint8_t majorType, uint64_t value)
{
  Buffer::Iterator it = buffer.Begin();
  
  if (value <= 23)
    {
      it.WriteU8(EncodeHeader(majorType, static_cast<uint8_t>(value)));
    }
  else if (value <= 0xFF)
    {
      it.WriteU8(EncodeHeader(majorType, 24));
      it.WriteU8(static_cast<uint8_t>(value));
    }
  else if (value <= 0xFFFF)
    {
      it.WriteU8(EncodeHeader(majorType, 25));
      it.WriteHtonU16(static_cast<uint16_t>(value));
    }
  else if (value <= 0xFFFFFFFF)
    {
      it.WriteU8(EncodeHeader(majorType, 26));
      it.WriteHtonU32(static_cast<uint32_t>(value));
    }
  else
    {
      it.WriteU8(EncodeHeader(majorType, 27));
      it.WriteHtonU64(value);
    }
}

std::optional<uint64_t> 
Cbor::DecodeUnsigned(Buffer& buffer, uint8_t majorType, size_t& bytesRead)
{
  Buffer::Iterator it = buffer.Begin();
  
  if (buffer.GetSize() < 1)
    {
      return std::nullopt;
    }
  
  uint8_t header = it.ReadU8();
  bytesRead = 1;
  
  uint8_t readMajorType, additionalInfo;
  DecodeHeader(header, readMajorType, additionalInfo);
  
  if (readMajorType != majorType)
    {
      return std::nullopt;
    }
  
  if (additionalInfo <= 23)
    {
      return static_cast<uint64_t>(additionalInfo);
    }
  else if (additionalInfo == 24)
    {
      if (buffer.GetSize() < 2)
        {
          return std::nullopt;
        }
      uint8_t value = it.ReadU8();
      bytesRead = 2;
      return static_cast<uint64_t>(value);
    }
  else if (additionalInfo == 25)
    {
      if (buffer.GetSize() < 3)
        {
          return std::nullopt;
        }
      uint16_t value = it.ReadNtohU16();
      bytesRead = 3;
      return static_cast<uint64_t>(value);
    }
  else if (additionalInfo == 26)
    {
      if (buffer.GetSize() < 5)
        {
          return std::nullopt;
        }
      uint32_t value = it.ReadNtohU32();
      bytesRead = 5;
      return static_cast<uint64_t>(value);
    }
  else if (additionalInfo == 27)
    {
      if (buffer.GetSize() < 9)
        {
          return std::nullopt;
        }
      uint64_t value = it.ReadNtohU64();
      bytesRead = 9;
      return value;
    }
  
  return std::nullopt;
}

} // namespace dtn7

} // namespace ns3