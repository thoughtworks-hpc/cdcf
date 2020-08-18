cmake_minimum_required(VERSION 3.10)
project(yanghui_triangle)

find_package(caf REQUIRED)
include_directories(${caf_INCLUDE_DIRS})
include_directories(../../common/include)

add_executable(${PROJECT_NAME}_server yanghui_server_example.cc)
target_link_libraries(${PROJECT_NAME}_server caf::caf common)

add_executable(${PROJECT_NAME}_worker yanghui_worker_example.cc)
target_link_libraries(${PROJECT_NAME}_worker caf::caf common)

add_executable(${PROJECT_NAME}_root yanghui_example.cc)
target_link_libraries(${PROJECT_NAME}_root caf::caf common)




