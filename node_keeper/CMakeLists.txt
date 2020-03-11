cmake_minimum_required(VERSION 3.10)
project(node_keeper)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

aux_source_directory(src ALL_SOURCES)
aux_source_directory(src/gossip GOSSIP_SOURCES)
aux_source_directory(protobuf PROTOBUF_SOURCES)
list(APPEND ALL_SOURCES ${GOSSIP_SOURCES} ${PROTOBUF_SOURCES})

find_package(asio REQUIRED)
find_package(Protobuf CONFIG REQUIRED)
find_package(protoc CONFIG REQUIRED)
find_package(gRPC CONFIG REQUIRED)

include_directories(${PROTOBUF_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_BINARY_DIR})
set(PROTO_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(PROTOBUF_IMPORT_DIRS ${PROTO_ROOT_DIR})

# There's no simple way to generate code for gRPC from protobuf AFAIK, following block is to do that.
set(PB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")
set(NODE_KEEPER_PROTO_SRCS "${PB_DIR}/node_keeper.pb.cc")
set(NODE_KEEPER_PROTO_HDRS "${PB_DIR}/node_keeper.pb.h")
set(NODE_KEEPER_GRPC_SRCS "${PB_DIR}/node_keeper.grpc.pb.cc")
set(NODE_KEEPER_GRPC_HDRS "${PB_DIR}/node_keeper.grpc.pb.h")
add_custom_command(
      OUTPUT "${NODE_KEEPER_PROTO_SRCS}" "${NODE_KEEPER_PROTO_HDRS}" "${NODE_KEEPER_GRPC_SRCS}" "${NODE_KEEPER_GRPC_HDRS}"
      COMMAND $<TARGET_FILE:protobuf::protoc>
      ARGS --grpc_out "${PB_DIR}"
          --cpp_out "${PB_DIR}"
          -I "${CMAKE_CURRENT_SOURCE_DIR}/protobuf"
          --plugin=protoc-gen-grpc=$<TARGET_FILE:gRPC::grpc_cpp_plugin>
          "${CMAKE_CURRENT_SOURCE_DIR}/protobuf/node_keeper.proto"
      DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/protobuf/node_keeper.proto")
list(APPEND ALL_SOURCES ${NODE_KEEPER_PROTO_SRCS} ${NODE_KEEPER_GRPC_SRCS})

set(EXE_SOURCES ${ALL_SOURCES})

list(FILTER EXE_SOURCES EXCLUDE REGEX "^.*_test.cc$")
add_executable(${PROJECT_NAME} ${EXE_SOURCES} ${NODE_KEEPER_PROTO_SRCS} ${NODE_KEEPER_GRPC_SRCS})

target_include_directories(${PROJECT_NAME} PUBLIC include protobuf)
target_link_libraries(${PROJECT_NAME} ${CONAN_LIBS} config_manager)

set(TEST_SOURCES ${ALL_SOURCES})
list(FILTER TEST_SOURCES EXCLUDE REGEX "main.cc$")
if (NOT "${TEST_SOURCES}" STREQUAL "")
    add_executable(${PROJECT_NAME}_test ${TEST_SOURCES})
    find_package(GTest REQUIRED)
    target_link_libraries(${PROJECT_NAME}_test GTest::GTest asio::asio ${CONAN_LIBS} config_manager)
    add_test(NAME ${PROJECT_NAME}_test COMMAND ${PROJECT_NAME}_test)
    set_tests_properties(${PROJECT_NAME}_test PROPERTIES TIMEOUT 10)
endif ()
