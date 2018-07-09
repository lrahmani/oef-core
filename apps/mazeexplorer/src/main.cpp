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

using fetch::oef::MultiClient;
using fetch::oef::Conversation;

using Position = std::pair<uint32_t,uint32_t>;

enum class ExplorerState {OEF_WAITING_FOR_MAZE = 1,
                          OEF_WAITING_FOR_REGISTER,
                          MAZE_WAITING_FOR_REGISTER,
                          OEF_WAITING_FOR_MOVE_DELIVERED,
                          MAZE_WAITING_FOR_MOVE};

enum class GridState : uint8_t { UNKNOWN, WALL, ROOM };

class Explorer : public MultiClient<ExplorerState,Explorer>
{
public:
  Explorer(asio::io_context &io_context, const std::string &id, const std::string &host)
    : MultiClient<ExplorerState,Explorer>(io_context, id, host)
  {
    Attribute version{"version", Type::Int, true};
    std::vector<Attribute> attributes{version};
    std::unordered_map<std::string,std::string> props{{"version", "1"}};
    DataModel maze{"maze", attributes, "Just a maze demo."};
    ConstraintType eqOne{Relation{Relation::Op::Eq, 1}};
    Constraint version_c{version, eqOne};
    QueryModel ql{{version_c}, maze};

    _query = std::unique_ptr<Query>(new Query{ql});

    Conversation<ExplorerState> c{"", ""};
    c.setState(ExplorerState::OEF_WAITING_FOR_MAZE);
    _conversations.insert({"", std::make_shared<Conversation<ExplorerState>>(c)});
    asyncWriteBuffer(_socket, serialize(_query->handle()), 5);
  }
  Explorer(const Explorer &) = delete;
  Explorer operator=(const Explorer &) = delete;

  std::unique_ptr<Query> _query;
  std::unique_ptr<Grid<GridState>> _grid;
  Position _current;
  std::string _maze;
  void processOEFStatus(const fetch::oef::pb::Server_AgentMessage &msg) {
    std::shared_ptr<Conversation<ExplorerState>> conv;
    if(msg.status().has_cid()) {
      auto iter = _conversations.find(msg.status().cid());
      assert(iter != _conversations.end());
      conv = iter->second;
    } else {
      conv = _conversations[""];
    }
    switch(conv->getState()) {
    case ExplorerState::OEF_WAITING_FOR_REGISTER:
      assert(conv->msgId() == 0);
      conv->setState(ExplorerState::MAZE_WAITING_FOR_REGISTER);
      break;
    case ExplorerState::OEF_WAITING_FOR_MOVE_DELIVERED:
      conv->setState(ExplorerState::MAZE_WAITING_FOR_MOVE);
      break;
    default:
      std::cerr << "Error processOEFStatus " << static_cast<int>(conv->getState()) << " msgId " << conv->msgId() << std::endl;
    }
  }
  void processClients(const fetch::oef::pb::Server_AgentMessage &msg, fetch::oef::Conversation<ExplorerState> &conversation) {
  }
  void processAgents(const fetch::oef::pb::Server_AgentMessage &msg, fetch::oef::Conversation<ExplorerState> &conversation) {
    assert(_maze == "");
    assert(msg.has_agents());
    if(msg.agents().agents_size() == 0) { // no answer yet, let's try again 
      asyncWriteBuffer(_socket, serialize(_query->handle()), 5);
    }
    _maze = msg.agents().agents(0);
    std::cerr << "Found maze " << _maze << std::endl;
    fetch::oef::pb::Explorer_Message outgoing;
    (void)outgoing.mutable_register_();
    Conversation<ExplorerState> maze_conversation{_maze};
    maze_conversation.setState(ExplorerState::OEF_WAITING_FOR_REGISTER);
    _conversations.insert({maze_conversation.uuid(), std::make_shared<Conversation<ExplorerState>>(maze_conversation)});
    asyncWriteBuffer(_socket, maze_conversation.envelope(outgoing),5);
  }
  
 public:
  void onMsg(const fetch::oef::pb::Server_AgentMessage &msg, fetch::oef::Conversation<ExplorerState> &conversation) {
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
    case fetch::oef::pb::Server_AgentMessage::kAgents: // answer for the query
      processAgents(msg, conversation);
      break;
    case fetch::oef::pb::Server_AgentMessage::PAYLOAD_NOT_SET:
    default:
      std::cerr << "Error onMsg " << static_cast<int>(conversation.getState()) << " msgId " << conversation.msgId() << std::endl;
    }
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

  std::vector<std::unique_ptr<Explorer>> explorers;
  std::vector<std::future<std::unique_ptr<Explorer>>> futures;
  try {
    for(size_t i = 1; i <= nbClients; ++i) {
      std::string name = prefix;
      name += std::to_string(i);
      futures.push_back(std::async(std::launch::async,
                                   [&host,&pool](const std::string &n){
                                     return std::make_unique<Explorer>(pool.getIoContext(),n, host);
                                   }, name));
    }
    std::cerr << "Futures created\n";
    for(auto &fut : futures) {
      explorers.emplace_back(fut.get());
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
