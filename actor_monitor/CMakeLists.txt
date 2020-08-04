cmake_minimum_required(VERSION 3.10)
project(actor_monitor)

find_package(caf REQUIRED)
include_directories(${caf_INCLUDE_DIRS})
include_directories(./include)

aux_source_directory(src ALL_SOURCES)

add_library(${PROJECT_NAME} ${ALL_SOURCES})

