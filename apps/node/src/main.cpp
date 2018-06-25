#include <iostream>
#include "server.h"

int main(int argc, char* argv[])
{
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
