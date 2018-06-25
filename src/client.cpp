#include <iostream>
#include <memory>
#include <string>
#include "common.h"

#include "client.h"
  
int main(int argc, char** argv) {
  fetch::oef::Client client("Agent1", "localhost", [](std::unique_ptr<Conversation>){});
  return 0;
}
