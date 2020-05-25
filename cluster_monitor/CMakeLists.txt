cmake_minimum_required(VERSION 3.10)
project(cluster_monitor)

include_directories(./include)
aux_source_directory(src ALL_SOURCES)
add_library(${PROJECT_NAME} ${ALL_SOURCES})