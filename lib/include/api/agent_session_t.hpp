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
#include "api/communicator_t.hpp"
#include "api/buffer_t.hpp"

#include "schema.hpp" // TOFIX

#include "agent.pb.h" // TOFIX

#include <memory>

namespace fetch {
namespace oef {
    /* 
     * Define API for Agent Session object.
     * Agent Session is responsible for:
     *   - mainting the connection with the agent it manages and answering its requests.
     *   - updating and querying the OEF Search on behalf if its agent, through the Oef Search Client
     * Each connected agent have one Agent Session created by Core Server and registred in
     * the Agent Directory.
     * Agent Session and agent interactions are governed by contracts in {agent,query,fipa}.proto files.
     * Agent Session and OEF Search interactions are governed by search.proto contract file
     */ 
    class agent_session_t {
    public:
        /* Start listing to agent messages */
        virtual void start() = 0;
        /* Get managed agent id (public key) */
        virtual std::string agent_id() const = 0;
        /* Send a message to managed agent 
         * params:
         *   - [in] msg: protobuf message, as defined in agent.proto file 
         *   - [in] continuation: callback function with error code and number of bytes sent */
        virtual void send(const fetch::oef::pb::Server_AgentMessage& msg, LengthContinuation continuation) = 0;
        /* Send a message to managed agent */
        virtual void send(const fetch::oef::pb::Server_AgentMessage& msg) = 0;
        virtual void send_error(uint32_t msg_id, fetch::oef::pb::Server_AgentMessage_OEFError_Operation error) = 0;
        
        virtual ~agent_session_t() {}
    private:
        /* OEF interface for agents, as specified in agent.proto file */
        virtual void process_register_description(uint32_t msg_id, const fetch::oef::pb::AgentDescription &desc) = 0;
        virtual void process_unregister_description(uint32_t msg_id) = 0;
        virtual void process_register_service(uint32_t msg_id, const fetch::oef::pb::AgentDescription &desc) = 0;
        virtual void process_unregister_service(uint32_t msg_id, const fetch::oef::pb::AgentDescription &desc) = 0;
        virtual void process_search_agents(uint32_t msg_id, const fetch::oef::pb::AgentSearch &search) = 0;
        virtual void process_search_service(uint32_t msg_id, const fetch::oef::pb::AgentSearch &search) = 0;
        virtual void process_message(uint32_t msg_id, fetch::oef::pb::Agent_Message *msg) = 0;
        virtual void send_dialog_error(uint32_t msg_id, uint32_t dialogue_id, const std::string &origin) = 0;
        /* Process received serialized data from agent */
        virtual void process(const std::shared_ptr<Buffer> &buffer) = 0;
    };
} // oef
} // fetch

