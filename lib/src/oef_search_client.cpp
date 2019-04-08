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

#include <arpa/inet.h>

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

std::error_code OefSearchClient::register_service_sync(const Instance& service, const std::string& agent, uint32_t msg_id) {
  std::lock_guard<std::mutex> lock(lock_); // TOFIX until a state is maintained
  // prepare header
  auto header = generate_header_("update", msg_id);
  auto header_buffer = pbs::serialize(header);

  // then prepare payload message
  auto update = generate_update_(service, agent, msg_id);
  auto update_buffer = pbs::serialize(update);
 
  // send message
  logger.debug("::register_service_sync sending update from agent {} to OefSearch: {} - {}", 
        agent, pbs::to_string(header), pbs::to_string(update));
  auto ec = search_send_sync_(header_buffer,update_buffer); 
  if(ec){
    logger.debug("::register_service_sync error while sending update from agent {} to OefSearch", agent);
    return ec;
  }
  
  // receive response
  pb::TransportHeader resp_header;
  std::shared_ptr<Buffer> resp_payload;
  logger.debug("::register_service_sync waiting for update confirmation from OefSearch");
  ec = search_receive_sync_(resp_header, resp_payload);
  if(ec) {
    logger.debug("::register_service_sync error while receiving confirmation from OefSearch");
    return ec;
  }
  logger.debug("::register_service_sync received header message from search: {} ", pbs::to_string(resp_header));
  
  // check if success
  if(!resp_header.status().success()) {
    logger.debug("::register_service_sync Oef Search operation non successfull");
    return std::make_error_code(std::errc::no_message_available);
  }

  // get payload

  return std::error_code{};
}

std::error_code OefSearchClient::unregister_service_sync(const std::string& agent, const Instance& service) {
  std::lock_guard<std::mutex> lock(lock_); // TOFIX until a state is maintained
  logger.warn("OefSearchClient::register_service_sync NOT implemented yet"); 
  return std::error_code{}; // success
}

std::error_code OefSearchClient::search_service_sync(const QueryModel& query, const std::string& agent, uint32_t msg_id, std::vector<agent_t>& agents) {
  std::lock_guard<std::mutex> lock(lock_); // TOFIX until a state is maintained
  /*
  // prepare search message
  pb::SearchMessage envelope;
  envelope.set_id(msg_id);
  envelope.set_uri("search");
  envelope.mutable_status()->set_success(true);

  // then prepare payload envelope
  pb::SearchQuery search_query;
  search_query.set_source_key(core_id_);
  search_query.mutable_model()->CopyFrom(search_query.model());
  search_query.set_ttl(1);

  envelope.set_body(search_query.SerializeAsString());

  // serialize envelope
  auto buffer = pbs::serialize(envelope);
  
  // send envelope
  
  logger.debug("OefSearchClient::search_service_sync sending query from agent {} to OefSearch: {}", 
        agent, pbs::to_string(envelope));

  auto ec = comm_->send_sync(buffer); 
  
  if(ec){
    logger.debug("OefSearchClient::search_service_sync error while sending query from agent {} to OefSearch", agent);
    return ec;
  }

  logger.debug("OefSearchClient::search_service_sync waiting for query answer from OefSearch");
  std::shared_ptr<Buffer> buffer_resp;
  ec = comm_->receive_sync(buffer_resp);
    
  if(ec) {
    logger.debug("OefSearchClient::search_service_sync error while receiving query answer from OefSearch");
    return ec;
  } 
  
  auto envelope_resp = pbs::deserialize<pb::SearchMessage>(*buffer_resp);
  logger.debug("received search envelope {} ", pbs::to_string(envelope_resp));

  // get query results
  auto search_resp = pbs::deserialize<pb::SearchResponse>(envelope_resp.body());
  logger.debug("received search results {} ", pbs::to_string(search_resp));
  
  auto items = search_resp.result();
  for (auto& item : items) {
    auto agts = item.agents();
    for (auto& a : agts) {
      std::string key{*a.mutable_key()};
      agents.emplace_back(agent_t{key,""});
    }
  }
  */

  return std::error_code{};
}
std::error_code OefSearchClient::search_agents_sync(const std::string& agent, const QueryModel& query, std::vector<agent_t>& agents) {
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
  /*
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
  */
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
  /*
  comm_->send_async(buffer, [this,agent,msg_id](std::error_code ec, uint32_t len){
                               if(ec) {
                                 agent_send_error_(agent,msg_id);
                               } else {
                                 search_schedule_rcv_();
                               }
                             });
  */
}

// TOFIX Not really implemented - sends only first buffer 
void OefSearchClient::search_send_async_(std::vector<std::shared_ptr<Buffer>> buffers, const std::string& agent, uint32_t msg_id) {
  /*
  comm_->send_async(buffers[0], [this,agent,msg_id](std::error_code ec, uint32_t len){
                               if(ec) {
                                 agent_send_error_(agent,msg_id);
                               } else {
                                 search_schedule_rcv_();
                               }
                             });
  */
}

void OefSearchClient::search_schedule_rcv_() {
  /*
  comm_->receive_async([this](std::error_code ec, std::shared_ptr<Buffer> buffer) {
                         if(ec){
                           logger.error("----------------- Non-handled situation -------------------");
                         } else {
                           search_handle_msg_(buffer);
                         }
                       });
  */
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
  
pb::TransportHeader OefSearchClient::generate_header_(const std::string& uri, uint32_t msg_id) {
  pb::TransportHeader header;
  header.set_uri(uri);
  header.set_id(msg_id);
  header.mutable_status()->set_success(true);
  return header;
}

pb::Update OefSearchClient::generate_update_(const Instance& service, const std::string& agent, uint32_t msg_id) {
  fetch::oef::pb::Update update;
  update.set_key(core_id_);

  fetch::oef::pb::Update_DataModelInstance* dm = update.add_data_models();
  dm->set_key(agent.c_str());
  dm->mutable_model()->CopyFrom(service.model());

  addNetworkAddress(update);
  return update;
}

std::error_code OefSearchClient::search_send_sync_(std::shared_ptr<Buffer> header, std::shared_ptr<Buffer> payload) {
  // check lib/proto/search_transport.proto for Oef Search communication protocol
  std::vector<void*> buffers;
  std::vector<size_t> nbytes;

  // first, pack sizes of buffers
  uint32_t header_size = htonl(header->size());
  uint32_t payload_size = htonl(payload->size());
  buffers.emplace_back(&header_size);
  buffers.emplace_back(&payload_size);
  nbytes.emplace_back(sizeof(header_size));
  nbytes.emplace_back(sizeof(payload_size));
  
  // then, pack the actual buffers
  buffers.emplace_back(header->data());
  buffers.emplace_back(payload->data());
  nbytes.emplace_back(header_size);
  nbytes.emplace_back(payload_size);

  // send message
  return comm_->send_sync(buffers, nbytes);
}

std::error_code OefSearchClient::search_receive_sync_(pb::TransportHeader& header, std::shared_ptr<Buffer>& payload) {
  // check lib/proto/search_transport.proto for Oef Search communication protocol
  std::error_code ec;
  
  // first, receive sizes
  uint32_t header_size;
  uint32_t payload_size;
  ec = comm_->receive_sync(&header_size, sizeof(uint32_t));
  if (ec) {
    logger.error("search_receive_sync_ Error while receiving header length : {}", ec.value());
    return ec;
  }
  header_size = ntohl(header_size);
  ec = comm_->receive_sync(&payload_size, sizeof(uint32_t));
  if (ec) {
    logger.error("search_receive_sync_ Error while receiving payload length : {}", ec.value());
    return ec;
  }
  payload_size = ntohl(payload_size);
  
  // then, receive actual data (the search message)
  // receive the header
  std::unique_ptr<Buffer> header_buffer = std::make_unique<Buffer>(header_size); 
  payload = std::make_shared<Buffer>(payload_size);
  ec = comm_->receive_sync(header_buffer.get(), header_size);
  if (ec) {
    logger.error("search_receive_sync_ Error while receiving header : {}", ec.value());
    return ec;
  }
  header = pbs::deserialize<pb::TransportHeader>(*header_buffer);
  // receive the payload
  if (!payload_size) {
    logger.debug("search_receive_sync_ No payload expected");
    return ec; 
  }
  ec = comm_->receive_sync(payload.get(), payload_size);
  if (ec) {
    logger.error("search_receive_sync_ Error while receiving payload : {}", ec.value());
    return ec;
  }
  return ec;
}

} //oef
} //fetch
