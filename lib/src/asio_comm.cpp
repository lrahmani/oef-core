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

#include "asio_communicator.hpp"
#include <iostream>
#include <string>

namespace fetch {
  namespace oef {

AsioComm::AsioComm(asio::io_context& io_context) : socket_(io_context) {}

AsioComm::AsioComm(asio::io_context& io_context, std::string to_ip_addr, uint32_t to_port) : socket_(io_context) {
  tcp::resolver resolver(io_context);
  try {
    asio::connect(socket_, resolver.resolve(to_ip_addr,std::to_string(to_port)));
  } catch (std::exception& e) {
    std::cerr << "AsioComm::AsioComm: error connecting to " << to_ip_addr << ":" << std::to_string(to_port) 
              << " : " << e.what() << std::endl;
    throw;
  }
}

void AsioComm::disconnect() {
  socket_.shutdown(asio::socket_base::shutdown_type::shutdown_both);
  socket_.close();
}

void AsioComm::send_async(std::shared_ptr<Buffer> buffer,
                          std::function<void(std::error_code,std::size_t)> continuation) {
  std::vector<asio::const_buffer> buffers;
  uint32_t len = uint32_t(buffer->size());
  buffers.emplace_back(asio::buffer(&len, sizeof(len)));
  buffers.emplace_back(asio::buffer(buffer->data(), len));
  uint32_t total = len+sizeof(len);
  asio::async_write(socket_, buffers,
      [total,continuation](std::error_code ec, std::size_t length) {
        if(ec) {
          std::cerr << "AsioComm::send_async: error while sending data and its size (grouped): " 
                    << length << " expected " << total << std::endl;
        } else {
          continuation(ec, length);
        }
      });
}

void AsioComm::send_async(std::shared_ptr<Buffer> buffer) {
  send_async(buffer, [](std::error_code ec, std::size_t length) {});
}

void AsioComm::receive_async(std::function<void(std::error_code,std::shared_ptr<Buffer>)> continuation) {
  auto len = std::make_shared<uint32_t>();
  asio::async_read(socket_, asio::buffer(len.get(), sizeof(uint32_t)), 
      [this,len,continuation](std::error_code ec, std::size_t length) {
        if(ec) {
          std::cerr << "AsioComm::receive_async: error while receiving the size of data " 
                    << ec.value() << std::endl;
          continuation(ec, std::make_shared<Buffer>());
        } else {
          assert(length == sizeof(uint32_t));
          auto buffer = std::make_shared<Buffer>(*len);
          asio::async_read(socket_, asio::buffer(buffer->data(), *len), 
              [buffer,continuation](std::error_code ec, std::size_t length) {
                if(ec) {
                  std::cerr << "AsioComm::receive_async: error while receiving the data " 
                            << ec.value() << std::endl;
                }
                continuation(ec, buffer);
              });
        }
      });
}

} // oef
} // fetch
