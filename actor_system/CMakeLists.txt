cmake_minimum_required(VERSION 3.10)
project(actor_system)

aux_source_directory(src ALL_SOURCES)

set(LIB_SOURCES ${ALL_SOURCES})
list(FILTER LIB_SOURCES EXCLUDE REGEX "^.*_test.cc$")
add_library(${PROJECT_NAME} ${LIB_SOURCES})
target_include_directories(${PROJECT_NAME} PUBLIC include)

set(TEST_SOURCES ${ALL_SOURCES})
list(FILTER TEST_SOURCES INCLUDE REGEX "^.*_test.cc$")
if(NOT "${TEST_SOURCES}" STREQUAL "")
    add_executable(${PROJECT_NAME}_test ${TEST_SOURCES})
    find_package(GTest REQUIRED)
    target_link_libraries(${PROJECT_NAME}_test GTest::GTest)
    enable_testing()
    add_test(NAME test COMMAND ${PROJECT_NAME}_test)
endif()
