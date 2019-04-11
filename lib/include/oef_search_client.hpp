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
#include "asio_basic_communicator.hpp"
#include "logger.hpp"

#include "search_message.pb.h"
#include "search_query.pb.h"
#include "search_remove.pb.h"
#include "search_response.pb.h"
#include "search_update.pb.h"
#include "search_transport.pb.h"

#include <memory>
#include <unordered_map>

namespace fetch {
namespace oef {
  class OefSearchClient : public oef_search_client_t {
  private:
    mutable std::mutex lock_;
    std::shared_ptr<AsioBasicComm> comm_;
    std::string core_ip_addr_;
    uint32_t core_port_ ;
    std::string core_id_;
    bool updated_address_;
    AgentDirectory& agent_directory_;
    std::unordered_map<uint32_t, AgentSessionContinuation> msg_handle_;
    std::unordered_map<uint32_t, std::string> msg_operation_;
    mutable std::mutex msg_handle_lock_;

    static fetch::oef::Logger logger;
  public:
    explicit OefSearchClient(std::shared_ptr<AsioBasicComm> comm, const std::string& core_id, 
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
    
    std::error_code register_description_sync(const Instance& service, const std::string& agent, uint32_t msg_id) override;
    std::error_code unregister_description_sync(const Instance& service, const std::string& agent, uint32_t msg_id) override;
    std::error_code register_service_sync(const Instance& service, const std::string& agent, uint32_t msg_id) override;
    std::error_code unregister_service_sync(const Instance& service, const std::string& agent, uint32_t msg_id) override;
    std::error_code search_agents_sync(const QueryModel& query, const std::string& agent, uint32_t msg_id, std::vector<agent_t>& agents) override;
    std::error_code search_service_sync(const QueryModel& query, const std::string& agent, uint32_t msg_id, std::vector<agent_t>& agents) override;

    void register_description(const Instance& desc, const std::string& agent, uint32_t msg_id, AgentSessionContinuation continuation);
    void unregister_description(const Instance& desc, const std::string& agent, uint32_t msg_id, AgentSessionContinuation continuation);
    void register_service(const Instance& service, const std::string& agent, uint32_t msg_id, AgentSessionContinuation continuation);
    void unregister_service(const Instance& service, const std::string& agent, uint32_t msg_id, AgentSessionContinuation continuation);
    void search_agents(const QueryModel& query, const std::string& agent, uint32_t msg_id, AgentSessionContinuation continuation);
    void search_service(const QueryModel& query, const std::string& agent, uint32_t msg_id, AgentSessionContinuation continuation);
    void search_service_wide(const QueryModel& query, const std::string& agent, uint32_t msg_id, AgentSessionContinuation continuation);
  
  private:
    //
    pb::TransportHeader generate_header_(const std::string& uri, uint32_t msg_id);
    pb::Update generate_update_(const Instance& service, const std::string& agent, uint32_t msg_id);
    pb::SearchQuery generate_search_(const QueryModel& query, const std::string& agent, uint32_t msg_id, uint32_t ttl);
    pb::Remove generate_remove_(const Instance& instance, const std::string& agent, uint32_t msg_id);
    void addNetworkAddress(fetch::oef::pb::Update &update); // TOFIX to merge in generate_update_()
    //
    /* check lib/proto/search_transport.proto for Oef Search communication protocol */
    std::error_code search_send_sync_(std::shared_ptr<Buffer> header, std::shared_ptr<Buffer> payload);
    std::error_code search_receive_sync_(pb::TransportHeader& header, std::shared_ptr<Buffer>& payload);
    //
    //
    void search_send_async_(std::shared_ptr<Buffer> header, std::shared_ptr<Buffer> payload, LengthContinuation continuation);
    void search_send_async_(std::vector<std::shared_ptr<Buffer>> buffers, const std::string& agent, uint32_t msg_id);
    void search_schedule_rcv_(uint32_t msg_id, std::string operation, AgentSessionContinuation continuation);
    void search_receive_async(std::function<void(pb::TransportHeader,std::shared_ptr<Buffer>)> continuation);
    void search_process_message_(pb::TransportHeader header, std::shared_ptr<Buffer> payload);
    //
    bool msg_handle_add(uint32_t msg_id, std::string operation, AgentSessionContinuation continuation) {
      std::lock_guard<std::mutex> lock(msg_handle_lock_);
      if(msg_handle_.find(msg_id) != msg_handle_.end())
        return false;
      msg_handle_[msg_id] = continuation;
      msg_operation_[msg_id] = operation;
      return true;
    }
    bool msg_handle_rmv(uint32_t msg_id) {
      std::lock_guard<std::mutex> lock(msg_handle_lock_);
      msg_operation_.erase(msg_id);
      return msg_handle_.erase(msg_id) == 1;
    }
    std::pair<std::string,AgentSessionContinuation> msg_handle_get(uint32_t msg_id) {
      std::lock_guard<std::mutex> lock(msg_handle_lock_);
      auto iter = msg_handle_.find(msg_id);
      if(iter != msg_handle_.end()) {
        auto iter_op = msg_operation_.find(msg_id);
        return std::make_pair(iter_op->second,iter->second);
      }
      return std::make_pair("",[msg_id](std::error_code, uint32_t length, std::vector<std::string> agents, pb::Server_SearchResultWide){std::cerr << "No handle registered for message " << msg_id << std::endl;});
    }
  };
  
} //oef
} //fetch
