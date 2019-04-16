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

#include "api/buffer_t.hpp"

#include <google/protobuf/text_format.h>

#include <iostream>
#include <sstream>

namespace fetch {
namespace oef {
namespace pbs {

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
T deserialize(const Buffer &buffer, bool& status) {
  T t;
  status = t.ParseFromArray(buffer.data(), buffer.size());
  return t;
}

template <typename T>
T deserialize(const Buffer &buffer) {
  T t;
  if(!t.ParseFromArray(buffer.data(), buffer.size()))
    std::cerr << "pbs::deserialize ParseFromArray returns false" << std::endl;
  return t;
}

template <typename T>
T deserialize(const std::string &buffer) {
  T t;
  if(!t.ParseFromArray(buffer.data(), buffer.size()))
    std::cerr << "pbs::deserialize ParseFromArray return false" << std::endl;
  return t;
}

std::string to_string(const google::protobuf::Message &msg);

std::string diagnostic(void *p, size_t sz);

} // pbs 

std::shared_ptr<Buffer> serialize(uint32_t size);

} //oef
} //fetch

