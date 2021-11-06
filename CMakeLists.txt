cmake_minimum_required(VERSION 3.10)
project(cdcf)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

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
add_subdirectory(common)
add_subdirectory(actor_system)
add_subdirectory(monitor_client)
add_subdirectory(demos)

if ("$ENV{ACTOR_SYS_ONLY}" STREQUAL "")
    add_subdirectory(node_keeper)
endif ()

include(InstallRequiredSystemLibraries)
set(CPACK_PACKAGE_DIRECTORY ${PROJECT_SOURCE_DIR}/pack)
include(CPack)

