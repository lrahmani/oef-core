#include "server.h"
#include <iostream>
#include <google/protobuf/text_format.h>
#include <sstream>
#include <iomanip>

namespace fetch {
  namespace oef {
    class AgentSession : public std::enable_shared_from_this<AgentSession>
    {
    private:
      const std::string _id;
      stde::optional<Instance> _description;
      AgentDiscovery &_ad;
      ServiceDirectory &_sd;
      tcp::socket _socket;
      
    public:
      explicit AgentSession(const std::string &id, AgentDiscovery &ad, ServiceDirectory &sd, tcp::socket socket)
        : _id{id}, _ad{ad}, _sd{sd}, _socket(std::move(socket)) {}
      virtual ~AgentSession() {
        std::cerr << "Session destroyed1\n";
        //_socket.shutdown(asio::socket_base::shutdown_both);
        std::cerr << "Session destroyed for " << _id << "\n";
      }
      AgentSession(const AgentSession &) = delete;
      AgentSession operator=(const AgentSession &) = delete;
      void start() {
        read();
      }
      void write(std::shared_ptr<Buffer> buffer) {
        //std::cerr << "AgentSession::write [" << s << "]\n";
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
        std::string output;
        google::protobuf::TextFormat::PrintToString(_description->handle(), &output);
        std::cerr << "Setting description to agent " << _id << " : " << output << std::endl;
        fetch::oef::pb::Server_AgentMessage answer;
        auto *status = answer.mutable_status();
        status->set_status(bool(_description));
        std::cerr << "Server::Sending registered " << status->status() << std::endl;
        send(answer);
      }
      void processRegister(const fetch::oef::pb::AgentDescription &desc) {
        std::string output;
        google::protobuf::TextFormat::PrintToString(desc, &output);
        std::cerr << "Registering agent " << _id << " : " << output << std::endl;
        fetch::oef::pb::Server_AgentMessage answer;
        auto *status = answer.mutable_status();
        status->set_status(_sd.registerAgent(Instance(desc.description()), _id));
        std::cerr << "Server::Sending registered " << status->status() << std::endl;
        send(answer);
      }
      void processUnregister(const fetch::oef::pb::AgentDescription &desc) {
        std::string output;
        google::protobuf::TextFormat::PrintToString(desc, &output);
        std::cerr << "Unregistering agent " << _id << " : " << output << std::endl;
        fetch::oef::pb::Server_AgentMessage answer;
        auto *status = answer.mutable_status();
        status->set_status(_sd.unregisterAgent(Instance(desc.description()), _id));
        std::cerr << "Server::Sending registered " << status->status() << std::endl;
        send(answer);
      }
      void processSearch(const fetch::oef::pb::AgentSearch &search) {
        QueryModel model{search.query()};
        std::string output;
        google::protobuf::TextFormat::PrintToString(search, &output);
        std::cerr << "processSearch " << output << std::endl;
        auto agents_vec = _ad.search(model);
        fetch::oef::pb::Server_AgentMessage answer;
        auto agents = answer.mutable_agents();
        for(auto &a : agents_vec) {
          agents->add_agents(a);
        }
        std::cerr << "Server::Sending agents" << std::endl;
        send(answer);
      }
      void processQuery(const fetch::oef::pb::AgentSearch &search) {
        QueryModel model{search.query()};
        std::string output;
        google::protobuf::TextFormat::PrintToString(search, &output);
        std::cerr << "processQuery " << output << std::endl;
        auto agents_vec = _sd.query(model);
        fetch::oef::pb::Server_AgentMessage answer;
        auto agents = answer.mutable_agents();
        for(auto &a : agents_vec) {
          agents->add_agents(a);
        }
        std::cerr << "Server::Sending " << agents_vec.size() << " agents" << std::endl;
        send(answer);
      }
      void processMessage(const fetch::oef::pb::Agent_Message &msg) {
        auto session = _ad.session(msg.destination());
        std::cerr << "Server::processMessage to " << msg.destination() << " from " << _id << std::endl;
        if(session) {
          fetch::oef::pb::Server_AgentMessage message;
          auto content = message.mutable_content();
          std::string cid = msg.cid();
          content->set_cid(cid);
          content->set_origin(_id);
          content->set_content(msg.content());
          auto buffer = serialize(message);
          fetch::oef::pb::Server_AgentMessage msg2 = deserialize<fetch::oef::pb::Server_AgentMessage>(*buffer); 
          asyncWriteBuffer(session->_socket, buffer, 5, [this,cid](std::error_code ec, std::size_t length) {
              fetch::oef::pb::Server_AgentMessage answer;
              auto *status = answer.mutable_status();
              status->set_cid(cid);
              status->set_status(true);
              std::cerr << "Server::Sending delivered " << status->status() << std::endl;
              send(answer);
            });
        }
      }
      void process(std::shared_ptr<Buffer> buffer) {
        auto envelope = deserialize<fetch::oef::pb::Envelope>(*buffer);
        auto payload_case = envelope.payload_case();
        switch(payload_case) {
        case fetch::oef::pb::Envelope::kMessage:
          processMessage(envelope.message());
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
        default:
          std::cerr << "Cannot process payload " << payload_case << std::endl;
        }
      }
      void read() {
        auto self(shared_from_this());
        asyncReadBuffer(_socket, 5, [this, self](std::error_code ec, std::shared_ptr<Buffer> buffer) {
                                if(ec) {
                                  _ad.remove(_id);
                                  _sd.unregisterAll(_id);
                                  std::cerr << "error! do_read remove " << _id << "\n";
                                  std::cerr << "error code:" << ec << "\n";
                                } else {
                                  process(buffer);
                                  read();
                                }});
      }
      
    };

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
      std::cerr << "Server::secretHandshake sending phrase size " << phrase_buffer->size() << std::endl;
      asyncWriteBuffer(context->_socket, phrase_buffer, 10 /* sec ? */);
      std::cerr << "Server::secretHandshake waiting answer\n";
      asyncReadBuffer(context->_socket, 5, [this,id,context](std::error_code ec, std::shared_ptr<Buffer> buffer) {
          if(ec) {
            std::cerr << "AgentServer::read failure " << ec.value() << std::endl;
          } else {
            try {
              auto ans = deserialize<fetch::oef::pb::Agent_Server_Answer>(*buffer);
              std::cout << "Node::Secret [" << ans.answer() << "]\n";
              auto session = std::make_shared<AgentSession>(id, _ad, _sd, std::move(context->_socket));
              if(_ad.add(id, session)) {
                session->start();
                fetch::oef::pb::Server_Connected status;
                status.set_status(true);
                session->write(serialize(status));
              } else {
                fetch::oef::pb::Server_Connected status;
                status.set_status(false);
                std::cerr << "Node::Error ID already connected (interleaved): [" << id << "]\n";
                asyncWriteBuffer(context->_socket, serialize(status), 10 /* sec ? */);
              }
              // should check the secret with the public key i.e. ID.
            } catch(std::exception &e) {
              std::cerr << "Node::Error on Answer of id [" << id << "]\n";
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
      asyncReadBuffer(context->_socket, 5, [this,context](std::error_code ec, std::shared_ptr<Buffer> buffer) {
          if(ec) {
            std::cerr << "AgentServer::read failure " << ec.value() << std::endl;
          } else {
            try {
              std::cerr << "Received " << buffer->size() << " bytes\n";
              auto id = deserialize<fetch::oef::pb::Agent_Server_ID>(*buffer);
              std::cerr << "Node::Connection from:[" << id.id() << "]\n";
              if(!_ad.exist(id.id())) { // not yet connected
                secretHandshake(id.id(), context);
              } else {
                std::cerr << "Node::Error ID already connected: [" << id.id() << "]\n";
                fetch::oef::pb::Server_Connected status;
                status.set_status(false);
                asyncWriteBuffer(context->_socket, serialize(status), 10 /* sec ? */);
              }
            } catch(std::exception &e) {
              std::cerr << "Node::Error parsing ID\n";
              fetch::oef::pb::Server_Connected status;
              status.set_status(false);
              asyncWriteBuffer(context->_socket, serialize(status), 10 /* sec ? */);
            }
          }
        });
    }
    void Server::do_accept() {
      std::cerr << "AgentServer::do_accept\n";
      _acceptor.async_accept([this](std::error_code ec, tcp::socket socket) {
                               if (!ec) {
                                 std::cerr << "Starting new session\n";
                                 newSession(std::move(socket));
                                 do_accept();
                               } else {
                                 std::cerr << "Node::do_accept error " << ec.value() << std::endl;
                               }
                             });
    }
    Server::~Server() {
      stop();
      std::cerr << "~Server\n";
      _ad.clear();
      //    _acceptor.close();
      std::cerr << "~Server2\n";
      for(auto &t : _threads)
        if(t)
          t->join();
      std::cerr << "~Server3\n";
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
