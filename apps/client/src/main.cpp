#include <iostream>
#include "multiclient.h"
#include "oefcoreproxy.hpp"

class SimpleAgent : public fetch::oef::Agent {
 public:
  SimpleAgent(const std::string &agentId, asio::io_context &io_context, const std::string &host)
    : fetch::oef::Agent{std::unique_ptr<fetch::oef::OEFCoreInterface>(new fetch::oef::OEFCoreNetworkProxy{agentId, io_context, host})} {
      start();
    }
  void onError(fetch::oef::pb::Server_AgentMessage_Error_Operation operation, const std::string &conversationId, uint32_t msgId) override {}
  void onSearchResult(uint32_t search_id, const std::vector<std::string> &results) override {}
  void onMessage(const std::string &from, const std::string &conversationId, const std::string &content) override {}
  void onCFP(const std::string &from, const std::string &conversationId, uint32_t msgId, uint32_t target, const fetch::oef::CFPType &constraints) override {}
  void onPropose(const std::string &from, const std::string &conversationId, uint32_t msgId, uint32_t target, const fetch::oef::ProposeType &proposals) override {}
  void onAccept(const std::string &from, const std::string &conversationId, uint32_t msgId, uint32_t target) override {}
  void onDecline(const std::string &from, const std::string &conversationId, uint32_t msgId, uint32_t target) override {}
};

int main(int argc, char* argv[])
{
  try
  {
    if (argc != 3)
    {
      std::cerr << "Usage: client <agentID> <host>\n";
      return 1;
    }
    IoContextPool pool(2);
    pool.run();
    SimpleAgent client(argv[1], pool.getIoContext(), argv[2]);
    std::cout << "Enter destination: ";
    std::string destId;
    std::getline(std::cin, destId);
    std::cout << "Enter message: ";
    std::string message;
    std::getline(std::cin, message);
    Uuid uuid = Uuid::uuid4();
    client.sendMessage(uuid.to_string(), destId, message);
    std::cout << "Reply is: ";
    //    std::string reply = client.read(message.size());
    //    std::cout << reply << std::endl;
  }
  catch (std::exception& e)
  {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}
