cmake_minimum_required(VERSION 3.10)
project(actor_system)


find_package(caf REQUIRED)
include_directories(${caf_INCLUDE_DIRS})
include_directories(include)
include_directories(../config_manager/include)

aux_source_directory(src ALL_SOURCES)

set(LIB_SOURCES ${ALL_SOURCES})
list(FILTER LIB_SOURCES EXCLUDE REGEX "^.*_test.cc$")

set(TEST_SOURCES ${ALL_SOURCES})
list(FILTER TEST_SOURCES INCLUDE REGEX "^.*_test.cc$")

add_executable(${PROJECT_NAME}_server_example src/example/server_example.cc src/example/comm_example.cc)
target_link_libraries(${PROJECT_NAME}_server_example caf::caf)

add_executable(${PROJECT_NAME}_client_example src/example/client_example.cc src/example/comm_example.cc)
target_link_libraries(${PROJECT_NAME}_client_example caf::caf)

add_executable(${PROJECT_NAME}_yanghui_server src/example/yanghui_server_example.cc)
target_link_libraries(${PROJECT_NAME}_yanghui_server caf::caf)

add_executable(${PROJECT_NAME}_yanghui_worker  src/example/yanghui_worker_example.cc)
target_link_libraries(${PROJECT_NAME}_yanghui_worker caf::caf)

add_executable(${PROJECT_NAME}_yanghui  src/example/yanghui_example.cc)
target_link_libraries(${PROJECT_NAME}_yanghui caf::caf)

add_executable(${PROJECT_NAME}_works  src/example/remove_work.cc)
target_link_libraries(${PROJECT_NAME}_works caf::caf)

if(NOT "${TEST_SOURCES}" STREQUAL "")
    add_executable(${PROJECT_NAME}_test ${TEST_SOURCES})
    find_package(GTest REQUIRED)
    target_link_libraries(${PROJECT_NAME}_test GTest::GTest)
    add_test(NAME ${PROJECT_NAME}_test COMMAND ${PROJECT_NAME}_test)
endif()
