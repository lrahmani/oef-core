#pragma once

#include "asio.hpp"
#include "common.h"
#include "uuid.h"
#include "agent.pb.h"

namespace fetch {
  namespace oef {
    class Conversation {
    private:
      Uuid _uuid;
      std::string _dest;
      uint32_t _msgId = 0;
    public:
    Conversation(const std::string &uuid, const std::string &dest) : _uuid{Uuid{uuid}}, _dest{dest} {}
    Conversation(const std::string &dest) : _uuid{Uuid::uuid4()}, _dest{dest} {}
      std::string dest() const { return _dest; }
      std::string uuid() const { return _uuid.to_string(); }
      uint32_t msgId() const { return _msgId; }
    };
    
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
        for(std::size_t i = 0; i < _io_contexts.size(); ++i) {
          _threads.emplace_back(std::shared_ptr<std::thread>(new std::thread{[this,i](){ _io_contexts[i]->run(); }}));
        }
      }
      void stop() {
        // Explicitly stop all io_contexts.
        for (std::size_t i = 0; i < _io_contexts.size(); ++i)
          _io_contexts[i]->stop();
      }
      asio::io_context& getIoContext() {
        // Use a round-robin scheme to choose the next io_context to use.
        asio::io_context& io_context = *_io_contexts[_next_io_context];
        _next_io_context = (_next_io_context + 1) % _io_contexts.size();
        return io_context;
      }
    };
      
    class MultiClient : public std::enable_shared_from_this<MultiClient> {
    private:
      asio::io_context &_io_context;
      const std::string _id;
      tcp::socket _socket;
      std::unordered_map<std::string,Conversation> _conversations;

      void start() {
        std::cerr << "MultiClient " << _id << " ready to go\n";
      }
      void secretHandshake() {
        fetch::oef::pb::Agent_Server_ID id;
        id.set_id(_id);
        std::cerr << "MultiClient::secretHandshake\n";
        asyncWriteBuffer(_socket, serialize(id), 5, [this](std::error_code, std::size_t length){
            std::cerr << "MultiClient::secretHandshake id sent\n";
            asyncReadBuffer(_socket, 5, [this](std::error_code ec,std::shared_ptr<Buffer> buffer) {
                auto p = deserialize<fetch::oef::pb::Server_Phrase>(*buffer);
                std::string answer_s = p.phrase();
                std::cerr << "MultiClient received phrase: [" << answer_s << "]\n";
                // normally should encrypt with private key
                std::reverse(std::begin(answer_s), std::end(answer_s));
                std::cerr << "MultiClient sending back phrase: [" << answer_s << "]\n";
                fetch::oef::pb::Agent_Server_Answer answer;
                answer.set_answer(answer_s);
                asyncWriteBuffer(_socket, serialize(answer), 5, [this](std::error_code, std::size_t length){
                    asyncReadBuffer(_socket, 5, [this](std::error_code ec,std::shared_ptr<Buffer> buffer) {
                        auto c = deserialize<fetch::oef::pb::Server_Connected>(*buffer);
                        if(c.status())
                          start();
                      });
                  });
              });
          });
      }
    public:
      explicit MultiClient(asio::io_context &io_context, const std::string &id, const std::string &host)
        : _io_context{io_context}, _id{id}, _socket{_io_context} {
        tcp::resolver resolver(_io_context);
        asio::connect(_socket, resolver.resolve(host, std::to_string(static_cast<int>(Ports::Agents))));
        secretHandshake();
      }
      ~MultiClient() {
        _socket.shutdown(asio::socket_base::shutdown_both);
        _socket.close();
      }
    };
  }
}
