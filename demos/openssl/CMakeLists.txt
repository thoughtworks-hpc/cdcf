cmake_minimum_required(VERSION 3.10)
project(openssl_demo)

find_package(caf REQUIRED)

add_executable(server server.cpp)
add_executable(client client.cpp)

target_link_libraries(server caf::caf)
target_link_libraries(client caf::caf)
