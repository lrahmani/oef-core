#include <iostream>
#include "multiclient.h"

class SimpleMultiClient : public virtual fetch::oef::AgentInterface, public virtual fetch::oef::OEFCoreNetworkProxy {
public:
  SimpleMultiClient(const std::string &agentId, asio::io_context &io_context, const std::string &host) :
    fetch::oef::OEFCoreInterface{agentId}, fetch::oef::OEFCoreNetworkProxy{agentId, io_context, host} {
      if(handshake())
        loop(*this);
    }
  void onError(fetch::oef::pb::Server_AgentMessage_Error_Operation operation, const std::string &conversationId, uint32_t msgId) override {}
  void onSearchResult(const std::vector<std::string> &results) override {}
  void onMessage(const std::string &from, const std::string &conversationId, const std::string &content) override {}
  void onCFP(const std::string &from, const std::string &conversationId, uint32_t msgId, uint32_t target, const fetch::oef::CFPType &constraints) override {}
  void onPropose(const std::string &from, const std::string &conversationId, uint32_t msgId, uint32_t target, const fetch::oef::ProposeType &proposals) override {}
  void onAccept(const std::string &from, const std::string &conversationId, uint32_t msgId, uint32_t target) override {}
  void onClose(const std::string &from, const std::string &conversationId, uint32_t msgId, uint32_t target) override {}
 };

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
    SimpleMultiClient agent{argv[1], pool.getIoContext(), argv[2]};
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
