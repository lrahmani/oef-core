#pragma once
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

#include "api/agent_directory_t.hpp"
#include "api/agent_session_t.hpp"

#include "logger.hpp"
#include "schema.hpp"

#include <memory>

namespace fetch {
namespace oef {

        class AgentDirectory : public agent_directory_t {
        private:
            mutable std::mutex lock_;
            std::unordered_map<std::string,std::shared_ptr<agent_session_t>> sessions_;

            static fetch::oef::Logger logger;
        public:
            AgentDirectory() = default;
            
            AgentDirectory(const AgentDirectory &) = delete;
            AgentDirectory operator=(const AgentDirectory &) = delete;
            
            bool add(const std::string &id, std::shared_ptr<agent_session_t> session) override {
                std::lock_guard<std::mutex> lock(lock_);
                if(exist(id))
                    return false;
                sessions_[id] = std::move(session);
                return true;
            }
            bool exist(const std::string &id) const override {
                return sessions_.find(id) != sessions_.end();
            }
            bool remove(const std::string &id) override {
                std::lock_guard<std::mutex> lock(lock_);
                return sessions_.erase(id) == 1;
            }
            std::shared_ptr<agent_session_t> session(const std::string &id) const override {
                std::lock_guard<std::mutex> lock(lock_);
                auto iter = sessions_.find(id);
                if(iter != sessions_.end()) {
                    return iter->second;
                }
                return std::shared_ptr<agent_session_t>(nullptr);
            }
            size_t size() const override {
                std::lock_guard<std::mutex> lock(lock_);
                return sessions_.size();
            }
            void clear() override {
                std::lock_guard<std::mutex> lock(lock_);
                sessions_.clear();
            }
            
        };
} // oef
} // fetch
