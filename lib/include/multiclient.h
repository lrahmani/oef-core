#pragma once

#include "common.h"
#include "uuid.h"
#include "agent.pb.h"
#include "logger.hpp"
#include "oefcoreproxy.hpp"
#include "clientmsg.h"
#include "queue.h"
#include "sd.h"

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

    class MessageDecoder {
    private:
      static fetch::oef::Logger logger;

      static void dispatch(AgentInterface &agent, const fetch::oef::pb::Fipa_Message &fipa,
                    const fetch::oef::pb::Server_AgentMessage_Content &content) {
        switch(fipa.msg_case()) {
        case fetch::oef::pb::Fipa_Message::kCfp:
          {
            auto &cfp = fipa.cfp();
            CFPType constraints;
            switch(cfp.payload_case()) {
            case fetch::oef::pb::Fipa_Cfp::kQuery:
              constraints = CFPType{QueryModel{cfp.query()}};
              break;
            case fetch::oef::pb::Fipa_Cfp::kContent:
              constraints = CFPType{cfp.content()};
              break;
            case fetch::oef::pb::Fipa_Cfp::kNothing:
              constraints = CFPType{stde::nullopt};
              break;
            case fetch::oef::pb::Fipa_Cfp::PAYLOAD_NOT_SET:
              break;
            }
            agent.onCFP(content.origin(), content.conversation_id(), fipa.msg_id(), fipa.target(),
                        constraints);
          }
          break;
        case fetch::oef::pb::Fipa_Message::kPropose:
          {
            auto &propose = fipa.propose();
            ProposeType proposals;
            switch(propose.payload_case()) {
            case fetch::oef::pb::Fipa_Propose::kProposals:
              {
                std::vector<Instance> instances;
                for(auto &instance : propose.proposals().objects()) {
                  instances.emplace_back(instance);
                }
                proposals = ProposeType{std::move(instances)};
              }
              break;
            case fetch::oef::pb::Fipa_Propose::kContent:
              proposals = ProposeType{propose.content()};
              break;
            case fetch::oef::pb::Fipa_Propose::PAYLOAD_NOT_SET:
              break;
            }
            agent.onPropose(content.origin(), content.conversation_id(), fipa.msg_id(), fipa.target(),
                            proposals);
          }
          break;
        case fetch::oef::pb::Fipa_Message::kAccept:
          agent.onAccept(content.origin(), content.conversation_id(), fipa.msg_id(), fipa.target());
          break;
        case fetch::oef::pb::Fipa_Message::kDecline:
          agent.onClose(content.origin(), content.conversation_id(), fipa.msg_id(), fipa.target());
          break;
        case fetch::oef::pb::Fipa_Message::MSG_NOT_SET:
          logger.error("MessageDecoder::loop error on fipa {}", static_cast<int>(fipa.msg_case()));
        }
      }
    public:
      static void decode(const std::string &agentPublicKey, const std::shared_ptr<Buffer> &buffer, AgentInterface &agent) {
        try {
          auto msg = deserialize<fetch::oef::pb::Server_AgentMessage>(*buffer);
          switch(msg.payload_case()) {
          case fetch::oef::pb::Server_AgentMessage::kError:
            {
              logger.trace("MessageDecoder::loop error");
              auto &error = msg.error();
              agent.onError(error.operation(), error.has_conversation_id() ? error.conversation_id() : "",
                            error.has_msgid() ? error.msgid() : 0);
            }
            break;
          case fetch::oef::pb::Server_AgentMessage::kAgents:
            {
              logger.trace("MessageDecoder::loop searchResults");
              std::vector<std::string> searchResults;
              for(auto &s : msg.agents().agents()) {
                searchResults.emplace_back(s);
              }
              agent.onSearchResult(searchResults);
            }
            break;
          case fetch::oef::pb::Server_AgentMessage::kContent:
            {
              logger.trace("MessageDecoder::loop content");
              auto &content = msg.content();
              switch(content.payload_case()) {
              case fetch::oef::pb::Server_AgentMessage_Content::kContent:
                {
                  logger.trace("MessageDecoder::loop onMessage {} from {} cid {}", agentPublicKey, content.origin(), content.conversation_id());
                  agent.onMessage(content.origin(), content.conversation_id(), content.content());
                }
                break;
              case fetch::oef::pb::Server_AgentMessage_Content::kFipa:
                {
                  logger.trace("MessageDecoder::loop fipa");
                  dispatch(agent, content.fipa(), content);
                }
                break;
              case fetch::oef::pb::Server_AgentMessage_Content::PAYLOAD_NOT_SET:
                logger.error("MessageDecoder::loop error on message {}", static_cast<int>(msg.payload_case()));
              }
            }
            break;
          case fetch::oef::pb::Server_AgentMessage::PAYLOAD_NOT_SET:
            logger.error("MessageDecoder::loop error {}", static_cast<int>(msg.payload_case()));
          }
        } catch(std::exception &e) {
          logger.error("MessageDecoder::loop cannot deserialize AgentMessage {}", e.what());
        }
      }
    };
    
    class SchedulerPB {
    private:
      struct LocalAgentSession {
        AgentInterface *_agent;
        stde::optional<Instance> _description;
        
        bool match(const QueryModel &query) const {
          if(!_description)
            return false;
          return query.check(*_description);
        }
      };
      std::unordered_map<std::string, LocalAgentSession> _agents;
      Queue<std::pair<std::string,std::shared_ptr<Buffer>>> _queue;
      std::unique_ptr<std::thread> _thread;
      bool _stopping = false;
      ServiceDirectory _sd;
      
      static fetch::oef::Logger logger;
      
      void process() {
        while(!_stopping) {
          auto p = _queue.pop();
          if(!_stopping) {
            if(_agents.find(p.first) != _agents.end() && _agents[p.first]._agent)
              MessageDecoder::decode(p.first, p.second, *_agents[p.first]._agent);
          }
        }
      }
    public:
      SchedulerPB() {
        _thread = std::unique_ptr<std::thread>(new std::thread([this]() { process(); }));
      }
      ~SchedulerPB() {
        if(!_stopping)
          _thread->join();
      }
      void stop() {
        _stopping = true;
        _queue.push(std::pair<std::string,std::shared_ptr<Buffer>>(std::string{""}, std::make_shared<Buffer>(Buffer{})));
        if(_thread)
          _thread->join();
      }
      size_t nbAgents() const { return _agents.size(); }
      bool connect(const std::string &agentPublicKey) {
        logger.trace("SchedulerPB::connect {} size {}", agentPublicKey, _agents.size());
        if(_agents.find(agentPublicKey) != _agents.end())
          return false;
        _agents[agentPublicKey] = LocalAgentSession{};
        return true;
      }
      void disconnect(const std::string &agentPublicKey) {
        logger.trace("SchedulerPB::disconnect {}", agentPublicKey);
        _agents.erase(agentPublicKey);
      }
      void loop(const std::string &agentPublicKey, AgentInterface &agent) {
        logger.trace("SchedulerPB::loop {}", agentPublicKey);
        _agents[agentPublicKey]._agent = &agent;
      }
      void registerDescription(const std::string &agentPublicKey, const Instance &instance) {
        logger.trace("SchedulerPB::registerDescription {}", agentPublicKey);
        if(_agents.find(agentPublicKey) == _agents.end())
          logger.error("SchedulerPB::registerDescription {} is not registered", agentPublicKey);
        else
          _agents[agentPublicKey]._description = instance;
      }
      void registerService(const std::string &agentPublicKey, const Instance &instance) {
        logger.trace("SchedulerPB::registerService {}", agentPublicKey);
        _sd.registerAgent(instance, agentPublicKey);
      }
      void unregisterService(const std::string &agentPublicKey, const Instance &instance) {
        logger.trace("SchedulerPB::unregisterService {}", agentPublicKey);
        _sd.unregisterAgent(instance, agentPublicKey);
      }
      std::vector<std::string> searchAgents(const QueryModel &model) const {
        std::vector<std::string> res;
        for(const auto &s : _agents) {
          if(s.second.match(model))
            res.emplace_back(s.first);
        }
        return res;
      }
      std::vector<std::string> searchServices(const QueryModel &model) const {
        return _sd.query(model);
      }
      void send(const std::string &agentPublicKey, const std::shared_ptr<Buffer> &buffer) {
        logger.trace("SchedulerPB::send {}", agentPublicKey);
        _queue.push(std::pair<std::string,std::shared_ptr<Buffer>>(agentPublicKey, buffer));
      }
      void sendTo(const std::string &agentPublicKey, const std::string &to, const std::shared_ptr<Buffer> &buffer) {
        logger.trace("SchedulerPB::sendTo {} to {}", agentPublicKey, to);
        if(_agents.find(to) == _agents.end())
          logger.error("SchedulerPB::sendTo {} is not connected.", to);
        else
          _queue.push(std::pair<std::string,std::shared_ptr<Buffer>>(to, buffer));
      }
    };
      
    
    class OEFCoreLocalPB : public virtual OEFCoreInterface {
    protected:
      const std::string _agentPublicKey;
    private:
      SchedulerPB &_scheduler;
      
      static fetch::oef::Logger logger;
    public:
      OEFCoreLocalPB(const std::string &agentPublicKey, SchedulerPB &scheduler) : _agentPublicKey{agentPublicKey},
                                                                                  _scheduler{scheduler} {}
      void stop() {
        _scheduler.disconnect(_agentPublicKey);
      }
      ~OEFCoreLocalPB() {
        _scheduler.disconnect(_agentPublicKey);
      }
      bool handshake() override {
        return _scheduler.connect(_agentPublicKey);
      }
      void loop(AgentInterface &agent) override {
        _scheduler.loop(_agentPublicKey, agent);
      }
      void registerDescription(const Instance &instance) override {
        _scheduler.registerDescription(_agentPublicKey, instance);
      }
      void registerService(const Instance &instance) override {
        _scheduler.registerService(_agentPublicKey, instance);
      }
      void searchAgents(const QueryModel &model) override {
        auto agents_vec = _scheduler.searchAgents(model);
        fetch::oef::pb::Server_AgentMessage answer;
        auto agents = answer.mutable_agents();
        for(auto &a : agents_vec) {
          agents->add_agents(a);
        }
        _scheduler.send(_agentPublicKey, serialize(answer));
      }
      void searchServices(const QueryModel &model) override {
        auto agents_vec = _scheduler.searchServices(model);
        fetch::oef::pb::Server_AgentMessage answer;
        auto agents = answer.mutable_agents();
        for(auto &a : agents_vec) {
          agents->add_agents(a);
        }
        _scheduler.send(_agentPublicKey, serialize(answer));
      }
      void unregisterService(const Instance &instance) override {
        _scheduler.unregisterService(_agentPublicKey, instance);
      }
      void sendMessage(const std::string &conversationId, const std::string &dest, const std::string &msg) {
        fetch::oef::pb::Server_AgentMessage message;
        auto content = message.mutable_content();
        content->set_conversation_id(conversationId);
        content->set_origin(_agentPublicKey);
        content->set_content(msg);
        // todo fipa
        auto buffer = serialize(message);
        _scheduler.sendTo(_agentPublicKey, dest, serialize(message));
      }
    };
    
    class OEFCoreNetworkProxy : virtual public OEFCoreInterface {
    protected:
      const std::string _agentPublicKey;
    private:
      asio::io_context &_io_context;
      tcp::socket _socket;
      
      static fetch::oef::Logger logger;

    public:
      OEFCoreNetworkProxy(const std::string &agentPublicKey, asio::io_context &io_context, const std::string &host)
        : _agentPublicKey{agentPublicKey}, _io_context{io_context}, _socket{_io_context} {
          tcp::resolver resolver(_io_context);
          asio::connect(_socket, resolver.resolve(host, std::to_string(static_cast<int>(Ports::Agents))));
      }
      OEFCoreNetworkProxy(OEFCoreNetworkProxy &&) = default;
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
        id.set_public_key(_agentPublicKey);
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
                        if(p.has_failure()) {
                          finished = true;
                          condVar.notify_all();
                        } else {
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
      void loop(AgentInterface &agent) override {
        asyncReadBuffer(_socket, 1000, [this,&agent](std::error_code ec, std::shared_ptr<Buffer> buffer) {
            if(ec) {
              logger.error("OEFCoreNetworkProxy::loop failure {}", ec.value());
            } else {
              logger.trace("OEFCoreNetworkProxy::loop");
              MessageDecoder::decode(_agentPublicKey, buffer, agent);
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
        id.set_public_key(_id);
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
