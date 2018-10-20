#pragma once

#include "asio.hpp"
//#include "asio/yield.hpp"
#include <iostream>
#include <functional>
#include <thread>

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

/*
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
*/
class IoContextPool {
  using context_work = asio::executor_work_guard<asio::io_context::executor_type>;
 private:
  std::vector<std::shared_ptr<asio::io_context>> _io_contexts;
  std::vector<context_work> _works; // to keep he io_contexts running
  std::size_t _next_io_context;
  std::vector<std::shared_ptr<std::thread>> _threads;
 public:
  explicit IoContextPool(std::size_t pool_size) : _next_io_context{0} {
    if(pool_size == 0)
      throw std::runtime_error("io_context_pool size is 0");
    // Give all the io_contexts work to do so that their run() functions will not
    // exit until they are explicitly stopped.
    for (std::size_t i = 0; i < pool_size; ++i) {
      auto io_context = std::make_shared<asio::io_context>();
      _io_contexts.emplace_back(io_context);
      _works.push_back(asio::make_work_guard(*io_context));
    }
  }
  ~IoContextPool() {
    join();
    stop();
  }
  void join() {
    for(auto &t : _threads)
      t->join();
  }
  void run() {
    // Create a pool of threads to run all of the io_contexts.
    for(auto &context : _io_contexts) {
      _threads.emplace_back(std::make_shared<std::thread>([&context](){ context->run(); }));
    }
  }
  void stop() {
    // Explicitly stop all io_contexts.
    for (auto &context : _io_contexts)
      context->stop();
  }
  asio::io_context& getIoContext() {
    // Use a round-robin scheme to choose the next io_context to use.
    asio::io_context& io_context = *_io_contexts[_next_io_context];
    _next_io_context = (_next_io_context + 1) % _io_contexts.size();
    return io_context;
  }
};
