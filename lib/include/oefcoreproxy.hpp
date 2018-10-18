#pragma once

#include "schema.h"
#include "agent.pb.h"
#include <experimental/optional>
#include "mapbox/variant.hpp"

namespace var = mapbox::util; // for the variant
namespace stde = std::experimental;

namespace fetch {
  namespace oef {
    using CFPType = var::variant<std::string,QueryModel,stde::nullopt_t>;
    using ProposeType = var::variant<std::string,std::vector<Instance>>;
    
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
      OEFCoreInterface(const std::string &agentPublicKey) : _agentPublicKey{agentPublicKey} {}
      virtual bool handshake() = 0;
      virtual void registerDescription(const Instance &instance) = 0;
      virtual void registerService(const Instance &instance) = 0;
      virtual void searchAgents(const QueryModel &model) = 0;
      virtual void searchServices(const QueryModel &model) = 0;
      virtual void unregisterService(const Instance &instance) = 0;
      virtual void sendMessage(const std::string &conversationId, const std::string &dest, const std::string &msg) = 0;
      virtual void loop(AgentInterface &server) = 0;
    };
  };
};
