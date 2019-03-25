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

#include <memory>
#include <vector>
#include "types/agent_session_t.hpp"
#include "schema.hpp" // TOFIX

namespace fetch {
  namespace oef {
    class agent_directory_t {
    public:
      //
      virtual bool add(const std::string& agent_id, std::shared_ptr<agent_session_t>) = 0;
      virtual bool exist(const std::string& agent_id) const = 0;
      virtual bool remove(const std::string& agent_id) = 0;
      //
      virtual std::shared_ptr<agent_session_t> session(const std::string& agent_id) const = 0; // not sure about this operation
      virtual size_t size() const = 0;
      virtual void clear() = 0; // not sure about this operation
      virtual const std::vector<std::string> search(const QueryModel&) const = 0; // change the name to query // TOFIX not needed anymore
      //
      virtual ~agent_directory_t() {}
    };
  } // oef
} // fetch

