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

#include "api/communicator_t.hpp"

#include "asio.hpp"

#include <vector>

using asio::ip::tcp;

namespace fetch {
namespace oef {
    class AsioComm : public communicator_t {
    public:
        //
        explicit AsioComm(asio::io_context& io_context) : socket_{io_context} {}
        explicit AsioComm(tcp::socket socket) : socket_(std::move(socket)) {}
        explicit AsioComm(asio::io_context& io_context, std::string to_ip_addr, uint32_t to_port);
        //
        explicit AsioComm(AsioComm&& asio_comm) : socket_(std::move(asio_comm.socket_)) {}
        //
        void connect() {};
        void disconnect() override;
        //
        std::error_code send_sync(std::shared_ptr<Buffer> buffer) override;
        std::error_code send_sync(std::vector<std::shared_ptr<Buffer>> buffers) override;
        std::error_code receive_sync(std::shared_ptr<Buffer>& buffer) override;
        //
        void send_async(std::shared_ptr<Buffer> buffer) override;
        void send_async(std::shared_ptr<Buffer> buffer,
                                LengthContinuation continuation) override;
        void send_async(std::vector<std::shared_ptr<Buffer>> buffer,
                                LengthContinuation continuation);// override;
        void receive_async(BufferContinuation continuation) override;
        //
        std::error_code send_sync(asio::const_buffer& buffer);
        //
        ~AsioComm() {
          disconnect();
        }
    private:
        void pack_core_protocol_(std::vector<std::shared_ptr<Buffer>> buffers, std::vector<asio::const_buffer>& out, uint32_t& out_total) {
          std::vector<uint32_t> lens;
          out_total = 0;
          for(auto& buffer : buffers) {
            lens.emplace_back(uint32_t(buffer->size()));
            auto* len = &lens.back();
            out.emplace_back(asio::buffer(len, sizeof(*len)));
            out.emplace_back(asio::buffer(buffer->data(), *len));
            out_total += *len+sizeof(*len);
          }
        }
    private:
        tcp::socket socket_; 
        
    };

    namespace as {
      template <typename T>
      asio::const_buffer serialize(T msg){
        return asio::buffer(msg);
      }
    }
  } // oef
} // fetch

