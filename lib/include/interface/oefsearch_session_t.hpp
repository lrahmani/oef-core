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

#include <memory>
#include "interface/communicator_t.hpp"
#include "interface/buffer_t.hpp"
#include "schema.hpp" // TOFIX

namespace fetch {
  namespace oef {
    //
    struct agent_t {
      std::string id;
      std::string core_ip_addr;
    };
    //
    class oefsearch_session_t {
      private:
        //
        std::string ip_addr_;
        uint32_t port_;
        //
        communicator_t comm_;
        //
        virtual void register_service_description(const std::string& agent, const Instance& instance) = 0;
        virtual std::vector<agent_t> query_search_service_sync(const QueryModel& query) = 0; 
      public:
        //
        virtual void connect() = 0;
        //
        virtual ~oefsearch_session_t() {}
    };
  } // oef
} // fetch

