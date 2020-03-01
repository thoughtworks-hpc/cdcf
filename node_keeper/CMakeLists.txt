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
find_package(Protobuf REQUIRED)

include_directories(${PROTOBUF_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_BINARY_DIR})
set(PROTO_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(PROTOBUF_IMPORT_DIRS ${PROTO_ROOT_DIR})

set(EXE_SOURCES ${ALL_SOURCES})
list(FILTER EXE_SOURCES EXCLUDE REGEX "^.*_test.cc$")
add_executable(${PROJECT_NAME} ${EXE_SOURCES})
target_include_directories(${PROJECT_NAME} PUBLIC include protobuf)
target_link_libraries(${PROJECT_NAME} ${CONAN_LIBS})

set(TEST_SOURCES ${ALL_SOURCES})
list(FILTER TEST_SOURCES EXCLUDE REGEX "main.cc$")
if (NOT "${TEST_SOURCES}" STREQUAL "")
    add_executable(${PROJECT_NAME}_test ${TEST_SOURCES})
    find_package(GTest REQUIRED)
    target_link_libraries(${PROJECT_NAME}_test GTest::GTest asio::asio ${CONAN_LIBS})
    add_test(NAME ${PROJECT_NAME}_test COMMAND ${PROJECT_NAME}_test)
    set_tests_properties(${PROJECT_NAME}_test PROPERTIES TIMEOUT 10)
endif ()
