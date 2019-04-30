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
#include "api/agent_directory_t.hpp"
#include "api/agent_session_t.hpp"
#include "api/oef_search_client_t.hpp"
#include "api/communicator_t.hpp"

#include <memory>
#include <string>

namespace fetch {
namespace oef {      
    /* 
     * Defines API for Core Server object. 
     * Core Server is the main component of the OEF Core.
     * It should ensure the following:
     *   - open a listening socket for agents and correctly handle their connections
     *   - open a client socket to the OEF Search
     *   - initialize the rest of components 
     * Usage: run() or run_in_thread() methods are supposed to be called from main() function
     */
    class core_server_t {
    public:
      /* Run OEF Core services */
      virtual void run() = 0;
      /* Run OEF Core services on the caller execution thread */
      virtual void run_in_thread() = 0;
      /* Stop OEF Core services */
      virtual void stop() = 0;
      /* Get the number of agents connected to this OEF Core */
      virtual size_t nb_agents() const = 0;
      
      virtual ~core_server_t() {};
    private:
      /* Wait for new agents connections 
       * params: 
       *  - [in] continuation: callback function to handle Communicator created by a new (agent) connection request */
      virtual void do_accept(CommunicatorContinuation continuation) = 0;
      /* Once connectivety established, authenticate the agent 
       * params:
       *  - [in] communicator: newly created communicator */
      virtual void process_agent_connection(const std::shared_ptr<communicator_t> communicator) = 0; 
    };
} // oef
} // fetch

