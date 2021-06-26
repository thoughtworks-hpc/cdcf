cmake_minimum_required(VERSION 3.10)
project(actor_fault_tolerance)

add_executable(${PROJECT_NAME}_guard actor_guard_example.cc)
add_executable(${PROJECT_NAME}_union actor_union_example.cc)
target_link_libraries(${PROJECT_NAME}_guard actor_system common)
target_link_libraries(${PROJECT_NAME}_union actor_system common)

