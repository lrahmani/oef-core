#pragma once

#include "common.h"
#include "uuid.h"
#include "agent.pb.h"
#include "logger.hpp"
#include <unordered_map>
#include <mutex>
#include <condition_variable>

namespace fetch {
  namespace oef {
    template <typename T>
    class Conversation {
    private:
      Uuid _uuid;
      std::string _dest;
      uint32_t _msgId = 0;
      T _state;
    public:
    Conversation(const std::string &uuid, const std::string &dest) : _uuid{Uuid{uuid}}, _dest{dest} {}
    Conversation(const std::string &dest) : _uuid{Uuid::uuid4()}, _dest{dest} {}
      std::string dest() const { return _dest; }
      std::string uuid() const { return _uuid.to_string(); }
      uint32_t msgId() const { return _msgId; }
      void incrementMsgId() { ++_msgId; }
      const T getState() const { return _state; }
      void setState(const T& t) { _state = t; }
      std::shared_ptr<Buffer> envelope(const std::string &outgoing) {
        fetch::oef::pb::Envelope env;
        auto *message = env.mutable_message();
        message->set_cid(_uuid.to_string());
        message->set_destination(_dest);
        message->set_content(outgoing);
        return serialize(env);
      }
      std::shared_ptr<Buffer> envelope(const google::protobuf::MessageLite &t) {
        return envelope(t.SerializeAsString());
      }
    };

    template <typename State,typename T>
      class MultiClient : public std::enable_shared_from_this<MultiClient<State,T>> {
    protected:
      asio::io_context &_io_context;
      const std::string _id;
      tcp::socket _socket;
      std::unordered_map<std::string,std::shared_ptr<Conversation<State>>> _conversations;
      bool _connected = false;
      std::mutex _lock;
      std::condition_variable _condVar;
      
      static fetch::oef::Logger logger;

      T &underlying() { return static_cast<T&>(*this); }
      T const &underlying() const { return static_cast<T const &>(*this); }
      
      void read() {
        asyncReadBuffer(_socket, 1000, [this](std::error_code ec, std::shared_ptr<Buffer> buffer) {
            if(ec) {
              logger.error("MultiClient::read failure {}", ec.value());
            } else {
              logger.trace("Multiclient::read");
              try {
                auto msg = deserialize<fetch::oef::pb::Server_AgentMessage>(*buffer);
                std::string cid = ""; // oef
                if(msg.has_content())
                  cid = msg.content().cid();
                auto iter = _conversations.find(cid);
                if(iter == _conversations.end()) {
                  std::string dest = "";
                  if(msg.has_content())
                    dest = msg.content().origin();
                  _conversations[cid] = std::make_shared<Conversation<State>>(cid, dest);
                }
                auto &c = _conversations[cid];
                c->incrementMsgId();
                logger.trace("Multiclient::read cid {} msgId {} dest {}", c->uuid(), c->msgId(), c->dest());
                underlying().onMsg(msg, *c);
              } catch(std::exception &e) {
                logger.error("Multiclient::read cannot deserialize AgentMessage");
              }
              read();
            }
          });
      }
      void secretHandshake() {
        fetch::oef::pb::Agent_Server_ID id;
        id.set_id(_id);
        logger.trace("MultiCilent::secretHandshake");
        asyncWriteBuffer(_socket, serialize(id), 5,
                         [this](std::error_code, std::size_t length){
                           logger.trace("MultiClient::secretHandshake id sent");
                           asyncReadBuffer(_socket, 5,
                                           [this](std::error_code ec,std::shared_ptr<Buffer> buffer) {
                                             auto p = deserialize<fetch::oef::pb::Server_Phrase>(*buffer);
                                             std::string answer_s = p.phrase();
                                             logger.trace("Multiclient::secretHandshake received phrase: [{}]", answer_s);
                                             // normally should encrypt with private key
                                             std::reverse(std::begin(answer_s), std::end(answer_s));
                                             logger.trace("Multiclient::secretHandshake sending back phrase: [{}]", answer_s);
                                             fetch::oef::pb::Agent_Server_Answer answer;
                                             answer.set_answer(answer_s);
                                             asyncWriteBuffer(_socket, serialize(answer), 5,
                                                              [this](std::error_code, std::size_t length){
                                                                asyncReadBuffer(_socket, 5,
                                                                                [this](std::error_code ec,std::shared_ptr<Buffer> buffer) {
                                                                                  auto c = deserialize<fetch::oef::pb::Server_Connected>(*buffer);
                                                                                  logger.info("Multiclient::secretHandshake received connected: {}", c.status());
                                                                                  if(c.status()) {
                                                                                    std::unique_lock<std::mutex> lck(_lock);
                                                                                    _connected = true;
                                                                                    _condVar.notify_all();
                                                                                    read();
                                                                                  }
                                                                                });
                                                              });
                                           });
                         });
        std::unique_lock<std::mutex> lck(_lock);
        while(!_connected) {
          _condVar.wait(lck);
        }
      }
    public:
      explicit MultiClient(asio::io_context &io_context, const std::string &id, const std::string &host)
        : _io_context{io_context}, _id{id}, _socket{_io_context} {
        tcp::resolver resolver(_io_context);
        asio::connect(_socket, resolver.resolve(host, std::to_string(static_cast<int>(Ports::Agents))));
        secretHandshake();
      }
      ~MultiClient() {
        _socket.shutdown(asio::socket_base::shutdown_both);
        _socket.close();
      }
    };
    template <typename State,typename T>
    fetch::oef::Logger MultiClient<State,T>::logger = fetch::oef::Logger("multiclient");
  }
}
