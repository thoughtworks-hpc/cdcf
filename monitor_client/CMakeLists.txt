cmake_minimum_required(VERSION 3.10)
project(monitor_client)

find_package(Protobuf REQUIRED)
find_package(gRPC REQUIRED)

include_directories(./include)
aux_source_directory(src ALL_SOURCES)

set(PB_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../node_keeper/protobuf")
set(PB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")
set(NODE_MONITOR_PROTO_SRCS "${PB_DIR}/node_monitor.pb.cc")
set(NODE_MONITOR_PROTO_HDRS "${PB_DIR}/node_monitor.pb.h")
set(NODE_MONITOR_GRPC_SRCS "${PB_DIR}/node_monitor.grpc.pb.cc")
set(NODE_MONITOR_GRPC_HDRS "${PB_DIR}/node_monitor.grpc.pb.h")

if (WIN32)
    set(GRPC_CPP_PLUGIN ${gRPC_LIB_DIRS}/../bin/grpc_cpp_plugin.exe)
else ()
    set(GRPC_CPP_PLUGIN ${gRPC_LIB_DIRS}/../bin/grpc_cpp_plugin)
endif ()

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


add_executable(${PROJECT_NAME} ${ALL_SOURCES})
install(TARGETS ${PROJECT_NAME} DESTINATION bin)

target_link_libraries(${PROJECT_NAME} common protobuf::protobuf gRPC::gRPC)
