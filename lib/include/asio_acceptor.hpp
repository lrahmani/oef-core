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

#include "api/communicator_acceptor_t.hpp"
#include "api/communicator_t.hpp"

#include "config.hpp"

#include "asio.hpp"

#include <memory>

using asio::ip::tcp;

namespace fetch {
namespace oef {
    class AsioAcceptor : public comm_acceptor_t {
    private:
      //asio::io_context io_context_;
      tcp::acceptor acceptor_;
    public:
      explicit AsioAcceptor(asio::io_context& io_context, uint32_t port, uint32_t backlog = 256) : 
        acceptor_{io_context, tcp::endpoint(tcp::v4(), port)} {
          acceptor_.listen(backlog); // pending connections
        }
      void do_accept_async(CommunicatorContinuation continuation) override;
      std::string local_address();
      uint32_t local_port();
      ~AsioAcceptor() {}
    };
} // oef
} // fetch
