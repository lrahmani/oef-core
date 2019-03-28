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
#include "agent.pb.h" // TOFIX
#include "schema.hpp" // TOFIX

namespace fetch {
  namespace oef {
    class agent_session_t {
      private:
        //
        std::shared_ptr<communicator_t> comm_; // TOFIX shouldn't it be unique_ptr?
        //
        virtual void process_register_description(uint32_t msg_id, const fetch::oef::pb::AgentDescription &desc) = 0;
        virtual void process_unregister_description(uint32_t msg_id) = 0;
        virtual void process_register_service(uint32_t msg_id, const fetch::oef::pb::AgentDescription &desc) = 0;
        virtual void process_unregister_service(uint32_t msg_id, const fetch::oef::pb::AgentDescription &desc) = 0;
        virtual void process_search_agents(uint32_t msg_id, const fetch::oef::pb::AgentSearch &search) = 0;
        virtual void process_search_service(uint32_t msg_id, const fetch::oef::pb::AgentSearch &search) = 0;
        virtual void process_message(uint32_t msg_id, fetch::oef::pb::Agent_Message *msg) = 0;
        virtual void send_dialog_error(uint32_t msg_id, uint32_t dialogue_id, const std::string &origin) = 0;
        virtual void send_error(uint32_t msg_id, fetch::oef::pb::Server_AgentMessage_OEFError_Operation error) = 0;
        virtual void process(const std::shared_ptr<Buffer> &buffer) = 0;
      public:
        //
        virtual void start() = 0;
        virtual std::string agent_id() const = 0;
        //
        virtual void send(const fetch::oef::pb::Server_AgentMessage& msg, 
                          std::function<void(std::error_code,std::size_t)> continuation) = 0;
        virtual void send(const fetch::oef::pb::Server_AgentMessage& msg) = 0;
        virtual void send(std::shared_ptr<Buffer> buffer) = 0; // TOFIX
        //
        virtual bool match(const QueryModel &query) const = 0;
        //
        virtual ~agent_session_t() {}
    };
  } // oef
} // fetch

