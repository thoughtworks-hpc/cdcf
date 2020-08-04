cmake_minimum_required(VERSION 3.10)
project(config_manager)

aux_source_directory(src ALL_SOURCES)

set(LIB_SOURCES ${ALL_SOURCES})
list(FILTER LIB_SOURCES EXCLUDE REGEX "^.*_test.cc$")
add_library(${PROJECT_NAME} ${LIB_SOURCES})
target_include_directories(${PROJECT_NAME} PUBLIC include)
target_link_libraries(${PROJECT_NAME} caf::caf)

find_package(caf REQUIRED)
include_directories(${caf_INCLUDE_DIRS})
include_directories(include)

set(TEST_INI_FILE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/test_source/test.ini)
set(TEST_SOURCES ${ALL_SOURCES} include/config_manager_test.h)


configure_file(${CMAKE_CURRENT_SOURCE_DIR}/include/config_manager_test.h.in ${CMAKE_CURRENT_SOURCE_DIR}/include/config_manager_test.h)

list(FILTER TEST_SOURCES INCLUDE REGEX "^.*_test.cc$")

if(NOT "${TEST_SOURCES}" STREQUAL "")
    add_executable(${PROJECT_NAME}_test ${TEST_SOURCES} ${LIB_SOURCES})
    find_package(GTest REQUIRED)
    target_link_libraries(${PROJECT_NAME}_test GTest::GTest)
    target_link_libraries(${PROJECT_NAME}_test caf::caf)
    #target_link_libraries(${PROJECT_NAME}_test ${PROJECT_NAME})
    enable_testing()
    add_test(NAME ${PROJECT_NAME}_test COMMAND ${PROJECT_NAME}_test)
endif()
