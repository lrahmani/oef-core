#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018-2019 Fetch.AI Limited
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.
//
//------------------------------------------------------------------------------

#include "interface/buffer_t.hpp"

namespace fetch {
namespace oef {
namespace serializer {

template <typename T>
T from_string(const std::string &s) {
  T t;
  t.ParseFromString(s);
  return t;
}

template <typename T>
std::shared_ptr<Buffer> serialize(const T &t) {
  size_t size = t.ByteSize();
  Buffer data;
  data.resize(size);
  (void)t.SerializeWithCachedSizesToArray(data.data());
  return std::make_shared<Buffer>(data);
}

template <typename T>
T deserialize(const Buffer &buffer) {
  T t;
  t.ParseFromArray(buffer.data(), buffer.size());
  return t;
}

} // serializer
} //oef
} //fetch

enum class Ports {
  ServiceDiscovery = 2222, Agents = 3333, OEFSearch = 7501
};

