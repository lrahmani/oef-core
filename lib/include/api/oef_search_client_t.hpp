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

#include "api/communicator_t.hpp"
#include "api/buffer_t.hpp"
#include "api/continuation_t.hpp"

#include "schema.hpp" // TOFIX

#include <memory>
#include <system_error>

namespace fetch {
namespace oef {
    /* 
     * Defines API for Oef Search Client object.
     * Oef Search Client is responsible for mainting a connection with the OEF Search and 
     * exchanging messages on behalf of Core Server.
     * It is created and owned by Core Server. 
     * Agent Sessions have a reference to it, to forward agents requests to OEF Search.
     * Interactions between Oef Seach Client and the OEF Search are governed by search.proto contract file
     */
    struct agent_t;
    class oef_search_client_t {
    public:
        /* Open a client connection to the OEF Search */
        virtual void connect() = 0;
        
        /* OEF Search operations, as specified in search_*.proto files */
        virtual void register_description(const Instance& desc, const std::string& agent, uint32_t msg_id, AgentSessionContinuation continuation) = 0;
        virtual void unregister_description(const Instance& desc, const std::string& agent, uint32_t msg_id, AgentSessionContinuation continuation) = 0;
        virtual void register_service(const Instance& service, const std::string& agent, uint32_t msg_id, AgentSessionContinuation continuation) = 0;
        virtual void unregister_service(const Instance& service, const std::string& agent, uint32_t msg_id, AgentSessionContinuation continuation) = 0;
        virtual void search_agents(const QueryModel& query, const std::string& agent, uint32_t msg_id, AgentSessionContinuation continuation) = 0;
        virtual void search_service(const QueryModel& query, const std::string& agent, uint32_t msg_id, AgentSessionContinuation continuation) = 0;
        virtual void search_service_wide(const QueryModel& query, const std::string& agent, uint32_t msg_id, AgentSessionContinuation continuation) = 0;
        
        virtual ~oef_search_client_t() {}
    };
    
    struct agent_t {
      std::string id;
      std::string core_ip_addr;
    };
} // oef
} // fetch

