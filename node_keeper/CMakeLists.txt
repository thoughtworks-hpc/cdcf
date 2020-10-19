cmake_minimum_required(VERSION 3.10)
project(node_keeper)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

aux_source_directory(src ALL_SOURCES)
aux_source_directory(src/gossip GOSSIP_SOURCES)
aux_source_directory(src/daemon DAEMON_SOURCES)
list(APPEND ALL_SOURCES ${GOSSIP_SOURCES} ${DAEMON_SOURCES} protobuf/message.proto)

find_package(asio REQUIRED)
find_package(protobuf REQUIRED)
find_package(protoc REQUIRED)
find_package(gRPC REQUIRED)

include_directories(${CMAKE_CURRENT_BINARY_DIR})

# There's no simple way to generate code for gRPC from protobuf AFAIK, following block is to do that.
set(PB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")
set(NODE_KEEPER_PROTO_SRCS "${PB_DIR}/node_keeper.pb.cc")
set(NODE_KEEPER_PROTO_HDRS "${PB_DIR}/node_keeper.pb.h")
set(NODE_KEEPER_GRPC_SRCS "${PB_DIR}/node_keeper.grpc.pb.cc")
set(NODE_KEEPER_GRPC_HDRS "${PB_DIR}/node_keeper.grpc.pb.h")
# Conan generated gRPC does not export plugin target, should set it manually, looks for gRPCConfig.cmake file installed by
# gRPC's cmake installation is alternative.

if (WIN32)
    set(GRPC_CPP_PLUGIN ${gRPC_LIB_DIRS}/../bin/grpc_cpp_plugin.exe)
else ()
    set(GRPC_CPP_PLUGIN ${gRPC_LIB_DIRS}/../bin/grpc_cpp_plugin)
endif ()

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

set(NODE_MONITOR_PROTO_SRCS "${PB_DIR}/node_monitor.pb.cc")
set(NODE_MONITOR_PROTO_HDRS "${PB_DIR}/node_monitor.pb.h")
set(NODE_MONITOR_GRPC_SRCS "${PB_DIR}/node_monitor.grpc.pb.cc")
set(NODE_MONITOR_GRPC_HDRS "${PB_DIR}/node_monitor.grpc.pb.h")
add_custom_command(
        OUTPUT "${NODE_MONITOR_PROTO_SRCS}" "${NODE_MONITOR_PROTO_HDRS}" "${NODE_MONITOR_GRPC_SRCS}" "${NODE_MONITOR_GRPC_HDRS}"
        COMMAND $<TARGET_FILE:protobuf::protoc>
        ARGS --grpc_out "${PB_DIR}"
        --cpp_out "${PB_DIR}"
        -I "${CMAKE_CURRENT_SOURCE_DIR}/protobuf"
        --plugin=protoc-gen-grpc=${GRPC_CPP_PLUGIN}
        "${CMAKE_CURRENT_SOURCE_DIR}/protobuf/node_monitor.proto"
        DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/protobuf/node_monitor.proto")
list(APPEND ALL_SOURCES ${NODE_MONITOR_PROTO_SRCS} ${NODE_MONITOR_GRPC_SRCS})

set(EXE_SOURCES ${ALL_SOURCES})
list(FILTER EXE_SOURCES EXCLUDE REGEX "^.*_test.cc$")
add_executable(${PROJECT_NAME} ${EXE_SOURCES} ${NODE_KEEPER_PROTO_SRCS} ${NODE_KEEPER_GRPC_SRCS})
install(TARGETS ${PROJECT_NAME} DESTINATION bin)

target_link_libraries(${PROJECT_NAME} asio::asio protobuf::protobuf gRPC::gRPC common)
protobuf_generate(LANGUAGE cpp TARGET ${PROJECT_NAME})

set(TEST_SOURCES ${ALL_SOURCES})
list(FILTER TEST_SOURCES EXCLUDE REGEX "main.cc$")
if (NOT "${TEST_SOURCES}" STREQUAL "")
    add_executable(${PROJECT_NAME}_test ${TEST_SOURCES})
    find_package(GTest REQUIRED)
    target_link_libraries(${PROJECT_NAME}_test GTest::GTest asio::asio protobuf::protobuf gRPC::gRPC common)
    protobuf_generate(LANGUAGE cpp TARGET ${PROJECT_NAME}_test)
    add_test(NAME ${PROJECT_NAME}_test COMMAND ${PROJECT_NAME}_test)
    set_tests_properties(${PROJECT_NAME}_test PROPERTIES TIMEOUT 60)
endif ()