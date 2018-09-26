#pragma once

#include "common.h"
#include "uuid.h"
#include "agent.pb.h"
#include "logger.hpp"
#include "oefcoreproxy.hpp"
#include "clientmsg.h"
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
      bool _finished = false;
    public:
    Conversation(const std::string &uuid, const std::string &dest) : _uuid{Uuid{uuid}}, _dest{dest} {}
    Conversation(const std::string &dest) : _uuid{Uuid::uuid4()}, _dest{dest} {}
      std::string dest() const { return _dest; }
      std::string uuid() const { return _uuid.to_string(); }
      uint32_t msgId() const { return _msgId; }
      void incrementMsgId() { ++_msgId; }
      bool finished() const { return _finished; }
      void setFinished() { _finished = true; }
      const T getState() const { return _state; }
      void setState(const T& t) { _state = t; }
      std::shared_ptr<Buffer> envelope(const std::string &outgoing) {
        fetch::oef::pb::Envelope env;
        auto *message = env.mutable_message();
        message->set_conversation_id(_uuid.to_string());
        message->set_destination(_dest);
        message->set_content(outgoing);
        return serialize(env);
      }
      std::shared_ptr<Buffer> envelope(const google::protobuf::MessageLite &t) {
        return envelope(t.SerializeAsString());
      }
    };

    class OEFCoreNetworkProxy : public OEFCoreBridgeBase {
    private:
      asio::io_context &_io_context;
      tcp::socket _socket;
      
      static fetch::oef::Logger logger;

    public:
      OEFCoreNetworkProxy(const std::string &agentPublicKey, asio::io_context &io_context, const std::string &host)
        : OEFCoreBridgeBase{agentPublicKey}, _io_context{io_context}, _socket{_io_context} {
          tcp::resolver resolver(_io_context);
          asio::connect(_socket, resolver.resolve(host, std::to_string(static_cast<int>(Ports::Agents))));
      }
      void stop() {
        _socket.shutdown(asio::socket_base::shutdown_both);
        _socket.close();
      }
      ~OEFCoreNetworkProxy() {
        if(_socket.is_open())
          stop();
      }
      bool handshake() override {
        bool connected = false;
        bool finished = false;
        std::mutex lock;
        std::condition_variable condVar;
        fetch::oef::pb::Agent_Server_ID id;
        id.set_id(_agentPublicKey);
        logger.trace("OEFCoreNetworkProxy::handshake");
        asyncWriteBuffer(_socket, serialize(id), 5,
            [this,&connected,&condVar,&finished,&lock](std::error_code ec, std::size_t length){
              if(ec) {
                std::unique_lock<std::mutex> lck(lock);
                finished = true;
                condVar.notify_all();
              } else {
                logger.trace("OEFCoreNetworkProxy::handshake id sent");
                asyncReadBuffer(_socket, 5,
                    [this,&connected,&condVar,&finished,&lock](std::error_code ec,std::shared_ptr<Buffer> buffer) {
                      if(ec) {
                        std::unique_lock<std::mutex> lck(lock);
                        finished = true;
                        condVar.notify_all();
                      } else {
                        auto p = deserialize<fetch::oef::pb::Server_Phrase>(*buffer);
                        std::string answer_s = p.phrase();
                        logger.trace("OEFCoreNetworkProxy::handshake received phrase: [{}]", answer_s);
                        // normally should encrypt with private key
                        std::reverse(std::begin(answer_s), std::end(answer_s));
                        logger.trace("OEFCoreNetworkProxy::handshake sending back phrase: [{}]", answer_s);
                        fetch::oef::pb::Agent_Server_Answer answer;
                        answer.set_answer(answer_s);
                        asyncWriteBuffer(_socket, serialize(answer), 5,
                            [this,&connected,&condVar,&finished,&lock](std::error_code ec, std::size_t length){
                              if(ec) {
                                std::unique_lock<std::mutex> lck(lock);
                                finished = true;
                                condVar.notify_all();
                              } else {
                                asyncReadBuffer(_socket, 5,
                                    [this,&connected,&condVar,&finished,&lock](std::error_code ec,std::shared_ptr<Buffer> buffer) {
                                      auto c = deserialize<fetch::oef::pb::Server_Connected>(*buffer);
                                      logger.info("OEFCoreNetworkProxy::handshake received connected: {}", c.status());
                                      connected = c.status();
                                      std::unique_lock<std::mutex> lck(lock);
                                      finished = true;
                                      condVar.notify_all();
                                    });
                              }
                            });
                      }
                    });
              }
            });
        std::unique_lock<std::mutex> lck(lock);
        while(!finished) {
          condVar.wait(lck);
        }
        return connected;
      }
      void dispatch(AgentInterface &agent, const fetch::oef::pb::Fipa_Message &fipa,
                    const fetch::oef::pb::Server_AgentMessage_Content &content) {
        switch(fipa.msg_case()) {
        case fetch::oef::pb::Fipa_Message::kCfp:
          {
            auto &cfp = fipa.cfp();
            stde::optional<QueryModel> constraints;
            if(cfp.has_query())
              constraints.emplace(cfp.query());
            agent.onCFP(content.origin(), content.conversation_id(), fipa.msg_id(), fipa.target(),
                        constraints);
          }
          break;
        case fetch::oef::pb::Fipa_Message::kPropose:
          {
            auto &propose = fipa.propose();
            std::vector<Instance> instances;
            for(auto &instance : propose.objects()) {
              instances.emplace_back(instance);
            }
            agent.onPropose(content.origin(), content.conversation_id(), fipa.msg_id(), fipa.target(),
                            instances);
          }
          break;
        case fetch::oef::pb::Fipa_Message::kAccept:
          {
            auto &accept = fipa.accept();
            std::vector<Instance> instances;
            for(auto &instance : accept.objects()) {
              instances.emplace_back(instance);
            }
            agent.onAccept(content.origin(), content.conversation_id(), fipa.msg_id(), fipa.target(),
                           instances);
          }
          break;
        case fetch::oef::pb::Fipa_Message::kClose:
          agent.onClose(content.origin(), content.conversation_id(), fipa.msg_id(), fipa.target());
          break;
        case fetch::oef::pb::Fipa_Message::MSG_NOT_SET:
          logger.error("OEFCoreNetworkProxy::loop error on fipa {}", static_cast<int>(fipa.msg_case()));
        }
      }
      void loop(AgentInterface &agent) override {
        asyncReadBuffer(_socket, 1000, [this,&agent](std::error_code ec, std::shared_ptr<Buffer> buffer) {
            if(ec) {
              logger.error("OEFCoreNetworkProxy::loop failure {}", ec.value());
            } else {
              logger.trace("OEFCoreNetworkProxy::loop");
              try {
                auto msg = deserialize<fetch::oef::pb::Server_AgentMessage>(*buffer);
                switch(msg.payload_case()) {
                case fetch::oef::pb::Server_AgentMessage::kError:
                  {
                    logger.trace("OEFCoreNetworkProxy::loop error");
                    auto &error = msg.error();
                    agent.onError(error.operation(), error.has_conversation_id() ? error.conversation_id() : "",
                                  error.has_msgid() ? error.msgid() : 0);
                  }
                  break;
                case fetch::oef::pb::Server_AgentMessage::kAgents:
                  {
                    logger.trace("OEFCoreNetworkProxy::loop searchResults");
                    std::vector<std::string> searchResults;
                    for(auto &s : msg.agents().agents()) {
                      searchResults.emplace_back(s);
                    }
                    agent.onSearchResult(searchResults);
                  }
                  break;
                case fetch::oef::pb::Server_AgentMessage::kContent:
                  {
                    logger.trace("OEFCoreNetworkProxy::loop content");
                    auto &content = msg.content();
                    switch(content.payload_case()) {
                    case fetch::oef::pb::Server_AgentMessage_Content::kContent:
                      {
                        logger.trace("OEFCoreNetworkProxy::loop onMessage {} from {} cid {}", _agentPublicKey, content.origin(), content.conversation_id());
                        agent.onMessage(content.origin(), content.conversation_id(), content.content());
                      }
                      break;
                    case fetch::oef::pb::Server_AgentMessage_Content::kFipa:
                      {
                        logger.trace("OEFCoreNetworkProxy::loop fipa");
                        dispatch(agent, content.fipa(), content);
                      }
                      break;
                    case fetch::oef::pb::Server_AgentMessage_Content::PAYLOAD_NOT_SET:
                      logger.error("OEFCoreNetworkProxy::loop error on message {}", static_cast<int>(msg.payload_case()));
                    }
                  }
                  break;
                case fetch::oef::pb::Server_AgentMessage::PAYLOAD_NOT_SET:
                  logger.error("OEFCoreNetworkProxy::loop error {}", static_cast<int>(msg.payload_case()));
                }
              } catch(std::exception &e) {
                logger.error("OEFCoreNetworkProxy::loop cannot deserialize AgentMessage {}", e.what());
              }
              loop(agent);
            }
          });
      }
      void registerDescription(const Instance &instance) override {
        Description description{instance};
        asyncWriteBuffer(_socket, serialize(description.handle()), 5);
      }
      void registerService(const Instance &instance) override {
        Register service{instance};
        asyncWriteBuffer(_socket, serialize(service.handle()), 5);
      }
      void searchAgents(const QueryModel &model) override {
        Search search{model};
        asyncWriteBuffer(_socket, serialize(search.handle()), 5);
      }
      void searchServices(const QueryModel &model) override {
        Query query{model};
        asyncWriteBuffer(_socket, serialize(query.handle()), 5);
      }
      void unregisterService(const Instance &instance) override {
        Unregister service{instance};
        asyncWriteBuffer(_socket, serialize(service.handle()), 5);
      }
      void sendMessage(const std::string &conversationId, const std::string &dest, const std::string &msg) {
        Message message{conversationId, dest, msg};
        asyncWriteBuffer(_socket, serialize(message.handle()), 5);
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
                  cid = msg.content().conversation_id();
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
                if(c->finished())
                  _conversations.erase(cid);
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
