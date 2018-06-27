// Company: FETCH.ai
// Author: Jerome Maloberti
// Creation: 09/01/18
//
#include <iostream>
#include "clara.hpp"
#include "client.h"
#include <google/protobuf/text_format.h>

using fetch::oef::Client;

class MeteoStation : public Client
{
public:
  MeteoStation(const std::string &id, const char *host) : Client{id, host, [this](std::unique_ptr<Conversation> c) { _conversations.push(std::move(c));}}
  {
    static std::vector<std::string> properties = { "true", "true", "true", "false"};
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
    std::cerr << id << " " << _unitPrice << std::endl;
    std::unordered_map<std::string,std::string> props;
    int i = 0;
    for(auto &a : attributes) {
      props[a.name()] = properties[i];
      ++i;
    }
    Instance instance{weather, props};
    registerAgent(instance);
    _thread = std::make_unique<std::thread>([this]() { run(); });
  }
  MeteoStation(const MeteoStation &) = delete;
  MeteoStation operator=(const MeteoStation &) = delete;

  ~MeteoStation()
  {
    if(_thread)
      _thread->join();
  }

private:

  float                                _unitPrice;
  std::unique_ptr<std::thread>         _thread;
  Queue<std::unique_ptr<Conversation>> _conversations;

  void process(std::unique_ptr<Conversation> conversation)
  {
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
  }
  void run()
  {
    process(std::move(_conversations.pop()));
  }
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
      std::vector<std::unique_ptr<Client>> stations;
      for(uint32_t i = 1; i <= nbStations; ++i)
      {
        std::string name = "Station_" + std::to_string(i);
        stations.emplace_back(std::unique_ptr<MeteoStation>(new MeteoStation{name, host.c_str()}));
      }
    } catch(std::exception &e) {
      std::cerr << "Exception " << e.what() << std::endl;
    }
  }

  return 0;
}
