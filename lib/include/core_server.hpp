#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018-2019 Fetch.AI Limited
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

#include "api/core_server_t.hpp"
#include "api/communicator_t.hpp"

#include "agent_directory.hpp"
#include "asio_communicator.hpp"
#include "asio_acceptor.hpp"
#include "asio_basic_communicator.hpp"
#include "oef_search_client.hpp"
#include "serialization.hpp"
#include "config.hpp"
#include "logger.hpp"

#include "agent.pb.h"
#include "asio.hpp"

#include <memory>

namespace fetch {
namespace oef {      

    class CoreServer : public core_server_t {
    private:
      asio::io_context io_context_;
      AsioAcceptor acceptor_;
      AgentDirectory agentDirectory_;
      std::shared_ptr<OefSearchClient> oef_search_; 
      std::vector<std::unique_ptr<std::thread>> threads_;
      //
      std::string core_key_;
      std::string core_ip_addr_;
      uint32_t core_port_;

      static fetch::oef::Logger logger;
    public:
      explicit CoreServer(
          std::string core_key      = "oef-core-pluto",
          std::string core_ip_addr  = config::default_ip,
          uint32_t core_port        = static_cast<uint32_t>(config::Ports::Agents),
          std::string s_ip_addr     = config::default_ip, 
          uint32_t s_port           = static_cast<uint32_t>(config::Ports::Search),
          uint32_t nbThreads        = config::core_default_nb_threads, 
          uint32_t backlog          = config::core_default_backlog) 
          : 
            acceptor_{io_context_, core_port}
          , core_key_{core_key}
          , core_ip_addr_{core_ip_addr}
          , core_port_{core_port}
      {
        threads_.resize(nbThreads);
        try {
          auto s_comm = std::make_shared<AsioBasicComm>(io_context_, s_ip_addr, s_port);
          oef_search_ = std::make_shared<OefSearchClient>(s_comm, 
              core_key, core_ip_addr, core_port);
        } catch (std::exception e) {
          logger.error("CoreServer::CoreServer error while initializing OefSearchClient {}", e.what());
          stop();
        }
      }
      
      CoreServer(const CoreServer &) = delete;
      CoreServer operator=(const CoreServer &) = delete;
      virtual ~CoreServer();
      
      void run() override;
      void run_in_thread() override;
      size_t nb_agents() const override { return agentDirectory_.size(); }
      void stop() override;
    private:
      void do_accept(CommunicatorContinuation continuation) override;
      void process_agent_connection(const std::shared_ptr<communicator_t> communicator) override {}
      void do_accept();
      
      void newSession(std::shared_ptr<communicator_t> comm);
      void secretHandshake(const std::string &publicKey, std::shared_ptr<communicator_t> comm);
    };
} // oef
} // fetch
