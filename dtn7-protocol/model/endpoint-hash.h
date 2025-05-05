#ifndef DTN7_ENDPOINT_HASH_H
#define DTN7_ENDPOINT_HASH_H

#include "endpoint.h"
#include <functional>

namespace std {
  // 为 EndpointID 提供哈希函数特化
  template<>
  struct hash<ns3::dtn7::EndpointID> {
    size_t operator()(const ns3::dtn7::EndpointID& eid) const {
      return std::hash<std::string>()(eid.ToString());
    }
  };
}

#endif /* DTN7_ENDPOINT_HASH_H */