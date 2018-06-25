// Company: FETCH.ai
// Author: Jerome Maloberti
// Creation: 09/01/18
//
// This file is used for the 'meteoclient' demo. It instantiates an instance of an AEA that wants to get weather data as cheaply as possible

#include <iostream>

// FETCH.ai
#include "messages.h"
#include "client.h"

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
    DataModel weather{"weather_data", attributes, stde::optional<std::string>{"All possible weather data."}};

    // Create constraints against our Attributes (whether the station CAN provide them)
    ConstraintType eqTrue{ConstraintType::ValueType{Relation{Relation::Op::Eq, true}}};
    Constraint temperature_c { temperature, eqTrue};
    Constraint air_c         { air,         eqTrue};
    Constraint humidity_c    { humidity,    eqTrue};

    // Construct a Query schema and send it to the Node
    QueryModel q1{{temperature_c,air_c,humidity_c}, stde::optional<DataModel>{weather}};
    std::vector<std::string> candidates = client.query(q1);

    // The return value should be a vector of strings (AEAs). Note this will crash if there are no matching AEAs
    // We now create conversations with these AEAs
    std::cout << "Candidates:\n";
    std::vector<Conversation> conversations;
    for(auto &c : candidates)
    {
      std::cout << c << std::endl;
      conversations.emplace_back(client.newConversation(c));
    }

    for(auto &c : conversations)
    {
      c.send("What price ?");
    }

    // Note: Since this executes sequentially, it will hang if any AEA does not respond
    std::string best_station;
    float best_price = -1.0;
    for(auto &c : conversations)
    {
      auto p = c.popMsg<Price>();
      if(best_price == -1.0 || best_price > p.price())
      {
        best_price = p.price();
        best_station = c.dest();
      }
    }

    // Note: the client needs to tell the AEA whether its accepted or the AEA will hang forever
    std::cerr << "Best station " << best_station << " price " << best_price << std::endl;
    for(auto &c : conversations)
    {
      c.sendMsg<Accepted>(Accepted{c.dest() == best_station});
    }

    auto iter = std::find_if(conversations.begin(), conversations.end(), [&](const Conversation &c){return c.dest() == best_station;});
    assert(iter != conversations.end());

    Data temp = iter->popMsg<Data>();
    std::cerr << "**Received temp: " << temp.name() << " = " << temp.values().front() << std::endl;
    Data air_data = iter->popMsg<Data>();
    std::cerr << "**Received air_data: " << air_data.name() << " = " << air_data.values().front() << std::endl;
    Data humid = iter->popMsg<Data>();
    std::cerr << "**Received humidity: " << humid.name() << " = " << humid.values().front() << std::endl;
  }
  catch (std::exception& e)
  {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}
