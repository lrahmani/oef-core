#define DEBUG_ON 1
#include "server.h"
#include <iostream>
#include <google/protobuf/text_format.h>
#include <sstream>
#include <iomanip>
#include "oefcoreproxy.hpp"
#include "multiclient.h"

namespace fetch {
  namespace oef {
    fetch::oef::Logger Server::logger = fetch::oef::Logger("oef-node");
    fetch::oef::Logger AgentDiscovery::logger = fetch::oef::Logger("agent-discovery");
    fetch::oef::Logger fetch::oef::OEFCoreNetworkProxy::logger = fetch::oef::Logger("oefcore-network");
    fetch::oef::Logger fetch::oef::OEFCoreLocalPB::logger = fetch::oef::Logger("oefcore-local-pb");
    fetch::oef::Logger fetch::oef::MessageDecoder::logger = fetch::oef::Logger("oefcore-pb");
    fetch::oef::Logger fetch::oef::SchedulerPB::logger = fetch::oef::Logger("oefcore-scheduler-pb");
    
    std::string to_string(const google::protobuf::Message &msg) {
      std::string output;
      google::protobuf::TextFormat::PrintToString(msg, &output);
      return output;
    }
    class AgentSession : public std::enable_shared_from_this<AgentSession>
    {
    private:
      const std::string _id;
      stde::optional<Instance> _description;
      AgentDiscovery &_ad;
      ServiceDirectory &_sd;
      tcp::socket _socket;

      static fetch::oef::Logger logger;
      
    public:
      explicit AgentSession(const std::string &id, AgentDiscovery &ad, ServiceDirectory &sd, tcp::socket socket)
        : _id{id}, _ad{ad}, _sd{sd}, _socket(std::move(socket)) {}
      virtual ~AgentSession() {
        logger.trace("~AgentSession");
        //_socket.shutdown(asio::socket_base::shutdown_both);
      }
      AgentSession(const AgentSession &) = delete;
      AgentSession operator=(const AgentSession &) = delete;
      void start() {
        read();
      }
      void write(std::shared_ptr<Buffer> buffer) {
        asyncWriteBuffer(_socket, buffer, 5);
      }
      void send(const fetch::oef::pb::Server_AgentMessage &msg) {
        asyncWriteBuffer(_socket, serialize(msg), 10 /* sec ? */);
      }
      std::string id() const { return _id; }
      bool match(const QueryModel &query) const {
        if(!_description)
          return false;
        return query.check(*_description);
      }
    private:
      void processDescription(const fetch::oef::pb::AgentDescription &desc) {
        _description = Instance(desc.description());
        DEBUG(logger, "AgentSession::processDescription setting description to agent {} : {}", _id, to_string(desc));
        if(!_description) {
          fetch::oef::pb::Server_AgentMessage answer;
          auto *error = answer.mutable_error();
          error->set_operation(fetch::oef::pb::Server_AgentMessage_Error::REGISTER_DESCRIPTION);
          logger.trace("AgentSession::processDescription sending error {} to {}", error->operation(), _id);
          send(answer);
        }
      }
      void processRegister(const fetch::oef::pb::AgentDescription &desc) {
        DEBUG(logger, "AgentSession::processRegister registering agent {} : {}", _id, to_string(desc));
        bool success = _sd.registerAgent(Instance(desc.description()), _id);
        if(!success) {
          fetch::oef::pb::Server_AgentMessage answer;
          auto *error = answer.mutable_error();
          error->set_operation(fetch::oef::pb::Server_AgentMessage_Error::REGISTER_SERVICE);
          logger.trace("AgentSession::processRegister sending error {} to {}", error->operation(), _id);
          send(answer);
        }
      }
      void processUnregister(const fetch::oef::pb::AgentDescription &desc) {
        DEBUG(logger, "AgentSession::processUnregister unregistering agent {} : {}", _id, to_string(desc));
        bool success = _sd.unregisterAgent(Instance(desc.description()), _id);
        if(!success) {
          fetch::oef::pb::Server_AgentMessage answer;
          auto *error = answer.mutable_error();
          error->set_operation(fetch::oef::pb::Server_AgentMessage_Error::UNREGISTER_SERVICE);
          logger.trace("AgentSession::processUnregister sending error {} to {}", error->operation(), _id);
          send(answer);
        }
      }
      void processSearch(const fetch::oef::pb::AgentSearch &search) {
        QueryModel model{search.query()};
        DEBUG(logger, "AgentSession::processSearch from agent {} : {}", _id, to_string(search));
        auto agents_vec = _ad.search(model);
        fetch::oef::pb::Server_AgentMessage answer;
        auto agents = answer.mutable_agents();
        for(auto &a : agents_vec) {
          agents->add_agents(a);
        }
        logger.trace("AgentSession::processSearch sending {} agents to {}", agents_vec.size(), _id);
        send(answer);
      }
      void processQuery(const fetch::oef::pb::AgentSearch &search) {
        QueryModel model{search.query()};
        DEBUG(logger, "AgentSession::processQuery from agent {} : {}", _id, to_string(search));
        auto agents_vec = _sd.query(model);
        fetch::oef::pb::Server_AgentMessage answer;
        auto agents = answer.mutable_agents();
        for(auto &a : agents_vec) {
          agents->add_agents(a);
        }
        logger.trace("AgentSession::processQuery sending {} agents to {}", agents_vec.size(), _id);
        send(answer);
      }
      void processMessage(fetch::oef::pb::Agent_Message *msg) {
        auto session = _ad.session(msg->destination());
        DEBUG(logger, "AgentSession::processMessage from agent {} : {}", _id, to_string(*msg));
        logger.trace("AgentSession::processMessage to {} from {}", msg->destination(), _id);
        if(session) {
          fetch::oef::pb::Server_AgentMessage message;
          auto content = message.mutable_content();
          std::string cid = msg->conversation_id();
          content->set_conversation_id(cid);
          content->set_origin(_id);
          if(msg->has_content())
            content->set_allocated_content(msg->release_content());
          if(msg->has_fipa())
            content->set_allocated_fipa(msg->release_fipa());
          DEBUG(logger, "AgentSession::processMessage to agent {} : {}", msg->destination(), to_string(message));
          auto buffer = serialize(message);
          asyncWriteBuffer(session->_socket, buffer, 5, [this,cid](std::error_code ec, std::size_t length) {
              if(ec) {
                fetch::oef::pb::Server_AgentMessage answer;
                auto *error = answer.mutable_error();
                error->set_operation(fetch::oef::pb::Server_AgentMessage_Error::SEND_MESSAGE);
                error->set_conversation_id(cid);
                logger.trace("AgentSession::processMessage sending error {} to {}", error->operation(), _id);
                send(answer);
              }
            });
        }
      }
      void process(std::shared_ptr<Buffer> buffer) {
        auto envelope = deserialize<fetch::oef::pb::Envelope>(*buffer);
        auto payload_case = envelope.payload_case();
        switch(payload_case) {
        case fetch::oef::pb::Envelope::kMessage:
          processMessage(envelope.release_message());
          break;
        case fetch::oef::pb::Envelope::kRegister:
          processRegister(envelope.register_());
          break;
        case fetch::oef::pb::Envelope::kUnregister:
          processUnregister(envelope.unregister());
          break;
        case fetch::oef::pb::Envelope::kDescription:
          processDescription(envelope.description());
          break;
        case fetch::oef::pb::Envelope::kSearch:
          processSearch(envelope.search());
          break;
        case fetch::oef::pb::Envelope::kQuery:
          processQuery(envelope.query());
          break;
        case fetch::oef::pb::Envelope::PAYLOAD_NOT_SET:
          logger.error("AgentSession::process cannot process payload {} from {}", payload_case, _id);
        }
      }
      void read() {
        auto self(shared_from_this());
        asyncReadBuffer(_socket, 5, [this, self](std::error_code ec, std::shared_ptr<Buffer> buffer) {
                                if(ec) {
                                  _ad.remove(_id);
                                  _sd.unregisterAll(_id);
                                  logger.info("AgentSession::read error on id {} ec {}", _id, ec);
                                } else {
                                  process(buffer);
                                  read();
                                }});
      }
      
    };
    fetch::oef::Logger AgentSession::logger = fetch::oef::Logger("oef-node::agent-session");

    std::vector<std::string> AgentDiscovery::search(const QueryModel &query) const {
      std::lock_guard<std::mutex> lock(_lock);
      std::vector<std::string> res;
      for(const auto &s : _sessions) {
        if(s.second->match(query))
          res.emplace_back(s.first);
      }
      return res;
    }

    void Server::secretHandshake(const std::string &id, const std::shared_ptr<Context> &context) {
      fetch::oef::pb::Server_Phrase phrase;
      phrase.set_phrase("RandomlyGeneratedString");
      auto phrase_buffer = serialize(phrase);
      logger.trace("Server::secretHandshake sending phrase size {}", phrase_buffer->size());
      asyncWriteBuffer(context->_socket, phrase_buffer, 10 /* sec ? */);
      logger.trace("Server::secretHandshake waiting answer");
      asyncReadBuffer(context->_socket, 5,
                      [this,id,context](std::error_code ec, std::shared_ptr<Buffer> buffer) {
                        if(ec) {
                          logger.error("Server::secretHandshake read failure {}", ec.value());
                        } else {
                          try {
                            auto ans = deserialize<fetch::oef::pb::Agent_Server_Answer>(*buffer);
                            logger.trace("Server::secretHandshake secret [{}]", ans.answer());
                            auto session = std::make_shared<AgentSession>(id, _ad, _sd, std::move(context->_socket));
                            if(_ad.add(id, session)) {
                              session->start();
                              fetch::oef::pb::Server_Connected status;
                              status.set_status(true);
                              session->write(serialize(status));
                            } else {
                              fetch::oef::pb::Server_Connected status;
                              status.set_status(false);
                              logger.info("Server::secretHandshake ID already connected (interleaved) id {}", id);
                              asyncWriteBuffer(context->_socket, serialize(status), 10 /* sec ? */);
                            }
                            // should check the secret with the public key i.e. ID.
                          } catch(std::exception &) {
                            logger.error("Server::secretHandshake error on Answer id {}", id);
                            fetch::oef::pb::Server_Connected status;
                            status.set_status(false);
                            asyncWriteBuffer(context->_socket, serialize(status), 10 /* sec ? */);
                          }
                          // everything is fine -> send connection OK.
                        }
                      });
    }
    void Server::newSession(tcp::socket socket) {
      auto context = std::make_shared<Context>(std::move(socket));
      asyncReadBuffer(context->_socket, 5,
                      [this,context](std::error_code ec, std::shared_ptr<Buffer> buffer) {
                        if(ec) {
                          logger.error("Server::newSession read failure {}", ec.value());
                        } else {
                          try {
                            logger.trace("Server::newSession received {} bytes", buffer->size());
                            auto id = deserialize<fetch::oef::pb::Agent_Server_ID>(*buffer);
                            logger.trace("Debug {}", to_string(id));
                            logger.trace("Server::newSession connection from {}", id.public_key());
                            if(!_ad.exist(id.public_key())) { // not yet connected
                              secretHandshake(id.public_key(), context);
                            } else {
                              logger.info("Server::newSession ID {} already connected", id.public_key());
                              fetch::oef::pb::Server_Phrase failure;
                              (void)failure.mutable_failure();
                              asyncWriteBuffer(context->_socket, serialize(failure), 10 /* sec ? */);
                            }
                          } catch(std::exception &) {
                            logger.error("Server::newSession error parsing ID");
                            fetch::oef::pb::Server_Phrase failure;
                            (void)failure.mutable_failure();
                            asyncWriteBuffer(context->_socket, serialize(failure), 10 /* sec ? */);
                          }
                        }
                      });
    }
    void Server::do_accept() {
      logger.trace("Server::do_accept");
      _acceptor.async_accept([this](std::error_code ec, tcp::socket socket) {
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
      _ad.clear();
      //    _acceptor.close();
      logger.trace("~Server waiting for threads");
      for(auto &t : _threads)
        if(t)
          t->join();
      logger.trace("~Server threads stopped");
    }
    void Server::run() {
      for(auto &t : _threads) {
        if(!t) {
          t = std::unique_ptr<std::thread>(new std::thread([this]() {do_accept(); _io_context.run();}));
        }
      }
    }
    void Server::run_in_thread() {
      do_accept();
      _io_context.run();
    }
    void Server::stop() {
      std::this_thread::sleep_for(std::chrono::seconds{1});
      _io_context.stop();
    }
  }
}
