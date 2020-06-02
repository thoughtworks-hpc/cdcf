cmake_minimum_required(VERSION 3.10)
project(cluster_monitor)


find_package(protobuf REQUIRED)
find_package(protoc REQUIRED)
find_package(grpc REQUIRED)

include_directories(./include)
aux_source_directory(src ALL_SOURCES)

set(PB_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../node_keeper/protobuf")
set(PB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")
set(NODE_MONITOR_PROTO_SRCS "${PB_DIR}/node_monitor.pb.cc")
set(NODE_MONITOR_PROTO_HDRS "${PB_DIR}/node_monitor.pb.h")
set(NODE_MONITOR_GRPC_SRCS "${PB_DIR}/node_monitor.grpc.pb.cc")
set(NODE_MONITOR_GRPC_HDRS "${PB_DIR}/node_monitor.grpc.pb.h")
set(GRPC_CPP_PLUGIN ${grpc_LIB_DIRS}/../bin/grpc_cpp_plugin)
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


add_executable(${PROJECT_NAME}_client ${ALL_SOURCES})


target_link_libraries(${PROJECT_NAME}_client config_manager protobuf::protobuf grpc::grpc)