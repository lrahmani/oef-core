#pragma once

#include "schema.h"
#include "agent.pb.h"
#include <experimental/optional>

namespace stde = std::experimental;

namespace fetch {
  namespace oef {
    class AgentInterface {
    public:
      virtual void onError(fetch::oef::pb::Server_AgentMessage_Error_Operation operation, const std::string &conversationId, uint32_t msgId) = 0;
      virtual void onSearchResult(const std::vector<std::string> &results) = 0;
      virtual void onMessage(const std::string &from, const std::string &conversationId, const std::string &content) = 0;
      virtual void onCFP(const std::string &from, const std::string &conversationId, uint32_t msgId, uint32_t target, const stde::optional<QueryModel> &constraints) = 0; // + extension
      virtual void onPropose(const std::string &from, const std::string &conversationId, uint32_t msgId, uint32_t target, const std::vector<Instance> &proposals) = 0; // + extension
      virtual void onAccept(const std::string &from, const std::string &conversationId, uint32_t msgId, uint32_t target, const std::vector<Instance> &proposals) = 0; // + extension
      virtual void onClose(const std::string &from, const std::string &conversationId, uint32_t msgId, uint32_t target) = 0;
    };
    class OEFCoreInterface {
    public:
      virtual bool handshake() = 0;
      virtual void registerDescription(const Instance &instance) = 0;
      virtual void registerService(const Instance &instance) = 0;
      virtual void searchAgents(const QueryModel &model) = 0;
      virtual void searchServices(const QueryModel &model) = 0;
      virtual void unregisterService(const Instance &instance) = 0;
      virtual void sendMessage(const std::string &conversationId, const std::string &dest, const std::string &msg) = 0;
      virtual void loop(AgentInterface &server) = 0;
    };
    class OEFCoreBridgeBase : public OEFCoreInterface {
    protected:
      const std::string _agentPublicKey;
    public:
      OEFCoreBridgeBase(const std::string &agentPublicKey) : _agentPublicKey{agentPublicKey} {}
      virtual ~OEFCoreBridgeBase() {}
    };
    class OEFCoreProxy : public AgentInterface, public OEFCoreInterface {
    private:
      OEFCoreBridgeBase &_oefCore;
      AgentInterface &_agent;
    public:
      OEFCoreProxy(OEFCoreBridgeBase &oefCore, AgentInterface &agent) : _oefCore{oefCore}, _agent{agent} {
        if(_oefCore.handshake())
          loop(_agent);
      }
      virtual void onError(fetch::oef::pb::Server_AgentMessage_Error_Operation operation, const std::string &conversationId, uint32_t msgId) override {
        _agent.onError(operation, conversationId, msgId);
      }
      virtual void onSearchResult(const std::vector<std::string> &results) override {
        _agent.onSearchResult(results);
      }
      virtual void onMessage(const std::string &from, const std::string &conversationId, const std::string &content) override {
        _agent.onMessage(from, conversationId, content);
      }
      virtual void onCFP(const std::string &from, const std::string &conversationId, uint32_t msgId, uint32_t target, const stde::optional<QueryModel> &constraints) override { // + extension
        _agent.onCFP(from, conversationId, msgId, target, constraints);
      }        
      virtual void onPropose(const std::string &from, const std::string &conversationId, uint32_t msgId, uint32_t target, const std::vector<Instance> &proposals) override { // + extension
        _agent.onPropose(from, conversationId, msgId, target, proposals);
      }
      virtual void onAccept(const std::string &from, const std::string &conversationId, uint32_t msgId, uint32_t target, const std::vector<Instance> &proposals) override { // + extension
        _agent.onAccept(from, conversationId, msgId, target, proposals);
      }
      virtual void onClose(const std::string &from, const std::string &conversationId, uint32_t msgId, uint32_t target) override {
        _agent.onClose(from, conversationId, msgId, target);
      }
      virtual bool handshake() override {
        return _oefCore.handshake();
      }
      virtual void registerDescription(const Instance &instance) override {
        _oefCore.registerDescription(instance);
      }
      virtual void registerService(const Instance &instance) override {
        _oefCore.registerService(instance);
      }
      virtual void searchAgents(const QueryModel &model) override {
        _oefCore.searchAgents(model);
      }
      virtual void searchServices(const QueryModel &model) override {
        _oefCore.searchServices(model);
      }
      virtual void unregisterService(const Instance &instance) override {
        _oefCore.unregisterService(instance);
      }
      virtual void sendMessage(const std::string &conversationId, const std::string &dest, const std::string &msg) override {
        _oefCore.sendMessage(conversationId, dest, msg);
      }      
      virtual void loop(AgentInterface &server) override {
        _oefCore.loop(_agent);
      }
    };
  };
};
