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

#include <iostream>
#include "core_server.hpp"

int main(int argc, char* argv[])
{
  spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [thread %t] [%n] [%l] %v");
  spdlog::set_level(spdlog::level::level_enum::trace);
  try
  {
    if (argc != 6)
    {
      std::cerr << "Usage: node\n";
      std::cerr << "Usage: node <core_key> <core_ip> <core_port> <search_ip> <search_port>\n";
      return 1;
    }

    fetch::oef::CoreServer s(argv[1], argv[2], std::stoi(argv[3]), argv[4], std::stoi(argv[5]));
    s.run_in_thread();

  } catch (std::exception& e)
  {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}
