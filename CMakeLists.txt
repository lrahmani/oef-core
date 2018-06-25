cmake_minimum_required(VERSION 3.2)

set(PROJECT_NAME oef)
project(${PROJECT_NAME})
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

add_subdirectory(3rd EXCLUDE_FROM_ALL)
include_directories(${THIRD_PARTY_INCLUDE_PATH})
add_subdirectory(proto)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/proto)
add_subdirectory(lib)
include_directories(lib/include)
set(SRC_FILES src/main.cpp)

message("PROTOBUF = ${PROTOBUF_LIBRARIES}")
add_executable(test_proto ${SRC_FILES})
target_link_libraries(test_proto proto ${PROTOBUF_LIBRARIES} oefLib Threads::Threads)

set(SRC_SERVER src/server.cpp)
add_executable(test_server ${SRC_SERVER})
target_link_libraries(test_server oefLib ${PROTOBUF_LIBRARIES} proto Threads::Threads)

set(SRC_CLIENT src/client.cpp)
add_executable(test_client ${SRC_CLIENT})
target_link_libraries(test_client oefLib ${PROTOBUF_LIBRARIES} proto Threads::Threads)

add_subdirectory(apps)

enable_testing()
