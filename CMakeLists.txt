cmake_minimum_required(VERSION 3.7.2)


set(CMAKE_POLICY_DEFAULT_CMP0079 NEW)

set(CMAKE_MACOSX_RPATH 1)
include(TargetArch.cmake)
project(RabbitmqFmuProject)
set(CMAKE_PROJECT_VERSION "1.0.0")
set(CMAKE_PROJECT_VERSION_MAJOR "")
set(CMAKE_PROJECT_VERSION_MINOR "")
set(CMAKE_PROJECT_VERSION_PATCH "")

#so that it can find the ssl stuff
if (APPLE)
    set(OPENSSL_ROOT_DIR "/usr/local/opt/openssl")
    set(OPENSSL_LIBRARIES "/usr/local/opt/openssl/lib")
ENDIF ()

set(CMAKE_INSTALL_PREFIX "${CMAKE_CURRENT_SOURCE_DIR}/build/install" )



message("#########################################")
message("${CMAKE_CURRENT_SOURCE_DIR}/build/install")
message("#########################################")

#get_filename_component(PARENT_DIR "${CMAKE_CURRENT_SOURCE_DIR}" PATH)
#set(CMAKE_INSTALL_PREFIX "${PARENT_DIR}/build/install" )

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
    set(THIRD_PARTY_LIBRARIES_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/build/external")
    if (UNIX)
        if (APPLE)
            set(THIRD_PARTY_LIBRARIES_ROOT ${THIRD_PARTY_LIBRARIES_ROOT}/darwin)
            set(XercesC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/build/external/darwin-${arch}/cmake")
            #set(CMAKE_PREFIX_PATH "${CMAKE_CURRENT_SOURCE_DIR}/build/external/darwin-${arch}/lib/libxerces-c-3.2.a")
        ELSE ()
            set(THIRD_PARTY_LIBRARIES_ROOT ${THIRD_PARTY_LIBRARIES_ROOT}/linux)
            set(XercesC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/build/external/linux-${arch}/cmake")
            #set(CMAKE_PREFIX_PATH "${CMAKE_CURRENT_SOURCE_DIR}/build/external/linux-${arch}/lib/libxerces-c-3.2.a")
        ENDIF ()
        set(THIRD_PARTY_LIBRARIES_ROOT ${THIRD_PARTY_LIBRARIES_ROOT}-${arch})
        message("Thirdparty libraries root:")
        message(${THIRD_PARTY_LIBRARIES_ROOT})
    ELSEIF (WIN32)
        set(XercesC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/build/external/win-${arch}/cmake")
    ENDIF ()
endif ()


message("Third parties libraries will use search path: ${THIRD_PARTY_LIBRARIES_ROOT}")


set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

# When done tweaking common stuff, configure the components (subprojects).
# NOTE: The order matters! The most independent ones should go first.

# set(USE_RBMQ_FMU_THREAD OFF CACHE BOOL "" FORCE)
# set(USE_RBMQ_FMU_PROF OFF CACHE BOOL "" FORCE)

set(ENABLE_RBMQ_FMU_TESTS ON CACHE BOOL "" FORCE)

option(USE_RBMQ_FMU_PRIORITY_QUEUE "Use RBMQ priority queue (incoming)" OFF)
message("USE_RBMQ_FMU_PRIORITY_QUEUE: ${USE_RBMQ_FMU_PRIORITY_QUEUE}")
if (USE_RBMQ_FMU_PRIORITY_QUEUE)
    add_definitions(-DUSE_RBMQ_FMU_PRIORITY_QUEUE)
    set(ENABLE_RBMQ_FMU_TESTS OFF CACHE BOOL "" FORCE)
endif ()

option(USE_RBMQ_FMU_THREAD "Use RBMQ consumer thread" ON)
message("USE_RBMQ_FMU_THREAD: ${USE_RBMQ_FMU_THREAD}")
if (USE_RBMQ_FMU_THREAD)
    add_definitions(-DUSE_RBMQ_FMU_THREAD)
endif ()

option(USE_RBMQ_FMU_HEALTH_THREAD "Use RBMQ health thread" ON)
message("USE_RBMQ_FMU_HEALTH_THREAD: ${USE_RBMQ_FMU_HEALTH_THREAD}")
if (USE_RBMQ_FMU_HEALTH_THREAD)
    add_definitions(-DUSE_RBMQ_FMU_HEALTH_THREAD)
endif ()

option(USE_RBMQ_FMU_PROF "Use RBMQ profiling of step function" OFF)
message("USE_RBMQ_FMU_PROF: ${USE_RBMQ_FMU_PROF}")
if (USE_RBMQ_FMU_PROF)
    add_definitions(-DUSE_RBMQ_FMU_PROF)
endif ()

message("ENABLE_RBMQ_FMU_TESTS: ${ENABLE_RBMQ_FMU_TESTS}")

set(RAPIDJSON_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)


add_subdirectory(thirdparty/rapidjson)
#add_subdirectory(thirdparty/amqp-cpp)
#### Comment line 118 to build locally.
add_subdirectory(thirdparty/googletest)

#execute_process(COMMAND ${CMAKE_EXECUTABLE} thirdparty/rapidjson -DCMAKE_INSTALL_PREFIX="${CMAKE_CURRENT_SOURCE_DIR}/build/install/thirdparty/rapidjson" WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/build/thirdparty/rapidjson")
#execute_process(COMMAND ${CMAKE_EXECUTABLE} --build "${CMAKE_CURRENT_SOURCE_DIR}/build/thirdparty/rapidjson" --target install)

add_subdirectory(thirdparty/xerces-c)

#set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
#set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
#set(BUILD_STATIC_LIBS ON CACHE BOOL "" FORCE)
#set(BUILD_TESTS OFF CACHE BOOL "" FORCE)
#set(BUILD_TOOLS OFF CACHE BOOL "" FORCE)
#set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(ENABLE_SSL_SUPPORT ON CACHE BOOL "" FORCE)
set(OPENSSL_USE_STATIC_LIBS ON CACHE BOOL "" FORCE)
#set(BUILD_TOOLS_DOCS OFF CACHE BOOL "" FORCE)
add_subdirectory(thirdparty/rabbitmq-c)
add_subdirectory(thirdparty/date)
add_subdirectory(rabbitmq-core)
if (UNIX AND ENABLE_RBMQ_FMU_TESTS)
add_subdirectory(rabbitmq-tla-tester)
endif()
add_subdirectory(rabbitmq-fmu)


