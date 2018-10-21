#pragma once

#include "schema.h"
#include "agent.pb.h"
#include <experimental/optional>
#include <memory>
#include "mapbox/variant.hpp"

namespace var = mapbox::util; // for the variant
namespace stde = std::experimental;

namespace fetch {
  namespace oef {
    class AgentInterface {
    public:
      virtual void onError(fetch::oef::pb::Server_AgentMessage_Error_Operation operation, const std::string &conversationId, uint32_t msgId) = 0;
      virtual void onSearchResult(const std::vector<std::string> &results) = 0;
      virtual void onMessage(const std::string &from, const std::string &conversationId, const std::string &content) = 0;
      virtual void onCFP(const std::string &from, const std::string &conversationId, uint32_t msgId, uint32_t target, const CFPType &constraints) = 0;
      virtual void onPropose(const std::string &from, const std::string &conversationId, uint32_t msgId, uint32_t target, const ProposeType &proposals) = 0;
      virtual void onAccept(const std::string &from, const std::string &conversationId, uint32_t msgId, uint32_t target) = 0;
      virtual void onClose(const std::string &from, const std::string &conversationId, uint32_t msgId, uint32_t target) = 0;
    };

    class OEFCoreInterface {
    protected:
      const std::string _agentPublicKey;
    public:
      explicit OEFCoreInterface(std::string agentPublicKey) : _agentPublicKey{std::move(agentPublicKey)} {}
      virtual bool handshake() = 0;
      virtual void registerDescription(const Instance &instance) = 0;
      virtual void registerService(const Instance &instance) = 0;
      virtual void searchAgents(const QueryModel &model) = 0;
      virtual void searchServices(const QueryModel &model) = 0;
      virtual void unregisterService(const Instance &instance) = 0;
      virtual void sendMessage(const std::string &conversationId, const std::string &dest, const std::string &msg) = 0;
      virtual void sendCFP(const std::string &conversationId, const std::string &dest, const CFPType &constraints, uint32_t msgId = 1, uint32_t target = 0) = 0;
      /*
      virtual void sendPropose(const std::string &conversationId, const std::string &dest, const ProposeType &proposals, uint32_t msgId, uint32_t target) = 0;
      virtual void sendAccept(const std::string &conversationId, const std::string &dest, uint32_t msgId, uint32_t target) = 0;
      virtual void sendClose(const std::string &conversationId, const std::string &dest, uint32_t msgId, uint32_t target) = 0;
       */
      virtual void loop(AgentInterface &agent) = 0;
      virtual void stop() = 0;
      virtual ~OEFCoreInterface() = default;
      std::string getPublicKey() const { return _agentPublicKey; }
    };

    class Agent : public AgentInterface {
    private:
      std::unique_ptr<OEFCoreInterface> _oefCore;
    protected:
      explicit Agent(std::unique_ptr<OEFCoreInterface> oefCore) : _oefCore{std::move(oefCore)} {}
      void start() {
        if(_oefCore->handshake())
          _oefCore->loop(*this);
      }
    public:
      std::string getPublicKey() const { return _oefCore->getPublicKey(); }
      void registerDescription(const Instance &instance) {
        _oefCore->registerDescription(instance);
      }
      void registerService(const Instance &instance) {
        _oefCore->registerService(instance);
      }
      void searchAgents(const QueryModel &model) {
        _oefCore->searchAgents(model);
      }
      void searchServices(const QueryModel &model) {
        _oefCore->searchServices(model);
      }
      void unregisterService(const Instance &instance) {
        _oefCore->unregisterService(instance);
      }
      void sendMessage(const std::string &conversationId, const std::string &dest, const std::string &msg) {
        _oefCore->sendMessage(conversationId, dest, msg);
      }
      void stop() {
        _oefCore->stop();
      }
    };
  };
};
