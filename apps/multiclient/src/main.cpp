#include <iostream>
#include "multiclient.h"

class SimpleMultiClient : public fetch::oef::MultiClient<bool,SimpleMultiClient> {
public:
  SimpleMultiClient(asio::io_context &io_context, const std::string &id, const std::string &host) :
    fetch::oef::MultiClient<bool,SimpleMultiClient>{io_context, id, host} {}
  void onMsg(const fetch::oef::pb::Server_AgentMessage &msg, fetch::oef::Conversation<bool> &Conversation) {
  }
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
    SimpleMultiClient client(pool.getIoContext(), argv[1], argv[2]);
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
