// Company: FETCH.ai
// Author: Jerome Maloberti
// Creation: 09/01/18
//
#include <iostream>
#include <unordered_map>

#include "oefcoreproxy.hpp"
#include "agent.hpp"
#include "uuid.hpp"
#include "dialogue.hpp"

using namespace fetch::oef;

struct Shared {
  std::string bestStation_;
  float bestPrice_ = -1.0f;
  size_t nbAnswers_ = 0;
  uint32_t dataReceived_ = 0;
};

class MeteoDialogue : public SingleDialogue {
  Shared &shared_;
  bool waitingForData_ = false;
public:
  MeteoDialogue(DialogueAgent &agent, std::string destination, Shared &shared)
    : SingleDialogue(agent, destination), shared_{shared} {}
  void onMessage(const std::string &content) override {
  }
  void onCFP(uint32_t msgId, uint32_t target, const CFPType &constraints) override {
  }
  void onPropose(uint32_t msgId, uint32_t target, const ProposeType &proposals) override {
  }
  void onAccept(uint32_t msgId, uint32_t target) override {
  }
  void onDecline(uint32_t msgId, uint32_t target) override {
  }
};

class MeteoClientAgent : public fetch::oef::Agent {
 private:
  std::unordered_map<std::string,uint32_t> dialoguesIds_;
  std::string bestStation_;
  float bestPrice_ = -1.0f;
  size_t nbAnswers_ = 0;
  bool waitingForData_ = false;
  uint32_t dataReceived_ = 0;

public:
  MeteoClientAgent(const std::string &agentId, asio::io_context &io_context, const std::string &host)
    : fetch::oef::Agent{std::unique_ptr<fetch::oef::OEFCoreInterface>(new fetch::oef::OEFCoreNetworkProxy{agentId, io_context, host})} {
      start();
    }
  void onError(fetch::oef::pb::Server_AgentMessage_Error_Operation operation, stde::optional<uint32_t> dialogueId, stde::optional<uint32_t> msgId) override {}
  void onSearchResult(uint32_t search_id, const std::vector<std::string> &results) override {
    if(results.empty()) {
      std::cerr << "No candidates\n";
    }
    for(auto &c : results) {
      auto uuid = Uuid32::uuid();
      dialoguesIds_[c] = uuid.val();
      sendCFP(uuid.val(), c, CFPType{stde::nullopt});
    }
  }
  void onMessage(const std::string &from, uint32_t dialogueId, const std::string &content) override {
    assert(from == bestStation_);
    Data temp{content};
    std::cerr << "**Received " << temp.name() << " = " << temp.values().front() << std::endl;
    ++dataReceived_;
    if(dataReceived_ == 3) {
      stop();
    }
  }
  void onCFP(const std::string &from, uint32_t dialogueId, uint32_t msgId, uint32_t target, const fetch::oef::CFPType &constraints) override {}
  void onPropose(const std::string &from, uint32_t dialogueId, uint32_t msgId, uint32_t target, const fetch::oef::ProposeType &proposals) override {
    stde::optional<std::string> s_price;
    proposals.match(
                    [this](const std::string &s) {
                      assert(false);
                    },
                    [this,&s_price](const std::vector<Instance> &is) {
                      assert(is.size() == 1);
                      s_price = is.front().value("price");
                    });
    assert(s_price);
    float currentPrice = std::stof(s_price.value());
    if(bestPrice_ == -1.0 || bestPrice_ > currentPrice) {
      bestPrice_ = currentPrice;
      bestStation_ = from;
    }
    ++nbAnswers_;
    std::cerr << "Nb Answers " << nbAnswers_ << " " << dialoguesIds_.size() << std::endl;
    if(nbAnswers_ == dialoguesIds_.size()) {
      waitingForData_ = true;
      std::cerr << "Best station " << bestStation_ << " price " << bestPrice_ << std::endl;
      fetch::oef::pb::Boolean accepted;
      for(auto &p : dialoguesIds_) {
        if(p.first == bestStation_) {
          sendAccept(p.second, p.first, msgId + 1, target + 1);
        } else {
          sendDecline(p.second, p.first, msgId + 1, target + 1);
        }
      }
    }
  }
  void onAccept(const std::string &from, uint32_t dialogueId, uint32_t msgId, uint32_t target) override {}
  void onDecline(const std::string &from, uint32_t dialogueId, uint32_t msgId, uint32_t target) override {}
 };

class MyAgent : public DialogueAgent {
public:
  MyAgent(std::unique_ptr<OEFCoreInterface> oefCore) : DialogueAgent{std::move(oefCore)} {}
  void onSearchResult(uint32_t search_id, const std::vector<std::string> &results) override {}
  void onNewMessage(const std::string &from, uint32_t dialogueId, const std::string &content) override {}
  void onNewCFP(const std::string &from, uint32_t dialogueId, uint32_t msgId, uint32_t target, const CFPType &constraints) override {}
  void onConnectionError(fetch::oef::pb::Server_AgentMessage_Error_Operation operation) override {}
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

    MyAgent agent{std::unique_ptr<fetch::oef::OEFCoreInterface>(new fetch::oef::OEFCoreNetworkProxy{argv[1], pool.getIoContext(), argv[2]})};
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
    client.searchServices(1, q1);
    pool.join();
  } catch (std::exception& e) {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}
