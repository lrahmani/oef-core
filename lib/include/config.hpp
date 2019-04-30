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

#include <string>
#include <cstdint>

namespace fetch {
namespace oef {
namespace config {

constexpr auto default_ip{"127.0.0.1"};
constexpr uint32_t core_default_backlog{256};
constexpr uint32_t core_default_nb_threads{4};

enum class Ports {
  ServiceDiscovery = 2222, Agents = 3333, Search = 7501
};

} // config
} //oef
} //fetch


