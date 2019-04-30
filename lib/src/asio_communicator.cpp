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

AsioComm::AsioComm(asio::io_context& io_context, std::string to_ip_addr, uint32_t to_port) 
  : socket_(io_context) 
{
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
  try {
    socket_.shutdown(asio::socket_base::shutdown_type::shutdown_both);
    socket_.close();
  } catch (std::exception& e) {
    std::cerr << "Exception during disconnect: " << e.what() << std::endl;
  }
}

void AsioComm::send_async(std::shared_ptr<Buffer> buffer, LengthContinuation continuation) {
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

void AsioComm::receive_async(BufferContinuation continuation) {
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

std::error_code AsioComm::send_sync(std::shared_ptr<Buffer> buffer) {
  std::vector<asio::const_buffer> buffers;
  uint32_t len = uint32_t(buffer->size());
  buffers.emplace_back(asio::buffer(&len, sizeof(len)));
  buffers.emplace_back(asio::buffer(buffer->data(), len));
  uint32_t total = len+sizeof(len);
  std::error_code ec;
  std::size_t length = asio::write(socket_, buffers, ec);
  if (length != total) {
    std::cerr << "AsioComm::send_sync error sent " << length << " expected " << total 
      << " : " << ec.value() << std::endl;
    // TOFIX should connection be closed?
  }
  return ec;
}

std::error_code AsioComm::send_sync(std::vector<std::shared_ptr<Buffer>> buffers) {
  std::vector<asio::const_buffer> buffers_all;
  std::vector<uint32_t> len_all;
  uint32_t total = 0;
  for(auto& buffer : buffers) {
    len_all.emplace_back(uint32_t(buffer->size()));
    auto* len = &len_all.back();
    buffers_all.emplace_back(asio::buffer(len, sizeof(*len)));
    buffers_all.emplace_back(asio::buffer(buffer->data(), *len));
    total += *len+sizeof(*len);
  }
  std::error_code ec;
  std::size_t length = asio::write(socket_, buffers_all, ec);
  if (length != total) {
    std::cerr << "AsioComm::send_sync error sent " << length << " expected " << total 
      << " : " << ec.value() << std::endl;
    // TOFIX should connection be closed?
  }
  return ec;
}

std::error_code AsioComm::receive_sync(std::shared_ptr<Buffer>& buffer) {
  auto len = std::make_shared<uint32_t>();
  std::error_code ec;
  auto length = asio::read(socket_, asio::buffer(len.get(), sizeof(uint32_t)), ec);
  if (ec || length!=sizeof(uint32_t)) { // TOFIX testing length is not needed
    std::cerr << "AsioComm::receive_sync error while receivin lenght of data, got " << length 
              << " : ec " << ec.value() << std::endl;
    // TOFIX should connection be closed?
    return ec;
  }
  buffer = std::make_shared<Buffer>(*len);
  length = asio::read(socket_, asio::buffer(buffer->data(), *len), ec);
  if (ec || length!=*len) { // TOFIX testing length is not needed
    std::cerr << "AsioComm::receive_sync error while receiving data " << ec.value() 
              << " - got " << length << "expected " << len << std::endl;
    // TOFIX should connection be closed?
    return ec;
  }
  return ec;
}

std::error_code AsioComm::send_sync(asio::const_buffer& buffer) {
  std::vector<asio::const_buffer> buffers;
  int32_t len = -int32_t(buffer.size());
  buffers.emplace_back(asio::buffer(&len, sizeof(len)));
  buffers.emplace_back(buffer);
  uint32_t total = len+sizeof(len);
  std::error_code ec;
  std::size_t length = asio::write(socket_, buffers, ec);
  if (length != total) {
    std::cerr << "AsioComm::send_sync error sent " << length << " expected " << total 
      << " : " << ec.value() << std::endl;
    // TOFIX should connection be closed?
  }
  return ec;
}

} // oef
} // fetch
