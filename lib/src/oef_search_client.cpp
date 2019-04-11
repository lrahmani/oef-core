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


/* 
 * *********************************
 * Synchronous Oef operations 
 * *********************************
*/


std::error_code OefSearchClient::register_description_sync(const Instance& desc, const std::string& agent, uint32_t msg_id) {
  logger.warn("::register_description_sync implemented using ::register_service_sync"); 
  return register_service_sync(desc, agent, msg_id);
}

std::error_code OefSearchClient::unregister_description_sync(const Instance& desc, const std::string& agent, uint32_t msg_id) {
  logger.warn("::unregister_description_sync implemented using ::unregister_service_sync"); 
  return unregister_service_sync(desc, agent, msg_id);
}

std::error_code OefSearchClient::search_agents_sync(const QueryModel& query, const std::string& agent, uint32_t msg_id, std::vector<agent_t>& agents) {
  logger.warn("::search_service_sync implemented using ::search_service_sync"); 
  return search_service_sync(query, agent, msg_id, agents);
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

  // get response payload
  if(!resp_payload) {
    return std::error_code{};
  }
  // get remove confirmation
  auto update_resp = pbs::deserialize<pb::UpdateResponse>(*resp_payload);
  logger.debug("::register_service_sync received update confirmation {} ", pbs::to_string(update_resp));

  return std::error_code{};
}

std::error_code OefSearchClient::unregister_service_sync(const Instance& service, const std::string& agent, uint32_t msg_id) {
  std::lock_guard<std::mutex> lock(lock_); // TOFIX until a state is maintained
  // prepare header
  auto header = generate_header_("remove", msg_id);
  auto header_buffer = pbs::serialize(header);

  // then prepare payload message
  auto remove = generate_remove_(service, agent, msg_id);
  auto remove_buffer = pbs::serialize(remove);
  
  // send message
  logger.debug("::unregister_service_sync sending remove from agent {} to OefSearch: {} - {}", 
        agent, pbs::to_string(header), pbs::to_string(remove));
  auto ec = search_send_sync_(header_buffer,remove_buffer); 
  if(ec){
    logger.debug("::unregister_service_sync error while sending remove from agent {} to OefSearch", agent);
    return ec;
  }
  
  // receive response
  pb::TransportHeader resp_header;
  std::shared_ptr<Buffer> resp_payload;
  logger.debug("::unregister_service_sync waiting for remove confirmation from OefSearch");
  ec = search_receive_sync_(resp_header, resp_payload);
  if(ec) {
    logger.debug("::unregister_service_sync error while receiving remove confirmation from OefSearch");
    return ec;
  }
  logger.debug("::unregister_service_sync received header message from search: {} ", pbs::to_string(resp_header));
  
  // check if response
  if(!resp_header.status().success()) {
    logger.debug("::unregister_service_sync Oef Search operation non successfull");
    return std::make_error_code(std::errc::no_message_available);
  }

  // get response payload
  if(!resp_payload) {
    return std::error_code{};
  }
  // get remove confirmation
  auto remove_resp = pbs::deserialize<pb::RemoveResponse>(*resp_payload);
  logger.debug("::unregister_service_sync received remove confirmation {} ", pbs::to_string(remove_resp));
  

  return std::error_code{}; // success
}

std::error_code OefSearchClient::search_service_sync(const QueryModel& query, const std::string& agent, uint32_t msg_id, std::vector<agent_t>& agents) {
  std::lock_guard<std::mutex> lock(lock_); // TOFIX until a state is maintained
  // prepare header
  auto header = generate_header_("search", msg_id);
  auto header_buffer = pbs::serialize(header);

  // then prepare payload message
  auto search = generate_search_(query, agent, msg_id);
  auto search_buffer = pbs::serialize(search);
  
  // send message
  logger.debug("::search_service_sync sending search from agent {} to OefSearch: {} - {}", 
        agent, pbs::to_string(header), pbs::to_string(search));
  auto ec = search_send_sync_(header_buffer,search_buffer); 
  if(ec){
    logger.debug("::search_service_sync error while sending search from agent {} to OefSearch", agent);
    return ec;
  }
  
  // receive response
  pb::TransportHeader resp_header;
  std::shared_ptr<Buffer> resp_payload;
  logger.debug("::search_service_sync waiting for search answer from OefSearch");
  ec = search_receive_sync_(resp_header, resp_payload);
  if(ec) {
    logger.debug("::search_service_sync error while receiving search answer from OefSearch");
    return ec;
  }
  logger.debug("::search_service_sync received header message from search: {} ", pbs::to_string(resp_header));
  
  // check if success
  if(!resp_header.status().success()) {
    logger.debug("::search_service_sync Oef Search operation non successfull");
    return std::make_error_code(std::errc::no_message_available);
  }

  // get payload
  if(!resp_payload) {
    return std::error_code{};
  }
  // get search results
  auto search_resp = pbs::deserialize<pb::SearchResponse>(*resp_payload);
  logger.debug("::search_service_sync received search results {} ", pbs::to_string(search_resp));
  
  auto items = search_resp.result();
  for (auto& item : items) {
    auto agts = item.agents();
    for (auto& a : agts) {
      std::string key{*a.mutable_key()};
      agents.emplace_back(agent_t{key,""});
    }
  }

  return std::error_code{};
}


/*
 * *********************************
 * Asynchronous Oef operations 
 * *********************************
*/


void OefSearchClient::register_description(const Instance& desc, const std::string& agent, uint32_t msg_id, AgentSessionContinuation continuation) {
  logger.warn("::register_description implemented using ::register_service");
  register_service(desc, agent, msg_id, continuation);
}

void OefSearchClient::unregister_description(const Instance& desc, const std::string& agent, uint32_t msg_id, AgentSessionContinuation continuation) {
  logger.warn("::unregister_description implemented using ::unregister_service"); 
  unregister_service(desc, agent, msg_id, continuation);
}

void OefSearchClient::search_agents(const QueryModel& query, const std::string& agent, uint32_t msg_id, AgentSessionContinuation continuation) {
  logger.warn("::search_agents implemented using ::search_service"); 
  search_service(query, agent, msg_id, continuation);
}

void OefSearchClient::register_service(const Instance& service, const std::string& agent, uint32_t msg_id, AgentSessionContinuation continuation) {
  // prepare header
  auto header = generate_header_("update", msg_id);
  auto header_buffer = pbs::serialize(header);

  // then prepare payload message
  auto update = generate_update_(service, agent, msg_id);
  auto update_buffer = pbs::serialize(update);
 
  // send message
  logger.debug("::register_service sending update from agent {} to OefSearch: {} - {}", 
        agent, pbs::to_string(header), pbs::to_string(update));
  
  search_send_async_(header_buffer, update_buffer, 
      [this,agent,msg_id,continuation](std::error_code ec, uint32_t length) {
        if (ec) {
          logger.debug("::register_service error while sending update from agent {} to OefSearch: {}",
              agent, ec.value());
          continuation(ec, length, std::vector<std::string>{});          
        } else {
          // schedule reception of answer
          logger.debug("::register_service update message sent to OefSearch");
          search_schedule_rcv_(msg_id, "update", continuation);
        }
      });
}

void OefSearchClient::unregister_service(const Instance& service, const std::string& agent, uint32_t msg_id, AgentSessionContinuation continuation) {
  // prepare header
  auto header = generate_header_("remove", msg_id);
  auto header_buffer = pbs::serialize(header);

  // then prepare payload message
  auto remove = generate_remove_(service, agent, msg_id);
  auto remove_buffer = pbs::serialize(remove);
  
  // send message
  logger.debug("::unregister_service sending remove from agent {} to OefSearch: {} - {}", 
        agent, pbs::to_string(header), pbs::to_string(remove));
  
  search_send_async_(header_buffer, remove_buffer, 
      [this,agent,msg_id,continuation](std::error_code ec, uint32_t length) {
        if (ec) {
          logger.debug("::unregister_service error while sending remove from agent {} to OefSearch: {}",
              agent, ec.value());
          continuation(ec, length, std::vector<std::string>{});          
        } else {
          // schedule reception of answer
          logger.debug("::unregister_service remove message sent to OefSearch");
          search_schedule_rcv_(msg_id, "remove", continuation);
        }
      });
}

void OefSearchClient::search_service(const QueryModel& query, const std::string& agent, uint32_t msg_id, AgentSessionContinuation continuation) {
  // prepare header
  auto header = generate_header_("search", msg_id);
  auto header_buffer = pbs::serialize(header);

  // then prepare payload message
  auto search = generate_search_(query, agent, msg_id);
  auto search_buffer = pbs::serialize(search);
  
  // send message
  logger.debug("::search_service sending search from agent {} to OefSearch: {} - {}", 
        agent, pbs::to_string(header), pbs::to_string(search));
  
  search_send_async_(header_buffer, search_buffer, 
      [this,agent,msg_id,continuation](std::error_code ec, uint32_t length) {
        if (ec) {
          logger.debug("::search_service error while sending search from agent {} to OefSearch: {}",
              agent, ec.value());
          continuation(ec, length, std::vector<std::string>{});          
        } else {
          // schedule reception of answer
          logger.debug("::search_service search message sent to OefSearch");
          search_schedule_rcv_(msg_id, "search", continuation);
        }
      });
}
  

/*
 * *********************************
 * Asynchronous helper functions
 * *********************************
*/


// TOFIX refactor to use std::vector<std::shared_ptr<Buffer>> overload
//       or, implicit conversion?
void OefSearchClient::search_send_async_(std::shared_ptr<Buffer> header, std::shared_ptr<Buffer> payload,
          LengthContinuation continuation) {
  std::vector<std::shared_ptr<Buffer>> buffers;
  std::vector<size_t> nbytes;
  
  // first, pack sizes of buffers
  auto header_size = oef::serialize(htonl(header->size()));
  auto payload_size = oef::serialize(htonl(payload->size()));
  buffers.emplace_back(header_size);
  buffers.emplace_back(payload_size);
  nbytes.emplace_back(sizeof(uint32_t));
  nbytes.emplace_back(sizeof(uint32_t));
  
  // then, pack the actual buffers
  buffers.emplace_back(header); // TOFIX why implicit conversion is happening for Buffer -> void but not for uint32_t (bc its a pointer type?)
  buffers.emplace_back(payload);
  nbytes.emplace_back(header->size());
  nbytes.emplace_back(payload->size());

  // send message
  comm_->send_async(buffers, nbytes, continuation);
}

void OefSearchClient::search_schedule_rcv_(uint32_t msg_id, std::string operation, AgentSessionContinuation continuation) {
  msg_handle_add(msg_id, operation, continuation);
  search_receive_async(
      [this](pb::TransportHeader header, std::shared_ptr<Buffer> payload) {
        search_process_message_(header, payload);
      });
}

void OefSearchClient::search_receive_async(std::function<void(pb::TransportHeader,std::shared_ptr<Buffer>)> continuation){
  // needs locking, to make sure sizes and header & body are received al together
  std::lock_guard<std::mutex> lock(lock_); // TOFIX be careful with deadlocks (which func should lock,mixing sync and async, ...)
  // first, receive sizes
  comm_->receive_async(2*sizeof(uint32_t),
      [this,continuation](std::error_code ec, std::shared_ptr<Buffer> buffer){
        if (ec) {
          logger.error("search_receive_async_ Error while receiving header and payload lengths : {}", ec.value());
          pb::TransportHeader header;
          continuation(header, nullptr);
        } else {
          uint32_t* sizes = (uint32_t*)buffer->data();
          uint32_t header_size = ntohl(sizes[0]);
          uint32_t payload_size = ntohl(sizes[1]);
          logger.debug("search_receive_async_ received lenghts : {} - {}", header_size, payload_size);
          // then receive the actual message
          comm_->receive_async(header_size+payload_size,
              [this,header_size,payload_size,continuation](std::error_code ec, std::shared_ptr<Buffer> buffer){
                if (ec) {
                  logger.error("search_receive_async_ Error while receiving header and payload : {}", ec.value());
                  pb::TransportHeader header;
                  continuation(header, nullptr);
                } else {
                  uint8_t* message = (uint8_t*)buffer->data();
                  std::vector<uint8_t> header_buffer(message, message+header_size);
                  std::vector<uint8_t> payload_buffer(message+header_size,message+header_size+payload_size);
                  // get header
                  pb::TransportHeader header = pbs::deserialize<pb::TransportHeader>(header_buffer);
                  if(!payload_size) {
                    continuation(header,nullptr);  
                  } else {
                    continuation(header, std::make_shared<Buffer>(payload_buffer));
                  }
                }
              }); 

        }
      });
}

void OefSearchClient::search_process_message_(pb::TransportHeader header, std::shared_ptr<Buffer> payload) {
  // get msg id
  uint32_t msg_id = header.id();
  logger.debug("::search_process_message processing message with header {} ", pbs::to_string(header)); 
  // get msg payload type and continuation
  auto msg_state = msg_handle_get(msg_id);
  msg_handle_rmv(msg_id);
  std::string msg_operation = msg_state.first;
  AgentSessionContinuation msg_continuation = msg_state.second;
  
  // answer to AgentSession
  std::error_code ec{};
  uint32_t length{0};
  std::vector<std::string> agents{};

  if(!header.status().success()) {
    logger.warn("::search_process_message_ answer to message {} unsuccessful", msg_id);
    ec = std::make_error_code(std::errc::no_message_available);
  }
  
  if(!payload) {
    logger.info("::search_process_message_ no payload received for message {} answer", msg_id);
    msg_continuation(ec, length, agents);
    return;
  }

  // get payload 
  if(msg_operation == "update") {
    auto update_resp = pbs::deserialize<pb::UpdateResponse>(*payload);
    logger.debug("::search_process_message_ received update confirmation for msg {} : {} ", msg_id, pbs::to_string(update_resp));
  } else 
  if(msg_operation == "remove") {
    auto remove_resp = pbs::deserialize<pb::RemoveResponse>(*payload);
    logger.debug("::search_process_message_ received remove confirmation for msg {} : {} ", msg_id, pbs::to_string(remove_resp));
  } else
  if(msg_operation == "search") {
    auto search_resp = pbs::deserialize<pb::SearchResponse>(*payload);
    logger.debug("::search_process_message_ received search results for msg {} : {} ", msg_id, pbs::to_string(search_resp));
    // get agents
    auto items = search_resp.result();
    for (auto& item : items) {
      auto agts = item.agents();
      for (auto& a : agts) {
        std::string key{*a.mutable_key()};
        agents.emplace_back(key);
      }
    }
  } else {
    logger.error("::search_process_message_ unknown operation '{}' for message {} answer", msg_operation, msg_id);
  }

  msg_continuation(ec, length, agents);
}

/*
 * *********************************
 * Synchronous helper functions 
 * *********************************
*/


std::error_code OefSearchClient::search_send_sync_(std::shared_ptr<Buffer> header, std::shared_ptr<Buffer> payload) {
  // check lib/proto/search_transport.proto for Oef Search communication protocol
  logger.debug("search_send_sync_ preparing to send {} and {}", header->size(), payload->size());
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
  nbytes.emplace_back(header->size());
  nbytes.emplace_back(payload->size());

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
  logger.debug("search_receive_sync_ received header size {}", header_size);
  ec = comm_->receive_sync(&payload_size, sizeof(uint32_t));
  if (ec) {
    logger.error("search_receive_sync_ Error while receiving payload length : {}", ec.value());
    return ec;
  }
  payload_size = ntohl(payload_size);
  logger.debug("search_receive_sync_ received payload size {}", payload_size);
  
  // then, receive actual data (the search message)
  // receive the header
  if (!header_size) {
    logger.debug("search_receive_sync_ No header expected");
    return std::make_error_code(std::errc::no_message_available); 
  }
  std::unique_ptr<Buffer> header_buffer = std::make_unique<Buffer>(header_size); 
  ec = comm_->receive_sync(header_buffer->data(), header_size);
  if (ec) {
    logger.error("search_receive_sync_ Error while receiving header : {}", ec.value());
    return ec;
  }
  
  header = pbs::deserialize<pb::TransportHeader>(*header_buffer);
  logger.debug("search_receive_sync_ received header {}", pbs::to_string(header));
  
  // receive the payload
  if (!payload_size) {
    logger.debug("search_receive_sync_ No payload expected");
    payload = nullptr;
    return ec; 
  }
  payload = std::make_shared<Buffer>(payload_size);
    logger.debug("search_receive_sync_ preparing to receive {} bytes of payload", payload_size);
  ec = comm_->receive_sync(payload->data(), payload_size);
  if (ec) {
    logger.error("search_receive_sync_ Error while receiving payload : {}", ec.value());
    return ec;
  }
  return ec;
}

/*
 * *********************************
 * Messages factories 
 * *********************************
*/


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

pb::SearchQuery OefSearchClient::generate_search_(const QueryModel& query, const std::string& agent, uint32_t msg_id) {
  pb::SearchQuery search_query;
  search_query.set_source_key(core_id_);
  // remove old core constraints
  pb::Query_Model query_no_cnstrs;
  query_no_cnstrs.mutable_model()->CopyFrom(query.handle().model());
  //
  search_query.mutable_model()->CopyFrom(query_no_cnstrs);
  search_query.set_ttl(1);
  return search_query;
}

pb::Remove OefSearchClient::generate_remove_(const Instance& instance, const std::string& agent, uint32_t msg_id) {
  pb::Remove remove;
  remove.set_key(core_id_);
  remove.set_all(false);
  remove.add_data_models()->CopyFrom(instance.model());
  return remove;
}

} //oef
} //fetch
