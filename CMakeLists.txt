cmake_minimum_required(VERSION 3.2)

set(PROJECT_NAME OEFCore)
project(${PROJECT_NAME} VERSION 0.5 LANGUAGES CXX)
add_compile_options(-std=c++1y)

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(THREADS_PREFER_PTHREAD_FLAG ON)
#set(CMAKE_CXX_INCLUDE_WHAT_YOU_USE "/usr/bin/iwyu")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -DASIO_STANDALONE")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -DASIO_STANDALONE -fsanitize=address")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -DASIO_STANDALONE -fsanitize=leak")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -DASIO_STANDALONE -fsanitize=thread")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -DASIO_STANDALONE -fsanitize=undefined")

find_package(Threads REQUIRED)
include(GNUInstallDirs)

add_subdirectory(3rd EXCLUDE_FROM_ALL)
set(THIRD_PARTY_INCLUDE_PATH ${THIRD_PARTY_INCLUDE_PATH} PARENT_SCOPE)
include_directories(${THIRD_PARTY_INCLUDE_PATH})

#add_subdirectory(proto)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/lib/)
add_subdirectory(lib)
include_directories(lib/include)

message("PROTOBUF = ${PROTOBUF_LIBRARIES}")

add_subdirectory(apps)

enable_testing()
