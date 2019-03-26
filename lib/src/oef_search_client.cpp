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
#include "search.pb.h"


namespace fetch {
namespace oef {
    
  fetch::oef::Logger OefSearchClient::logger = fetch::oef::Logger("oef-search-client");
    /*
    void OefSearchClient::connect(tcp::resolver::iterator endpoint_iterator)
    {
      asio::async_connect(*socket_, endpoint_iterator, [](const asio::error_code& error,
                                                          asio::ip::tcp::resolver::iterator iterator){
        if (error){
          std::cerr << "Failed to connect to Search Engine! " << error.message();
          return;
        }
        std::cout << "Established connection with " << iterator->endpoint() << std::endl;
      });
    }

    void OefSearchClient::RegisterServiceDescription(const std::string &agent, const Instance &instance)
    {
      std::vector<asio::const_buffer> buffers;
      std::string cmd = "update";
      uint32_t cmd_size = static_cast<uint32_t>(cmd.size()+1);
      int32_t len = -static_cast<int32_t>(cmd_size);
      buffers.emplace_back(asio::buffer(&len, sizeof(len)));
      buffers.emplace_back(asio::buffer(&cmd, cmd_size));

      fetch::oef::pb::Update update;
      update.set_key(core_id_);

      fetch::oef::pb::Update_DataModelInstance* dm = update.add_data_models();

      dm->set_key(agent.c_str());
      dm->mutable_model()->CopyFrom(instance.model());

      addNetworkAddress(update);

      auto data = serializer::serialize(update);

      uint32_t data_len = static_cast<uint32_t>(data->size());
      buffers.emplace_back(asio::buffer(&data_len, sizeof(data_len)));
      buffers.emplace_back(asio::buffer(data->data(), data_len));
      uint32_t total = -len+sizeof(len)+data_len+sizeof(data_len);
      asio::async_write(*socket_, buffers, [total,data](std::error_code ec, std::size_t length) {
        if(ec) {
          std::cerr << "Grouped Async write error, wrote " << length << " expected " << total << std::endl;
        }
      });
    }
    */

  void OefSearchClient::register_description_sync(const std::string& agent, const Instance& desc) {};
  void OefSearchClient::unregister_description_sync(const std::string& agent) {};
  void OefSearchClient::register_service_sync(const std::string& agent, const Instance& service) {};
  void OefSearchClient::unregister_service_sync(const std::string& agent, const Instance& service) {};
  std::vector<agent_t> OefSearchClient::search_agents_sync(const std::string& agent, const QueryModel& query) {
    return std::vector<agent_t>();
  }
  std::vector<agent_t> OefSearchClient::search_service_sync(const std::string& agent, const QueryModel& query) {
    return std::vector<agent_t>();
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
