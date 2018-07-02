// Company: FETCH.ai
// Author: Jerome Maloberti
// Creation: 09/01/18
//
#include <iostream>
#include "clara.hpp"
#include "client.h"
#include <google/protobuf/text_format.h>
#include "grid.h"
#include "maze.pb.h"

using fetch::oef::Client;

using Position = std::pair<uint32_t,uint32_t>;

class Maze : public Client
{
private:
  uint32_t _nbRows, _nbCols;
  Grid<bool> _grid;
  uint32_t _exitRow, _exitCol;
  std::unique_ptr<std::thread> _thread;
  Queue<std::unique_ptr<Conversation>> _conversations;
  std::unordered_map<std::string,Position> _explorers;
  std::random_device _rd;
  std::mt19937 _gen;

public:
  Maze(const std::string &id, const char *host, uint32_t nbRows, uint32_t nbCols)
    : Client{id, host, [this](std::unique_ptr<Conversation> c) { _conversations.push(std::move(c));}},
      _nbRows{nbRows}, _nbCols{nbCols}, _grid{nbRows, nbCols}, _gen{_rd()}
  {
    static Attribute version{"version", Type::Int, true};
    static std::vector<Attribute> attributes{version};
    static std::unordered_map<std::string,std::string> props{{"version", "1"}};
    static DataModel weather{"maze", attributes, "Just a maze demo."};

    initExit();
    //    std::cerr << "Grid:\n" << _grid.to_string();
    Instance instance{weather, props};
    registerAgent(instance);
    _thread = std::make_unique<std::thread>([this]() { run(); });
  }
  Maze(const Maze &) = delete;
  Maze operator=(const Maze &) = delete;

  ~Maze() {
    if(_thread)
      _thread->join();
  }

private:
  void initExit() {
    std::uniform_int_distribution<> sides(0, 3);
    std::uniform_int_distribution<> colsDist(0, _nbCols - 1);
    std::uniform_int_distribution<> rowsDist(0, _nbRows - 1);
    int side = sides(_gen);
    switch(side) {
    case 0:
      _exitRow = 0;
    case 1:
      _exitRow = _nbRows - 1;
      _exitCol = colsDist(_gen);
      break;
    case 2:
      _exitCol = 0;
    case 3:
      _exitCol = _nbCols - 1;
      _exitRow = rowsDist(_gen);
      break;
    default:
      assert(false);
    }
  }

  Position randomPosition() {
    std::uniform_int_distribution<> colsDist(0, _nbCols - 1);
    std::uniform_int_distribution<> rowsDist(0, _nbRows - 1);
    uint32_t row = rowsDist(_gen);
    uint32_t col = colsDist(_gen);
    if(row == _exitRow && col == _exitCol)
      return randomPosition();
    if(_grid.get(row,col)) // it's a wall
      return randomPosition();
    return std::make_pair(row,col);
  }

  void processRegister(const fetch::oef::pb::Explorer_Register &reg,
                       Conversation &conversation,
                       const fetch::oef::pb::Server_AgentMessage_Content &content) {
    std::uniform_int_distribution<> colsDist(0, _nbCols - 1);
    std::uniform_int_distribution<> rowsDist(0, _nbRows - 1);
    auto iter = _explorers.find(content.origin());
    if(iter != _explorers.end()) {
      std::cerr << "Error " << content.origin() << " is already registered\n";
      return;
    }
    Position pos = randomPosition();
    _explorers[content.origin()] = pos;
    fetch::oef::pb::Maze_Message outgoing;
    auto *registered = outgoing.mutable_registered();
    auto position = registered->mutable_pos();
    position->set_row(pos.first);
    position->set_col(pos.second);
    auto dimension = registered->mutable_dim();
    dimension->set_rows(_nbRows);
    dimension->set_cols(_nbCols);
    conversation.send(outgoing);
  }
  
  fetch::oef::pb::Maze_Response checkPosition(const Position &pos) {
    uint32_t row = pos.first;
    uint32_t col = pos.second;
    if(row >= _nbRows || col >= _nbCols)
      return fetch::oef::pb::Maze_Response_WALL;
    if(row == _exitRow && col == _exitCol)
      return fetch::oef::pb::Maze_Response_EXIT;
    if(_grid.get(row, col))
      return fetch::oef::pb::Maze_Response_WALL;
    return fetch::oef::pb::Maze_Response_OK;
  }
  
  void processMove(const fetch::oef::pb::Explorer_Move &mv, Conversation &conversation,
                   const fetch::oef::pb::Server_AgentMessage_Content &content) {
    auto iter = _explorers.find(content.origin());
    assert(iter != _explorers.end());
    Position pos = iter->second;
    fetch::oef::pb::Maze_Response response;
    switch(mv.dir()) {
    case fetch::oef::pb::Explorer_Direction_N:
      if(pos.first == 0) {
        response = fetch::oef::pb::Maze_Response_WALL;
      } else {
        --pos.first;
        response = checkPosition(pos);
      }
      break;
    case fetch::oef::pb::Explorer_Direction_S:
      ++pos.first;
      response = checkPosition(pos);
      break;
    case fetch::oef::pb::Explorer_Direction_E:
      ++pos.second;
      response = checkPosition(pos);
      break;
    case fetch::oef::pb::Explorer_Direction_W:
      if(pos.second == 0) {
        response = fetch::oef::pb::Maze_Response_WALL;
      } else {
        --pos.second;
        response = checkPosition(pos);
      }
      break;
    default:
      assert(false);
    }
    if(response == fetch::oef::pb::Maze_Response_EXIT
       || response == fetch::oef::pb::Maze_Response_OK)
      _explorers[content.origin()] = pos;
    fetch::oef::pb::Maze_Message outgoing;
    auto *moved = outgoing.mutable_moved();
    moved->set_resp(response);
    conversation.send(outgoing);
  }
  void process(std::unique_ptr<Conversation> conversation)
  {
    std::unique_ptr<fetch::oef::pb::Server_AgentMessage> agentMsg = conversation->pop();
    assert(agentMsg->has_content());
    assert(agentMsg->content().has_origin());
    fetch::oef::pb::Explorer_Message incoming;
    incoming.ParseFromString(agentMsg->content().content());
    switch(incoming.msg_case()) {
    case fetch::oef::pb::Explorer_Message::kRegister:
      processRegister(incoming.register_(), *conversation, agentMsg->content());
      break;
    case fetch::oef::pb::Explorer_Message::kMove:
      processMove(incoming.move(), *conversation, agentMsg->content());
      break;
    default:
      assert(false);
    }
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
    | clara::Opt(nbCols, "cols")["--cols"]["-c"]("Number of columns in the mazes.");

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
