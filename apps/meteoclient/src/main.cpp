// Company: FETCH.ai
// Author: Jerome Maloberti
// Creation: 09/01/18
//
#include <iostream>
#include <unordered_map>

#include "oefcoreproxy.hpp"
#include "multiclient.h"
#include "uuid.h"

class MeteoClientAgent : public fetch::oef::AgentInterface, public fetch::oef::OEFCoreNetworkProxy {
 private:
  fetch::oef::OEFCoreProxy _oefCore;
  std::unordered_map<std::string,std::string> _conversationsIds;
  std::string _bestStation;
  float _bestPrice = -1.0;
  size_t _nbAnswers = 0;
  bool _waitingForData = false;
  uint32_t _dataReceived = 0;

public:
  MeteoClientAgent(const std::string &agentId, asio::io_context &io_context, const std::string &host)
    : fetch::oef::OEFCoreNetworkProxy{agentId, io_context, host}, _oefCore{*this, *this} {
  }
  void onError(fetch::oef::pb::Server_AgentMessage_Error_Operation operation, const std::string &conversationId, uint32_t msgId) override {}
  void onSearchResult(const std::vector<std::string> &results) override {
    if(results.size() == 0)
      std::cerr << "No candidates\n";
    for(auto &c : results) {
      auto uuid = Uuid::uuid4();
      _conversationsIds[c] = uuid.to_string();
      sendMessage(uuid.to_string(), c, ""); // should be cfp
    }
  }
  void onMessage(const std::string &from, const std::string &conversationId, const std::string &content) override {
    if(!_waitingForData) {
      Data price{content};
      assert(price.values().size() == 1);
      float currentPrice = std::stof(price.values().front());
      if(_bestPrice == -1.0 || _bestPrice > currentPrice) {
        _bestPrice = currentPrice;
        _bestStation = from;
      }
      ++_nbAnswers;
      std::cerr << "Nb Answers " << _nbAnswers << " " << _conversationsIds.size() << std::endl;
      if(_nbAnswers == _conversationsIds.size()) {
        _waitingForData = true;
        std::cerr << "Best station " << _bestStation << " price " << _bestPrice << std::endl;
        fetch::oef::pb::Boolean accepted;
        for(auto &p : _conversationsIds) {
          accepted.set_status(p.first == _bestStation);
          sendMessage(p.second, p.first, accepted.SerializeAsString());
        }
      }
    } else {
      assert(from == _bestStation);
      Data temp{content};
      std::cerr << "**Received " << temp.name() << " = " << temp.values().front() << std::endl;
      ++_dataReceived;
      if(_dataReceived == 3)
        stop();
    }
  }
  void onCFP(const std::string &from, const std::string &conversationId, uint32_t msgId, uint32_t target, const stde::optional<QueryModel> &constraints) override {}
  void onPropose(const std::string &from, const std::string &conversationId, uint32_t msgId, uint32_t target, const std::vector<Instance> &proposals) override {}
  void onAccept(const std::string &from, const std::string &conversationId, uint32_t msgId, uint32_t target, const std::vector<Instance> &proposals) override {}
  void onClose(const std::string &from, const std::string &conversationId, uint32_t msgId, uint32_t target) override {}
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
    spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [thread %t] [%n] [%l] %v");
    spdlog::set_level(spdlog::level::level_enum::trace);
    IoContextPool pool(2);
    pool.run();
    
    MeteoClientAgent client(argv[1], pool.getIoContext(), argv[2]);

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
    client.searchServices(q1);
    pool.join();
  } catch (std::exception& e) {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}
