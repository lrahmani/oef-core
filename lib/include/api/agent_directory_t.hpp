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

#include "api/agent_session_t.hpp"

#include "schema.hpp" // TOFIX

#include <memory>
#include <vector>

namespace fetch {
namespace oef {
    /* 
     * Defines API for Agent Directory object.
     * Agent Directory is responsible for keeping track of connected agents and their Agent Sessions
     * Agent Directory is owned by the Core Server.
     */
    class agent_directory_t {
    public:
      /* Add a new agent 
       * params:
       *   - [in] agent_id: new agent public key
       *   - [in] session: new agent Agent Session object (object responsible for communicating with agent `agent_id`)
       */   
      virtual bool add(const std::string& agent_id, std::shared_ptr<agent_session_t> session) = 0;
      /* Test if `agent_id` agent already exists (is connected) */
      virtual bool exist(const std::string& agent_id) const = 0;
      /* Remove `agent_id` agent from directory */
      virtual bool remove(const std::string& agent_id) = 0;
      /* Get `agent_id` session */
      virtual std::shared_ptr<agent_session_t> session(const std::string& agent_id) const = 0; // not sure about this operation
      /* Get the number of connected agents */
      virtual size_t size() const = 0;
      /* Reset Agent Directory */
      virtual void clear() = 0;
      
      virtual ~agent_directory_t() {}
    };
} // oef
} // fetch

