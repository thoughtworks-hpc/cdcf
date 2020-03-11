cmake_minimum_required(VERSION 3.10)
project(node_keeper)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

aux_source_directory(src ALL_SOURCES)
aux_source_directory(src/gossip GOSSIP_SOURCES)
list(APPEND ALL_SOURCES ${GOSSIP_SOURCES} protobuf/message.proto)

find_package(asio REQUIRED)
find_package(protobuf REQUIRED)
find_package(protoc REQUIRED)
find_package(grpc REQUIRED)

include_directories(${PROTOBUF_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_BINARY_DIR})

# There's no simple way to generate code for gRPC from protobuf AFAIK, following block is to do that.
set(PB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")
set(NODE_KEEPER_PROTO_SRCS "${PB_DIR}/node_keeper.pb.cc")
set(NODE_KEEPER_PROTO_HDRS "${PB_DIR}/node_keeper.pb.h")
set(NODE_KEEPER_GRPC_SRCS "${PB_DIR}/node_keeper.grpc.pb.cc")
set(NODE_KEEPER_GRPC_HDRS "${PB_DIR}/node_keeper.grpc.pb.h")
# Conan generated gRPC does not export plugin target, should set it manually, looks for gRPCConfig.cmake file installed by
# gRPC's cmake installation is alternative.
set(GRPC_CPP_PLUGIN ${grpc_LIB_DIRS}/../bin/grpc_cpp_plugin)
message("GRPC_CPP_PLUGIN: ${GRPC_CPP_PLUGIN}")
add_custom_command(
      OUTPUT "${NODE_KEEPER_PROTO_SRCS}" "${NODE_KEEPER_PROTO_HDRS}" "${NODE_KEEPER_GRPC_SRCS}" "${NODE_KEEPER_GRPC_HDRS}"
      COMMAND $<TARGET_FILE:protobuf::protoc>
      ARGS --grpc_out "${PB_DIR}"
          --cpp_out "${PB_DIR}"
          -I "${CMAKE_CURRENT_SOURCE_DIR}/protobuf"
          --plugin=protoc-gen-grpc=${GRPC_CPP_PLUGIN}
          "${CMAKE_CURRENT_SOURCE_DIR}/protobuf/node_keeper.proto"
      DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/protobuf/node_keeper.proto")
list(APPEND ALL_SOURCES ${NODE_KEEPER_PROTO_SRCS} ${NODE_KEEPER_GRPC_SRCS})

set(EXE_SOURCES ${ALL_SOURCES})
list(FILTER EXE_SOURCES EXCLUDE REGEX "^.*_test.cc$")
add_executable(${PROJECT_NAME} ${EXE_SOURCES} ${NODE_KEEPER_PROTO_SRCS} ${NODE_KEEPER_GRPC_SRCS})

target_link_libraries(${PROJECT_NAME} ${CONAN_LIBS} asio::asio protobuf::protobuf grpc::grpc config_manager)
protobuf_generate(LANGUAGE cpp TARGET ${PROJECT_NAME})

set(TEST_SOURCES ${ALL_SOURCES})
list(FILTER TEST_SOURCES EXCLUDE REGEX "main.cc$")
if (NOT "${TEST_SOURCES}" STREQUAL "")
    add_executable(${PROJECT_NAME}_test ${TEST_SOURCES})
    find_package(GTest REQUIRED)
    target_link_libraries(${PROJECT_NAME}_test GTest::GTest asio::asio protobuf::protobuf grpc::grpc config_manager)
    protobuf_generate(LANGUAGE cpp TARGET ${PROJECT_NAME}_test)
    add_test(NAME ${PROJECT_NAME}_test COMMAND ${PROJECT_NAME}_test)
    set_tests_properties(${PROJECT_NAME}_test PROPERTIES TIMEOUT 10)
endif ()
