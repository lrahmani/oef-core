// Company: FETCH.ai
// Author: Jerome Maloberti
// Creation: 09/01/18
//
// This file contains the basic functionality required to create an agent (also known as AEA).
// This is the ability to:
//   - Connect to a Node
//   - Register/deregister its service with the Node
//   - Query the Node for other AEAs

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <algorithm>

#include "common.h"
#include "agent.pb.h"
#include "client.h"
#include "clientmsg.h"

namespace fetch {
  namespace oef {
    Client::Client(const std::string &id, const char *host,
                   const std::function<void(std::unique_ptr<Conversation>)> &onNew) :
      _id{id},
      _proxy{_id, host, std::to_string(static_cast<int>(Ports::Agents)), onNew}
    {
      std::cerr << "Debug::Client " << _id << std::endl;
      if(!_proxy.secretHandshake()) {
        std::cerr << "Agent::Error on handshake.\n";
        throw std::logic_error("Not connected");
      }
      std::cerr << "Debug::Client run " << std::endl;
      run();
    }

    Client::~Client()
    {
      _proxy.stop();
      std::cerr << "~Client " << _id << "\n";
    }

    bool Client::registerAgent(const Instance &description)
    {
      Register reg{description};
      auto buffer = serialize(reg.handle());
      std::cerr << "Agent::registerAgent from " << _id << std::endl;
      _proxy.push(buffer);
      return true;
    }
    
    bool Client::unregisterAgent(const Instance &description)
    {
      Unregister unreg{description};
      auto buffer = serialize(unreg.handle());
      std::cerr << "Agent::unregisterAgent from " << _id << std::endl;
      _proxy.push(buffer);
      // wait for registered
      return true;
    }
    
    std::vector<std::string> Client::query(const QueryModel &query)
    {
      Query q{query};
      auto buffer = serialize(q.handle());
      _proxy.push(buffer);
      std::cerr << "Agent::query from " << _id << std::endl;
      // wait for answer
      auto agents = _proxy.pop("");
      std::cerr << "Received agents\n";
      std::vector<std::string> res;
      try {
        assert(agents->has_agents());
        for(auto &s : agents->agents().agents()) {
          res.emplace_back(s);
        }
      } catch(std::exception &) {
        std::cerr << "Agent::query " << _id << " not returned\n";
      }
      return res;
    }

    bool Client::send(const std::string &dest, const std::string &message)
    {
      Uuid conversationId = Uuid::uuid4();
      Message msg(conversationId.to_string(), dest, message);
      auto buffer = serialize(msg.handle());
      _proxy.push(buffer);
      return true;
    }

    std::vector<std::string> Client::search(const QueryModel &query)
    {
      Search search{query};
      auto buffer = serialize(search.handle());
      _proxy.push(buffer);
      std::cerr << "Agent::search from " << _id << std::endl;
      // wait for answer
      auto agents = _proxy.pop("");
      std::cerr << "Received agents\n";
      std::vector<std::string> res;
      try {
        assert(agents->has_agents());
        for(auto &s : agents->agents().agents()) {
          res.emplace_back(s);
        }
      } catch(std::exception &) {
        std::cerr << "Agent::search " << _id << " not returned\n";
      }
      return res;
    }
    
    bool Client::addDescription(const Instance &description)
    {
      Description desc{description};
      auto buffer = serialize(desc.handle());
      _proxy.push(buffer);
      std::cerr << "Agent::addDescription from " << _id << std::endl;
      return true;
    }
  }
}
