cmake_minimum_required(VERSION 3.7.2)
project(rabbitmq-core CXX)

enable_testing()
include(CheckCXXCompilerFlag)

set(CMAKE_VERBOSE_MAKEFILE on)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
CHECK_CXX_COMPILER_FLAG("-std=c++17" COMPILER_SUPPORTS_CXX17)

#if (COMPILER_SUPPORTS_CXX11)
#    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")
#
#else ()
#    message(STATUS "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support. Please use a different C++ compiler.")
#endif ()




include_directories(${gtest_SOURCE_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR}/src)


file(GLOB_RECURSE CPP_FILES src/*.cpp)
file(GLOB C_FILES src/*.c)

# Main
#add_executable(${PROJECT_NAME}-main ${C_FILES} ${CPP_FILES} ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp)
#target_link_libraries(${PROJECT_NAME}-main XercesC::XercesC rabbitmq-static tz)

# TLA tester
#add_executable(${PROJECT_NAME}-tla ${C_FILES} ${CPP_FILES} ${CMAKE_CURRENT_SOURCE_DIR}/src/tla/TlaTester.cpp)
#target_link_libraries(${PROJECT_NAME}-tla XercesC::XercesC rabbitmq-static tz)

add_library(rabbitmq-core STATIC ${C_FILES} ${CPP_FILES})       

target_link_libraries(rabbitmq-core tz)
set_target_properties(rabbitmq-core
        PROPERTIES POSITION_INDEPENDENT_CODE ON)
#set_target_properties(rabbitmq PROPERTIES PREFIX "")

target_include_directories(rabbitmq-core INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/src)

if (ENABLE_RBMQ_FMU_TESTS)
    #################################### TESTS ####################################

    list(FILTER CPP_FILES EXCLUDE REGEX ".*src\\/main\\.cpp")




    # ------------------------------- UNIT TESTS ----------------------------------
    file(GLOB CPP_TEST_FILES test/*.cpp)
    list(FILTER CPP_TEST_FILES EXCLUDE REGEX ".*_it.cpp")

    add_executable(unit-test-${PROJECT_NAME} ${C_FILES} ${CPP_FILES} ${CPP_TEST_FILES})
    target_link_libraries(unit-test-${PROJECT_NAME} gtest_main tz)

    add_test(NAME unit-test-${PROJECT_NAME} COMMAND unit-test-${PROJECT_NAME}  --gtest_output=xml:gtestresults.xml)
    # ---------------------------- Integration TESTS ------------------------------

    file(GLOB CPP_TEST_FILES test/*.cpp)
    list(FILTER CPP_TEST_FILES INCLUDE REGEX ".*_it.cpp")

    add_executable(it-test-${PROJECT_NAME} ${C_FILES} ${CPP_FILES} ${CPP_TEST_FILES})
    target_link_libraries(it-test-${PROJECT_NAME} gtest_main tz)
endif ()


