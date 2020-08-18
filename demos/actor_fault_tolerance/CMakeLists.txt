cmake_minimum_required(VERSION 3.10)
project(actor_fault_tolerance)

find_package(caf REQUIRED)
include_directories(${caf_INCLUDE_DIRS})

add_executable(${PROJECT_NAME}_guard actor_guard_example.cc)
add_executable(${PROJECT_NAME}_union actor_union_example.cc)
target_link_libraries(${PROJECT_NAME}_guard caf::caf)
target_link_libraries(${PROJECT_NAME}_guard actor_system common)
target_link_libraries(${PROJECT_NAME}_union caf::caf)
target_link_libraries(${PROJECT_NAME}_union actor_system common)

