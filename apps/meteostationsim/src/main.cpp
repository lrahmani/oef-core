// Company: FETCH.ai
// Author: Jerome Maloberti
// Creation: 09/01/18
//
#include <iostream>
#include "clara.hpp"
#include <google/protobuf/text_format.h>
#include "multiclient.h"
#include "oefcoreproxy.hpp"
#include <unordered_set>

class MeteoStation : public virtual fetch::oef::AgentInterface, public virtual fetch::oef::OEFCoreNetworkProxy
{
private:
  float _unitPrice;
  std::unordered_set<std::string> _conversations;
  
public:
  MeteoStation(const std::string &agentId, asio::io_context &io_context, const std::string &host)
    : fetch::oef::OEFCoreInterface{agentId}, fetch::oef::OEFCoreNetworkProxy{agentId, io_context, host} {
      if(handshake())
        loop(*this);
      
      static std::vector<VariantType> properties = { VariantType{true}, VariantType{true}, VariantType{true}, VariantType{false}};
      static std::random_device rd;
      static std::mt19937 g(rd());
      static Attribute wind{"wind_speed", Type::Bool, true};
      static Attribute temp{"temperature", Type::Bool, true};
      static Attribute air{"air_pressure", Type::Bool, true};
      static Attribute humidity{"humidity", Type::Bool, true};
      static std::vector<Attribute> attributes{wind,temp,air,humidity};
      static DataModel weather{"weather_data", attributes, "All possible weather data."};
      
      std::shuffle(properties.begin(), properties.end(), g);
      static std::normal_distribution<float> dist{1.0, 0.1}; // mean,stddev
      _unitPrice = dist(g);
      std::cerr << _agentPublicKey << " " << _unitPrice << std::endl;
      std::unordered_map<std::string,VariantType> props;
      int i = 0;
      for(auto &a : attributes) {
        props[a.name()] = properties[i];
        ++i;
      }
      Instance instance{weather, props};
      registerService(instance);
    }
  MeteoStation(const MeteoStation &) = delete;
  MeteoStation operator=(const MeteoStation &) = delete;

  void onError(fetch::oef::pb::Server_AgentMessage_Error_Operation operation, const std::string &conversationId, uint32_t msgId) override {}
  void onSearchResult(const std::vector<std::string> &results) override {}
  void onMessage(const std::string &from, const std::string &conversationId, const std::string &content) override {
    if(_conversations.find(conversationId) == _conversations.end()) { // first contact
      _conversations.insert(conversationId);
      Data price{"price", "float", {std::to_string(_unitPrice)}};
      sendMessage(conversationId, from, price.handle().SerializeAsString());
    } else {
      fetch::oef::pb::Boolean accepted;
      accepted.ParseFromString(content);
      if(accepted.status()) {
        // let's send the data
        std::cerr << "I won!\n";
        std::random_device rd;
        std::mt19937 g(rd());
        std::normal_distribution<float> dist{15.0, 2.0};
        Data temp{"temperature", "float", {std::to_string(dist(g))}};
        sendMessage(conversationId, from, temp.handle().SerializeAsString());
        Data air{"air pressure", "float", {std::to_string(dist(g))}};
        sendMessage(conversationId, from, air.handle().SerializeAsString());
        Data humid{"humidity", "float", {std::to_string(dist(g))}};
        sendMessage(conversationId, from, humid.handle().SerializeAsString());
      } else {
        std::cerr << "I lost\n";
      }
    }
  }
  void onCFP(const std::string &from, const std::string &conversationId, uint32_t msgId, uint32_t target, const fetch::oef::CFPType &constraints) override {}
  void onPropose(const std::string &from, const std::string &conversationId, uint32_t msgId, uint32_t target, const fetch::oef::ProposeType &proposals) override {}
  void onAccept(const std::string &from, const std::string &conversationId, uint32_t msgId, uint32_t target) override {}
  void onClose(const std::string &from, const std::string &conversationId, uint32_t msgId, uint32_t target) override {}
};

int main(int argc, char* argv[])
{
  uint32_t nbStations = 1;
  bool showHelp = false;
  std::string host;

  auto parser = clara::Help(showHelp)
    | clara::Arg(host, "host")("Host address to connect.")
    | clara::Opt(nbStations, "stations")["--stations"]["-n"]("Number of meteo stations to simulate.");

  auto result = parser.parse(clara::Args(argc, argv));
  if(showHelp || argc == 1)
    std::cout << parser << std::endl;
  else
  {
    try
    {
      IoContextPool pool(2);
      pool.run();
      std::vector<std::unique_ptr<MeteoStation>> stations;
      for(uint32_t i = 1; i <= nbStations; ++i)
      {
        std::string name = "Station_" + std::to_string(i);
        stations.emplace_back(std::unique_ptr<MeteoStation>(new MeteoStation{name, pool.getIoContext(), host}));
      }
      pool.join();
    } catch(std::exception &e) {
      std::cerr << "Exception " << e.what() << std::endl;
    }
  }

  return 0;
}
