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
#include <functional>

namespace fetch {
namespace oef {
    
fetch::oef::Logger OefSearchClient::logger = fetch::oef::Logger("oef-search-client");


/*
 * *********************************
 * Oef operations 
 * *********************************
*/


void OefSearchClient::register_description(const Instance& desc, 
    const std::string& agent, uint32_t msg_id, AgentSessionContinuation continuation) 
{
  logger.warn("::register_description implemented using ::register_service");
  register_service(desc, agent, msg_id, continuation);
}

void OefSearchClient::unregister_description(const Instance& desc, 
    const std::string& agent, uint32_t msg_id, AgentSessionContinuation continuation) 
{
  logger.warn("::unregister_description implemented using ::unregister_service"); 
  unregister_service(desc, agent, msg_id, continuation);
}

void OefSearchClient::search_agents(const QueryModel& query, 
    const std::string& agent, uint32_t msg_id, AgentSessionContinuation continuation) 
{
  logger.warn("::search_agents implemented using ::search_service"); 
  search_service(query, agent, msg_id, continuation);
}

void OefSearchClient::register_service(const Instance& service, 
    const std::string& agent, uint32_t msg_id, AgentSessionContinuation continuation) 
{
  // prepare header
  size_t smsg_id = generate_smsg_id_(agent, msg_id);
  auto header = generate_header_("update", smsg_id);
  auto header_buffer = pbs::serialize(header);

  // then prepare payload message
  auto update = generate_update_(service, agent);
  auto update_buffer = pbs::serialize(update);
 
  // send message
  logger.debug("::register_service sending update from agent {} to OefSearch: {} - {}", 
        agent, pbs::to_string(header), pbs::to_string(update));
  
  send_(header_buffer, update_buffer, 
      [this,agent,smsg_id,msg_id,continuation](std::error_code ec, uint32_t length) {
        if (ec) {
          logger.debug("::register_service error while sending update from agent {} to OefSearch: {}",
              agent, ec.value());
          continuation(ec, OefSearchResponse{});          
        } else {
          // schedule reception of answer
          logger.debug("::register_service update message sent to OefSearch");
          schedule_rcv_callback_(smsg_id, "update", continuation, msg_id, agent);
        }
      });
}

void OefSearchClient::unregister_service(const Instance& service, 
    const std::string& agent, uint32_t msg_id, AgentSessionContinuation continuation) 
{
  // prepare header
  size_t smsg_id = generate_smsg_id_(agent, msg_id);
  auto header = generate_header_("remove", smsg_id);
  auto header_buffer = pbs::serialize(header);

  // then prepare payload message
  auto remove = generate_remove_(service);
  auto remove_buffer = pbs::serialize(remove);
  
  // send message
  logger.debug("::unregister_service sending remove from agent {} to OefSearch: {} - {}", 
        agent, pbs::to_string(header), pbs::to_string(remove));
  
  send_(header_buffer, remove_buffer, 
      [this,agent,smsg_id,msg_id,continuation](std::error_code ec, uint32_t length) {
        if (ec) {
          logger.debug("::unregister_service error while sending remove from agent {} to OefSearch: {}",
              agent, ec.value());
          continuation(ec, OefSearchResponse{});          
        } else {
          // schedule reception of answer
          logger.debug("::unregister_service remove message sent to OefSearch");
          schedule_rcv_callback_(smsg_id, "remove", continuation, msg_id, agent);
        }
      });
}

void OefSearchClient::search_service(const QueryModel& query, 
    const std::string& agent, uint32_t msg_id, AgentSessionContinuation continuation) 
{
  // prepare header
  size_t smsg_id = generate_smsg_id_(agent, msg_id);
  auto header = generate_header_("search", smsg_id);
  auto header_buffer = pbs::serialize(header);

  // then prepare payload message
  auto search = generate_search_(query, 1);
  auto search_buffer = pbs::serialize(search);
  
  // send message
  logger.debug("::search_service sending search from agent {} to OefSearch: {} - {}", 
        agent, pbs::to_string(header), pbs::to_string(search));
  
  send_(header_buffer, search_buffer, 
      [this,agent,smsg_id,msg_id,continuation](std::error_code ec, uint32_t length) {
        if (ec) {
          logger.debug("::search_service error while sending search from agent {} to OefSearch: {}",
              agent, ec.value());
          continuation(ec, OefSearchResponse{});          
        } else {
          // schedule reception of answer
          logger.debug("::search_service search message sent to OefSearch");
          schedule_rcv_callback_(smsg_id, "search-local", continuation, msg_id, agent);
        }
      });
}

void OefSearchClient::search_service_wide(const QueryModel& query, 
    const std::string& agent, uint32_t msg_id, AgentSessionContinuation continuation) 
{
  // prepare header
  size_t smsg_id = generate_smsg_id_(agent, msg_id);
  auto header = generate_header_("search", smsg_id);
  auto header_buffer = pbs::serialize(header);

  // then prepare payload message
  auto search = generate_search_(query, 3);
  auto search_buffer = pbs::serialize(search);
  
  // send message
  logger.debug("::search_service_wide sending search from agent {} to OefSearch: {} - {}", 
        agent, pbs::to_string(header), pbs::to_string(search));
  
  send_(header_buffer, search_buffer, 
      [this,agent,smsg_id,msg_id,continuation](std::error_code ec, uint32_t length) {
        if (ec) {
          logger.debug("::search_service_wide error while sending search from agent {} to OefSearch: {}",
              agent, ec.value());
          continuation(ec, OefSearchResponse{});          
        } else {
          // schedule reception of answer
          logger.debug("::search_service_wide search message sent to OefSearch");
          schedule_rcv_callback_(smsg_id, "search-wide", continuation, msg_id, agent);
        }
      });
}
  

/*
 * *********************************
 * Asynchronous helper functions
 * *********************************
*/


void OefSearchClient::send_(std::shared_ptr<Buffer> header, std::shared_ptr<Buffer> payload,
          LengthContinuation continuation) 
{
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
  buffers.emplace_back(header); 
  buffers.emplace_back(payload);
  nbytes.emplace_back(header->size());
  nbytes.emplace_back(payload->size());

  // send message
  comm_->send_async(buffers, nbytes, continuation);
}

void OefSearchClient::schedule_rcv_callback_(uint32_t smsg_id, std::string operation, 
    AgentSessionContinuation continuation, uint32_t amsg_id, const std::string& agent) 
{
  msg_handle_save(smsg_id, MsgHandle{operation, continuation, amsg_id, agent});
  logger.debug("schedule_rcv_callback_ scheduled receive for message {} (aka {})", 
      smsg_id, amsg_id);
}

void OefSearchClient::receive_(
    std::function<void(std::error_code,pb::TransportHeader,std::shared_ptr<Buffer>)> continuation)
{
  // first, receive sizes
  comm_->receive_async(2*sizeof(uint32_t),
      [this,continuation](std::error_code ec, std::shared_ptr<Buffer> buffer){
        if (ec) {
          logger.error("receive_ Error while receiving header and payload lengths : {}", ec.value());
          continuation(ec, pb::TransportHeader{}, nullptr);
        } else {
          uint32_t* sizes = (uint32_t*)buffer->data();
          uint32_t header_size = ntohl(sizes[0]);
          uint32_t payload_size = ntohl(sizes[1]);
          logger.debug("receive_ received lenghts : {} - {}", header_size, payload_size);
          // then receive the actual message
          std::unique_ptr<Buffer> data_buffer = std::make_unique<Buffer>(header_size+payload_size);
          ec = comm_->receive_sync(data_buffer->data(), header_size+payload_size);
          if (ec) {
            logger.error("receive_ Error while receiving header and payload : {}", ec.value());
            logger.error("receive_ message discarded"); // TOFIX don't know which msg it was supposed to answer
          } else {
            uint8_t* data_ptr = (uint8_t*)data_buffer->data();
            std::vector<uint8_t> header_buffer(data_ptr, data_ptr+header_size);
            std::vector<uint8_t> payload_buffer(data_ptr+header_size, data_ptr+header_size+payload_size);
            // get header
            bool hstatus = false;
            logger.error("receive_ received data (serialized) : header {} - payload {}", 
                pbs::diagnostic(header_buffer.data(), header_buffer.size()),
                pbs::diagnostic(payload_buffer.data(), payload_buffer.size()));
            pb::TransportHeader header = pbs::deserialize<pb::TransportHeader>(header_buffer, hstatus);
            if(!hstatus) {
              logger.error("::receive__ failed to deserialize header, message discarded "); // TOFIX don't know which msg it was supposed to answer 
              return;
            }
            if(!payload_size) {
              continuation(ec,header,nullptr);  
            } else {
              continuation(ec, header, std::make_shared<Buffer>(payload_buffer));
            }
          }
        }
      });
}

void OefSearchClient::process_message_(pb::TransportHeader header, std::shared_ptr<Buffer> payload) 
{
  // get msg id
  uint32_t smsg_id = header.id();
  logger.debug("::search_process_message processing message with header {} ", pbs::to_string(header)); 
  // get msg payload type and continuation
  auto msg_handle = msg_handle_get(smsg_id);
  msg_handle_erase(smsg_id);
  uint32_t amsg_id = msg_handle.amsg_id;
  std::string msg_operation = msg_handle.operation;
  AgentSessionContinuation msg_continuation = msg_handle.continuation;
  
  // answer to AgentSession
  std::error_code ec{};

  if(!header.status().success()) {
    logger.warn("::process_message_ answer to message {} (aka {}) unsuccessful", smsg_id, amsg_id);
    ec = std::make_error_code(std::errc::no_message_available);
  }
  
  if(!payload) {
    logger.info("::process_message_ no payload received for message {} (aka {}) answer", smsg_id, amsg_id);
    msg_continuation(ec, OefSearchResponse{});
    return;
  }

  // get payload 
  if(msg_operation == "update") {
    auto update_resp = pbs::deserialize<pb::UpdateResponse>(*payload);
    logger.debug("::process_message_ received update confirmation for msg {} (aka {})  : {} ",
        smsg_id, amsg_id, pbs::to_string(update_resp));
    msg_continuation(ec, OefSearchResponse{});
    return;
  } else

  if(msg_operation == "remove") {
    auto remove_resp = pbs::deserialize<pb::RemoveResponse>(*payload);
    logger.debug("::process_message_ received remove confirmation for msg {} (aka {}) : {} ", 
        smsg_id, amsg_id, pbs::to_string(remove_resp));
    msg_continuation(ec, OefSearchResponse{});
    return;
  } else
  
  if(msg_operation == "search-local") {
    auto search_resp = pbs::deserialize<pb::SearchResponse>(*payload);
    logger.debug("::process_message_ received local search results for msg {} (aka {}) : {} ", 
        smsg_id, amsg_id, pbs::to_string(search_resp));
    // get agents
    std::vector<std::string> agents{};
    auto items = search_resp.result();
    for (auto& item : items) {
      auto agts = item.agents();
      for (auto& a : agts) {
        std::string key{a.key()};
        agents.emplace_back(key);
      }
    }
    msg_continuation(ec, OefSearchResponse{agents});
    return;
  } else
  
  if(msg_operation == "search-wide") {
    auto search_resp = pbs::deserialize<pb::SearchResponse>(*payload);
    logger.debug("::process_message_ received wide search results for msg {} (aka {}) : {} ", 
        smsg_id, amsg_id, pbs::to_string(search_resp));
    // get SearchResultWide
    pb::Server_SearchResultWide agents_wide;
    auto items = search_resp.result();
    for (auto& item : items) {
      auto* aw_item = agents_wide.add_result();
      aw_item->set_key(item.key());
      aw_item->set_ip(item.ip());
      aw_item->set_score(item.score());
      aw_item->set_info(item.info());
      aw_item->set_distance(item.distance());
      auto agts = item.agents();
      for (auto& a : agts) {
        auto *aw = aw_item->add_agents();
        aw->set_key(a.key());
      }
    }
    msg_continuation(ec, OefSearchResponse{agents_wide});
    return;
  } 
  
  else {
    logger.error("::process_message_ unknown operation '{}' for message {} (aka {}) answer", 
        msg_operation, smsg_id, amsg_id);
    ec = std::make_error_code(std::errc::no_message_available);
  }

  msg_continuation(ec, OefSearchResponse{});
}


/*
 * *********************************
 * Messages factories 
 * *********************************
*/


void OefSearchClient::generate_update_add_naddr_(fetch::oef::pb::Update &update)
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

std::size_t OefSearchClient::generate_smsg_id_(const std::string& agent, uint32_t msg_id) {
  return std::hash<std::string>{}(agent) ^ (msg_id << 1); 
  // TOFIX what is the best way (less collision) to combine to integers?
  // cppreference: or use boost::hash_combine (see Discussion)
  // Discussion: https://en.cppreference.com/w/Talk:cpp/utility/hash
}
  
pb::TransportHeader OefSearchClient::generate_header_(const std::string& uri, uint32_t msg_id) {
  pb::TransportHeader header;
  header.set_uri(uri);
  header.set_id(msg_id);
  header.mutable_status()->set_success(true);
  return header;
}

pb::Update OefSearchClient::generate_update_(const Instance& service, const std::string& agent) {
  fetch::oef::pb::Update update;
  update.set_key(core_id_);

  fetch::oef::pb::Update_DataModelInstance* dm = update.add_data_models();
  dm->set_key(agent.c_str());
  dm->mutable_model()->CopyFrom(service.model());

  generate_update_add_naddr_(update);
  return update;
}

pb::SearchQuery OefSearchClient::generate_search_(const QueryModel& query, uint32_t ttl) {
  pb::SearchQuery search_query;
  search_query.set_source_key(core_id_);
  // remove old core constraints
  pb::Query_Model query_no_cnstrs;
  query_no_cnstrs.mutable_model()->CopyFrom(query.handle().model());
  //
  search_query.mutable_model()->CopyFrom(query_no_cnstrs);
  search_query.set_ttl(ttl);
  return search_query;
}

pb::Remove OefSearchClient::generate_remove_(const Instance& instance) {
  pb::Remove remove;
  remove.set_key(core_id_);
  remove.set_all(false);
  remove.add_data_models()->CopyFrom(instance.model());
  return remove;
}

} //oef
} //fetch
