cmake_minimum_required(VERSION 3.10)
project(openssl_demo)

find_package(CAF REQUIRED)

add_executable(server server.cpp)
add_executable(client client.cpp)

target_link_libraries(server CAF::CAF)
target_link_libraries(client CAF::CAF)
