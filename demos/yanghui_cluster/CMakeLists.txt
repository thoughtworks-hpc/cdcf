cmake_minimum_required(VERSION 3.10)
project(yanghui_cluster)

find_package(caf REQUIRED)
include_directories(${caf_INCLUDE_DIRS})
include_directories(../../config_manager/include)

add_executable(${PROJECT_NAME}_root_v2 yanghui_example_v2.cc)
target_link_libraries(${PROJECT_NAME}_root_v2 caf::caf)
target_link_libraries(${PROJECT_NAME}_root_v2 config_manager actor_monitor actor_fault_tolerance actor_system)



