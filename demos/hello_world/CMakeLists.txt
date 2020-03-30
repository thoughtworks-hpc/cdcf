cmake_minimum_required(VERSION 3.10)
project(hello_world)

find_package(asio REQUIRED)
add_executable(${PROJECT_NAME} main.cc)
target_link_libraries(${PROJECT_NAME} actor_system asio::asio)
