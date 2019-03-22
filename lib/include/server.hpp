#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch.AI Limited
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.
//
//------------------------------------------------------------------------------

#include <memory>
#include "agent.pb.h"
#include "common.hpp"
#include "servicedirectory.hpp"
#include "logger.hpp"
#include "agentdirectory.hpp"
#include "search_com.hpp"

namespace fetch {
  namespace oef {      
    struct OefSearch {
      tcp::socket socket_;
      explicit OefSearch(asio::io_context &io_context, const std::string ip_addr, uint32_t port = 3334) : socket_(io_context){
        tcp::resolver resolver(io_context);
        asio::connect(socket_, resolver.resolve(ip_addr,std::to_string(port)));
      }
      // TORM Only for Server's bw compatibility
      explicit OefSearch(asio::io_context &io_context) : socket_(io_context) {}
    };

    class Server {
    private:
      struct Context {
        tcp::socket socket_;
      explicit Context(tcp::socket socket) : socket_{std::move(socket)} {}
      };

      // Careful: order matters.
      asio::io_context io_context_;
      std::vector<std::unique_ptr<std::thread>> threads_;
      tcp::acceptor acceptor_;
      AgentDirectory agentDirectory_;
      ServiceDirectory serviceDirectory_;
      OefSearch oef_search_;

      static fetch::oef::Logger logger;

      void secretHandshake(const std::string &publicKey, const std::shared_ptr<Context> &context);  
      void newSession(tcp::socket socket);
      void do_accept();
    public:
      explicit Server(uint32_t nbThreads = 4, uint32_t backlog = 256) :
      acceptor_(io_context_, tcp::endpoint(tcp::v4(), static_cast<int>(Ports::Agents))), oef_search_(io_context_) {
        acceptor_.listen(backlog); // pending connections
        threads_.resize(nbThreads);
      }
      explicit Server(const std::string oefsearch_addr, uint32_t port = 3334) : Server() {
          logger.debug("Server::Server connection to oef search {}:{}", oefsearch_addr, port);
          oef_search_ = OefSearch(io_context_, oefsearch_addr, port);
      }
      Server(const Server &) = delete;
      Server operator=(const Server &) = delete;
      virtual ~Server();
      void run();
      void run_in_thread();
      size_t nbAgents() const { return agentDirectory_.size(); }
      void stop();
    };
  }
}
