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

#include "serialization.hpp"

namespace fetch {
namespace oef {
namespace pbs {

std::string to_string(const google::protobuf::Message &msg) {
  std::string output;
  google::protobuf::TextFormat::PrintToString(msg, &output);
  return output;
}

std::string diagnostic(void *p, size_t sz)
{
  // from oef-search-pluto/cpp-sdk/src/cpp/char_array_buffer.hpp
  std::stringstream output;
  for(unsigned int i=0;i<sz;i++)
  {
    char c = ((char*)(p))[i];

    switch(c)
    {
      case 10:
        std::cout << "\\n";
        break;
      default:
        if (::isprint(c))
        {
          output << c;
        }
        else
        {
          int cc = (unsigned char)c;
          output << "\\x";
          for(int j=0;j<2;j++)
          {
            output << "0123456789ABCDEF"[((cc&0xF0) >> 4)];
            cc <<= 4;
          }
        }
    }
  }
  //output << std::endl;
  return output.str();
}
} // pbs 

std::shared_ptr<Buffer> serialize(uint32_t size) {
  uint8_t* addr = (uint8_t*) &size;
  Buffer data(addr,addr+sizeof(uint32_t));
  return std::make_shared<Buffer>(data);
}

} //oef
} //fetch

