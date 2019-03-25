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

#include "interface/communicator_t.hpp"
#include "asio.hpp"

using asio::ip::tcp;

namespace fetch {
  namespace oef {
    class AsioComm : public communicator_t {
      public:
        //
        explicit AsioComm(asio::io_context& io_context);
        explicit AsioComm(asio::io_context& io_context, std::string to_ip_addr, uint32_t to_port);
        //
        AsioComm(AsioComm&& asio_comm) : socket_(std::move(asio_comm.socket_)) {}
        //
        void connect() {};
        void disconnect() override;
        //
        std::error_code  send_sync(std::shared_ptr<Buffer>) override { return std::error_code();};
        std::error_code  receive_sync(std::shared_ptr<Buffer>) override { return std::error_code();};
        void send_async(std::shared_ptr<Buffer> buffer) override;
        void send_async(std::shared_ptr<Buffer> buffer,
                                std::function<void(std::error_code,std::size_t)> continuation) override;
        void receive_async(std::function<void(std::error_code,std::shared_ptr<Buffer>)> continuation) override;
        //
        ~AsioComm() {
          disconnect();
        }
        
      private:
        tcp::socket socket_; 
    };
  } // oef
} // fetch

