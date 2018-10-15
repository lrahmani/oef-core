#include "catch.hpp"
#include "server.h"
#include <iostream>
#include <chrono>
#include <future>
#include <cassert>
#include "uuid.h"
#include "oefcoreproxy.hpp"
#include "multiclient.h"

using fetch::oef::Server;

class SimpleAgentTransfer : public fetch::oef::AgentInterface, public fetch::oef::OEFCoreNetworkProxy {
 private:
  fetch::oef::OEFCoreProxy _oefCore;
  
 public:
  std::string _from;
  std::string _conversationId;
  std::string _content;

  SimpleAgentTransfer(const std::string &agentId, asio::io_context &io_context, const std::string &host)
    : fetch::oef::OEFCoreNetworkProxy{agentId, io_context, host}, _oefCore{*this, *this} {
  }
  void onError(fetch::oef::pb::Server_AgentMessage_Error_Operation operation, const std::string &conversationId, uint32_t msgId) override {}
  void onSearchResult(const std::vector<std::string> &results) override {}
  void onMessage(const std::string &from, const std::string &conversationId, const std::string &content) override {
    _from = from;
    _conversationId = conversationId;
    _content = content;
  }
  void onCFP(const std::string &from, const std::string &conversationId, uint32_t msgId, uint32_t target, const fetch::oef::CFPType &constraints) override {}
  void onPropose(const std::string &from, const std::string &conversationId, uint32_t msgId, uint32_t target, const fetch::oef::ProposeType &proposals) override {}
  void onAccept(const std::string &from, const std::string &conversationId, uint32_t msgId, uint32_t target) override {}
  void onClose(const std::string &from, const std::string &conversationId, uint32_t msgId, uint32_t target) override {}
 };

class SimpleAgentTransferLocal : public fetch::oef::AgentInterface, public fetch::oef::OEFCoreLocalPB {
 private:
  fetch::oef::OEFCoreProxy _oefCore;
  
 public:
  std::string _from;
  std::string _conversationId;
  std::string _content;

  SimpleAgentTransferLocal(const std::string &agentId, fetch::oef::SchedulerPB &scheduler)
    : fetch::oef::OEFCoreLocalPB{agentId, scheduler}, _oefCore{*this, *this} {
  }
  void onError(fetch::oef::pb::Server_AgentMessage_Error_Operation operation, const std::string &conversationId, uint32_t msgId) override {}
  void onSearchResult(const std::vector<std::string> &results) override {}
  void onMessage(const std::string &from, const std::string &conversationId, const std::string &content) override {
    std::cerr << "onMessage " << _agentPublicKey << " from " << from << " cid " << conversationId << " content " << content << std::endl;
    _from = from;
    _conversationId = conversationId;
    _content = content;
  }
  void onCFP(const std::string &from, const std::string &conversationId, uint32_t msgId, uint32_t target, const fetch::oef::CFPType &constraints) override {}
  void onPropose(const std::string &from, const std::string &conversationId, uint32_t msgId, uint32_t target, const fetch::oef::ProposeType &proposals) override {}
  void onAccept(const std::string &from, const std::string &conversationId, uint32_t msgId, uint32_t target) override {}
  void onClose(const std::string &from, const std::string &conversationId, uint32_t msgId, uint32_t target) override {}
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
      c1.sendMessage("1", "Agent2", "Hello world");
      c1.sendMessage("1", "Agent3", "Hello world");
      std::this_thread::sleep_for(std::chrono::seconds{1});
      REQUIRE(c2._from == "Agent1");
      REQUIRE(c3._from == "Agent1");
      REQUIRE(c2._conversationId == "1");
      REQUIRE(c3._conversationId == "1");
      REQUIRE(c2._content == "Hello world");
      REQUIRE(c3._content == "Hello world");
      c2.sendMessage("2", "Agent3", "Welcome back");
      c2.sendMessage("2", "Agent1", "Welcome back");
      std::this_thread::sleep_for(std::chrono::seconds{1});
      REQUIRE(c1._from == "Agent2");
      REQUIRE(c3._from == "Agent2");
      REQUIRE(c1._conversationId == "2");
      REQUIRE(c3._conversationId == "2");
      REQUIRE(c1._content == "Welcome back");
      REQUIRE(c3._content == "Welcome back");
      c3.sendMessage("3", "Agent1", "Here I am");
      c3.sendMessage("3", "Agent2", "Here I am");
      std::this_thread::sleep_for(std::chrono::seconds{1});
      REQUIRE(c1._from == "Agent3");
      REQUIRE(c2._from == "Agent3");
      REQUIRE(c1._conversationId == "3");
      REQUIRE(c2._conversationId == "3");
      REQUIRE(c1._content == "Here I am");
      REQUIRE(c2._content == "Here I am");
      std::cerr << "Data sent\n";
      std::this_thread::sleep_for(std::chrono::seconds{1});
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
      c1.sendMessage("1", "Agent2", "Hello world");
      c1.sendMessage("1", "Agent3", "Hello world");
      std::this_thread::sleep_for(std::chrono::seconds{1});
      REQUIRE(c2._from == "Agent1");
      REQUIRE(c3._from == "Agent1");
      REQUIRE(c2._conversationId == "1");
      REQUIRE(c3._conversationId == "1");
      REQUIRE(c2._content == "Hello world");
      REQUIRE(c3._content == "Hello world");
      c2.sendMessage("2", "Agent3", "Welcome back");
      c2.sendMessage("2", "Agent1", "Welcome back");
      std::this_thread::sleep_for(std::chrono::seconds{1});
      REQUIRE(c1._from == "Agent2");
      REQUIRE(c3._from == "Agent2");
      REQUIRE(c1._conversationId == "2");
      REQUIRE(c3._conversationId == "2");
      REQUIRE(c1._content == "Welcome back");
      REQUIRE(c3._content == "Welcome back");
      c3.sendMessage("3", "Agent1", "Here I am");
      c3.sendMessage("3", "Agent2", "Here I am");
      std::this_thread::sleep_for(std::chrono::seconds{1});
      REQUIRE(c1._from == "Agent3");
      REQUIRE(c2._from == "Agent3");
      REQUIRE(c1._conversationId == "3");
      REQUIRE(c2._conversationId == "3");
      REQUIRE(c1._content == "Here I am");
      REQUIRE(c2._content == "Here I am");
      std::cerr << "Data sent\n";
      std::this_thread::sleep_for(std::chrono::seconds{1});
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
