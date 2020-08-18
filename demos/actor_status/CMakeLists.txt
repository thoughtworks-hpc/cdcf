cmake_minimum_required(VERSION 3.10)
project(actor_status_service)

add_executable(${PROJECT_NAME} actor_status_service.cc)
target_link_libraries(${PROJECT_NAME} actor_system common)