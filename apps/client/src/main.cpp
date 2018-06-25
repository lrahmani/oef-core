#include <iostream>
#include "client.h"

int main(int argc, char* argv[])
{
  try
  {
    if (argc != 3)
    {
      std::cerr << "Usage: client <agentID> <host>\n";
      return 1;
    }
    fetch::oef::Client client(argv[1], argv[2],[](std::unique_ptr<Conversation>){});
    std::cout << "Enter destination: ";
    std::string destId;
    std::getline(std::cin, destId);
    std::cout << "Enter message: ";
    std::string message;
    std::getline(std::cin, message);
    client.send(destId, message);
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
