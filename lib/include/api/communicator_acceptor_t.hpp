#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch.AI Limited
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

#include "api/continuation_t.hpp"
#include "api/communicator_t.hpp"

#include <memory>
#include <functional>
#include <system_error>

namespace fetch {
namespace oef {
    /* 
     * Defines API for Communication Acceptor object.
     * Acceptor is a listening socket for new connections.
     * A Communicator object will be created for each successful new connection.
     */
    class comm_acceptor_t {
    public:
        /* Listen for new connections asynchronously 
         * params:
         *   - [in] continuation: callback function with the newly created Communicator, or error */
        virtual void do_accept_async(CommunicatorContinuation  continuation) = 0;
        
        virtual ~comm_acceptor_t() {}
    };
} // oef
} // fetch

