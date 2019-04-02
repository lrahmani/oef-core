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

#include "oef_search_client.hpp"
#include "asio_communicator.hpp"
#include "serialization.hpp"

namespace fetch {
namespace oef {
    
fetch::oef::Logger OefSearchClient::logger = fetch::oef::Logger("oef-search-client");

std::error_code OefSearchClient::register_description_sync(const std::string& agent, const Instance& desc) {
  std::lock_guard<std::mutex> lock(lock_); // TOFIX until a state is maintained
  logger.warn("OefSearchClient::register_description_sync NOT implemented yet"); 
  return std::error_code{}; // success
}

std::error_code OefSearchClient::unregister_description_sync(const std::string& agent) {
  std::lock_guard<std::mutex> lock(lock_); // TOFIX until a state is maintained
  logger.warn("OefSearchClient::unregister_description_sync NOT implemented yet"); 
  return std::error_code{}; // success
}

std::error_code OefSearchClient::register_service_sync(const std::string& agent, const Instance& service) {
  std::lock_guard<std::mutex> lock(lock_); // TOFIX until a state is maintained
  // first, prepare cmd message and send it
  std::string cmd("update"); // TOFIX using a string field proto msg for serialization
  auto buffer_cmd = as::serialize(cmd);
  
  comm_->send_sync(buffer_cmd);

  // then prepare update proto message
  fetch::oef::pb::Update update;
  update.set_key(core_id_);

  fetch::oef::pb::Update_DataModelInstance* dm = update.add_data_models();

  dm->set_key(agent.c_str());
  dm->mutable_model()->CopyFrom(service.model());

  addNetworkAddress(update);

  auto buffer_update = pbs::serialize(update);
  
  // send messages 
  std::vector<std::shared_ptr<Buffer>> buffers;
  //buffers.emplace_back(buffer_cmd);
  buffers.emplace_back(buffer_update);
  
  logger.debug("OefSearchClient::register_service_sync sending update from agent {} to OefSearch: {}", 
        agent, pbs::to_string(update));

  return comm_->send_sync(buffers); // TOFIX not sure it's appropriate to return network error_code s
}

std::error_code OefSearchClient::unregister_service_sync(const std::string& agent, const Instance& service) {
  std::lock_guard<std::mutex> lock(lock_); // TOFIX until a state is maintained
  logger.warn("OefSearchClient::register_service_sync NOT implemented yet"); 
  return std::error_code{}; // success
}

std::error_code OefSearchClient::search_agents_sync(const std::string& agent, const QueryModel& query, std::vector<agent_t>& agents) {
  std::lock_guard<std::mutex> lock(lock_); // TOFIX until a state is maintained
  logger.warn("OefSearchClient::search_agents_sync NOT implemented yet"); 
  return std::error_code{}; // success
}
std::error_code OefSearchClient::search_service_sync(const std::string& agent, const QueryModel& query, std::vector<agent_t>& agents) {
  std::lock_guard<std::mutex> lock(lock_); // TOFIX until a state is maintained
  logger.warn("OefSearchClient::search_service_sync NOT implemented yet"); 
  return std::error_code{}; // success
}

/* async operation */

void OefSearchClient::register_description(const std::string& agent, const Instance& desc) {
}

void OefSearchClient::unregister_description(const std::string& agent) {
}

void OefSearchClient::register_service(const Instance& service, const std::string& agent, uint32_t msg_id) {
  std::string cmd("update"); // TOFIX using a string field proto msg for serialization
  auto buffer_cmd = as::serialize(cmd);
  
  comm_->send_sync(buffer_cmd);

  // then prepare update proto message
  fetch::oef::pb::Update update;
  update.set_key(core_id_);

  fetch::oef::pb::Update_DataModelInstance* dm = update.add_data_models();

  dm->set_key(agent.c_str());
  dm->mutable_model()->CopyFrom(service.model());

  addNetworkAddress(update);

  auto buffer_update = pbs::serialize(update);
  
  // send messages 
  //std::vector<std::shared_ptr<Buffer>> buffers;
  //buffers.emplace_back(buffer_cmd);
  //buffers.emplace_back(buffer_update);
  
  logger.debug("OefSearchClient::register_service sending update from agent {} to OefSearch: {}", 
        agent, pbs::to_string(update));

  search_send_async_(buffer_update, agent, msg_id);
}

void OefSearchClient::unregister_service(const std::string& agent, const Instance& service) {
}

void OefSearchClient::search_agents(const std::string& agent, const QueryModel& query) {
}

void OefSearchClient::search_service(const std::string& agent, const QueryModel& query) {
}
  

// TOFIX refactor to use std::vector<std::shared_ptr<Buffer>> overload
//       or, implicit conversion?
void OefSearchClient::search_send_async_(std::shared_ptr<Buffer> buffer, const std::string& agent, uint32_t msg_id) {
  comm_->send_async(buffer, [this,agent,msg_id](std::error_code ec, uint32_t len){
                               if(ec) {
                                 agent_send_error_(agent,msg_id);
                               } else {
                                 search_schedule_rcv_();
                               }
                             });
}

// TOFIX Not really implemented - sends only first buffer 
void OefSearchClient::search_send_async_(std::vector<std::shared_ptr<Buffer>> buffers, const std::string& agent, uint32_t msg_id) {
  comm_->send_async(buffers[0], [this,agent,msg_id](std::error_code ec, uint32_t len){
                               if(ec) {
                                 agent_send_error_(agent,msg_id);
                               } else {
                                 search_schedule_rcv_();
                               }
                             });
}

void OefSearchClient::search_schedule_rcv_() {
  comm_->receive_async([this](std::error_code ec, std::shared_ptr<Buffer> buffer) {
                         if(ec){
                           logger.error("----------------- Non-handled situation -------------------");
                         } else {
                           search_handle_msg_(buffer);
                         }
                       });
}

void OefSearchClient::agent_send_error_(const std::string& agent, uint32_t msg_id) {
  auto session = agent_directory_.session(agent);
  if(session){
    session->send_error(msg_id, fetch::oef::pb::Server_AgentMessage_OEFError::UNREGISTER_SERVICE); // TOFIX add OEFError::SEARCH
  } else {
    logger.error("OefSearchClient::agent_send_error agent {} not found", agent);
  }
}

void OefSearchClient::search_handle_msg_(std::shared_ptr<Buffer> buffer) {
  //auto envelope = pbs::deserialize<fetch::oef::pb::Message>(*buffer);  
  std::string agent;
  fetch::oef::pb::Server_AgentMessage message;
  auto session = agent_directory_.session(agent);
  if(session){
    session->send(message);  
  } else {
    logger.error("OefSearchClient::agent_send_error agent {} not found", agent);
  }
}

void OefSearchClient::addNetworkAddress(fetch::oef::pb::Update &update)
{
  if (!updated_address_) return;
  updated_address_ = false;

  fetch::oef::pb::Update_Address address;
  address.set_ip(core_ip_addr_);
  address.set_port(core_port_);
  address.set_key(core_id_);
  address.set_signature("Sign");

  fetch::oef::pb::Update_Attribute attr;
  attr.set_name(fetch::oef::pb::Update_Attribute_Name::Update_Attribute_Name_NETWORK_ADDRESS);
  auto *val = attr.mutable_value();
  val->set_type(10);
  val->mutable_a()->CopyFrom(address);

  update.add_attributes()->CopyFrom(attr);
}
  

} //oef
} //fetch
