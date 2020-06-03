cmake_minimum_required(VERSION 3.7.2)
project(rabbitmq-tla-tester CXX)

enable_testing()
include(CheckCXXCompilerFlag)

set(CMAKE_VERBOSE_MAKEFILE on)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
CHECK_CXX_COMPILER_FLAG("-std=c++17" COMPILER_SUPPORTS_CXX17)

include_directories(${RapidJSON_SOURCE_DIR}/include)

file(GLOB_RECURSE CPP_FILES src/*.cpp)
file(GLOB C_FILES src/*.c)

# Main
add_executable(${PROJECT_NAME}-main ${C_FILES} ${CPP_FILES} )
target_link_libraries(${PROJECT_NAME}-main rabbitmq-core)


