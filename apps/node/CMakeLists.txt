# Distributed under the MIT License (See accompanying file /LICENSE )

# CMake build : main application

#configure variables
set (APP_NAME "OEFNode")

#configure directories
set (APP_MODULE_PATH "${PROJECT_SOURCE_DIR}/apps/node")
set (APP_SRC_PATH  "${APP_MODULE_PATH}/src" )

#set includes
include_directories (${LIBRARY_INCLUDE_PATH} ${THIRD_PARTY_INCLUDE_PATH} ${APP_SRC_PATH})

#set target executable
add_executable (${APP_NAME} "${APP_SRC_PATH}/main.cpp")

#add the library
target_link_libraries (${APP_NAME} ${LIB_NAME} ${PROTOBUF_LIBRARIES} Threads::Threads)


