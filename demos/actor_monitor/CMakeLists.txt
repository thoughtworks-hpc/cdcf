cmake_minimum_required(VERSION 3.10)
project(actor_monitor)

find_package(CAF REQUIRED)

add_executable(${PROJECT_NAME}_supervisor actor_monitor_supervisor.cc)
target_link_libraries(${PROJECT_NAME}_supervisor CAF::CAF)
target_link_libraries(${PROJECT_NAME}_supervisor actor_system common)

add_executable(${PROJECT_NAME}_actor actor_monitor_worker.cc)
target_link_libraries(${PROJECT_NAME}_actor CAF::CAF)
target_link_libraries(${PROJECT_NAME}_actor actor_system common)
