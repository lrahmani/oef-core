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
#include "api/communicator_t.hpp"

#include "agent_directory.hpp"
#include "oef_search_client.hpp"
#include "asio_communicator.hpp"
#include "serialization.hpp"
#include "logger.hpp"

#include "agent.pb.h" // TOFIX

namespace fetch {
namespace oef {
    class AgentSession : public agent_session_t, public std::enable_shared_from_this<AgentSession> {
    private:
      const std::string publicKey_;
      stde::optional<Instance> description_;
      AgentDirectory &agentDirectory_;
      OefSearchClient& oef_search_; // could oef_search_client_t&
      std::shared_ptr<communicator_t> comm_;

      static fetch::oef::Logger logger;
    public:
      explicit AgentSession(
          std::string agent_id, std::shared_ptr<communicator_t> comm, 
          AgentDirectory& agentDirectory, 
          OefSearchClient& oef_search) 
        : publicKey_{std::move(agent_id)} 
        , agentDirectory_{agentDirectory}
        , oef_search_{oef_search} 
        , comm_{std::move(comm)}
      {}
      
      AgentSession(const AgentSession &) = delete;
      AgentSession operator=(const AgentSession &) = delete;
      
      virtual ~AgentSession() {
        logger.trace("~AgentSession");
      }
      
      void start() override {
        read();
      }

      std::string agent_id() const override {
        return publicKey_;
      }
      
      void send(std::shared_ptr<Buffer> buffer) { // TOFIX needed to send status messages at handshake
        comm_->send_async(std::move(buffer));
      }
      void send(const fetch::oef::pb::Server_AgentMessage &msg) override {
        comm_->send_async(pbs::serialize(msg));
      }
      void send(const fetch::oef::pb::Server_AgentMessage& msg, LengthContinuation continuation) override {
        comm_->send_async(pbs::serialize(msg), continuation);
      }
      void send_error(uint32_t msg_id, fetch::oef::pb::Server_AgentMessage_OEFError_Operation error) override;

      bool match(const QueryModel &query) const {
        if(!description_) {
          return false;
        }
        return query.check(*description_);
      }
    private:
      void process_register_description(uint32_t msg_id, const fetch::oef::pb::AgentDescription &desc) override;
      void process_unregister_description(uint32_t msg_id) override;
      void process_register_service(uint32_t msg_id, const fetch::oef::pb::AgentDescription &desc) override;
      void process_unregister_service(uint32_t msg_id, const fetch::oef::pb::AgentDescription &desc) override;
      void process_search_agents(uint32_t msg_id, const fetch::oef::pb::AgentSearch &search) override;
      void process_search_service(uint32_t msg_id, const fetch::oef::pb::AgentSearch &search) override;
      void process_search_service_wide(uint32_t msg_id, const fetch::oef::pb::AgentSearch &search);
      void send_dialog_error(uint32_t msg_id, uint32_t dialogue_id, const std::string &origin) override;
      void process_message(uint32_t msg_id, fetch::oef::pb::Agent_Message *msg) override;
      void process(const std::shared_ptr<Buffer> &buffer) override;
      
      void read();
};

} //oef
} //fetch
