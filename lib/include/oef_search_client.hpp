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

#include "api/oef_search_client_t.hpp"

#include "agent_directory.hpp"
#include "asio_communicator.hpp"
#include "logger.hpp"

#include "search_message.pb.h"
#include "search_query.pb.h"
#include "search_remove.pb.h"
#include "search_response.pb.h"
#include "search_update.pb.h"

#include <memory>

namespace fetch {
namespace oef {
  class OefSearchClient : public oef_search_client_t {
  private:
    mutable std::mutex lock_;
    std::shared_ptr<AsioComm> comm_;
    std::string core_ip_addr_;
    uint32_t core_port_ ;
    std::string core_id_;
    bool updated_address_;
    AgentDirectory& agent_directory_;
     
    static fetch::oef::Logger logger;
  public:
    explicit OefSearchClient(std::shared_ptr<AsioComm> comm, const std::string& core_id, 
        const std::string& core_ip_addr, uint32_t core_port, AgentDirectory& agent_directory)
        : comm_(std::move(comm))
        , core_ip_addr_{core_ip_addr}
        , core_port_{core_port}
        , core_id_{core_id}
        , updated_address_{true}
        , agent_directory_{agent_directory}
    {
    }
    
    virtual ~OefSearchClient() {}
    
    /* TODO */
    void connect() override {};
    
    std::error_code register_description_sync(const std::string& agent, const Instance& desc) override;
    std::error_code unregister_description_sync(const std::string& agent) override;
    std::error_code register_service_sync(const Instance& service, const std::string& agent, uint32_t msg_id) override;
    std::error_code unregister_service_sync(const std::string& agent, const Instance& service) override;
    // TOFIX QueryModel don't save constraintExpr s (you sure?)
    std::error_code search_agents_sync(const std::string& agent, const QueryModel& query, std::vector<agent_t>& agents) override;
    std::error_code search_service_sync(const QueryModel& query, const std::string& agent, uint32_t msg_id, std::vector<agent_t>& agents) override;

    void register_description(const std::string& agent, const Instance& desc);
    void unregister_description(const std::string& agent);
    void register_service(const Instance& service, const std::string& agent, uint32_t msg_id);
    void unregister_service(const std::string& agent, const Instance& service);
    // TOFIX QueryModel don't save constraintExpr s (you sure?)
    void search_agents(const std::string& agent, const QueryModel& query);
    void search_service(const std::string& agent, const QueryModel& query);
  
  private:
    void search_send_async_(std::shared_ptr<Buffer> buffer, const std::string& agent, uint32_t msg_id);
    void search_send_async_(std::vector<std::shared_ptr<Buffer>> buffers, const std::string& agent, uint32_t msg_id);
    void search_schedule_rcv_();
    void search_handle_msg_(std::shared_ptr<Buffer> buffer);
    void agent_send_error_(const std::string& agent, uint32_t msg_id);
    void addNetworkAddress(fetch::oef::pb::Update &update);
  };
  
} //oef
} //fetch
