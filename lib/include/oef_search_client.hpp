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

#include "interface/oef_search_client_t.hpp"

#include "asio_communicator.hpp"
#include "logger.hpp"
#include "search.pb.h"


namespace fetch {
namespace oef {
  class OefSearchClient : public oef_search_client_t {
  public:
    explicit OefSearchClient(std::shared_ptr<AsioComm> comm, const std::string& core_id, 
        const std::string& core_ip_addr, uint32_t core_port)
        : comm_(std::move(comm))
        , core_ip_addr_{core_ip_addr}
        , core_port_{core_port}
        , core_id_{core_id}
        , updated_address_{true}
    {
    }
    
    virtual ~OefSearchClient() {}
    
    /*

    void connect(tcp::resolver::iterator endpoint_iterator);
    void RegisterServiceDescription(const std::string &agent, const Instance &instance);
    */

    void connect() override {};
    
    void register_description_sync(const std::string& agent, const Instance& desc) override;
    void unregister_description_sync(const std::string& agent) override;
    void register_service_sync(const std::string& agent, const Instance& service) override;
    void unregister_service_sync(const std::string& agent, const Instance& service) override;
    // TOFIX QueryModel don't save constraintExpr s
    std::vector<agent_t> search_agents_sync(const std::string& agent, const QueryModel& query) override;
    std::vector<agent_t> search_service_sync(const std::string& agent, const QueryModel& query) override;

  private:
    void addNetworkAddress(fetch::oef::pb::Update &update);

    mutable std::mutex lock_;
    std::shared_ptr<AsioComm> comm_;
    //char *server_key_;
    std::string core_ip_addr_;
    uint32_t core_port_ ;
    std::string core_id_;
    bool updated_address_;
    
    static fetch::oef::Logger logger;
  };
  
} //oef
} //fetch
