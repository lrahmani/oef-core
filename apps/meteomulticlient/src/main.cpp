// Company: FETCH.ai
// Author: Jerome Maloberti
// Creation: 09/01/18
//
#include <iostream>
#include "client.h"

using fetch::oef::Client;

int main(int argc, char* argv[])
{
  try
  {
    if (argc != 3)
    {
      std::cerr << "Usage: meteoclient <agentID> <host>\n";
      return 1;
    }
    Client client(argv[1], argv[2], [](std::unique_ptr<Conversation>){});

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
    std::vector<std::string> candidates = client.query(q1);

    // The return value should be a vector of strings (AEAs). Note this will crash if there are no matching AEAs
    // We now create conversations with these AEAs
    std::cout << "Candidates:\n";
    std::vector<Conversation> conversations;
    for(auto &c : candidates) {
      std::cout << c << std::endl;
      conversations.emplace_back(client.newConversation(c));
    }

    for(auto &c : conversations) {
      std::cerr << "Debug client cuid " << c.uuid() << " to " << c.dest() << std::endl;
      c.send("");
    }

    // Note: Since this executes sequentially, it will hang if any AEA does not respond
    std::string best_station;
    float best_price = -1.0;
    for(auto &c : conversations) {
      auto p = c.pop();
      assert(p->has_content());
      Data price{p->content().content()};
      assert(price.values().size() == 1);
      float current_price = std::stof(price.values().front());
      if(best_price == -1.0 || best_price > current_price) {
        best_price = current_price;
        best_station = c.dest();
      }
    }

    // Note: the client needs to tell the AEA whether its accepted or the AEA will hang forever
    std::cerr << "Best station " << best_station << " price " << best_price << std::endl;
    fetch::oef::pb::Boolean accepted;
    for(auto &c : conversations) {
      accepted.set_status(c.dest() == best_station);
      c.send(accepted);
    }

    auto iter = std::find_if(conversations.begin(), conversations.end(), [&](const Conversation &c){return c.dest() == best_station;});
    if(iter != conversations.end()) {
      Data temp{iter->popContent()};
      std::cerr << "**Received temp: " << temp.name() << " = " << temp.values().front() << std::endl;
      Data air_data{iter->popContent()};
      std::cerr << "**Received air_data: " << air_data.name() << " = " << air_data.values().front() << std::endl;
      Data humid{iter->popContent()};
      std::cerr << "**Received humidity: " << humid.name() << " = " << humid.values().front() << std::endl;
    } else {
      std::cerr << "No station available.\n";
    }
  } catch (std::exception& e) {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}
