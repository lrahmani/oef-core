//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch.AI Limited
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
#include "server.hpp"
#include <iostream>
#include <google/protobuf/text_format.h>
#include <sstream>
#include <iomanip>

namespace fetch {
  namespace oef {
    fetch::oef::Logger Server::logger = fetch::oef::Logger("oef-node");
    fetch::oef::Logger AgentDirectory::logger = fetch::oef::Logger("agent-directory");
    
    std::string to_string(const google::protobuf::Message &msg) {
      std::string output;
      google::protobuf::TextFormat::PrintToString(msg, &output);
      return output;
    }
    class AgentSession : public std::enable_shared_from_this<AgentSession>
    {
    private:
      const std::string publicKey_;
      stde::optional<Instance> description_;
      AgentDirectory &agentDirectory_;
      ServiceDirectory &serviceDirectory_;
      tcp::socket socket_;
      OefSearch &oef_search_;
      SearchEngineCom &oef_search_new_;

      static fetch::oef::Logger logger;
      
    public:
      explicit AgentSession(std::string publicKey, AgentDirectory &agentDirectory, ServiceDirectory &serviceDirectory, tcp::socket socket, OefSearch& oef_search, SearchEngineCom& oef_search_new)
        : publicKey_{std::move(publicKey)}, agentDirectory_{agentDirectory}, serviceDirectory_{serviceDirectory}, socket_(std::move(socket)), oef_search_(oef_search), oef_search_new_(oef_search_new) {}
      virtual ~AgentSession() {
        logger.trace("~AgentSession");
        //socket_.shutdown(asio::socket_base::shutdown_both);
      }
      AgentSession(const AgentSession &) = delete;
      AgentSession operator=(const AgentSession &) = delete;
      void start() {
        read();
      }
      void start_pluto() {
        read_pluto();
      }
      void write(std::shared_ptr<Buffer> buffer) {
        asyncWriteBuffer(socket_, std::move(buffer), 5);
      }
      void send(const fetch::oef::pb::Server_AgentMessage &msg) {
        asyncWriteBuffer(socket_, serialize(msg), 10 /* sec ? */);
      }
      void query_oef_search(std::shared_ptr<Buffer> query_buffer, 
          std::function<void(std::error_code, std::shared_ptr<Buffer>)> process_answer) {
        fetch::oef::pb::Agent_Server_ID id_msg;
        id_msg.set_public_key(publicKey_);
        auto aid_buffer = serialize(id_msg);
        logger.trace("AgentSession::query_oef_search sending agent id ...");
        asyncWriteBuffer(oef_search_.socket_, std::move(aid_buffer), 5,
            [this,query_buffer,process_answer](std::error_code ec, std::size_t length){
              if(ec) {
                logger.error("AgentSession::query_oef_search error sending serialized public key {}", publicKey_);
                process_answer(ec, nullptr);
              } else {
                logger.trace("AgentSession::query_oef_search fwding agent operation ...");
                asyncWriteBuffer(oef_search_.socket_, std::move(query_buffer), 5, 
                    [this,process_answer](std::error_code ec, std::size_t length) {
                      if(ec){
                        logger.error("AgentSession::query_oef_search error fwding agent operation");
                        process_answer(ec, nullptr);
                      } else {
                        logger.trace("AgentSession::query_oef_search getting oef search answer ...");
                        asyncReadBuffer(oef_search_.socket_, 10, process_answer);
                      }
                });
              }
            });
      }
      void update_oef_search(std::shared_ptr<Buffer> update_buffer, 
          std::function<void(std::error_code, std::size_t length)> err_handler) {
        fetch::oef::pb::Agent_Server_ID id_msg;
        id_msg.set_public_key(publicKey_);
        auto aid_buffer = serialize(id_msg);
        logger.trace("AgentSession::update_oef_search sending agent id ...");
        asyncWriteBuffer(oef_search_.socket_, std::move(aid_buffer), 5,
            [this,update_buffer,err_handler](std::error_code ec, std::size_t length){
              if(ec) {
                logger.error("AgentSession::update_oef_search error sending serialized public key {}", publicKey_);
                err_handler(ec, 0);
              } else {
                logger.trace("AgentSession::update_oef_search fwding agent operation ...");
                asyncWriteBuffer(oef_search_.socket_, std::move(update_buffer), 5, 
                    [this,err_handler](std::error_code ec, std::size_t length) {
                      if(ec){
                        logger.error("AgentSession::update_oef_search error fwding agent operation");
                        err_handler(ec, 0);
                      }
                });
              }
            });
      }
      std::string id() const { return publicKey_; }
      bool match(const QueryModel &query) const {
        if(!description_) {
          return false;
        }
        return query.check(*description_);
      }
    private:
      void processRegisterDescription(uint32_t msg_id, const fetch::oef::pb::AgentDescription &desc) {
        description_ = Instance(desc.description());
        DEBUG(logger, "AgentSession::processRegisterDescription setting description to agent {} : {}", publicKey_, to_string(desc));
        if(!description_) {
          fetch::oef::pb::Server_AgentMessage answer;
          answer.set_answer_id(msg_id);
          auto *error = answer.mutable_oef_error();
          error->set_operation(fetch::oef::pb::Server_AgentMessage_OEFError::REGISTER_DESCRIPTION);
          logger.trace("AgentSession::processRegisterDescription sending error {} to {}", error->operation(), publicKey_);
          send(answer);
        }
      }
      void processUnregisterDescription(uint32_t msg_id) {
        description_ = stde::nullopt;
        DEBUG(logger, "AgentSession::processUnregisterDescription setting description to agent {}", publicKey_);
      }
      void processRegisterService(uint32_t msg_id, const fetch::oef::pb::AgentDescription &desc) {
        DEBUG(logger, "AgentSession::processRegisterService registering agent {} : {}", publicKey_, to_string(desc));
        bool success = serviceDirectory_.registerAgent(Instance(desc.description()), publicKey_);
        if(!success) {
          fetch::oef::pb::Server_AgentMessage answer;
          answer.set_answer_id(msg_id);
          auto *error = answer.mutable_oef_error();
          error->set_operation(fetch::oef::pb::Server_AgentMessage_OEFError::REGISTER_SERVICE);
          logger.trace("AgentSession::processRegisterService sending error {} to {}", error->operation(), publicKey_);
          send(answer);
        }
      }
      void processUnregisterService(uint32_t msg_id, const fetch::oef::pb::AgentDescription &desc) {
        DEBUG(logger, "AgentSession::processUnregisterService unregistering agent {} : {}", publicKey_, to_string(desc));
        bool success = serviceDirectory_.unregisterAgent(Instance(desc.description()), publicKey_);
        if(!success) {
          fetch::oef::pb::Server_AgentMessage answer;
          answer.set_answer_id(msg_id);
          auto *error = answer.mutable_oef_error();
          error->set_operation(fetch::oef::pb::Server_AgentMessage_OEFError::UNREGISTER_SERVICE);
          logger.trace("AgentSession::processUnregisterService sending error {} to {}", error->operation(), publicKey_);
          send(answer);
        }
      }
      void processSearchAgents(uint32_t msg_id, const fetch::oef::pb::AgentSearch &search) {
        QueryModel model{search.query()};
        DEBUG(logger, "AgentSession::processSearchAgents from agent {} : {}", publicKey_, to_string(search));
        auto agents_vec = agentDirectory_.search(model);
        fetch::oef::pb::Server_AgentMessage answer;
        answer.set_answer_id(msg_id);
        auto agents = answer.mutable_agents();
        for(auto &a : agents_vec) {
          agents->add_agents(a);
        }
        logger.trace("AgentSession::processSearchAgents sending {} agents to {}", agents_vec.size(), publicKey_);
        send(answer);
      }
      void processQuery(uint32_t msg_id, const fetch::oef::pb::AgentSearch &search) {
        QueryModel model{search.query()};
        DEBUG(logger, "AgentSession::processQuery from agent {} : {}", publicKey_, to_string(search));
        auto agents_vec = serviceDirectory_.query(model);
        fetch::oef::pb::Server_AgentMessage answer;
        answer.set_answer_id(msg_id);
        auto agents = answer.mutable_agents();
        for(auto &a : agents_vec) {
          agents->add_agents(a);
        }
        logger.trace("AgentSession::processQuery sending {} agents to {}", agents_vec.size(), publicKey_);
        send(answer);
      }
      void sendDialogError(uint32_t msg_id, uint32_t dialogue_id, const std::string &origin) {
        fetch::oef::pb::Server_AgentMessage answer;
        answer.set_answer_id(msg_id);
        auto *error = answer.mutable_dialogue_error();
        error->set_dialogue_id(dialogue_id);
        error->set_origin(origin);
        logger.trace("AgentSession::processMessage sending dialogue error {} to {}", dialogue_id, publicKey_);
        send(answer);
      }
      void processMessage(uint32_t msg_id, fetch::oef::pb::Agent_Message *msg) {
        auto session = agentDirectory_.session(msg->destination());
        DEBUG(logger, "AgentSession::processMessage from agent {} : {}", publicKey_, to_string(*msg));
        logger.trace("AgentSession::processMessage to {} from {}", msg->destination(), publicKey_);
        uint32_t did = msg->dialogue_id();
        if(session) {
          fetch::oef::pb::Server_AgentMessage message;
          message.set_answer_id(msg_id);
          auto content = message.mutable_content();
          content->set_dialogue_id(did);
          content->set_origin(publicKey_);
          if(msg->has_content()) {
            content->set_allocated_content(msg->release_content());
          }
          if(msg->has_fipa()) {
            content->set_allocated_fipa(msg->release_fipa());
          }
          DEBUG(logger, "AgentSession::processMessage to agent {} : {}", msg->destination(), to_string(message));
          auto buffer = serialize(message);
          asyncWriteBuffer(session->socket_, buffer, 5, [this,did,msg_id,msg](std::error_code ec, std::size_t length) {
              if(ec) {
                sendDialogError(msg_id, did, msg->destination());
              }
            });
        } else {
          sendDialogError(msg_id, did, msg->destination());
        }
      }
      void process(const std::shared_ptr<Buffer> &buffer) {
        auto envelope = deserialize<fetch::oef::pb::Envelope>(*buffer);
        auto payload_case = envelope.payload_case();
        uint32_t msg_id = envelope.msg_id();
        switch(payload_case) {
        case fetch::oef::pb::Envelope::kSendMessage:
          processMessage(msg_id, envelope.release_send_message());
          break;
        case fetch::oef::pb::Envelope::kRegisterService:
          processRegisterService(msg_id, envelope.register_service());
          break;
        case fetch::oef::pb::Envelope::kUnregisterService:
          processUnregisterService(msg_id, envelope.unregister_service());
          break;
        case fetch::oef::pb::Envelope::kRegisterDescription:
          processRegisterDescription(msg_id, envelope.register_description());
          break;
        case fetch::oef::pb::Envelope::kUnregisterDescription:
          processUnregisterDescription(msg_id);
          break;
        case fetch::oef::pb::Envelope::kSearchAgents:
          processSearchAgents(msg_id, envelope.search_agents());
          break;
        case fetch::oef::pb::Envelope::kSearchServices:
          processQuery(msg_id, envelope.search_services());
          break;
        case fetch::oef::pb::Envelope::PAYLOAD_NOT_SET:
          logger.error("AgentSession::process cannot process payload {} from {}", payload_case, publicKey_);
        }
      }
      void process_pluto(const std::shared_ptr<Buffer> &buffer) {
        auto envelope = deserialize<fetch::oef::pb::Envelope>(*buffer);
        auto payload_case = envelope.payload_case();
        uint32_t msg_id = envelope.msg_id();
        switch(payload_case) {
        case fetch::oef::pb::Envelope::kSendMessage:
          processMessage(msg_id, envelope.release_send_message());
          break;
        case fetch::oef::pb::Envelope::kRegisterService:
        case fetch::oef::pb::Envelope::kUnregisterService:
        case fetch::oef::pb::Envelope::kRegisterDescription:
        case fetch::oef::pb::Envelope::kUnregisterDescription:
          update_oef_search(buffer, [this,msg_id](std::error_code ec, std::size_t length) {
            if(ec) {
              logger.error("AgentSession::process_pluto oef search error on agent msg {}", msg_id);
              fetch::oef::pb::Server_AgentMessage answer;
              answer.set_answer_id(msg_id);
              auto *error = answer.mutable_oef_error();
              //TOFIX get exact type of message 
              error->set_operation(fetch::oef::pb::Server_AgentMessage_OEFError::REGISTER_SERVICE); 
              send(answer);
            }
            });
          break;
        case fetch::oef::pb::Envelope::kSearchAgents:
        case fetch::oef::pb::Envelope::kSearchServices:
          query_oef_search(buffer, [this,msg_id](std::error_code ec, std::shared_ptr<Buffer> answer_buffer) {
            if(ec) {
              logger.error("AgentSession::process_pluto oef search error on agent msg {}", msg_id);
              fetch::oef::pb::Server_AgentMessage answer;
              answer.set_answer_id(msg_id);
              auto *error = answer.mutable_oef_error();
              //TOFIX get exact type of message 
              error->set_operation(fetch::oef::pb::Server_AgentMessage_OEFError::REGISTER_SERVICE); 
              send(answer);
            } else {
              // TOFIX core expects Server.AgentMessage from oef search
              auto answer = deserialize<fetch::oef::pb::Server_AgentMessage>(*answer_buffer);
              send(answer);
            }
            });
          break;
        case fetch::oef::pb::Envelope::PAYLOAD_NOT_SET:
          logger.error("AgentSession::process cannot process payload {} from {}", payload_case, publicKey_);
        }
      }
      void process_pluto_new(const std::shared_ptr<Buffer> &buffer) {
        auto envelope = deserialize<fetch::oef::pb::Envelope>(*buffer);
        auto payload_case = envelope.payload_case();
        uint32_t msg_id = envelope.msg_id();
        switch(payload_case) {
        case fetch::oef::pb::Envelope::kSendMessage:
          processMessage(msg_id, envelope.release_send_message());
          break;
        case fetch::oef::pb::Envelope::kRegisterService:
          oef_search_new_.RegisterServiceDescription(publicKey_, Instance(envelope.register_service().description()));
          // TOFIX API: change Instance to Query.Instance? to AgentDescription?
        case fetch::oef::pb::Envelope::kUnregisterService:
        case fetch::oef::pb::Envelope::kRegisterDescription:
        case fetch::oef::pb::Envelope::kUnregisterDescription:
        case fetch::oef::pb::Envelope::kSearchAgents:
        case fetch::oef::pb::Envelope::kSearchServices:
          break;
        case fetch::oef::pb::Envelope::PAYLOAD_NOT_SET:
          logger.error("AgentSession::process cannot process payload {} from {}", payload_case, publicKey_);
        }
      }
      void read_pluto() {
        auto self(shared_from_this());
        asyncReadBuffer(socket_, 5, [this, self](std::error_code ec, std::shared_ptr<Buffer> buffer) {
                                if(ec) {
                                  agentDirectory_.remove(publicKey_);
                                  serviceDirectory_.unregisterAll(publicKey_); // TORM
                                  logger.info("AgentSession::read error on id {} ec {}", publicKey_, ec);
                                } else {
                                  process_pluto_new(buffer);
                                  read_pluto();
                                }});
      }
      void read() {
        auto self(shared_from_this());
        asyncReadBuffer(socket_, 5, [this, self](std::error_code ec, std::shared_ptr<Buffer> buffer) {
                                if(ec) {
                                  agentDirectory_.remove(publicKey_);
                                  serviceDirectory_.unregisterAll(publicKey_);
                                  logger.info("AgentSession::read error on id {} ec {}", publicKey_, ec);
                                } else {
                                  process(buffer);
                                  read();
                                }});
      }
      
    };
    fetch::oef::Logger AgentSession::logger = fetch::oef::Logger("oef-node::agent-session");

    const std::vector<std::string> AgentDirectory::search(const QueryModel &query) const {
      std::lock_guard<std::mutex> lock(lock_);
      std::vector<std::string> res;
      for(const auto &s : sessions_) {
        if(s.second->match(query)) {
          res.emplace_back(s.first);
        }
      }
      return res;
    }

    void Server::secretHandshake(const std::string &publicKey, const std::shared_ptr<Context> &context) {
      fetch::oef::pb::Server_Phrase phrase;
      phrase.set_phrase("RandomlyGeneratedString");
      auto phrase_buffer = serialize(phrase);
      logger.trace("Server::secretHandshake sending phrase size {}", phrase_buffer->size());
      asyncWriteBuffer(context->socket_, phrase_buffer, 10 /* sec ? */);
      logger.trace("Server::secretHandshake waiting answer");
      asyncReadBuffer(context->socket_, 5,
                      [this,publicKey,context](std::error_code ec, std::shared_ptr<Buffer> buffer) {
                        if(ec) {
                          logger.error("Server::secretHandshake read failure {}", ec.value());
                        } else {
                          try {
                            auto ans = deserialize<fetch::oef::pb::Agent_Server_Answer>(*buffer);
                            logger.trace("Server::secretHandshake secret [{}]", ans.answer());
                            auto session = std::make_shared<AgentSession>(publicKey, agentDirectory_, serviceDirectory_, std::move(context->socket_), oef_search_, oef_search_new_);
                            if(agentDirectory_.add(publicKey, session)) {
                              session->start_pluto();
                              fetch::oef::pb::Server_Connected status;
                              status.set_status(true);
                              session->write(serialize(status));
                            } else {
                              fetch::oef::pb::Server_Connected status;
                              status.set_status(false);
                              logger.info("Server::secretHandshake PublicKey already connected (interleaved) publicKey {}", publicKey);
                              asyncWriteBuffer(context->socket_, serialize(status), 10 /* sec ? */);
                            }
                            // should check the secret with the public key i.e. ID.
                          } catch(std::exception &) {
                            logger.error("Server::secretHandshake error on Answer publicKey {}", publicKey);
                            fetch::oef::pb::Server_Connected status;
                            status.set_status(false);
                            asyncWriteBuffer(context->socket_, serialize(status), 10 /* sec ? */);
                          }
                          // everything is fine -> send connection OK.
                        }
                      });
    }
    void Server::newSession(tcp::socket socket) {
      auto context = std::make_shared<Context>(std::move(socket));
      asyncReadBuffer(context->socket_, 5,
                      [this,context](std::error_code ec, std::shared_ptr<Buffer> buffer) {
                        if(ec) {
                          logger.error("Server::newSession read failure {}", ec.value());
                        } else {
                          try {
                            logger.trace("Server::newSession received {} bytes", buffer->size());
                            auto id = deserialize<fetch::oef::pb::Agent_Server_ID>(*buffer);
                            logger.trace("Debug {}", to_string(id));
                            logger.trace("Server::newSession connection from {}", id.public_key());
                            if(!agentDirectory_.exist(id.public_key())) { // not yet connected
                              secretHandshake(id.public_key(), context);
                            } else {
                              logger.info("Server::newSession ID {} already connected", id.public_key());
                              fetch::oef::pb::Server_Phrase failure;
                              (void)failure.mutable_failure();
                              asyncWriteBuffer(context->socket_, serialize(failure), 10 /* sec ? */);
                            }
                          } catch(std::exception &) {
                            logger.error("Server::newSession error parsing ID");
                            fetch::oef::pb::Server_Phrase failure;
                            (void)failure.mutable_failure();
                            asyncWriteBuffer(context->socket_, serialize(failure), 10 /* sec ? */);
                          }
                        }
                      });
    }
    void Server::do_accept() {
      logger.trace("Server::do_accept");
      acceptor_.async_accept([this](std::error_code ec, tcp::socket socket) {
                               if (!ec) {
                                 logger.trace("Server::do_accept starting new session");
                                 newSession(std::move(socket));
                                 do_accept();
                               } else {
                                 logger.error("Server::do_accept error {}", ec.value());
                               }
                             });
    }
    Server::~Server() {
      logger.trace("~Server stopping");
      stop();
      logger.trace("~Server stopped");
      agentDirectory_.clear();
      //    acceptor_.close();
      logger.trace("~Server waiting for threads");
      for(auto &t : threads_) {
        if(t) {
          t->join();
        }
      }
      logger.trace("~Server threads stopped");
    }
    void Server::run() {
      for(auto &t : threads_) {
        if(!t) {
          t = std::make_unique<std::thread>([this]() {do_accept(); io_context_.run();});
        }
      }
    }
    void Server::run_in_thread() {
      do_accept();
      io_context_.run();
    }
    void Server::stop() {
      std::this_thread::sleep_for(std::chrono::seconds{1});
      io_context_.stop();
    }
  }
}
