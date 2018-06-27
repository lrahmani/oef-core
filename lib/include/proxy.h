#pragma once

#include "asio.hpp"
#include "queue.h"
#include <iostream>
#include <thread>
#include "common.h"
#include <unordered_map>
#include <mutex>
#include "uuid.h"
#include "queue.h"
#include "agent.pb.h"

using asio::ip::tcp;

class Conversation;

class Proxy {
 private:
  const std::string _id;
  asio::io_context _io_context;
  tcp::socket _socket;
  std::unordered_map<std::string,Queue<std::unique_ptr<fetch::oef::pb::Server_AgentMessage>>> _inMsgBox;
  std::mutex _mutex;
  std::unique_ptr<std::thread> _proxyThread;
  std::function<void(std::unique_ptr<Conversation>)> _onNew;

  void write(std::shared_ptr<Buffer> buffer) {
    asyncWriteBuffer(_socket, buffer, 5);
  }
 public:
  explicit Proxy(const std::string &id, const std::string &host, const std::string &port,
                 const std::function<void(std::unique_ptr<Conversation>)> onNew)
    : _id{id}, _socket{_io_context}, _onNew{std::move(onNew)} {
    tcp::resolver resolver(_io_context);
    asio::connect(_socket, resolver.resolve(host, port));
  }
  ~Proxy() {
    _socket.shutdown(asio::socket_base::shutdown_both);
    _socket.close();
    _io_context.stop(); // ?????
    _proxyThread->join();
  }
  void read();
  bool secretHandshake();
  Queue<std::unique_ptr<fetch::oef::pb::Server_AgentMessage>> &getQueue(const std::string &uuid) {
    std::unique_lock<std::mutex> mlock(_mutex);
    return _inMsgBox[uuid];
  }
  void run() {
    if(!_proxyThread) {
      _proxyThread = std::unique_ptr<std::thread>(new std::thread([this]() { read(); _io_context.run();}));
    }
  }
  void stop() {
    _io_context.stop(); // ?????
  }
  void push(std::shared_ptr<Buffer> buffer) {
    //    std::cerr << "Proxy::push " << msg << std::endl;
    write(buffer);
  }
  std::unique_ptr<fetch::oef::pb::Server_AgentMessage> pop(const std::string &uuid) {
    std::unique_lock<std::mutex> mlock(_mutex);
    auto iter = _inMsgBox.find(uuid);
    if(iter == _inMsgBox.end()) {
      auto &q = _inMsgBox[uuid];
      mlock.unlock();
      auto ret = q.pop();
      return ret;
    }
    mlock.unlock();
    return iter->second.pop();
  }
};

class Conversation {
 private:
  Uuid _uuid;
  std::string _dest;
  Queue<std::unique_ptr<fetch::oef::pb::Server_AgentMessage>> &_queue;
  Proxy &_proxy;
 public:
  Conversation(const std::string &uuid, const std::string &dest,
               Queue<std::unique_ptr<fetch::oef::pb::Server_AgentMessage>> &queue,
               Proxy &proxy)
    : _uuid{Uuid{uuid}}, _dest{dest}, _queue{queue}, _proxy{proxy} {}
  Conversation(const std::string &dest, Proxy &proxy)
    : _uuid{Uuid::uuid4()}, _dest{dest}, _queue{proxy.getQueue(_uuid.to_string())},
      _proxy{proxy} {}
  bool send(const std::string &s);
  std::unique_ptr<fetch::oef::pb::Server_AgentMessage> pop() {
    return _queue.pop();
  }
  std::string popContent() {
    auto msg = pop();
    assert(msg->has_content());
    return msg->content().content();
  }
  template <typename T>
  T popMsg() {
    T t;
    t.ParseFromString(popContent());
    return t;
  }
  bool send(const google::protobuf::MessageLite &t) {
    return send(t.SerializeAsString());
  }
  size_t nbMsgs() const { return _queue.size(); }
  std::string dest() const { return _dest; }
  std::string uuid() const { return _uuid.to_string(); }
};

