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

#include "api/continuation_t.hpp"

namespace fetch {
namespace oef {
  struct MsgHandle {
    explicit MsgHandle(){}
    explicit MsgHandle(uint32_t msg_id) 
      : operation{""}
      , continuation{[msg_id](std::error_code, uint32_t length, std::vector<std::string> agents, pb::Server_SearchResultWide){std::cerr << "No handle registered for message " << msg_id << std::endl;}}
    {}
    explicit MsgHandle(std::string op, AgentSessionContinuation cont)
      : operation{op}, continuation{cont}
    {}
    //
    std::string operation;
    AgentSessionContinuation continuation;
  };
  
} //oef
} //fetch