cmake_minimum_required(VERSION 3.10)
project(message_priority_actor)

find_package(caf REQUIRED)
include_directories(${caf_INCLUDE_DIRS})
include_directories(./include)
include_directories(../actor_system/include)
include_directories(../config_manager/include)

aux_source_directory(src ALL_SOURCES)

add_library(${PROJECT_NAME} ${ALL_SOURCES})
