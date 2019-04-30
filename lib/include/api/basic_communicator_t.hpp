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
     * Defines API for a Basic Communicator object.
     * Communicator establishes a bi-directional communication channel between two participant, on 
     * the same machine or accross a network, depending on its implementation.
     * Communicator implements primitive send and receive operations, both synchronous and asynchronous.
     * Data is sent and received as pointer to void with number of bytes to send/received.
     * Important: No protocol is defined.
     */ 
    class basic_communicator_t {
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
        
        /* TOFIX add constness */ 
        
        virtual std::error_code send_sync(const void* buffer, std::size_t nbytes) = 0; 
        virtual std::error_code send_sync(std::vector<void*> buffers, std::vector<std::size_t> nbytes) = 0;
        
        /* Receive data synchronously through Communicator. Will block until all data has been received or an error occured
         * params: 
         *   - [overload1][out] buffer: received serialized message 
         * return: error_code for error or success : if(ec) error; if(!ec) success */
        virtual std::error_code receive_sync(void* buffer, const std::size_t& nbytes ) = 0;
        
        /* Send data asynchronously. Will not block.
         * params:
         *   - [in] buffer: serialized message to be sent
         *   - [in] continuation: callback function to handle successful transmission, or errors 
         *   - [overload][in] buffers: a grouped send */
        virtual void send_async(std::vector<std::shared_ptr<Buffer>> buffers, std::vector<std::size_t> nbytes, 
                                LengthContinuation continuation) = 0;
        virtual void send_async(std::shared_ptr<Buffer> buffer, std::size_t nbytes, LengthContinuation continuation) = 0;
        //virtual void send_async(std::shared_ptr<void> buffer, std::size_t nbytes) = 0;
        
        /* Receive data asynchronously. Will not block.
         * params:
         *   - [in] continuation: callback function to handle received data, or errors */
        virtual void receive_async(std::size_t nbytes, BufferContinuation continuation) = 0;
        
        virtual ~basic_communicator_t() {}
    };
} // oef
} // fetch

