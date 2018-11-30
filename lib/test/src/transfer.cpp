#include "catch.hpp"
#include "server.h"
#include <iostream>
#include <chrono>
#include <future>
#include <cassert>
#include "uuid.h"
#include "oefcoreproxy.hpp"
#include "multiclient.h"

using namespace fetch::oef;

enum class AgentAction {
                        NONE,
                        ON_ERROR,
                        ON_SEARCH_RESULT,
                        ON_MESSAGE,
                        ON_CFP,
                        ON_PROPOSE,
                        ON_ACCEPT,
                        ON_DECLINE
};
class SimpleAgentTransfer : public fetch::oef::Agent {
 public:
  std::string from_;
  uint32_t dialogueId_;
  std::string content_;
  AgentAction action_ = AgentAction::NONE;
  
  SimpleAgentTransfer(const std::string &agentId, asio::io_context &io_context, const std::string &host)
    : fetch::oef::Agent{std::unique_ptr<fetch::oef::OEFCoreInterface>(new fetch::oef::OEFCoreNetworkProxy{agentId, io_context, host})}
  {
    start();
  }
  void onError(fetch::oef::pb::Server_AgentMessage_Error_Operation operation, stde::optional<uint32_t> dialogueId, stde::optional<uint32_t> msgId) override {
    action_ = AgentAction::ON_ERROR;
  }
  void onSearchResult(uint32_t, const std::vector<std::string> &results) override {
    action_ = AgentAction::ON_SEARCH_RESULT;
  }
  void onMessage(const std::string &from, uint32_t dialogueId, const std::string &content) override {
    from_ = from;
    dialogueId_ = dialogueId;
    content_ = content;
    action_ = AgentAction::ON_MESSAGE;
  }
  void onCFP(const std::string &from, uint32_t dialogueId, uint32_t msgId, uint32_t target, const fetch::oef::CFPType &constraints) override {
    action_ = AgentAction::ON_CFP;
  }
  void onPropose(const std::string &from, uint32_t dialogueId, uint32_t msgId, uint32_t target, const fetch::oef::ProposeType &proposals) override {
    action_ = AgentAction::ON_PROPOSE;
  }
  void onAccept(const std::string &from, uint32_t dialogueId, uint32_t msgId, uint32_t target) override {
    action_ = AgentAction::ON_ACCEPT;
  }
  void onDecline(const std::string &from, uint32_t dialogueId, uint32_t msgId, uint32_t target) override {
    action_ = AgentAction::ON_DECLINE;
  }
 };

class SimpleAgentTransferLocal : public fetch::oef::Agent {
 public:
  std::string from_;
  uint32_t dialogueId_;
  std::string content_;
  AgentAction action_ = AgentAction::NONE;

  SimpleAgentTransferLocal(const std::string &agentId, fetch::oef::SchedulerPB &scheduler)
    : fetch::oef::Agent{std::unique_ptr<fetch::oef::OEFCoreInterface>(new fetch::oef::OEFCoreLocalPB{agentId, scheduler})}
  {
    start();
  }
  void onError(fetch::oef::pb::Server_AgentMessage_Error_Operation operation, stde::optional<uint32_t> dialogueId, stde::optional<uint32_t> msgId) override {
    action_ = AgentAction::ON_ERROR;
  }
  void onSearchResult(uint32_t search_id, const std::vector<std::string> &results) override {
    action_ = AgentAction::ON_SEARCH_RESULT;
  }
  void onMessage(const std::string &from, uint32_t dialogueId, const std::string &content) override {
    std::cerr << "onMessage " << getPublicKey() << " from " << from << " cid " << dialogueId << " content " << content << std::endl;
    from_ = from;
    dialogueId_ = dialogueId;
    content_ = content;
    action_ = AgentAction::ON_MESSAGE;
  }
  void onCFP(const std::string &from, uint32_t dialogueId, uint32_t msgId, uint32_t target, const fetch::oef::CFPType &constraints) override {
    std::cerr << "onCFP " << getPublicKey() << " from " << from << " cid " << dialogueId << std::endl;
    action_ = AgentAction::ON_CFP;
  }
  void onPropose(const std::string &from, uint32_t dialogueId, uint32_t msgId, uint32_t target, const fetch::oef::ProposeType &proposals) override {
    action_ = AgentAction::ON_PROPOSE;
  }
  void onAccept(const std::string &from, uint32_t dialogueId, uint32_t msgId, uint32_t target) override {
    action_ = AgentAction::ON_ACCEPT;
  }
  void onDecline(const std::string &from, uint32_t dialogueId, uint32_t msgId, uint32_t target) override {
    action_ = AgentAction::ON_DECLINE;
  }
 };

namespace Test {
  TEST_CASE("transfer between agents", "[transfer]") {
    spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [thread %t] [%n] [%l] %v");
    spdlog::set_level(spdlog::level::level_enum::trace);
    Uuid id = Uuid::uuid4();
    std::cerr << id.to_string() << std::endl;
    std::string s = id.to_string();
    Uuid id2{s};
    std::cerr << id2.to_string() << std::endl;
    Server as;
    std::cerr << "Server created\n";
    as.run();
    std::cerr << "Server started\n";
    
    REQUIRE(as.nbAgents() == 0);
    {
      IoContextPool pool(2);
      pool.run();
      SimpleAgentTransfer c1("Agent1", pool.getIoContext(), "127.0.0.1");
      SimpleAgentTransfer c2("Agent2", pool.getIoContext(), "127.0.0.1");
      SimpleAgentTransfer c3("Agent3", pool.getIoContext(), "127.0.0.1");
      REQUIRE(as.nbAgents() == 3);
      std::cerr << "Clients created\n";
      c1.sendMessage(1, "Agent2", "Hello world");
      c1.sendMessage(1, "Agent3", "Hello world");
      std::this_thread::sleep_for(std::chrono::seconds{1});
      REQUIRE(c1.action_ == AgentAction::NONE);
      REQUIRE(c2.action_ == AgentAction::ON_MESSAGE);
      REQUIRE(c3.action_ == AgentAction::ON_MESSAGE);
      REQUIRE(c2.from_ == "Agent1");
      REQUIRE(c3.from_ == "Agent1");
      REQUIRE(c2.dialogueId_ == 1);
      REQUIRE(c3.dialogueId_ == 1);
      REQUIRE(c2.content_ == "Hello world");
      REQUIRE(c3.content_ == "Hello world");
      c2.sendMessage(2, "Agent3", "Welcome back");
      c2.sendMessage(2, "Agent1", "Welcome back");
      std::this_thread::sleep_for(std::chrono::seconds{1});
      REQUIRE(c1.from_ == "Agent2");
      REQUIRE(c3.from_ == "Agent2");
      REQUIRE(c1.dialogueId_ == 2);
      REQUIRE(c3.dialogueId_ == 2);
      REQUIRE(c1.content_ == "Welcome back");
      REQUIRE(c3.content_ == "Welcome back");
      c3.sendMessage(3, "Agent1", "Here I am");
      c3.sendMessage(3, "Agent2", "Here I am");
      std::this_thread::sleep_for(std::chrono::seconds{1});
      REQUIRE(c1.from_ == "Agent3");
      REQUIRE(c2.from_ == "Agent3");
      REQUIRE(c1.dialogueId_ == 3);
      REQUIRE(c2.dialogueId_ == 3);
      REQUIRE(c1.content_ == "Here I am");
      REQUIRE(c2.content_ == "Here I am");
      std::cerr << "Data sent\n";
      c1.sendCFP(4, "Agent2", fetch::oef::CFPType{stde::nullopt});
      c1.sendCFP(4, "Agent3", fetch::oef::CFPType{std::string{"message"}});
      std::this_thread::sleep_for(std::chrono::seconds{1});
      REQUIRE(c2.action_ == AgentAction::ON_CFP);
      REQUIRE(c3.action_ == AgentAction::ON_CFP);
      c1.sendPropose(5, "Agent2", fetch::oef::ProposeType{std::vector<Instance>{}}, 2, 1);
      c1.sendPropose(5, "Agent3", fetch::oef::ProposeType{std::string{"message"}}, 2, 1);
      std::this_thread::sleep_for(std::chrono::seconds{1});
      REQUIRE(c2.action_ == AgentAction::ON_PROPOSE);
      REQUIRE(c3.action_ == AgentAction::ON_PROPOSE);
      c1.sendAccept(6, "Agent2", 3, 2);
      c1.sendAccept(6, "Agent3", 3, 2);
      std::this_thread::sleep_for(std::chrono::seconds{1});
      REQUIRE(c2.action_ == AgentAction::ON_ACCEPT);
      REQUIRE(c3.action_ == AgentAction::ON_ACCEPT);
      c1.sendDecline(7, "Agent2", 4, 3);
      c1.sendDecline(7, "Agent3", 4, 3);
      std::this_thread::sleep_for(std::chrono::seconds{1});
      REQUIRE(c2.action_ == AgentAction::ON_DECLINE);
      REQUIRE(c3.action_ == AgentAction::ON_DECLINE);
      
      c1.stop();
      c2.stop();
      c3.stop();
      pool.stop();
    }
    std::cerr << "Waiting\n";
    std::this_thread::sleep_for(std::chrono::seconds{2});
    std::cerr << "NbAgents " << as.nbAgents() << "\n";
    
    as.stop();
    std::cerr << "Server stopped\n";
  }
  TEST_CASE("local transfer between agents", "[transfer]") {
    spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [thread %t] [%n] [%l] %v");
    spdlog::set_level(spdlog::level::level_enum::trace);
    Uuid id = Uuid::uuid4();
    std::cerr << id.to_string() << std::endl;
    std::string s = id.to_string();
    Uuid id2{s};
    std::cerr << id2.to_string() << std::endl;
    fetch::oef::SchedulerPB scheduler;
    std::cerr << "Scheduler created\n";
    
    REQUIRE(scheduler.nbAgents() == 0);
    {
      SimpleAgentTransferLocal c1("Agent1", scheduler);
      SimpleAgentTransferLocal c2("Agent2", scheduler);
      SimpleAgentTransferLocal c3("Agent3", scheduler);
      REQUIRE(scheduler.nbAgents() == 3);
      std::cerr << "Clients created\n";
      c1.sendMessage(1, "Agent2", "Hello world");
      c1.sendMessage(1, "Agent3", "Hello world");
      std::this_thread::sleep_for(std::chrono::seconds{1});
      REQUIRE(c1.action_ == AgentAction::NONE);
      REQUIRE(c2.action_ == AgentAction::ON_MESSAGE);
      REQUIRE(c3.action_ == AgentAction::ON_MESSAGE);
      REQUIRE(c2.from_ == "Agent1");
      REQUIRE(c3.from_ == "Agent1");
      REQUIRE(c2.dialogueId_ == 1);
      REQUIRE(c3.dialogueId_ == 1);
      REQUIRE(c2.content_ == "Hello world");
      REQUIRE(c3.content_ == "Hello world");
      c2.sendMessage(2, "Agent3", "Welcome back");
      c2.sendMessage(2, "Agent1", "Welcome back");
      std::this_thread::sleep_for(std::chrono::seconds{1});
      REQUIRE(c1.from_ == "Agent2");
      REQUIRE(c3.from_ == "Agent2");
      REQUIRE(c1.dialogueId_ == 2);
      REQUIRE(c3.dialogueId_ == 2);
      REQUIRE(c1.content_ == "Welcome back");
      REQUIRE(c3.content_ == "Welcome back");
      c3.sendMessage(3, "Agent1", "Here I am");
      c3.sendMessage(3, "Agent2", "Here I am");
      std::this_thread::sleep_for(std::chrono::seconds{1});
      REQUIRE(c1.from_ == "Agent3");
      REQUIRE(c2.from_ == "Agent3");
      REQUIRE(c1.dialogueId_ == 3);
      REQUIRE(c2.dialogueId_ == 3);
      REQUIRE(c1.content_ == "Here I am");
      REQUIRE(c2.content_ == "Here I am");
      std::cerr << "Data sent\n";
      c1.sendCFP(4, "Agent2", fetch::oef::CFPType{stde::nullopt});
      c1.sendCFP(4, "Agent3", fetch::oef::CFPType{std::string{"message"}});
      std::this_thread::sleep_for(std::chrono::seconds{1});
      REQUIRE(c2.action_ == AgentAction::ON_CFP);
      REQUIRE(c3.action_ == AgentAction::ON_CFP);
      c1.sendPropose(5, "Agent2", fetch::oef::ProposeType{std::vector<Instance>{}}, 2, 1);
      c1.sendPropose(5, "Agent3", fetch::oef::ProposeType{std::string{"message"}}, 2, 1);
      std::this_thread::sleep_for(std::chrono::seconds{1});
      REQUIRE(c2.action_ == AgentAction::ON_PROPOSE);
      REQUIRE(c3.action_ == AgentAction::ON_PROPOSE);
      c1.sendAccept(6, "Agent2", 3, 2);
      c1.sendAccept(6, "Agent3", 3, 2);
      std::this_thread::sleep_for(std::chrono::seconds{1});
      REQUIRE(c2.action_ == AgentAction::ON_ACCEPT);
      REQUIRE(c3.action_ == AgentAction::ON_ACCEPT);
      c1.sendDecline(7, "Agent2", 4, 3);
      c1.sendDecline(7, "Agent3", 4, 3);
      std::this_thread::sleep_for(std::chrono::seconds{1});
      REQUIRE(c2.action_ == AgentAction::ON_DECLINE);
      REQUIRE(c3.action_ == AgentAction::ON_DECLINE);
      c1.stop();
      c2.stop();
      c3.stop();
    }
    std::cerr << "Waiting\n";
    std::this_thread::sleep_for(std::chrono::seconds{2});
    std::cerr << "NbAgents " << scheduler.nbAgents() << "\n";
    
    scheduler.stop();
    std::cerr << "Server stopped\n";
  }
}
