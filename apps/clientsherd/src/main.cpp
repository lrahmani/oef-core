// Company: FETCH.ai
// Author: Jerome Maloberti
// Creation: 09/01/18
//
#include <iostream>
#include "clara.hpp"
#include <future>
#include "multiclient.h"
#include "oefcoreproxy.hpp"

class SimpleAgent : public fetch::oef::AgentInterface, public fetch::oef::OEFCoreNetworkProxy {
 private:
  fetch::oef::OEFCoreProxy _oefCore;
  
 public:
  std::vector<std::string> _results;
  SimpleAgent(const std::string &agentId, asio::io_context &io_context, const std::string &host)
    : fetch::oef::OEFCoreNetworkProxy{agentId, io_context, host}, _oefCore{*this, *this} {
  }
  void onError(fetch::oef::pb::Server_AgentMessage_Error_Operation operation, const std::string &conversationId, uint32_t msgId) override {}
  void onSearchResult(const std::vector<std::string> &results) override {
    _results = results;
  }
  void onMessage(const std::string &from, const std::string &conversationId, const std::string &content) override {}
  void onCFP(const std::string &from, const std::string &conversationId, uint32_t msgId, uint32_t target, const fetch::oef::CFPType &constraints) override {}
  void onPropose(const std::string &from, const std::string &conversationId, uint32_t msgId, uint32_t target, const fetch::oef::ProposeType &proposals) override {}
  void onAccept(const std::string &from, const std::string &conversationId, uint32_t msgId, uint32_t target) override {}
  void onClose(const std::string &from, const std::string &conversationId, uint32_t msgId, uint32_t target) override {}
 };


int main(int argc, char* argv[])
{
  bool showHelp = false;
  std::string host = "127.0.0.1";
  uint32_t nbAgents = 100;
  std::string prefix = "Agent_";
  
  auto parser = clara::Help(showHelp)
    | clara::Opt(nbAgents, "nbAgents")["--nbAgents"]["-n"]("Number of agents. Default 100.")
    | clara::Opt(prefix, "prefix")["--prefix"]["-p"]("Prefix used for all agents name. Default: Agent_")
    | clara::Opt(host, "host")["--host"]["-h"]("Host address to connect. Default: 127.0.0.1");
  auto result = parser.parse(clara::Args(argc, argv));

  if(showHelp || argc == 1) {
    std::cout << parser << std::endl;
  } 
  // need to increase max nb file open
  // > ulimit -n 8000
  // ulimit -n 1048576

  std::vector<std::unique_ptr<SimpleAgent>> agents;
  std::vector<std::future<std::unique_ptr<SimpleAgent>>> futures;
  IoContextPool pool(10);
  pool.run();
  try {
    for(size_t i = 1; i <= nbAgents; ++i) {
      std::string name = prefix;
      name += std::to_string(i);
      futures.push_back(std::async(std::launch::async, [&pool,&host](const std::string &n){
          return std::make_unique<SimpleAgent>(n, pool.getIoContext(), host.c_str());
      }, name));
    }
    std::cerr << "Futures created\n";
    for(auto &fut : futures) {
      agents.emplace_back(fut.get());
    }
    std::cerr << "Futures got\n";
  } catch(std::exception &e) {
    std::cerr << "BUG " << e.what() << "\n";
  }
  return 0;
}
