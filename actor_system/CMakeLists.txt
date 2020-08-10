cmake_minimum_required(VERSION 3.10)
project(actor_system)

aux_source_directory(src ALL_SOURCES)
aux_source_directory(src/load_balancer LOAD_BALANCER_SOURCES)
list(APPEND ALL_SOURCES ${LOAD_BALANCER_SOURCES})

find_package(protobuf REQUIRED)
find_package(protoc REQUIRED)
find_package(grpc REQUIRED)
# There's no simple way to generate code for gRPC from protobuf AFAIK, following block is to do that.
set(PB_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../node_keeper/protobuf")
set(PB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")
set(NODE_KEEPER_PROTO_SRCS "${PB_DIR}/node_keeper.pb.cc")
set(NODE_KEEPER_PROTO_HDRS "${PB_DIR}/node_keeper.pb.h")
set(NODE_KEEPER_GRPC_SRCS "${PB_DIR}/node_keeper.grpc.pb.cc")
set(NODE_KEEPER_GRPC_HDRS "${PB_DIR}/node_keeper.grpc.pb.h")
# Conan generated gRPC does not export plugin target, should set it manually, looks for gRPCConfig.cmake file installed by
# gRPC's cmake installation is alternative.

if (WIN32)
    set(GRPC_CPP_PLUGIN ${grpc_LIB_DIRS}/../bin/grpc_cpp_plugin.exe)
else()
    set(GRPC_CPP_PLUGIN ${grpc_LIB_DIRS}/../bin/grpc_cpp_plugin)
endif()


add_custom_command(
        OUTPUT "${NODE_KEEPER_PROTO_SRCS}" "${NODE_KEEPER_PROTO_HDRS}" "${NODE_KEEPER_GRPC_SRCS}" "${NODE_KEEPER_GRPC_HDRS}"
        COMMAND $<TARGET_FILE:protobuf::protoc>
        ARGS --grpc_out "${PB_DIR}"
        --cpp_out "${PB_DIR}"
        -I "${PB_SOURCE_DIR}"
        --plugin=protoc-gen-grpc=${GRPC_CPP_PLUGIN}
        "${PB_SOURCE_DIR}/node_keeper.proto"
        DEPENDS "${PB_SOURCE_DIR}/node_keeper.proto")
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
        -I "${PB_SOURCE_DIR}"
        --plugin=protoc-gen-grpc=${GRPC_CPP_PLUGIN}
        "${PB_SOURCE_DIR}/node_monitor.proto"
        DEPENDS "${PB_SOURCE_DIR}/node_monitor.proto")
list(APPEND ALL_SOURCES ${NODE_MONITOR_PROTO_SRCS} ${NODE_MONITOR_GRPC_SRCS})

set(LIB_SOURCES ${ALL_SOURCES})
list(FILTER LIB_SOURCES EXCLUDE REGEX "^.*_test.cc$")
add_library(${PROJECT_NAME} ${LIB_SOURCES})

target_include_directories(${PROJECT_NAME}
        PUBLIC include
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
find_package(caf REQUIRED)
target_link_libraries(${PROJECT_NAME} config_manager caf::caf grpc::grpc)
target_include_directories(${PROJECT_NAME} PUBLIC include)

set(TEST_SOURCES ${ALL_SOURCES})
list(FILTER TEST_SOURCES INCLUDE REGEX "^.*_test.cc$")
if(NOT "${TEST_SOURCES}" STREQUAL "")
    add_executable(${PROJECT_NAME}_test ${TEST_SOURCES})
    find_package(GTest REQUIRED)
    target_link_libraries(${PROJECT_NAME}_test ${PROJECT_NAME} GTest::GTest actor_monitor)
    enable_testing()
    add_test(NAME ${PROJECT_NAME}_test COMMAND ${PROJECT_NAME}_test)
endif()
