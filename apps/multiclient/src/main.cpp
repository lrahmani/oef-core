#include <iostream>
#include "multiclient.h"

class SimpleMultiClient : public fetch::oef::AgentInterface {
public:
  SimpleMultiClient() {}
  void onError(fetch::oef::pb::Server_AgentMessage_Error_Operation operation, const std::string &conversationId, uint32_t msgId) override {}
  void onSearchResult(const std::vector<std::string> &results) override {}
  void onMessage(const std::string &from, const std::string &conversationId, const std::string &content) override {}
};
fetch::oef::Logger fetch::oef::OEFCoreNetworkProxy::logger = fetch::oef::Logger("oefcore-network");
int main(int argc, char* argv[])
{
  IoContextPool pool(2);
  pool.run();
  try
  {
    if (argc != 3)
    {
      std::cerr << "Usage: client <agentID> <host>\n";
      return 1;
    }
    SimpleMultiClient agent;
    fetch::oef::OEFCoreNetworkProxy oefCore{argv[1], pool.getIoContext(), argv[2]};
    fetch::oef::OEFCoreProxy proxy{oefCore, agent};
    // std::cout << "Enter destination: ";
    // std::string destId;
    // std::getline(std::cin, destId);
    // std::cout << "Enter message: ";
    // std::string message;
    // std::getline(std::cin, message);
    // //    client.send(destId, message);
    // std::cout << "Reply is: ";
    //    std::string reply = client.read(message.size());
    //    std::cout << reply << std::endl;
    pool.join();
  }
  catch (std::exception& e)
  {
    std::cerr << "Exception: " << e.what() << "\n";
  }
  return 0;
}
