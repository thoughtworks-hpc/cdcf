cmake_minimum_required(VERSION 3.10)
project(hello_world)

aux_source_directory(. ALL_SOURCES)

add_executable(${PROJECT_NAME} ${ALL_SOURCES})
target_link_libraries(${PROJECT_NAME} actor_system)
