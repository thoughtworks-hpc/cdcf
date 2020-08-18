cmake_minimum_required(VERSION 3.10)
project(cdcf)

# ignore asio template compile warning
add_definitions(-D _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS)

if (WIN32)
    add_definitions(-D _WIN32_WINNT=0x600)
endif ()

set(CMAKE_CXX_STANDARD 17)
if ("${CONAN_COMPILER}" STREQUAL "gcc")
    set(CMAKE_C_FLAGS ${CONAN_C_FLAGS})
    set(CMAKE_CXX_FLAGS ${CONAN_CXX_FLAGS})
    set(CMAKE_SHARED_LINKER_FLAGS "${CONAN_SHARED_LINKER_FLAGS}")
endif ()

include(CTest)
add_subdirectory(config_manager)
add_subdirectory(actor_monitor)
add_subdirectory(actor_system)
add_subdirectory(logger)
add_subdirectory(cluster_monitor)
add_subdirectory(demos)
add_subdirectory(router_pool)
add_subdirectory(message_priority_actor)

if ("$ENV{ACTOR_SYS_ONLY}" STREQUAL "")
    add_subdirectory(node_keeper)
    add_subdirectory(daemon)
endif ()