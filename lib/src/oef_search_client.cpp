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
  // first, prepare cmd message  
  fetch::oef::pb::Server_Phrase cmd; // TOFIX using a string field proto msg for serialization
  cmd.set_phrase("update");
  auto buffer_cmd = pbs::serialize(cmd);
  
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
  buffers.emplace_back(buffer_cmd);
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
