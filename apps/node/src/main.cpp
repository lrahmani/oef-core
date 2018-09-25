#include <iostream>
#include "server.h"

int main(int argc, char* argv[])
{
  spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [thread %t] [%n] [%l] %v");
  spdlog::set_level(spdlog::level::level_enum::trace);
  try
  {
    if (argc != 1)
    {
      std::cerr << "Usage: node\n";
      return 1;
    }

    fetch::oef::Server s;
    s.run_in_thread();

  } catch (std::exception& e)
  {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}
