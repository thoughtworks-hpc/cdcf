cmake_minimum_required(VERSION 3.10)
project(actor_system)

aux_source_directory(src ALL_SOURCES)
aux_source_directory(demo ALL_SOURCES)

set(LIB_SOURCES ${ALL_SOURCES})
list(FILTER LIB_SOURCES EXCLUDE REGEX "^.*_test.cc$")
add_library(${PROJECT_NAME} ${LIB_SOURCES})
target_include_directories(${PROJECT_NAME} PUBLIC include)

set(CONFIG_MANAGER_DIR "../config_manager")

find_package(caf REQUIRED)
link_directories(${CONFIG_MANAGER_DIR})
include_directories(${caf_INCLUDE_DIRS})
include_directories(include)
include_directories(${CONFIG_MANAGER_DIR}/include)

add_executable(additional_allownance demo/additional_allownance.cc)
target_link_libraries(additional_allownance caf::caf)
target_link_libraries(additional_allownance libconfig_manager.a)

set(TEST_SOURCES ${ALL_SOURCES})
list(FILTER TEST_SOURCES INCLUDE REGEX "^.*_test.cc$")
if(NOT "${TEST_SOURCES}" STREQUAL "")
    add_executable(${PROJECT_NAME}_test ${TEST_SOURCES})
    find_package(GTest REQUIRED)
    target_link_libraries(${PROJECT_NAME}_test GTest::GTest)
	target_link_libraries(${PROJECT_NAME}_test caf::caf)
	target_link_libraries(${PROJECT_NAME}_test libconfig_manager.a)
	enable_testing()
    add_test(NAME ${PROJECT_NAME}_test COMMAND ${PROJECT_NAME}_test)
endif()
