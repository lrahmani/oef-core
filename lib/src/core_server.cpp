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

#define DEBUG_ON 1
#include "core_server.hpp"
#include "agent_session.hpp"

namespace fetch {
namespace oef {
    fetch::oef::Logger CoreServer::logger = fetch::oef::Logger("oef-node");
    
    void CoreServer::run() {
      for(auto &t : threads_) {
        if(!t) {
          t = std::make_unique<std::thread>([this]() {do_accept(); io_context_.run();});
        }
      }
    }

    void CoreServer::run_in_thread() {
      do_accept();
      io_context_.run();
    }

    void CoreServer::do_accept() {
      logger.trace("CoreServer::do_accept (port {})", core_port_);
      
      do_accept([this](std::error_code ec, std::shared_ptr<communicator_t> comm) {
                               if (!ec) {
                                 logger.trace("CoreServer::do_accept starting new session");
                                 newSession(std::move(comm));
                                 do_accept();
                               } else {
                                 logger.error("CoreServer::do_accept error {}", ec.value());
                               }
                             });
    }

    void CoreServer::do_accept(CommunicatorContinuation continuation) {
      acceptor_.do_accept_async(continuation);
    }

    void CoreServer::newSession(std::shared_ptr<communicator_t> comm_agent) {
      comm_agent->receive_async(
          [this,comm_agent](std::error_code ec, std::shared_ptr<Buffer> buffer) {
            if(ec) {
              logger.error("CoreServer::newSession read failure {}", ec.value());
            } else {
              try {
                logger.trace("CoreServer::newSession received {} bytes", buffer->size());
                auto id = pbs::deserialize<fetch::oef::pb::Agent_Server_ID>(*buffer);
                logger.trace("Debug {}", pbs::to_string(id));
                logger.trace("CoreServer::newSession connection from {}", id.public_key());
                if(!agentDirectory_.exist(id.public_key())) { // not yet connected
                  secretHandshake(id.public_key(), comm_agent);
                } else {
                  logger.info("CoreServer::newSession ID {} already connected", id.public_key());
                  fetch::oef::pb::Server_Phrase failure;
                  (void)failure.mutable_failure();
                  comm_agent->send_async(pbs::serialize(failure));
                }
              } catch(std::exception &) {
                logger.error("CoreServer::newSession error parsing ID");
                fetch::oef::pb::Server_Phrase failure;
                (void)failure.mutable_failure();
                comm_agent->send_async(pbs::serialize(failure));
              }
            }
          });
    }

    void CoreServer::secretHandshake(const std::string &publicKey, std::shared_ptr<communicator_t> comm) {
      fetch::oef::pb::Server_Phrase phrase;
      phrase.set_phrase("RandomlyGeneratedString");
      auto phrase_buffer = pbs::serialize(phrase);
      logger.trace("CoreServer::secretHandshake sending phrase size {}", phrase_buffer->size());
      comm->send_async(phrase_buffer);
      logger.trace("CoreServer::secretHandshake waiting answer");
      comm->receive_async(
          [this,publicKey,comm](std::error_code ec, std::shared_ptr<Buffer> buffer) {
            if(ec) {
              logger.error("CoreServer::secretHandshake read failure {}", ec.value());
            } else {
              try {
                auto ans = pbs::deserialize<fetch::oef::pb::Agent_Server_Answer>(*buffer);
                logger.trace("CoreServer::secretHandshake secret [{}]", ans.answer());
                auto session = std::make_shared<AgentSession>(publicKey, std::move(comm), agentDirectory_, *oef_search_);
                if(agentDirectory_.add(publicKey, std::static_pointer_cast<agent_session_t>(session))) {
                // everything is fine -> send connection OK.
                  session->start();
                  fetch::oef::pb::Server_Connected status;
                  status.set_status(true);
                  session->send(pbs::serialize(status));
                } else {
                  fetch::oef::pb::Server_Connected status;
                  status.set_status(false);
                  logger.info("CoreServer::secretHandshake PublicKey already connected (interleaved) publicKey {}", publicKey);
                  session->send(pbs::serialize(status));
                }
              // should check the secret with the public key i.e. ID.
              } catch(std::exception &) {
                logger.error("CoreServer::secretHandshake error on Answer publicKey {}", publicKey);
                fetch::oef::pb::Server_Connected status;
                status.set_status(false);
                comm->send_async(pbs::serialize(status));
              }
            }
          });
    }
    
    void CoreServer::stop() {
      std::this_thread::sleep_for(std::chrono::seconds{1});
      io_context_.stop();
    }
    
    CoreServer::~CoreServer() {
      logger.trace("~CoreServer stopping");
      stop();
      logger.trace("~CoreServer stopped");
      agentDirectory_.clear();
      //    acceptor_.close();
      logger.trace("~CoreServer waiting for threads");
      for(auto &t : threads_) {
        if(t) {
          t->join();
        }
      }
      logger.trace("~CoreServer threads stopped");
    }
} // oef
} // fetch
