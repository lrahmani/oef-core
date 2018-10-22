#pragma once

#include "schema.h"

#include <unordered_map>
#include <set>
#include <unordered_set>
#include <mutex>

namespace fetch {
  namespace oef {
    class Agents {
    private:
      std::unordered_set<std::string> _agents;
    public:
      explicit Agents() {}
      template <typename Archive>
      explicit Agents(const Archive &ar) {
        std::function<void(const Archive &)> f = [this](const Archive &iar) {
                                                   _agents.emplace(iar.getString());
                                                 };      
        ar.parseObjects(f);
      }
      bool insert(const std::string &agent) {
        return _agents.insert(agent).second;
      }
      bool erase(const std::string &agent) {
        return _agents.erase(agent) == 1;
      }
      size_t size() const {
        return _agents.size();
      }
      void copy(std::unordered_set<std::string> &s) const {
        std::copy(_agents.begin(), _agents.end(), std::inserter(s, s.end()));
      }
    };

    class ServiceDirectory {
    private:
      mutable std::mutex _lock;
      std::unordered_map<Instance,Agents> _data;
    public:
      explicit ServiceDirectory() = default;
      bool registerAgent(const Instance &instance, const std::string &agent) {
        std::lock_guard<std::mutex> lock(_lock);
        return _data[instance].insert(agent);
      }
      bool unregisterAgent(const Instance &instance, const std::string &agent) {
        std::lock_guard<std::mutex> lock(_lock);
        auto iter = _data.find(instance);
        if(iter == _data.end())
          return false;
        bool res = iter->second.erase(agent);
        if(iter->second.size() == 0) {
          _data.erase(instance);
        }
        return res;
      }
      void unregisterAll(const std::string &agent) {
        std::lock_guard<std::mutex> lock(_lock);
        for(auto &p : _data) {
          p.second.erase(agent);
        }
      }
      size_t size() const {
        std::lock_guard<std::mutex> lock(_lock);
        return _data.size();
      }
      std::vector<std::string> query(const QueryModel &query) const {
        std::lock_guard<std::mutex> lock(_lock);
        std::unordered_set<std::string> res;
        for(auto &d : _data) {
          if(query.check(d.first)) {
            d.second.copy(res);
          }
        }
        return std::vector<std::string>(res.begin(), res.end());
      }
    };
  };
};
