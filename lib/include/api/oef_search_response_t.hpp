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

#include "agent.pb.h" // TOFIX

#include <vector>
#include <string>

namespace fetch {
namespace oef {
    
    enum class kOefSearchResponse {
      NOTHING,
      L_SEARCH_AGTS,
      W_SEARCH_AGTS
    };

    struct OefSearchResponse {
      //union {
      std::vector<std::string> agents;
      pb::Server_SearchResultWide search_result_wide;
      //}; // TOFIX unmaed union make lambda conversion to std::function fails ...
      
      kOefSearchResponse type;
      
      explicit OefSearchResponse() : type{kOefSearchResponse::NOTHING} {}
      explicit OefSearchResponse(std::vector<std::string> agts) 
        : agents{agts}, type{kOefSearchResponse::L_SEARCH_AGTS} {}
      explicit OefSearchResponse(pb::Server_SearchResultWide wsr) 
        : search_result_wide{wsr}, type{kOefSearchResponse::W_SEARCH_AGTS} {}
    };

} // oef
} // fetch

