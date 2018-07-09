// Company: FETCH.ai
// Author: Jerome Maloberti
// Creation: 09/01/18
//
#include <iostream>
#include "clara.hpp"
#include "multiclient.h"
#include <google/protobuf/text_format.h>
#include "grid.h"
#include "maze.pb.h"
#include "schema.h"
#include "clientmsg.h"

using fetch::oef::MultiClient;
using fetch::oef::Conversation;

using Position = std::pair<uint32_t,uint32_t>;

enum class MazeState {OEF_WAITING_FOR_REGISTER = 1,
                      OEF_WAITING_FOR_DELIVERED,
                      MAZE_WAITING_FOR_REGISTER,
                      MAZE_WAITING_FOR_MOVE,
                      OEF_WAITING_FOR_MOVE_DELIVERED};

class Maze : public MultiClient<MazeState,Maze>
{
private:
  uint32_t _nbRows, _nbCols;
  Grid<bool> _grid;
  uint32_t _exitRow, _exitCol;
  std::unordered_map<std::string,Position> _explorers;
  std::random_device _rd;
  std::mt19937 _gen;

public:
  Maze(asio::io_context &io_context, const std::string &id, const std::string &host, uint32_t nbRows, uint32_t nbCols)
    : MultiClient<MazeState,Maze>(io_context, id, host),
      _nbRows{nbRows}, _nbCols{nbCols}, _grid{nbRows, nbCols}, _gen{_rd()}
  {
    static Attribute version{"version", Type::Int, true};
    static std::vector<Attribute> attributes{version};
    static std::unordered_map<std::string,std::string> props{{"version", "1"}};
    static DataModel maze{"maze", attributes, "Just a maze demo."};

    initExit();
    //    std::cerr << "Grid:\n" << _grid.to_string();
    Instance instance{maze, props};
    Register reg{instance};
    Conversation<MazeState> c{"",""};
    c.setState(MazeState::OEF_WAITING_FOR_REGISTER);
    _conversations.insert({"", std::make_shared<Conversation<MazeState>>(c)});
    asyncWriteBuffer(_socket, serialize(reg.handle()), 5);
   }

  Maze(const Maze &) = delete;
  Maze operator=(const Maze &) = delete;

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

  void processRegister(const fetch::oef::pb::Explorer_Register &msg, Conversation<MazeState> &conversation) {
    std::uniform_int_distribution<> colsDist(0, _nbCols - 1);
    std::uniform_int_distribution<> rowsDist(0, _nbRows - 1);
    std::string explorer = conversation.dest();
    auto iter = _explorers.find(explorer);
    if(iter != _explorers.end()) {
      std::cerr << "Error " << explorer << " is already registered\n";
      return;
    }
    Position pos = randomPosition();
    _explorers[explorer] = pos;
    fetch::oef::pb::Maze_Message outgoing;
    auto *registered = outgoing.mutable_registered();
    auto position = registered->mutable_pos();
    position->set_row(pos.first);
    position->set_col(pos.second);
    auto dimension = registered->mutable_dim();
    dimension->set_rows(_nbRows);
    dimension->set_cols(_nbCols);
    conversation.setState(MazeState::OEF_WAITING_FOR_DELIVERED);
    asyncWriteBuffer(_socket, conversation.envelope(outgoing), 5);
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
  
  void processMove(const fetch::oef::pb::Explorer_Move &mv, Conversation<MazeState> &conversation) {
    std::string explorer = conversation.dest();
    auto iter = _explorers.find(explorer);
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
      _explorers[explorer] = pos;
    fetch::oef::pb::Maze_Message outgoing;
    auto *moved = outgoing.mutable_moved();
    moved->set_resp(response);
    conversation.setState(MazeState::OEF_WAITING_FOR_MOVE_DELIVERED);
    asyncWriteBuffer(_socket, conversation.envelope(outgoing), 5);
  }
  void processOEFStatus(const fetch::oef::pb::Server_AgentMessage &msg) {
    std::shared_ptr<Conversation<MazeState>> conv;
    if(msg.status().has_cid()) {
      auto iter = _conversations.find(msg.status().cid());
      assert(iter != _conversations.end());
      conv = iter->second;
    } else {
      conv = _conversations[""];
      assert(conv->getState() == MazeState::OEF_WAITING_FOR_REGISTER);
    }
    switch(conv->getState()) {
    case MazeState::OEF_WAITING_FOR_REGISTER:
      assert(conv->msgId() == 1);
      conv->setState(MazeState::MAZE_WAITING_FOR_REGISTER);
      break;
    case MazeState::OEF_WAITING_FOR_DELIVERED:
      conv->setState(MazeState::MAZE_WAITING_FOR_MOVE);
      assert(conv->msgId() == 1);
      break;
    case MazeState::OEF_WAITING_FOR_MOVE_DELIVERED:
      conv->setState(MazeState::MAZE_WAITING_FOR_MOVE);
      assert(conv->msgId() >= 2);
      break;
    default:
      std::cerr << "Error processOEFStatus " << static_cast<int>(conv->getState()) << " msgId " << conv->msgId() << std::endl;
    }
  }
  void processClients(const fetch::oef::pb::Server_AgentMessage &msg, fetch::oef::Conversation<MazeState> &conversation) {
    assert(msg.has_content());
    assert(msg.content().has_origin());
    fetch::oef::pb::Explorer_Message incoming;
    incoming.ParseFromString(msg.content().content());
    std::cerr << "Message from " << msg.content().origin() << " == " << conversation.dest() << std::endl;
    auto explorer_case = incoming.msg_case();
    switch(explorer_case) {
    case fetch::oef::pb::Explorer_Message::kRegister:
      conversation.setState(MazeState::MAZE_WAITING_FOR_REGISTER);
      processRegister(incoming.register_(), conversation);
      break;
    case fetch::oef::pb::Explorer_Message::kMove:
      assert(conversation.getState() == MazeState::MAZE_WAITING_FOR_MOVE);
      processMove(incoming.move(), conversation);
      break;
    default:
      std::cerr << "Error processClients " << static_cast<int>(conversation.getState()) << " msgId " << conversation.msgId() << std::endl;
    }
  }
public:
  void onMsg(const fetch::oef::pb::Server_AgentMessage &msg, fetch::oef::Conversation<MazeState> &conversation) {
    std::string output;
    assert(google::protobuf::TextFormat::PrintToString(msg, &output));
    std::cerr << "OnMsg cid " << conversation.uuid() << " dest " << conversation.dest() << " id " << conversation.msgId() << ": " << output << std::endl;
    switch(msg.payload_case()) {
    case fetch::oef::pb::Server_AgentMessage::kStatus: // oef
      processOEFStatus(msg);
      break;
    case fetch::oef::pb::Server_AgentMessage::kContent: // from an explorer
      processClients(msg, conversation);
      break;
    case fetch::oef::pb::Server_AgentMessage::PAYLOAD_NOT_SET:
    case fetch::oef::pb::Server_AgentMessage::kAgents: // answer for the query
    default:
      std::cerr << "Error onMsg " << static_cast<int>(conversation.getState()) << " msgId " << conversation.msgId() << std::endl;
    }
  }
};

int main(int argc, char* argv[])
{
  bool showHelp = false;
  std::string host;
  uint32_t nbRows = 100, nbCols = 100;
  
  auto parser = clara::Help(showHelp)
    | clara::Arg(host, "host")("Host address to connect.")
    | clara::Opt(nbRows, "rows")["--rows"]["-r"]("Number of rows in the mazes.")
    | clara::Opt(nbCols, "cols")["--cols"]["-c"]("Number of columns in the mazes.");

  auto result = parser.parse(clara::Args(argc, argv));
  if(showHelp || argc == 1)
    std::cout << parser << std::endl;
  else
  {
    IoContextPool pool(1);
    pool.run();
    Maze maze(pool.getIoContext(), "Maze_1", host, nbRows, nbCols);
    pool.join();
  }

  return 0;
}
