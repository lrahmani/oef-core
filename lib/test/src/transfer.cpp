#include "catch.hpp"
#include "client.h"
#include "server.h"
#include <iostream>
#include <chrono>
#include <future>
#include <cassert>
#include "uuid.h"

using fetch::oef::Server;
using fetch::oef::Client;

namespace Test {
  TEST_CASE("transfer between agents", "[transfer]") {
    Uuid id = Uuid::uuid4();
    std::cerr << id.to_string() << std::endl;
    std::string s = id.to_string();
    Uuid id2{s};
    std::cerr << id2.to_string() << std::endl;
    Server as;
    std::cerr << "Server created\n";
    as.run();
    std::cerr << "Server started\n";
    
    REQUIRE(as.nbAgents() == 0);
    {
      Client c1("Agent1", "127.0.0.1", [](std::unique_ptr<Conversation>){});
      Client c2("Agent2", "127.0.0.1", [](std::unique_ptr<Conversation>){});
      Client c3("Agent3", "127.0.0.1", [](std::unique_ptr<Conversation>){});
      REQUIRE(as.nbAgents() == 3);
      std::cerr << "Clients created\n";
      Conversation c1_c2 = c1.newConversation("Agent2");
      Conversation c1_c3 = c1.newConversation("Agent3");
      // REQUIRE(c1_c2.send("Hello world"));
      // REQUIRE(c1_c3.send("Hello world"));
      std::cerr << "Debug######\n";
      Conversation c2_c3 = c2.newConversation("Agent3");
      Conversation c2_c1 = c2.newConversation("Agent1");
      // REQUIRE(c2_c3.send("Welcome back"));
      // REQUIRE(c2_c1.send("Welcome back"));
      Conversation c3_c1 = c3.newConversation("Agent1");
      Conversation c3_c2 = c3.newConversation("Agent2");
      // REQUIRE(c3_c1.send("Here I am"));
      // REQUIRE(c3_c2.send("Here I am"));
      std::cerr << "Data sent\n";
      std::this_thread::sleep_for(std::chrono::seconds{1});
      c1.stop();
      c2.stop();
      c3.stop();
      // REQUIRE(c1rec.size() == 2);
      // std::cerr << "Agent1 received\n";
      // for(auto &s : c1rec) {
      //   std::cerr << s << std::endl;
      // }
      // auto c2rec = c2.received();
      // REQUIRE(c2rec.size() == 2);
      // std::cerr << "Agent2 received\n";
      // for(auto &s : c2rec) {
      //   std::cerr << s << std::endl;
      // }
      // auto c3rec = c3.received();
      // REQUIRE(c3rec.size() == 2);
      // std::cerr << "Agent3 received\n";
      // for(auto &s : c3rec) {
      //   std::cerr << s << std::endl;
      // }
      std::cerr << "Clients stopped\n";
    }
    std::cerr << "Waiting\n";
    std::this_thread::sleep_for(std::chrono::seconds{2});
    std::cerr << "NbAgents " << as.nbAgents() << "\n";
    
    as.stop();
    std::cerr << "Server stopped\n";
  }
}
