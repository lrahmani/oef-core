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
          std::cerr << "AsioBasicComm::AsioBasicComm: error connecting to " 
                    << to_ip_addr << ":" << std::to_string(to_port) 
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
      
      // sync operations
      
      std::error_code send_sync(const void* buffer, std::size_t nbytes) override 
      {
        std::error_code ec;
        auto len = asio_send_sync_(std::vector<asio::const_buffer>{asio::buffer(buffer, nbytes)}, ec);
        if (len != nbytes) {
          std::cerr << "AsioBasicComm::send_sync error sent " << len << " expected " << nbytes
                    << " : " << ec.value() << std::endl;
          // TOFIX should connection be closed?
        }
        return ec;
      } 
      
      std::error_code send_sync(std::vector<void*> buffers, std::vector<std::size_t> nbytes) override 
      {
        assert(buffers.size()==nbytes.size());
        std::error_code ec;
        std::vector<asio::const_buffer> asio_buffers;
        size_t n = buffers.size();
        size_t nbytes_acc = 0;
        for (size_t i = 0; i < n ; ++i) {
          asio_buffers.emplace_back(asio::buffer(buffers[i],nbytes[i]));
          nbytes_acc+=nbytes[i];
        }
        auto len = asio_send_sync_(asio_buffers, ec);
        if (len != nbytes_acc) {
          std::cerr << "AsioBasicComm::send_sync error grouped sent " << len << " expected " << nbytes_acc 
                    << " : " << ec.value() << std::endl;
          // TOFIX should connection be closed?
        }
        return ec;
      }
      
      std::error_code receive_sync(void* buffer, const std::size_t& nbytes ) override 
      {
        std::error_code ec;
        auto asio_buffer = asio::buffer(buffer, nbytes);
        auto len = asio_receive_sync_(asio_buffer, ec);
        if (len != nbytes) {
          std::cerr << "AsioBasicComm::receive_sync error while receiving data - got " << len 
                    << " expected " << nbytes << " : " << ec.value() << std::endl;
          // TOFIX should connection be closed?
        }
        return ec;
      }
      
      // async operations
      void send_async(std::vector<std::shared_ptr<Buffer>> buffers, std::vector<std::size_t> nbytes, 
                      LengthContinuation continuation) override 
      {
        assert(buffers.size()==nbytes.size());
        std::vector<asio::const_buffer> asio_buffers;
        size_t n = buffers.size();
        size_t nbytes_acc = 0;
        for (size_t i = 0; i < n ; ++i) {
          asio_buffers.emplace_back(asio::buffer(buffers[i]->data(),nbytes[i]));
          nbytes_acc+=nbytes[i];
        }
        asio::async_write(socket_, asio_buffers,
            [nbytes_acc,continuation](std::error_code ec, std::size_t length) {
              if(ec) {
                std::cerr << "AsioBasicComm::send_async: error while sending data (grouped) sent " 
                    << length << " expected " << nbytes_acc << " : " << ec.value() << std::endl;
              }
              continuation(ec, length);
            });
      }

      void send_async(std::shared_ptr<Buffer> buffer, std::size_t nbytes, LengthContinuation continuation) override 
      {
        asio::async_write(socket_, asio::buffer(buffer->data(), nbytes),
            [nbytes,continuation](std::error_code ec, std::size_t length) {
              if(ec) {
                std::cerr << "AsioBasicComm::send_async: error while sending data sent " 
                    << length << " expected " << nbytes << " : " << ec.value() << std::endl;
              }
              continuation(ec, length);
            });
      }

      void receive_async(std::size_t nbytes, BufferContinuation continuation) override 
      {
        std::shared_ptr<Buffer> buffer = std::make_shared<Buffer>(nbytes);
        asio::async_read(socket_, asio::buffer(buffer->data(), nbytes), 
            [buffer,nbytes,continuation](std::error_code ec, std::size_t length) {
              if(ec) {
                std::cerr << "AsioBasicComm::receive_async: error while receiving data, expected " 
                          << nbytes << " got " << length << " : " << ec.value() << std::endl;
                continuation(ec, std::make_shared<Buffer>());
              } else {
                continuation(ec, buffer);
              }
            });
      }
      
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

