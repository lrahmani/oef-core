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

namespace fetch {
  namespace oef {
    Client::Client(const std::string &id, const char *host, const std::function<void(std::unique_ptr<Conversation>)> &onNew) :
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
    class Register {
    private:
      fetch::oef::pb::Envelope _envelope;
    public:
      explicit Register(const Instance &instance) {
        auto *reg = _envelope.mutable_register_();
        auto *inst = reg->mutable_description();
        inst->CopyFrom(instance.handle());
      }
      const fetch::oef::pb::Envelope &handle() const { return _envelope; }
    };

    bool Client::registerAgent(const Instance &description)
    {
      Register reg{description};
      auto buffer = serialize(reg.handle());
      std::cerr << "Agent::registerAgent from " << _id << std::endl;
      _proxy.push(buffer);
      // wait for registered
      auto registered = _proxy.pop("");
      std::cerr << "Received registered\n";
      try {
        assert(registered->has_status());
        return registered->status();
      } catch(std::exception &e) {
        std::cerr << "Agent::register " << _id << " not registered\n";
      }
      return false;
    }
    
    class Unregister {
    private:
      fetch::oef::pb::Envelope _envelope;
    public:
      explicit Unregister(const Instance &instance) {
        auto *reg = _envelope.mutable_unregister();
        auto *inst = reg->mutable_description();
        inst->CopyFrom(instance.handle());
      }
      const fetch::oef::pb::Envelope &handle() const { return _envelope; }
    };

    bool Client::unregisterAgent(const Instance &description)
    {
      Unregister unreg{description};
      auto buffer = serialize(unreg.handle());
      std::cerr << "Agent::unregisterAgent from " << _id << std::endl;
      _proxy.push(buffer);
      // wait for registered
      auto registered = _proxy.pop("");
      std::cerr << "Received registered\n";
      try {
        assert(registered->has_status());
        return registered->status();
      } catch(std::exception &e) {
        std::cerr << "Agent::unregister " << _id << " not unregistered\n";
      }
      return false;
    }
    
    class Query {
    private:
      fetch::oef::pb::Envelope _envelope;
    public:
      explicit Query(const QueryModel &model) {
        auto *desc = _envelope.mutable_query();
        auto *mod = desc->mutable_query();
        mod->CopyFrom(model.handle());
      }
      const fetch::oef::pb::Envelope &handle() const { return _envelope; }
    };
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
      } catch(std::exception &e) {
        std::cerr << "Agent::query " << _id << " not returned\n";
      }
      return res;
    }

    
    class Message {
    private:
      fetch::oef::pb::Envelope _envelope;
    public:
      explicit Message(const std::string &conversationID, const std::string &dest, const std::string msg) {
        auto *message = _envelope.mutable_message();
        message->set_cid(conversationID);
        message->set_destination(dest);
        message->set_content(msg);
      }
      const fetch::oef::pb::Envelope &handle() const { return _envelope; }
    };
    bool Client::send(const std::string &dest, const std::string &message)
    {
      Uuid conversationId = Uuid::uuid4();
      Message msg(conversationId.to_string(), dest, message);
      auto buffer = serialize(msg.handle());
      _proxy.push(buffer);
      // wait for delivered
      auto delivered = _proxy.pop("");
      std::cerr << "Received delivered\n";
      try {
        assert(delivered->has_status());
        return delivered->status();
      } catch(std::exception &e) {
        std::cerr << "Agent::delivered from " << _id << " not delivered\n";
      }
      return false;
    }

    class Search {
    private:
      fetch::oef::pb::Envelope _envelope;
    public:
      explicit Search(const QueryModel &model) {
        auto *desc = _envelope.mutable_search();
        auto *mod = desc->mutable_query();
        mod->CopyFrom(model.handle());
      }
      const fetch::oef::pb::Envelope &handle() const { return _envelope; }
    };
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
      } catch(std::exception &e) {
        std::cerr << "Agent::search " << _id << " not returned\n";
      }
      return res;
    }
    
    class Description {
    private:
      fetch::oef::pb::Envelope _envelope;
    public:
      explicit Description(const Instance &instance) {
        auto *desc = _envelope.mutable_description();
        auto *inst = desc->mutable_description();
        inst->CopyFrom(instance.handle());
      }
      const fetch::oef::pb::Envelope &handle() const { return _envelope; }
    };
    bool Client::addDescription(const Instance &description)
    {
      Description desc{description};
      auto buffer = serialize(desc.handle());
      _proxy.push(buffer);
      std::cerr << "Agent::addDescription from " << _id << std::endl;
      // wait for registered
      auto registered = _proxy.pop("");
      std::cerr << "Received registered\n";
      try {
        assert(registered->has_status());
        return registered->status();
      } catch(std::exception &e) {
        std::cerr << "Agent::register " << _id << " not registered\n";
      }
      return false;
    }
  }
}
