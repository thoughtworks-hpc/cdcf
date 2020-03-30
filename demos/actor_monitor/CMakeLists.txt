cmake_minimum_required(VERSION 3.10)
project(actor_monitor)

find_package(caf REQUIRED)
include_directories(${caf_INCLUDE_DIRS})

add_executable(${PROJECT_NAME}_supervisor actor_monitor_supervisor.cc)
target_link_libraries(${PROJECT_NAME}_supervisor caf::caf)
target_link_libraries(${PROJECT_NAME}_supervisor actor_monitor)

add_executable(${PROJECT_NAME}_actor actor_monitor_worker.cc)
target_link_libraries(${PROJECT_NAME}_actor caf::caf)
target_link_libraries(${PROJECT_NAME}_actor actor_monitor)
