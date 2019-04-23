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
#include "api/oef_search_response_t.hpp"

namespace fetch {
namespace oef {
  struct MsgHandle {
    explicit MsgHandle(){}
    explicit MsgHandle(uint32_t smsg_id) 
      : operation{""}
      , continuation{[smsg_id](std::error_code ec, oef::OefSearchResponse response) -> void {
                       std::cerr << "No handle registered for message " << smsg_id << std::endl;
                     }}
      , amsg_id{0}
      , agent_id{"NoOne"}
    {}
    explicit MsgHandle(std::string op, AgentSessionContinuation cont)
      : operation{op}, continuation{cont}
    {}
    explicit MsgHandle(std::string op, AgentSessionContinuation cont, 
        uint32_t amsg_id, const std::string& agent = "")
      : operation{op}, continuation{cont}, amsg_id{amsg_id}, agent_id{agent}
    {}
    //
    std::string operation;
    AgentSessionContinuation continuation;
    // not needed, only for debug
    uint32_t amsg_id;
    std::string agent_id;
  };
  
} //oef
} //fetch
