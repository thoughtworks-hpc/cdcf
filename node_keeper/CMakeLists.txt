cmake_minimum_required(VERSION 3.10)
project(node_keeper)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

aux_source_directory(src ALL_SOURCES)
aux_source_directory(src/gossip GOSSIP_SOURCES)
list(APPEND ALL_SOURCES ${GOSSIP_SOURCES})

find_package(asio REQUIRED)

find_package(Protobuf REQUIRED)
if (PROTOBUF_FOUND)
    message(STATUS "protobuf library found")
else ()
    message(FATAL_ERROR "protobuf library is needed but cant be found")
endif ()

include_directories(${PROTOBUF_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_BINARY_DIR})
set(PROTO_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(PROTOBUF_IMPORT_DIRS ${PROTO_ROOT_DIR})
PROTOBUF_GENERATE_CPP(PROTO_SRCS PROTO_HDRS message.proto)

set(EXE_SOURCES ${ALL_SOURCES})
list(FILTER EXE_SOURCES EXCLUDE REGEX "^.*_test.cc$")
add_executable(${PROJECT_NAME} ${EXE_SOURCES} ${PROTO_SRCS} ${PROTO_HDRS})
target_include_directories(${PROJECT_NAME} PUBLIC include)
target_link_libraries(${PROJECT_NAME} ${PROTOBUF_LIBRARIES})

set(TEST_SOURCES ${ALL_SOURCES})
list(FILTER TEST_SOURCES EXCLUDE REGEX "main.cc$")
if (NOT "${TEST_SOURCES}" STREQUAL "")
    add_executable(${PROJECT_NAME}_test ${TEST_SOURCES} ${PROTO_SRCS} ${PROTO_HDRS})
    find_package(GTest REQUIRED)
    target_link_libraries(${PROJECT_NAME}_test GTest::GTest asio::asio ${PROTOBUF_LIBRARIES})
    add_test(NAME ${PROJECT_NAME}_test COMMAND ${PROJECT_NAME}_test)
    set_tests_properties(${PROJECT_NAME}_test PROPERTIES TIMEOUT 10)
endif ()
