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

#include "api/continuation_t.hpp"
#include "api/buffer_t.hpp"

#include <memory>

namespace fetch {
namespace oef {
    /* 
     * Defines API for a Communicator object.
     * Communicator establishes a bi-directional communication channel between two participant, on 
     * the same machine or accross a network, depending on its implementation.
     * Communicator implements primitive send and receive operations, both synchronous and asynchronous.
     * Data is sent and received as Buffer objects.
     * Important: Communication protocol imposes to: 
     *   - first, send the size of the data as a (serialized) uint32_t
     *   - then, send the actual data
     * Same protocol applies when receiving it.
     */ 
    class communicator_t {
    public:
        /* Establish a new connection */
        virtual void connect() = 0;
        
        /* Disconnect from the communication. Usually, this implies disconnecting Communicators at both ends */
        virtual void disconnect() = 0;
        
        /* Send data synchronously through Communicator. Will block until all data has been sent or an error occured
         * params: 
         *   - [overload1][in] buffer: serialized message to be sent
         *   - [overload2][in] buffers: a grouped send
         * return: error_code for error or success : if(ec) error; if(!ec) success */
        virtual std::error_code send_sync(std::shared_ptr<Buffer> buffer) = 0;
        virtual std::error_code send_sync(std::vector<std::shared_ptr<Buffer>> buffers) = 0;
        
        /* Receive data synchronously through Communicator. Will block until all data has been received or an error occured
         * params: 
         *   - [overload1][out] buffer: received serialized message 
         * return: error_code for error or success : if(ec) error; if(!ec) success */
        virtual std::error_code receive_sync(std::shared_ptr<Buffer>& buffer) = 0;
        
        /* Send data asynchronously. Will not block.
         * params:
         *   - [in] buffer: serialized message to be sent
         *   - [overload][in] continuation: callback function to handle successful transmission, or errors */
        virtual void send_async(std::shared_ptr<Buffer> buffer, LengthContinuation continuation) = 0;
        virtual void send_async(std::shared_ptr<Buffer> buffer) = 0;
        
        /* Receive data asynchronously. Will not black.
         * params:
         *   - [in] continuation: callback function to handle received data, or errors */
        virtual void receive_async(BufferContinuation continuation) = 0;
        
        virtual ~communicator_t() {}
    };
} // oef
} // fetch

