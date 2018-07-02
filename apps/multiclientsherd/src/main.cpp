// Company: FETCH.ai
// Author: Jerome Maloberti
// Creation: 09/01/18
//
#include <iostream>
#include "multiclient.h"
#include "clara.hpp"
#include <future>

using fetch::oef::MultiClient;

class SimpleMultiClient : public MultiClient<bool,SimpleMultiClient> {
public:
  SimpleMultiClient(asio::io_context &io_context, const std::string &id, const std::string &host) :
    fetch::oef::MultiClient<bool,SimpleMultiClient>{io_context, id, host} {}
  void onMsg(const fetch::oef::pb::Server_AgentMessage &msg, fetch::oef::Conversation<bool> &Conversation) {
  }
};

int main(int argc, char* argv[])
{
  bool showHelp = false;
  std::string host = "127.0.0.1";
  uint32_t nbClients = 100;
  std::string prefix = "Agent_";
  
  auto parser = clara::Help(showHelp)
    | clara::Opt(nbClients, "nbClients")["--nbClients"]["-n"]("Number of clients. Default 100.")
    | clara::Opt(prefix, "prefix")["--prefix"]["-p"]("Prefix used for all agents name. Default: Agent_")
    | clara::Opt(host, "host")["--host"]["-h"]("Host address to connect. Default: 127.0.0.1");
  auto result = parser.parse(clara::Args(argc, argv));

  if(showHelp || argc == 1) {
    std::cout << parser << std::endl;
  } 
  // need to increase max nb file open
  // > ulimit -n 8000
  // ulimit -n 1048576
  IoContextPool pool(4);
  pool.run();

  std::vector<std::unique_ptr<SimpleMultiClient>> clients;
  std::vector<std::future<std::unique_ptr<SimpleMultiClient>>> futures;
  try {
    for(size_t i = 1; i <= nbClients; ++i) {
      std::string name = prefix;
      name += std::to_string(i);
      futures.push_back(std::async(std::launch::async,
                                   [&host,&pool](const std::string &n){
                                     return std::make_unique<SimpleMultiClient>(pool.getIoContext(),n, host.c_str());
                                   }, name));
    }
    std::cerr << "Futures created\n";
    for(auto &fut : futures) {
      clients.emplace_back(fut.get());
    }
    std::cerr << "Futures got\n";
  } catch(std::exception &e) {
    std::cerr << "BUG " << e.what() << "\n";
  }
  std::cerr << "Start sleeping ...\n";
  std::this_thread::sleep_for(std::chrono::seconds{(nbClients / 500) + 2});
  std::cerr << "Stopped sleeping ...\n";
  pool.stop();
  return 0;
}
