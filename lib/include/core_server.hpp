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

#include "interface/core_server_t.hpp"
#include "interface/communicator_t.hpp"

#include "agent_directory.hpp"
#include "asio_communicator.hpp"
#include "asio_acceptor.hpp"
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
      std::vector<std::unique_ptr<std::thread>> threads_;
      AsioAcceptor acceptor_;
      AgentDirectory_ agentDirectory_;

      static fetch::oef::Logger logger;
      
      void do_accept();
      void do_accept(std::function<void(std::error_code,std::shared_ptr<communicator_t>)> continuation) override;
      void process_agent_connection(const std::shared_ptr<communicator_t> communicator) override {}
      
      void newSession(std::shared_ptr<communicator_t> comm);
      void secretHandshake(const std::string &publicKey, std::shared_ptr<communicator_t> comm);
    public:
      explicit CoreServer(uint32_t nbThreads = 4, uint32_t backlog = 256) :
      acceptor_(io_context_, static_cast<uint32_t>(config::Ports::Agents)) {
        threads_.resize(nbThreads);
      }
      CoreServer(const CoreServer &) = delete;
      CoreServer operator=(const CoreServer &) = delete;
      virtual ~CoreServer();
      void run() override;
      void run_in_thread() override;
      size_t nb_agents() const override { return agentDirectory_.size(); }
      void stop() override;
    };
  }
}
