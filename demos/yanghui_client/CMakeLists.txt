cmake_minimum_required(VERSION 3.10)
project(yanghui_client)

find_package(caf REQUIRED)
include_directories(${caf_INCLUDE_DIRS})
include_directories(./include)
include_directories(../actor_system/include)
include_directories(../config_manager/include)

aux_source_directory(src ALL_SOURCES)

add_executable(${PROJECT_NAME} ${ALL_SOURCES})
target_link_libraries(${PROJECT_NAME} config_manager actor_monitor actor_fault_tolerance actor_system logger router_pool message_priority_actor)