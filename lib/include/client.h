#pragma once

#include "proxy.h"
#include <iostream>
#include <thread>
#include "schema.h"

namespace fetch {
  namespace oef {
    class Client {
    protected:
      const std::string _id;
      Proxy _proxy;
      std::vector<std::unique_ptr<Conversation>> _conversations;
      void run() {
        _proxy.run();
      }
      
    public:
      explicit Client(const std::string &id, const char *host, const std::function<void(std::unique_ptr<Conversation>)> &onNew);
      virtual ~Client();
      bool send(const std::string &dest, const std::string &message);
      Conversation newConversation(const std::string &dest) {
        return Conversation(dest, _proxy);
      }
      bool registerAgent(const Instance &description);
      bool unregisterAgent(const Instance &description);
      std::vector<std::string> query(const QueryModel &query);
      std::vector<std::string> search(const QueryModel &query);
      bool addDescription(const Instance &description);
      void stop() {
        std::cerr << "Stopping proxy\n";
        _proxy.stop();
      }
    };
  }
}
