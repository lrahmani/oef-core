// Company: FETCH.ai
// Author: Jerome Maloberti
// Creation: 09/01/18
//
#include <iostream>
#include "clara.hpp"
#include "multiclient.h"
#include <google/protobuf/text_format.h>
#include <future>
#include "grid.h"
#include "maze.pb.h"
#include "schema.h"
#include "clientmsg.h"
#include <stack>
#include "mapbox/variant.hpp"

namespace var = mapbox::util; // for the variant

using fetch::oef::MultiClient;
using fetch::oef::Conversation;

enum class OEFState {OEF_WAITING_FOR_MAZE = 1,
                     OEF_WAITING_FOR_NOTHING,
                     OEF_WAITING_FOR_REGISTER,
                     OEF_WAITING_FOR_SELLERS};
enum class ExplorerState {OEF_WAITING_FOR_REGISTER = 1,
                          MAZE_WAITING_FOR_REGISTER,
                          OEF_WAITING_FOR_MOVE_DELIVERED,
                          MAZE_WAITING_FOR_MOVE};

enum class SellerState {WAITING_FOR_CFP = 1,
                        OEF_WAITING_FOR_PROPOSE,
                        WAITING_FOR_AGREEMENT,
                        OEF_WAITING_FOR_TRANSACTION,
                        OEF_WAITING_FOR_RESOURCES};

enum class BuyerState {OEF_WAITING_FOR_AGENTS = 1,
                       OEF_WAITING_FOR_CFP,
                       WAITING_FOR_PROPOSE,
                       OEF_WAITING_FOR_ACCEPT,
                       OEF_WAITING_FOR_REFUSE,
                       WAITING_FOR_TRANSACTION,
                       WAITING_FOR_RESOURCES};
                        
using VariantState = var::variant<OEFState,ExplorerState,SellerState,BuyerState>;

std::string to_string(const VariantState &s) {
  std::string res;
  s.match([&res](ExplorerState state) {
            res = "ExplorerState " + std::to_string(static_cast<int>(state));
          }, [&res](OEFState state) {
            res = "OEFState " + std::to_string(static_cast<int>(state));
          }, [&res](SellerState state) {
            res = "SellerState " + std::to_string(static_cast<int>(state));
          }, [&res](BuyerState state) {
            res = "BuyerState " + std::to_string(static_cast<int>(state));
             });
  return res;
}

enum class GridState : uint8_t { UNKNOWN, WALL, ROOM, VISITED_ROOM };

class Explorer : public MultiClient<VariantState,Explorer>
{
private:
  uint64_t _account;
  uint32_t _steps = 0;
  std::unique_ptr<Query> _mazeQuery;
  std::unique_ptr<Grid<GridState>> _grid;
  Position _current;
  fetch::oef::pb::Explorer_Direction _dir;
  std::string _maze;
  std::random_device _rd;
  std::mt19937 _gen;
  std::stack<fetch::oef::pb::Explorer_Direction> _path;

  void processOEFStatus(const fetch::oef::pb::Server_AgentMessage &msg) {
    // it is getting complicated with sellers and buyers, then oef messages for maze and trades are mixed.
    // not clear how to disembiguate. Maybe just ignore for now.
    std::shared_ptr<Conversation<VariantState>> conv;
    if(msg.status().has_cid()) {
      auto iter = _conversations.find(msg.status().cid());
      assert(iter != _conversations.end());
      conv = iter->second;
    } else {
      conv = _conversations[""];
    }
    conv->getState().match([&conv](ExplorerState s) {
                             switch(s) {
                             case ExplorerState::OEF_WAITING_FOR_REGISTER:
                               assert(conv->msgId() == 0);
                               conv->setState(ExplorerState::MAZE_WAITING_FOR_REGISTER);
                               break;
                             case ExplorerState::OEF_WAITING_FOR_MOVE_DELIVERED:
                               conv->setState(ExplorerState::MAZE_WAITING_FOR_MOVE);
                               break;
                             default:
                               std::cerr << "Error processOEFStatus ExplorerState " << static_cast<int>(s) << " msgId " << conv->msgId() << std::endl;
                             }},[&conv](OEFState s) {
                                  switch(s) {
                                  case OEFState::OEF_WAITING_FOR_REGISTER:
                                    std::cerr << "Seller registered\n";
                                    conv->setState(OEFState::OEF_WAITING_FOR_NOTHING);
                                    break;
                                  default:
                                    std::cerr << "Error processOEFStatus OEFState " << static_cast<int>(s) << " msgId " << conv->msgId() << std::endl;
                                  }
                                },[](SellerState s) {
                                  }, [](BuyerState s) {});
  }
  std::vector<fetch::oef::pb::Explorer_Direction> filterMove(const Position &pos, GridState val) const {
    std::vector<fetch::oef::pb::Explorer_Direction> res;
    if((pos.first > 0) && (_grid->get(pos.first - 1, pos.second) == val)) {
      res.push_back(fetch::oef::pb::Explorer_Direction_N);
    }
    if((pos.first < (_grid->rows() - 1)) && (_grid->get(pos.first + 1, pos.second) == val)) {
      res.push_back(fetch::oef::pb::Explorer_Direction_S);
    }
    if((pos.second > 0) && (_grid->get(pos.first, pos.second - 1) == val)) {
      res.push_back(fetch::oef::pb::Explorer_Direction_W);
    }
    if((pos.second < (_grid->cols() - 1)) && (_grid->get(pos.first, pos.second + 1) == val)) {
      res.push_back(fetch::oef::pb::Explorer_Direction_E);
    }
    return res;
  }
  fetch::oef::pb::Explorer_Direction choose(const std::vector<fetch::oef::pb::Explorer_Direction> &vals) {
    if(vals.size() == 1)
      return vals.front();
    std::uniform_int_distribution<> _dist(0, vals.size() - 1);
    int r = _dist(_gen);
    return vals[r];
  }
  fetch::oef::pb::Explorer_Direction generateRandomMove() {
    auto unknowns = filterMove(_current, GridState::UNKNOWN);
    assert(unknowns.size() == 0);
    auto rooms = filterMove(_current, GridState::ROOM);
    if(rooms.size() > 0)
      return choose(rooms);
    return choose(filterMove(_current, GridState::VISITED_ROOM));
  }
  fetch::oef::pb::Explorer_Direction backtrack() {
    assert(!_path.empty());
    auto dir = _path.top();
    _path.pop();
    switch(dir) {
    case fetch::oef::pb::Explorer_Direction_N:
      return fetch::oef::pb::Explorer_Direction_S;
    case fetch::oef::pb::Explorer_Direction_S:
      return fetch::oef::pb::Explorer_Direction_N;
    case fetch::oef::pb::Explorer_Direction_W:
      return fetch::oef::pb::Explorer_Direction_E;
    case fetch::oef::pb::Explorer_Direction_E:
      return fetch::oef::pb::Explorer_Direction_W;
    }
  }
  fetch::oef::pb::Explorer_Direction generateMove() {
    auto rooms = filterMove(_current, GridState::ROOM);
    if(rooms.size() == 0)
      return backtrack();
    _path.push(rooms.front());
    return rooms.front();
  }
  void sendMove(Conversation<VariantState> &conversation) {
    _dir = generateMove();
    std::cerr << "Sending move " << static_cast<int>(_dir) << std::endl;
    fetch::oef::pb::Explorer_Message outgoing;
    auto *explorer_mv = outgoing.mutable_move();
    explorer_mv->set_dir(_dir);
    conversation.setState(ExplorerState::OEF_WAITING_FOR_MOVE_DELIVERED);
    asyncWriteBuffer(_socket, conversation.envelope(outgoing), 5);
  }
  Position newPos(const Position &oldpos, fetch::oef::pb::Explorer_Direction dir) const {
    Position pos = oldpos;
    switch(dir) {
    case fetch::oef::pb::Explorer_Direction_N:
      --pos.first; break;
    case fetch::oef::pb::Explorer_Direction_S:
      ++pos.first; break;
    case fetch::oef::pb::Explorer_Direction_W:
      --pos.second; break;
    case fetch::oef::pb::Explorer_Direction_E:
      ++pos.second; break;
    }
    return pos;
  }
  void updateGrid(fetch::oef::pb::Maze_Cell cell, const Position &pos, int deltaRow, int deltaCol) {
    if((deltaRow < 0 && pos.first < abs(deltaRow)) || (deltaCol < 0 && pos.second < abs(deltaCol)))
      return ;
    Position newPos = std::make_pair(pos.first + deltaRow, pos.second + deltaCol);
    if(newPos.first >= _grid->rows() || newPos.second >= _grid->cols()) 
      return ;
    auto gridCell = cell == fetch::oef::pb::Maze_Cell::Maze_Cell_WALL ? GridState::WALL : GridState::ROOM;
    auto current = _grid->get(newPos);
    if(current != GridState::VISITED_ROOM) {
      assert(current == GridState::UNKNOWN || current == gridCell);
      _grid->set(newPos, gridCell);
    } else {
      assert(gridCell == GridState::ROOM);
    }
  }
  void updateGrid(const fetch::oef::pb::Maze_Environment &env, const Position &pos) {
    // set the grid appropriately and check that it is consistent with previous info
    updateGrid(env.north(), pos, -1, 0);
    updateGrid(env.south(), pos, 1, 0);
    updateGrid(env.west(), pos, 0, -1);
    updateGrid(env.east(), pos, 0, 1);
  }
  void registerSeller() {
    static Attribute mazeName{"maze_name", Type::String, true};
    static std::vector<Attribute> attributes{mazeName};
    static DataModel seller{"maze_seller", attributes, "Just a maze demo."};
    static bool registered = false;
    if(_steps == 10 && !registered) {
      registered = true;
      std::unordered_map<std::string,std::string> props{{"maze_name", _maze}};
      Instance instance{seller, props};
      Register reg{instance};
      std::cerr << "Registering seller " << _id << std::endl;
      auto conv = _conversations[""];
      conv->setState(OEFState::OEF_WAITING_FOR_REGISTER);
      asyncWriteBuffer(_socket, serialize(reg.handle()), 5);
    }
  }
  void processMoved(const fetch::oef::pb::Maze_Moved &mv, Conversation<VariantState> &conversation) {
    auto state = conversation.getState().get<ExplorerState>();
    assert(state == ExplorerState::MAZE_WAITING_FOR_MOVE);
    std::string output;
    assert(google::protobuf::TextFormat::PrintToString(mv, &output));
    std::cerr << "Moved " << output << std::endl;
    auto response = mv.resp();
    switch(response) {
    case fetch::oef::pb::Maze_Response_IMPOSSIBLE: // should not happen, unless the agent is dumb.
      {
        Position pos = newPos(_current, _dir);
        _grid->set(pos, GridState::WALL);
        sendMove(conversation);
      }
      break;
    case fetch::oef::pb::Maze_Response_OK:
      _current = newPos(_current, _dir);
      _grid->set(_current, GridState::VISITED_ROOM);
      updateGrid(mv.env(), _current);
      sendMove(conversation);
      ++_steps;
      break;
    case fetch::oef::pb::Maze_Response_EXITED:
      _current = newPos(_current, _dir);
      updateGrid(mv.env(), _current);
      ++_steps;
      std::cerr << "Youhou, exit is " << _current.first << ":" << _current.second << std::endl << _grid->to_string() << std::endl;
      break;
    case fetch::oef::pb::Maze_Response_NOT_NOW:
    default:
      std::cerr << "Error processMoved " << static_cast<int>(state) << " msgId " << conversation.msgId() << std::endl;
    }
    registerSeller();
    std::cerr << "Moved\n" << _grid->to_string() << std::endl;
  }
  void processRegistered(const fetch::oef::pb::Maze_Registered &reg, Conversation<VariantState> &conversation) {
    std::string output;
    assert(google::protobuf::TextFormat::PrintToString(reg, &output));
    std::cerr << "Registered " << output << std::endl;
    auto &pos = reg.pos();
    auto &dim = reg.dim();
    _grid = std::unique_ptr<Grid<GridState>>(new Grid<GridState>(dim.rows(), dim.cols()));
    _current = std::make_pair<uint32_t, uint32_t>(pos.row(), pos.col());
    _grid->set(_current, GridState::VISITED_ROOM);
    updateGrid(reg.env(), _current);
    std::cerr << "Grid:\n" << _grid->to_string() << std::endl;
    sendMove(conversation);
  }
  void processClients(const fetch::oef::pb::Server_AgentMessage &msg, fetch::oef::Conversation<VariantState> &conversation) {
    assert(msg.has_content());
    assert(msg.content().has_origin());
    conversation.getState().match([&msg, &conversation, this](ExplorerState state) {
                                    fetch::oef::pb::Maze_Message incoming;
                                    incoming.ParseFromString(msg.content().content());
                                    std::cerr << "Message from " << msg.content().origin() << " == " << conversation.dest() << std::endl;
                                    auto maze_case = incoming.msg_case();
                                    switch(maze_case) {
                                    case fetch::oef::pb::Maze_Message::kRegistered:
                                      assert(state == ExplorerState::MAZE_WAITING_FOR_REGISTER);
                                      processRegistered(incoming.registered(), conversation);
                                      break;
                                    case fetch::oef::pb::Maze_Message::kMoved:
                                      assert(state == ExplorerState::MAZE_WAITING_FOR_MOVE);
                                      processMoved(incoming.moved(), conversation);
                                      break;
                                    default:
                                      std::cerr << "Error processClients " << static_cast<int>(state) << " msgId " << conversation.msgId() << std::endl;
                                    }
                                  },[](OEFState s) {
                                    },[](SellerState s) {
                                      }, [](BuyerState s) {});
  }
  void processMaze(const fetch::oef::pb::Server_AgentMessage &msg) {
    if(msg.agents().agents_size() == 0) { // no answer yet, let's try again 
      asyncWriteBuffer(_socket, serialize(_mazeQuery->handle()), 5);
    } else {
      _maze = msg.agents().agents(0);
      std::cerr << "Found maze " << _maze << std::endl;
      fetch::oef::pb::Explorer_Message outgoing;
      (void)outgoing.mutable_register_();
      Conversation<VariantState> maze_conversation{_maze};
      maze_conversation.setState(ExplorerState::OEF_WAITING_FOR_REGISTER);
      _conversations.insert({maze_conversation.uuid(), std::make_shared<Conversation<VariantState>>(maze_conversation)});
      asyncWriteBuffer(_socket, maze_conversation.envelope(outgoing),5);
    }
  }
  void processSellers(const fetch::oef::pb::Server_AgentMessage &msg) {
    if(msg.agents().agents_size() > 0) { // no answer yet, let's try again
      // TODO store the sellers and send CFP to each, create conversations too.
    }
  }
  void processAgents(const fetch::oef::pb::Server_AgentMessage &msg, fetch::oef::Conversation<VariantState> &conversation) {
    assert(_maze == "");
    assert(msg.has_agents());
    conversation.getState().match([&msg,&conversation,this](OEFState s) {
                                    switch(s) {
                                    case OEFState::OEF_WAITING_FOR_MAZE:
                                      processMaze(msg);
                                      conversation.setState(OEFState::OEF_WAITING_FOR_NOTHING);
                                      break;
                                    case OEFState::OEF_WAITING_FOR_SELLERS:
                                      processSellers(msg);
                                      conversation.setState(OEFState::OEF_WAITING_FOR_NOTHING);
                                      break;
                                    case OEFState::OEF_WAITING_FOR_NOTHING:
                                    case OEFState::OEF_WAITING_FOR_REGISTER:
                                      std::cerr << "Error processAgents " << to_string(s) << std::endl;
                                      assert(false);
                                    }
                                  }, [](ExplorerState s) {
                                       std::cerr << "Error processAgents " << to_string(s) << std::endl;
                                       assert(false);
                                     }, [](BuyerState s) {
                                       std::cerr << "Error processAgents " << to_string(s) << std::endl;
                                       assert(false);
                                     }, [](SellerState s) {
                                       std::cerr << "Error processAgents " << to_string(s) << std::endl;
                                       assert(false);
                                        });
  }
  
public:
  Explorer(asio::io_context &io_context, const std::string &id, const std::string &host, uint64_t account)
    : MultiClient<VariantState,Explorer>{io_context, id, host}, _account{account}, _gen{_rd()}
  {
    static Attribute version{"version", Type::Int, true};
    static std::vector<Attribute> attributes{version};
    static std::unordered_map<std::string,std::string> props{{"version", "1"}};
    static DataModel maze{"maze", attributes, "Just a maze demo."};
    static ConstraintType eqOne{Relation{Relation::Op::Eq, 1}};
    static Constraint version_c{version, eqOne};
    static QueryModel ql{{version_c}, maze};

    _mazeQuery = std::unique_ptr<Query>(new Query{ql});

    Conversation<VariantState> c{"", ""};
    c.setState(VariantState{OEFState::OEF_WAITING_FOR_MAZE});
    _conversations.insert({"", std::make_shared<Conversation<VariantState>>(c)});
    asyncWriteBuffer(_socket, serialize(_mazeQuery->handle()), 5);
  }
  Explorer(const Explorer &) = delete;
  Explorer operator=(const Explorer &) = delete;

  void onMsg(const fetch::oef::pb::Server_AgentMessage &msg, fetch::oef::Conversation<VariantState> &conversation) {
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
    case fetch::oef::pb::Server_AgentMessage::kAgents: // answer for the queries
      processAgents(msg, conversation);
      break;
    case fetch::oef::pb::Server_AgentMessage::PAYLOAD_NOT_SET:
    default:
      std::cerr << "Error onMsg " << to_string(conversation.getState()) << " msgId " << conversation.msgId() << std::endl;
    }
  }  
};

int main(int argc, char* argv[])
{
  bool showHelp = false;
  std::string host = "127.0.0.1";
  uint32_t nbClients = 100;
  std::string prefix = "Agent_";
  uint64_t account = 0;
  
  auto parser = clara::Help(showHelp)
    | clara::Opt(nbClients, "nbClients")["--nbClients"]["-n"]("Number of clients. Default 100.")
    | clara::Opt(account, "account")["--account"]["-a"]("Initial amount of tokens. Default 0.")
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

  std::vector<std::unique_ptr<Explorer>> explorers;
  try {
    for(size_t i = 1; i <= nbClients; ++i) {
      std::string name = prefix;
      name += std::to_string(i);
      explorers.emplace_back(std::make_unique<Explorer>(pool.getIoContext(), name, host, account));
    }
  } catch(std::exception &e) {
    std::cerr << "BUG " << e.what() << "\n";
  }
  pool.join();
  return 0;
}
