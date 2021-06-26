cmake_minimum_required(VERSION 3.10)
project(yanghui_triangle)

add_executable(${PROJECT_NAME}_server yanghui_server_example.cc)
target_link_libraries(${PROJECT_NAME}_server common)

add_executable(${PROJECT_NAME}_worker yanghui_worker_example.cc)
target_link_libraries(${PROJECT_NAME}_worker common)

add_executable(${PROJECT_NAME}_root yanghui_example.cc)
target_link_libraries(${PROJECT_NAME}_root common)
