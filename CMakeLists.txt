cmake_minimum_required(VERSION 3.7.2)
set(CMAKE_MACOSX_RPATH 1)
include(TargetArch.cmake)
project(RabbitmqFmuProject)
set(CMAKE_PROJECT_VERSION "1.0.0")
set(CMAKE_PROJECT_VERSION_MAJOR "")
set(CMAKE_PROJECT_VERSION_MINOR "")
set(CMAKE_PROJECT_VERSION_PATCH "")

set(CMAKE_INSTALL_PREFIX "${CMAKE_CURRENT_SOURCE_DIR}/build/install" )

message("FMU will be installed to: ${CMAKE_INSTALL_PREFIX}")

#include(toolchains/clang-format.cmake)
# You can tweak some common (for all subprojects) stuff here. For example:

#set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)
set(CMAKE_DISABLE_SOURCE_CHANGES ON)

enable_testing()

#if ("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")
#  message(SEND_ERROR "In-source builds are not allowed.")
#endif ()

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_COLOR_MAKEFILE ON)

# Remove 'lib' prefix for shared libraries on Windows
if (WIN32)
    set(CMAKE_SHARED_LIBRARY_PREFIX "")
endif ()

#set_property(GLOBAL PROPERTY GLOBAL_DEPENDS_DEBUG_MODE 1)
target_architecture(arch)
message(${arch})

if (NOT DEFINED THIRD_PARTY_LIBRARIES_ROOT)
    set(THIRD_PARTY_LIBRARIES_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/external")
    if (UNIX)
        if (APPLE)
            set(THIRD_PARTY_LIBRARIES_ROOT ${THIRD_PARTY_LIBRARIES_ROOT}/darwin)
        ELSE ()
            set(THIRD_PARTY_LIBRARIES_ROOT ${THIRD_PARTY_LIBRARIES_ROOT}/linux)
        ENDIF ()
        set(THIRD_PARTY_LIBRARIES_ROOT ${THIRD_PARTY_LIBRARIES_ROOT}-${arch})
    ELSEIF (WIN32)
        set(XercesC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/external/win-${arch}/cmake")
    ENDIF ()
endif ()


message("Third parties libraries will use search path: ${THIRD_PARTY_LIBRARIES_ROOT}")


set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

# When done tweaking common stuff, configure the components (subprojects).
# NOTE: The order matters! The most independent ones should go first.

set(RAPIDJSON_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

add_subdirectory(thirdparty/rapidjson)
#add_subdirectory(thirdparty/amqp-cpp)
add_subdirectory(thirdparty/googletest)
#add_subdirectory(thirdparty/xerces-c)

#set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
#set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
#set(BUILD_STATIC_LIBS ON CACHE BOOL "" FORCE)
#set(BUILD_TESTS OFF CACHE BOOL "" FORCE)
#set(BUILD_TOOLS OFF CACHE BOOL "" FORCE)
#set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
#set(ENABLE_SSL_SUPPORT OFF CACHE BOOL "" FORCE)
#set(BUILD_TOOLS_DOCS OFF CACHE BOOL "" FORCE)
add_subdirectory(thirdparty/rabbitmq-c)
add_subdirectory(thirdparty/date/date)
add_subdirectory(rabbitmq-fmu)


