// Company: FETCH.ai
// Author: Jerome Maloberti
// Creation: 09/01/18
//
#include <iostream>
#include <memory>
#include "multiclient.h"
#include "schema.h"
#include "clientmsg.h"
#include <google/protobuf/text_format.h>

using fetch::oef::MultiClient;
using fetch::oef::Conversation;

enum class MeteoClientState {OEF_WAITING_FOR_CANDIDATES = 1,
                             OEF_WAITING_FOR_DELIVERED = 2,
                             CLIENT_WAITING_FOR_PRICE = 3,
                             CLIENT_WAITING_FOR_DATA = 4,
                             CLIENT_DONE};

class MeteoMultiClient : public MultiClient<MeteoClientState,MeteoMultiClient> {
private:
  std::vector<std::string> _candidates;
  std::string _bestStation;
  float _bestPrice = -1.0;
  uint32_t _nbPropose = 0;

  void processCandidates(const fetch::oef::pb::Server_AgentMessage &msg) {
    for(auto &s : msg.agents().agents()) {
      _candidates.emplace_back(s);
      Conversation<MeteoClientState> c{s};
      c.setState(MeteoClientState::OEF_WAITING_FOR_DELIVERED);
      _conversations.insert({c.uuid(), std::make_shared<Conversation<MeteoClientState>>(c)});
      Message cfp(c.uuid(), s, "");
      asyncWriteBuffer(_socket, serialize(cfp.handle()), 5);
    }
  }
  void processOEFStatus(const fetch::oef::pb::Server_AgentMessage &msg) {
    assert(msg.status().has_cid());
    auto iter = _conversations.find(msg.status().cid());
    assert(iter != _conversations.end());
    auto conv = iter->second;
    assert(conv->getState() == MeteoClientState::OEF_WAITING_FOR_DELIVERED);
    switch(conv->msgId()) {
    case 0:
      conv->setState(MeteoClientState::CLIENT_WAITING_FOR_PRICE);
      break;
    case 1:
      if(conv->dest() == _bestStation)
        conv->setState(MeteoClientState::CLIENT_WAITING_FOR_DATA);
      else
        conv->setState(MeteoClientState::CLIENT_DONE);
      break;
    default:
      std::cerr << "Error::Status msgId: " << conv->msgId() << std::endl;
    }
  }
  void processPrice(const std::string &content, const std::string &dest) {
    Data price{content};
    ++_nbPropose;
    assert(price.values().size() == 1);
    float current_price = std::stof(price.values().front());
    if(_bestPrice == -1.0 || _bestPrice > current_price) {
      _bestPrice = current_price;
      _bestStation = dest;
    }
  }
  void processData(const std::string &content) {
    Data data{content};
    std::cerr << "**Received " << data.name() << " = " << data.values().front() << std::endl;
  }
  void processClients(const fetch::oef::pb::Server_AgentMessage &msg, fetch::oef::Conversation<MeteoClientState> &conversation) {
    switch(conversation.getState()) {
    case MeteoClientState::CLIENT_WAITING_FOR_PRICE:
      processPrice(msg.content().content(), conversation.dest());
      if(_nbPropose == _candidates.size()) {
        fetch::oef::pb::Boolean accepted;
        for(auto &p : _conversations) {
          if(p.first != "") {
            auto &conv = p.second;
            accepted.set_status(conv->dest() == _bestStation);
            conv->setState(MeteoClientState::OEF_WAITING_FOR_DELIVERED);
            asyncWriteBuffer(_socket, conv->envelope(accepted), 5);
          }
        }
      }
      break;
    case MeteoClientState::CLIENT_WAITING_FOR_DATA:
      assert(conversation.dest() == _bestStation);
      processData(msg.content().content());
      std::cerr << "Data msgId " << conversation.msgId() << std::endl;
      break;
    default:
      std::cerr << "processClients error " << static_cast<int>(conversation.getState()) << std::endl;
    }
  }
public:
  MeteoMultiClient(asio::io_context &io_context, const std::string &id, const std::string &host) :
    fetch::oef::MultiClient<MeteoClientState,MeteoMultiClient>{io_context, id, host} {
    //    std::this_thread::sleep_for(std::chrono::seconds{1});
    
    // Build up our DataModel (this is identical to the meteostations DataModel, wouldn't work if not)
    Attribute wind        { "wind_speed",   Type::Bool, true};
    Attribute temperature { "temperature",  Type::Bool, true};
    Attribute air         { "air_pressure", Type::Bool, true};
    Attribute humidity    { "humidity",     Type::Bool, true};
    std::vector<Attribute> attributes{wind,temperature,air,humidity};
    DataModel weather{"weather_data", attributes, "All possible weather data."};

    // Create constraints against our Attributes (whether the station CAN provide them)
    ConstraintType eqTrue{Relation{Relation::Op::Eq, true}};
    Constraint temperature_c { temperature, eqTrue};
    Constraint air_c         { air,         eqTrue};
    Constraint humidity_c    { humidity,    eqTrue};

    // Construct a Query schema and send it to the Node
    QueryModel q1{{temperature_c,air_c,humidity_c}, weather};
    // send query
    Query q{q1};
    std::cerr << "Sending query\n";
    asyncWriteBuffer(_socket, serialize(q.handle()), 5);
    Conversation<MeteoClientState> c{""};
    c.setState(MeteoClientState::OEF_WAITING_FOR_CANDIDATES);
    _conversations.insert({"", std::make_shared<Conversation<MeteoClientState>>(c)});
  }
  void onMsg(const fetch::oef::pb::Server_AgentMessage &msg, fetch::oef::Conversation<MeteoClientState> &conversation) {
    std::string output;
    assert(google::protobuf::TextFormat::PrintToString(msg, &output));
    std::cerr << "OnMsg cid " << conversation.uuid() << " dest " << conversation.dest() << " id " << conversation.msgId() << ": " << output << std::endl;
    switch(msg.payload_case()) {
    case fetch::oef::pb::Server_AgentMessage::kAgents: // answer for the query
      assert(_candidates.size() == 0);
      std::cerr << "Expected msgId 1 == " << conversation.msgId() << std::endl;
      processCandidates(msg);
      break;
    case fetch::oef::pb::Server_AgentMessage::kStatus: // oef
      processOEFStatus(msg);
      break;
    case fetch::oef::pb::Server_AgentMessage::kContent: // from a meteostation
      processClients(msg, conversation);
      break;
    case fetch::oef::pb::Server_AgentMessage::PAYLOAD_NOT_SET:
    default:
      assert(false);
    }
  }
};


int main(int argc, char* argv[])
{
  try
  {
    if (argc != 3)
    {
      std::cerr << "Usage: meteoclient <agentID> <host>\n";
      return 1;
    }
    IoContextPool pool(1);
    pool.run();
    MeteoMultiClient client(pool.getIoContext(), argv[1], argv[2]);
    pool.join();
  
  } catch (std::exception& e) {
    std::cerr << "Exception: " << e.what() << "\n";
  }
  return 0;
}
