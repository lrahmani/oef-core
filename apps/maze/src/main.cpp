// Company: FETCH.ai
// Author: Jerome Maloberti
// Creation: 09/01/18
//
#include <iostream>
#include "clara.hpp"
#include "maze.hpp"

int main(int argc, char* argv[])
{
  bool showHelp = false;
  std::string host;
  uint32_t nbRows = 100, nbCols = 100;
  
  auto parser = clara::Help(showHelp)
    | clara::Arg(host, "host")("Host address to connect.")
    | clara::Opt(nbRows, "rows")["--rows"]["-r"]("Number of rows in the mazes.")
    | clara::Opt(nbCols, "cols")["--cols"]["-c"]("Number of columns in the mazes.");

  auto result = parser.parse(clara::Args(argc, argv));
  if(!result || showHelp || argc == 1)
    std::cout << parser << std::endl;
  else
  {
    IoContextPool pool(1);
    pool.run();
    Maze maze(pool.getIoContext(), "Maze_1", host, nbRows, nbCols);
    pool.join();
  }

  return 0;
}
