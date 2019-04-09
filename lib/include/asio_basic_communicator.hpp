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

#include "api/basic_communicator_t.hpp"

#include "asio.hpp"

#include <vector>

using asio::ip::tcp;

namespace fetch {
namespace oef {
    class AsioBasicComm : public basic_communicator_t {
    public:
      //
      explicit AsioBasicComm(asio::io_context& io_context) : socket_{io_context} {}
      explicit AsioBasicComm(tcp::socket socket) : socket_(std::move(socket)) {}
      explicit AsioBasicComm(asio::io_context& io_context, std::string to_ip_addr, uint32_t to_port)
        : socket_{io_context} 
      {
        tcp::resolver resolver(io_context);
        try {
          asio::connect(socket_, resolver.resolve(to_ip_addr,std::to_string(to_port)));
        } catch (std::exception& e) {
          std::cerr << "AsioBasicComm::AsioBasicComm: error connecting to " << to_ip_addr << ":" << std::to_string(to_port) 
                    << " : " << e.what() << std::endl;
          throw;
        }
      }
      //
      explicit AsioBasicComm(AsioBasicComm&& asio_comm) : socket_(std::move(asio_comm.socket_)) {}
      //
      void connect() override {}
      void disconnect() override {
        socket_.shutdown(asio::socket_base::shutdown_type::shutdown_both);
        socket_.close();
      }
      //
      std::error_code send_sync(const void* buffer, std::size_t nbytes) override {
        std::error_code ec;
        auto len = asio_send_sync_(std::vector<asio::const_buffer>{asio::buffer(buffer, nbytes)}, ec);
        if (len != nbytes) {
          std::cerr << "AsioBasicComm::send_sync error sent " << len << " expected " << nbytes
                    << " : " << ec.value() << std::endl;
          // TOFIX should connection be closed?
        }
        return ec;
      } 
      std::error_code send_sync(std::vector<void*> buffers, std::vector<std::size_t> nbytes) override {
        assert(buffers.size()==nbytes.size());
        //std::cerr << "AsioBasicComm::send_sync preparing to send buffers " << buffers.size() << " with nbytes " << nbytes.size() << std::endl;
        std::error_code ec;
        std::vector<asio::const_buffer> asio_buffers;
        size_t n = buffers.size();
        size_t nbytes_acc = 0;
        for (size_t i = 0; i < n ; ++i) {
          asio_buffers.emplace_back(asio::buffer(buffers[i],nbytes[i]));
          //std::cerr << "AsioBasicComm::nbytes_acc " << nbytes_acc;
          nbytes_acc+=nbytes[i];
          //std::cerr << " adding " << nbytes[i] << " = " << nbytes_acc << std::endl;
        }
        auto len = asio_send_sync_(asio_buffers, ec);
        if (len != nbytes_acc) {
          std::cerr << "AsioBasicComm::send_sync error grouped sent " << len << " expected " << nbytes_acc 
                    << " : " << ec.value() << std::endl;
          // TOFIX should connection be closed?
        }
        return ec;
      }
      std::error_code receive_sync(void* buffer, const std::size_t& nbytes ) override {
        //std::cerr << "AsioBasicComm::receive_sync attempting to receive " << nbytes << std::endl;
        std::error_code ec;
        auto asio_buffer = asio::buffer(buffer, nbytes);
        auto len = asio_receive_sync_(asio_buffer, ec);
        if (len != nbytes) {
          std::cerr << "AsioBasicComm::receive_sync error while receiving data - got " << len 
                    << " expected " << nbytes << " : " << ec.value() << std::endl;
          // TOFIX should connection be closed?
        }
        //std::cerr << "AsioBasicComm::receive_sync received " << len << std::endl;
        return ec;
      }
      //
      void send_async(std::shared_ptr<void> buffer, std::size_t nbytes, LengthContinuation continuation) override {};
      void send_async(std::shared_ptr<void> buffer, std::size_t nbytes) override {};
      void receive_async(VoidBuffContinuation continuation) override {};
      //
      ~AsioBasicComm() {
        disconnect();
      }
    private:
      //
      std::size_t asio_send_sync_(const std::vector<asio::const_buffer>& buffers, std::error_code& ec) {
        return asio::write(socket_, buffers, ec);
      }
      std::size_t asio_receive_sync_(asio::mutable_buffer buffer, std::error_code& ec) {
        return asio::read(socket_, buffer, ec);
      }
    private:
      tcp::socket socket_; 
        
    };
} // oef
} // fetch

