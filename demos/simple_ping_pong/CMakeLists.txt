cmake_minimum_required(VERSION 3.10)
project(ping_pong)

find_package(caf REQUIRED)

add_executable(ping ping.cc)
add_executable(pong pong.cc)

target_link_libraries(ping actor_system common)
target_link_libraries(pong actor_system common)