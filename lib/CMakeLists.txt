################################################################################
# F E T C H   O E F   C O R E   P L U T O   L I B R A R Y
################################################################################
find_package(Protobuf REQUIRED)

PROTOBUF_GENERATE_CPP(PROTO_SRC PROTO_HEADER proto/query.proto proto/fipa.proto proto/agent.proto 
  proto/search_message.proto proto/search_query.proto proto/search_remove.proto 
  proto/search_response.proto proto/search_update.proto proto/search_transport.proto)
set (PROTOBUF_LIBRARIES  ${PROTOBUF_LIBRARIES} PARENT_SCOPE)

add_library(proto OBJECT ${PROTO_HEADER} ${PROTO_SRC})
# CMake build : library

#configure variables
set (LIB_NAME "${PROJECT_NAME}")
set (LIB_CONFIG "${LIB_NAME}LibConfig")

#configure directories
set (LIBRARY_MODULE_PATH "${PROJECT_SOURCE_DIR}/lib")
set (LIBRARY_SRC_PATH  "${LIBRARY_MODULE_PATH}/src" )
set (LIBRARY_INCLUDE_PATH  "${LIBRARY_MODULE_PATH}/include")

#set includes
include_directories (${LIBRARY_INCLUDE_PATH} ${THIRD_PARTY_INCLUDE_PATH} ${PROTOBUF_INCLUDE_DIRS})


#set sources
file (GLOB LIB_HEADER_FILES "${LIBRARY_INCLUDE_PATH}/*.hpp")
file (GLOB LIB_SOURCE_FILES "${LIBRARY_SRC_PATH}/*.cpp")

#set library
add_library (${LIB_NAME} STATIC $<TARGET_OBJECTS:proto> ${LIB_SOURCE_FILES} ${LIB_HEADER_FILES})
add_dependencies("${LIB_NAME}" proto)

target_include_directories(${LIB_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>  
    PRIVATE src)

#export vars
set (LIBRARY_INCLUDE_PATH  ${LIBRARY_INCLUDE_PATH} PARENT_SCOPE)
set (LIB_NAME ${LIB_NAME} PARENT_SCOPE)

install(TARGETS ${LIB_NAME} EXPORT ${LIB_CONFIG}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT library
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT library
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT library
)
set (PROTO_HEADER_FILES "${CMAKE_CURRENT_BINARY_DIR}/agent.pb.h"
    "${CMAKE_CURRENT_BINARY_DIR}/fipa.pb.h" "${CMAKE_CURRENT_BINARY_DIR}/query.pb.h"
    "${CMAKE_CURRENT_BINARY_DIR}/search_message.pb.h" "${CMAKE_CURRENT_BINARY_DIR}/search_query.pb.h"
    "${CMAKE_CURRENT_BINARY_DIR}/search_remove.pb.h" "${CMAKE_CURRENT_BINARY_DIR}/search_response.pb.h"
    "${CMAKE_CURRENT_BINARY_DIR}/search_update.pb.h" "${CMAKE_CURRENT_BINARY_DIR}/search_transport.pb.h")
message("Proto file ${PROTO_HEADER_FILES}")
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${LIB_NAME})
#install(DIRECTORY proto DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${LIB_NAME})
install(FILES ${PROTO_HEADER_FILES} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${LIB_NAME})
install(EXPORT ${LIB_CONFIG} DESTINATION cmake)

# this makes the project importable from the build directory.
export(TARGETS ${LIB_NAME} FILE "${LIB_CONFIG}.cmake")

#test
enable_testing ()
add_subdirectory (test)
add_subdirectory (benchmark)
