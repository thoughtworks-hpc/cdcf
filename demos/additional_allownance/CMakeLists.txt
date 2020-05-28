cmake_minimum_required(VERSION 3.10)
project(additional_allownance)

add_executable(${PROJECT_NAME} main.cc)
target_link_libraries(${PROJECT_NAME} actor_system)
