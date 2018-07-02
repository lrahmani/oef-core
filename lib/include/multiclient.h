#pragma once

#include "common.h"
#include "uuid.h"
#include "agent.pb.h"
#include <unordered_map>

namespace fetch {
  namespace oef {
    template <typename T>
    class Conversation {
    private:
      Uuid _uuid;
      std::string _dest;
      uint32_t _msgId = 0;
      T _state;
    public:
    Conversation(const std::string &uuid, const std::string &dest) : _uuid{Uuid{uuid}}, _dest{dest} {}
    Conversation(const std::string &dest) : _uuid{Uuid::uuid4()}, _dest{dest} {}
      std::string dest() const { return _dest; }
      std::string uuid() const { return _uuid.to_string(); }
      uint32_t msgId() const { return _msgId; }
      void incrementMsgId() { ++_msgId; }
      const T getState() const { return _state; }
      void setState(const T& t) { _state = t; }
    };

    template <typename State,typename T>
      class MultiClient : public std::enable_shared_from_this<MultiClient<State,T>> {
    protected:
      asio::io_context &_io_context;
      const std::string _id;
      tcp::socket _socket;
      std::unordered_map<std::string,std::shared_ptr<Conversation<State>>> _conversations;

      T &underlying() { return static_cast<T&>(*this); }
      T const &underlying() const { return static_cast<T const &>(*this); }
      
      void read() {
        asyncReadBuffer(_socket, 1000, [this](std::error_code ec, std::shared_ptr<Buffer> buffer) {
            if(ec) {
              std::cerr << "MultiClient::read failure " << ec.value() << std::endl;
            } else {
              std::cerr << "MultiClient::read " << std::endl;
              try {
                auto msg = deserialize<fetch::oef::pb::Server_AgentMessage>(*buffer);
                std::string uuid = msg.uuid();
                auto iter = _conversations.find(uuid);
                if(iter == _conversations.end()) {
                  std::string dest = "";
                  if(msg.has_content())
                    dest = msg.content().origin();
                  _conversations[uuid] = std::make_shared<Conversation<State>>(uuid, dest);
                }
                auto &c = _conversations[uuid];
                c->incrementMsgId();
                underlying().onMsg(msg, *c);
              } catch(std::exception &e) {
                std::cerr << "Wrong message sent\n";
              }
              read();
            }
      
          });
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
                        std::cerr << "MultiClient::secretHandshake received connected\n";
                        auto c = deserialize<fetch::oef::pb::Server_Connected>(*buffer);
                        if(c.status())
                          read();
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
