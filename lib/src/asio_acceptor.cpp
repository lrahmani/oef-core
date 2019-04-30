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

#include "asio_acceptor.hpp"
#include "asio_communicator.hpp"

#include <iostream>

namespace fetch {
namespace oef {
    
  void AsioAcceptor::do_accept_async(
      CommunicatorContinuation continuation) {
    acceptor_.async_accept([continuation](std::error_code ec, tcp::socket socket) {
                               if (ec) {
                                 std::cerr << "AsioAcceptor::do_accept_async error " 
                                           << ec.value () << std::endl;
                               } else {
                                 continuation(ec,std::make_shared<AsioComm>(std::move(socket)));
                               }
                             });

  }
  std::string AsioAcceptor::local_address() {
    return acceptor_.local_endpoint().address().to_string();
  }
  
  uint32_t AsioAcceptor::local_port() {
    return acceptor_.local_endpoint().port();
  }

} // oef
} // fetch
