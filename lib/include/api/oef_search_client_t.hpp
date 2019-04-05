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
        
        /* OEF Search operations, as specified in search.proto */
        virtual std::error_code register_description_sync(const std::string& agent, const Instance& desc) = 0;
        virtual std::error_code unregister_description_sync(const std::string& agent) = 0;
        virtual std::error_code register_service_sync(const Instance& service, const std::string& agent, uint32_t msg_id) = 0;
        virtual std::error_code unregister_service_sync(const std::string& agent, const Instance& service) = 0;
        virtual std::error_code search_agents_sync(const std::string& agent, const QueryModel& query, std::vector<agent_t>& agents) = 0; 
        virtual std::error_code search_service_sync(const QueryModel& query, const std::string& agent, uint32_t msg_id, std::vector<agent_t>& agents) = 0;
        
        virtual ~oef_search_client_t() {}
    };
    
    struct agent_t {
      std::string id;
      std::string core_ip_addr;
    };
} // oef
} // fetch

