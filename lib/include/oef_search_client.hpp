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
    std::unordered_map<uint32_t, LengthContinuation> msgs_handles_;
    mutable std::mutex msgs_handles_lock_;

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

    void register_description(const Instance& desc, const std::string& agent, uint32_t msg_id, LengthContinuation continuation);
    void unregister_description(const Instance& desc, const std::string& agent, uint32_t msg_id, LengthContinuation continuation);
    void register_service(const Instance& service, const std::string& agent, uint32_t msg_id, LengthContinuation continuation);
    void unregister_service(const Instance& service, const std::string& agent, uint32_t msg_id, LengthContinuation continuation);
    void search_agents(const std::string& agent, const QueryModel& query);
    void search_service(const std::string& agent, const QueryModel& query);
  
  private:
    //
    pb::TransportHeader generate_header_(const std::string& uri, uint32_t msg_id);
    pb::Update generate_update_(const Instance& service, const std::string& agent, uint32_t msg_id);
    pb::SearchQuery generate_search_(const QueryModel& query, const std::string& agent, uint32_t msg_id);
    pb::Remove generate_remove_(const Instance& instance, const std::string& agent, uint32_t msg_id);
    void addNetworkAddress(fetch::oef::pb::Update &update); // TOFIX to merge in generate_update_()
    //
    /* check lib/proto/search_transport.proto for Oef Search communication protocol */
    std::error_code search_send_sync_(std::shared_ptr<Buffer> header, std::shared_ptr<Buffer> payload);
    std::error_code search_receive_sync_(pb::TransportHeader& header, std::shared_ptr<Buffer>& payload);
    //
    //
    void search_send_async_(std::shared_ptr<Buffer> header, std::shared_ptr<Buffer> payload, LengthContinuation agent_session_cont);
    void search_send_async_(std::vector<std::shared_ptr<Buffer>> buffers, const std::string& agent, uint32_t msg_id);
    void search_schedule_rcv_(uint32_t msg_id, LengthContinuation continuation);
    void search_receive_async(std::function<void(pb::TransportHeader,std::shared_ptr<Buffer>)> continuation);
    void search_process_message_(pb::TransportHeader header, std::shared_ptr<Buffer> payload);
    void search_handle_msg_(std::shared_ptr<Buffer> buffer);
    void agent_send_error_(const std::string& agent, uint32_t msg_id);
    //
    bool msgs_handles_add(uint32_t msg_id, LengthContinuation continuation) {
      std::lock_guard<std::mutex> lock(msgs_handles_lock_);
      if(msgs_handles_.find(msg_id) != msgs_handles_.end())
        return false;
      msgs_handles_[msg_id] = continuation;
      return true;
    }
    bool msgs_handles_rmv(uint32_t msg_id) {
      std::lock_guard<std::mutex> lock(msgs_handles_lock_);
      return msgs_handles_.erase(msg_id) == 1;
    }
    LengthContinuation msgs_handles_get(uint32_t msg_id) {
      std::lock_guard<std::mutex> lock(msgs_handles_lock_);
      auto iter = msgs_handles_.find(msg_id);
      if(iter != msgs_handles_.end()) {
        return iter->second;
      }
      return [msg_id](std::error_code, uint32_t length){std::cerr << "No handle registered for message " << msg_id << std::endl;};
    }
  };
  
} //oef
} //fetch
