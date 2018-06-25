#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
#include "catch.hpp"
#include "client.h"
#include "server.h"
#include <iostream>
#include <chrono>
#include <future>
#include "sd.h"

using fetch::oef::Server;
using fetch::oef::Client;

TEST_CASE("testing register", "[ServiceDiscovery]") {
  fetch::oef::Server as;
  std::cerr << "Server created\n";
  as.run();
  std::cerr << "Server started\n";
  std::cerr << "Waiting\n";
  std::this_thread::sleep_for(std::chrono::seconds{1});
  std::cerr << "NbAgents " << as.nbAgents() << "\n";
  REQUIRE(as.nbAgents() == 0);
  {
    Client c1("Agent1", "127.0.0.1", [](std::unique_ptr<Conversation>){});
    Client c2("Agent2", "127.0.0.1", [](std::unique_ptr<Conversation>){});
    Client c3("Agent3", "127.0.0.1", [](std::unique_ptr<Conversation>){});
    REQUIRE(as.nbAgents() == 3);
    Attribute manufacturer{"manufacturer", Type::String, true};
    Attribute colour{"colour", Type::String, false};
    Attribute luxury{"luxury", Type::Bool, true};
    DataModel car{"car", {manufacturer, colour, luxury}, "Car sale."};
    Instance ferrari{car, {{"manufacturer", "Ferrari"}, {"colour", "Aubergine"}, {"luxury", "true"}}};
    REQUIRE(c1.registerAgent(ferrari));
    std::this_thread::sleep_for(std::chrono::seconds{1});
    REQUIRE(c1.unregisterAgent(ferrari));
    std::this_thread::sleep_for(std::chrono::seconds{1});
    REQUIRE(c1.registerAgent(ferrari));
    Instance lamborghini{car, {{"manufacturer", "Lamborghini"}, {"luxury", "true"}}};
    REQUIRE(c2.registerAgent(lamborghini));
    std::this_thread::sleep_for(std::chrono::seconds{1});
    ConstraintType eqTrue{Relation{Relation::Op::Eq, true}};
    Constraint luxury_c{luxury, eqTrue};
    QueryModel q1{{luxury_c}, car};
    auto agents = c3.query(q1);
    std::sort(agents.begin(), agents.end());
    REQUIRE(agents.size() == 2);
    REQUIRE(agents == std::vector<std::string>({"Agent1", "Agent2"}));
    c1.stop();
    c2.stop();
    c3.stop();
    std::cerr << "Agent3 received\n";
    for(auto &s : agents) {
      std::cerr << s << std::endl;
    }
  }
  //  std::this_thread::sleep_for(std::chrono::seconds{1});
  as.stop();
  std::cerr << "Server stopped\n";
}

TEST_CASE("description", "[ServiceDiscovery]") {
  fetch::oef::Server as;
  std::cerr << "Server created\n";
  as.run();
  std::cerr << "Server started\n";
  std::cerr << "Waiting\n";
  std::this_thread::sleep_for(std::chrono::seconds{1});
  std::cerr << "NbAgents " << as.nbAgents() << "\n";
  REQUIRE(as.nbAgents() == 0);
  {
    Client c1("Agent1", "127.0.0.1", [](std::unique_ptr<Conversation>){});
    Client c2("Agent2", "127.0.0.1", [](std::unique_ptr<Conversation>){});
    Client c3("Agent3", "127.0.0.1", [](std::unique_ptr<Conversation>){});
    REQUIRE(as.nbAgents() == 3);

    Attribute manufacturer{"manufacturer", Type::String, true};
    Attribute model{"model", Type::String, true};
    Attribute wireless{"wireless", Type::Bool, true};
    DataModel station{"weather_station", {manufacturer, model, wireless}, "Weather station"};
    Instance youshiko{station, {{"manufacturer", "Youshiko"}, {"model", "YC9315"}, {"wireless", "true"}}};
    Instance opes{station, {{"manufacturer", "Opes"}, {"model", "17500"}, {"wireless", "true"}}};
    REQUIRE(c1.addDescription(youshiko));
    REQUIRE(c2.addDescription(opes));
    std::this_thread::sleep_for(std::chrono::seconds{1});
    ConstraintType eqTrue{Relation{Relation::Op::Eq, true}};
    Constraint wireless_c{wireless, eqTrue};
    QueryModel q1{{wireless_c}, station};
    auto agents = c3.search(q1);
    std::sort(agents.begin(), agents.end());
    REQUIRE(agents.size() == 2);
    REQUIRE(agents == std::vector<std::string>({"Agent1", "Agent2"}));
    ConstraintType eqYoushiko{Relation{Relation::Op::Eq, std::string{"Youshiko"}}};
    Constraint manufacturer_c{manufacturer, eqYoushiko};
    QueryModel q2{{manufacturer_c}};
    auto agents2 = c3.search(q2);
    REQUIRE(agents2.size() == 1);
    REQUIRE(agents2 == std::vector<std::string>({"Agent1"}));
    
    c1.stop();
    c2.stop();
    c3.stop();
  }
  as.stop();
  std::cerr << "Server stopped\n";
}

TEST_CASE( "testing Server", "[Server]" ) {
  fetch::oef::Server as;
  std::cerr << "Server created\n";
  as.run();
  std::cerr << "Server started\n";
  REQUIRE(as.nbAgents() == 0);
  
  SECTION("1 agent") {
    Client c1("Agent1", "127.0.0.1", [](std::unique_ptr<Conversation>){});
    std::cerr << "Debug1 before stop" << std::endl;
    REQUIRE(as.nbAgents() == 1);
    //    c1.stop();
    std::cerr << "Debug1 after stop" << std::endl;
  }
  std::this_thread::sleep_for(std::chrono::seconds{1});
  REQUIRE(as.nbAgents() == 0);
  //too fast ?
  SECTION("1000 agents") {
    // need to increase max nb file open
    // > ulimit -n 8000
    // ulimit -n 1048576
    
    std::vector<std::unique_ptr<Client>> clients;
    std::vector<std::future<std::unique_ptr<Client>>> futures;
    size_t nbClients = 1000;
    try {
      for(size_t i = 1; i <= nbClients; ++i) {
        std::string name = "Agent_";
        name += std::to_string(i);
        futures.push_back(std::async(std::launch::async, [](const std::string &n){
                                                           return std::make_unique<Client>(n, "127.0.0.1", [](std::unique_ptr<Conversation>){});
                                                         }, name));
      }
      std::cerr << "Futures created\n";
      for(auto &fut : futures) {
        clients.emplace_back(fut.get());
      }
      std::cerr << "Futures got\n";
    } catch(std::exception &e) {
      std::cerr << "BUG " << e.what() << "\n";
    }
    REQUIRE(as.nbAgents() == nbClients);
  }
  std::this_thread::sleep_for(std::chrono::seconds{1});
  REQUIRE(as.nbAgents() == 0);
  
  as.stop();
  std::cerr << "Server stopped\n";
}
