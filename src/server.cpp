#include <iostream>
#include <memory>
#include <unordered_map>
#include <mutex>
#include <atomic>
#include <memory>
#include "server.h"

int main(int argc, char *argv[]) {
  GOOGLE_PROTOBUF_VERIFY_VERSION;
  fetch::oef::Server server;
  server.run_in_thread();
  return 0;
}
