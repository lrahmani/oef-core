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

#include "api/buffer_t.hpp"

#include <memory>
#include <functional>
#include <system_error>

namespace fetch {
namespace oef {
    class communicator_t {
    public:
        //
        virtual void connect() = 0;
        virtual void disconnect() = 0;
        //
        virtual std::error_code send_sync(std::shared_ptr<Buffer> buffer) = 0;
        virtual std::error_code send_sync(std::vector<std::shared_ptr<Buffer>> buffers) = 0;
        virtual std::error_code receive_sync(std::shared_ptr<Buffer>& buffer) = 0;
        //
        virtual void send_async(std::shared_ptr<Buffer> buffer) = 0;
        virtual void send_async(std::shared_ptr<Buffer> buffer,
                                std::function<void(std::error_code,std::size_t)> continuation) = 0;
        virtual void receive_async(std::function<void(std::error_code,std::shared_ptr<Buffer>)> continuation) = 0;
        //
        virtual ~communicator_t() {}
    };
} // oef
} // fetch

