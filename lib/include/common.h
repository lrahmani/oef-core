#pragma once

#include "asio.hpp"
#include "asio/yield.hpp"
#include <iostream>
#include <functional>

enum class Ports {
  ServiceDiscovery = 2222, Agents = 3333
};

using asio::ip::tcp;
using Buffer = std::vector<uint8_t>;

template <typename T>
T from_string(const std::string &s) {
  T t;
  t.ParseFromString(s);
  return t;
}

template <typename T>
std::shared_ptr<Buffer> serialize(const T &t) {
  size_t size = t.ByteSize();
  Buffer data;
  data.resize(size);
  (void)t.SerializeWithCachedSizesToArray(data.data());
  return std::make_shared<Buffer>(data);
}

template <typename T>
T deserialize(const Buffer &buffer) {
  T t;
  t.ParseFromArray(buffer.data(), buffer.size());
  return t;
}

void asyncReadBuffer(asio::ip::tcp::socket &socket, uint32_t timeout, std::function<void(std::error_code,std::shared_ptr<Buffer>)> handler);
void asyncWriteBuffer(asio::ip::tcp::socket &socket, std::shared_ptr<Buffer> s, uint32_t timeout);
void asyncWriteBuffer(asio::ip::tcp::socket &socket, std::shared_ptr<Buffer> s, uint32_t timeout, std::function<void(std::error_code, std::size_t length)> handler);

class Connection : asio::coroutine {
  void operator()(tcp::socket &socket, std::error_code ec = std::error_code(), std::size_t n = 0) {
    std::vector<char> buffer;
    if(!ec) reenter (this)
    {
        uint32_t len;
        yield asio::async_read(socket, asio::buffer(&len, sizeof(len)), [this,&socket](std::error_code ec, std::size_t length) {
            this->operator()(socket, ec, length); });
        buffer.resize(len);
        yield asio::async_read(socket, asio::buffer(buffer.data(), len), [this,&socket](std::error_code ec, std::size_t length) {
            this->operator()(socket, ec, length); });
    }
  }
};
