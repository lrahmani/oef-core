// Company: FETCH.ai
// Author: Jerome Maloberti
// Creation: 09/01/18
//
#include <iostream>
#include "clara.hpp"
#include "client.h"
#include <google/protobuf/text_format.h>

using fetch::oef::Client;

using Position = std::pair<uint32_t,uint32_t>;

class Maze : public Client
{
public:
  Maze(const std::string &id, const char *host, uint32_t nbRows, uint32_t nbCols)
    : Client{id, host, [this](std::unique_ptr<Conversation> c) { _conversations.push(std::move(c));}},
      _nbRows{nbRows}, _nbCols{nbCols}
  {
    static Attribute version{"version", Type::Int, true};
    static std::vector<Attribute> attributes{version};
    static std::unordered_map<std::string,std::string> props{{"version", "1"}};
    static DataModel weather{"maze", attributes, "Just a maze demo."};

    Instance instance{weather, props};
    registerAgent(instance);
    _thread = std::make_unique<std::thread>([this]() { run(); });
  }
  Maze(const Maze &) = delete;
  Maze operator=(const Maze &) = delete;

  ~Maze()
  {
    if(_thread)
      _thread->join();
  }

private:
  uint32_t _nbRows, _nbCols;
  std::unique_ptr<std::thread> _thread;
  Queue<std::unique_ptr<Conversation>> _conversations;
  std::unordered_map<std::string,Position> _explorers;
  
  void process(std::unique_ptr<Conversation> conversation)
  {
    // waiting for registration -> keep track of each explorer's position
    // waiting for explorer's movement

    /*
    std::unique_ptr<fetch::oef::pb::Server_AgentMessage> agentMsg = conversation->pop();
    Data price{"price", "float", {std::to_string(_unitPrice)}};
    conversation->send(price.handle());
    std::cerr << "Price sent\n";
    auto accepted = conversation->popMsg<fetch::oef::pb::Boolean>();
    if(accepted.status()) {
      // let's send the data
      std::cerr << "I won!\n";
      std::random_device rd;
      std::mt19937 g(rd());
      std::normal_distribution<float> dist{15.0, 2.0};
      Data temp{"temperature", "float", {std::to_string(dist(g))}};
      conversation->send(temp.handle());
      Data air{"air pressure", "float", {std::to_string(dist(g))}};
      conversation->send(air.handle());
      Data humid{"humidity", "float", {std::to_string(dist(g))}};
      conversation->send(humid.handle());
    } else {
      // too bad
      std::cerr << "I lost\n";
    }
    */
  }
  void run()
  {
    process(std::move(_conversations.pop()));
  }
};

int main(int argc, char* argv[])
{
  uint32_t nbMazes = 1;
  bool showHelp = false;
  std::string host;
  uint32_t nbRows = 100, nbCols = 100;
  
  auto parser = clara::Help(showHelp)
    | clara::Arg(host, "host")("Host address to connect.")
    | clara::Opt(nbMazes, "mazes")["--mazes"]["-n"]("Number mazes to generate.")
    | clara::Opt(nbRows, "rows")["--rows"]["-r"]("Number of rows in the mazes.")
    | clara::Opt(nbRows, "cols")["--cols"]["-r"]("Number of columns in the mazes.");

  auto result = parser.parse(clara::Args(argc, argv));
  if(showHelp || argc == 1)
    std::cout << parser << std::endl;
  else
  {
    try
    {
      std::vector<std::unique_ptr<Maze>> mazes;
      for(uint32_t i = 1; i <= nbMazes; ++i)
      {
        std::string name = "Maze_" + std::to_string(i);
        mazes.emplace_back(std::unique_ptr<Maze>(new Maze{name, host.c_str(), nbRows, nbCols}));
      }
    } catch(std::exception &e) {
      std::cerr << "Exception " << e.what() << std::endl;
    }
  }

  return 0;
}
