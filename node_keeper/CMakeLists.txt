cmake_minimum_required(VERSION 3.10)
project(node_keeper)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

aux_source_directory(src ALL_SOURCES)
aux_source_directory(src/gossip GOSSIP_SOURCES)
list(APPEND ALL_SOURCES ${GOSSIP_SOURCES})

find_package(asio REQUIRED)

set(EXE_SOURCES ${ALL_SOURCES})
list(FILTER EXE_SOURCES EXCLUDE REGEX "^.*_test.cc$")
add_executable(${PROJECT_NAME} ${EXE_SOURCES})
target_link_libraries(${PROJECT_NAME} asio::asio)

set(TEST_SOURCES ${ALL_SOURCES})
list(FILTER TEST_SOURCES EXCLUDE REGEX "main.cc$")
if(NOT "${TEST_SOURCES}" STREQUAL "")
    add_executable(${PROJECT_NAME}_test ${TEST_SOURCES})
    find_package(GTest REQUIRED)
    target_link_libraries(${PROJECT_NAME}_test GTest::GTest asio::asio)
    add_test(NAME ${PROJECT_NAME}_test COMMAND ${PROJECT_NAME}_test)
    set_tests_properties(${PROJECT_NAME}_test PROPERTIES TIMEOUT 10)
endif()
