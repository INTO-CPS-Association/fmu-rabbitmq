cmake_minimum_required(VERSION 3.7.2)
project(rabbitmq CXX)



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

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../thirdparty/fmi/include)
include_directories(${RapidJSON_SOURCE_DIR}/include)
include_directories(${gtest_SOURCE_DIR}/include)


#if (UNIX)
#    if (APPLE)
#        set(xercerc_lib_path ${CMAKE_CURRENT_SOURCE_DIR}/../thirdparty/external/darwin-x64)
#    ELSE ()
#        set(xercerc_lib_path ${CMAKE_CURRENT_SOURCE_DIR}/../thirdparty/external/linux-x64/lib/cmake/XercesC)
#    ENDIF ()
#ELSEIF (WIN32)
#    set(xercerc_lib_path ${CMAKE_CURRENT_SOURCE_DIR}/../thirdparty/external/win-x64/lib/cmake/XercesC)
#ENDIF ()

#target_link_libraries(${PROJECT_NAME} ${PTHREAD_LIBRARY})

# I need this: -static-libgcc -static-libstdc++ -Wl,-Bstatic -lstdc++ -lpthread -Wl,-Bdynamic
#message(path to xercesc is ${xercerc_lib_path})

find_package(XercesC CONFIG PATHS ${THIRD_PARTY_LIBRARIES_ROOT} REQUIRED) # XSD DEPENDS ON ${XERCESC_FOUND}
if (XercesC_FOUND)
    message(STATUS "Package Xerces-C++ found: ${XercesC_INCLUDE_DIR}")
else (XercesC_FOUND)
    message(FATAL_ERROR "The package Apache Xerces-C++ has not been found")
endif (XercesC_FOUND)
include_directories(${XercesC_INCLUDE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/src)
add_definitions(-DFMI_COSIMULATION)

if (NOT BUILD_SHARED_LIBS)
    add_definitions(-DAMQP_STATIC)
endif ()

file(GLOB_RECURSE CPP_FILES src/*.cpp)
list(FILTER CPP_FILES EXCLUDE REGEX "main.cpp")
file(GLOB C_FILES src/*.c)

# Main
add_executable(${PROJECT_NAME}-main ${C_FILES} ${CPP_FILES} ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp)
target_link_libraries(${PROJECT_NAME}-main XercesC::XercesC rabbitmq-static tz rabbitmq-core)

if (UNIX AND NOT APPLE)
    find_package(ICU REQUIRED data uc)
endif ()
add_library(rabbitmq SHARED ${C_FILES} ${CPP_FILES})

if(UNIX AND NOT APPLE)
   # set(THREADS_PREFER_PTHREAD_FLAG ON)
   # find_package(Threads REQUIRED)
    #target_link_libraries(rabbitmq XercesC::XercesC rabbitmq-static tz rabbitmq-core Threads::Threads -static)
    #libpthread.a
    find_library(PTHREAD_LIBRARYAA NAMES libpthread.a)
#target_link_libraries(rabbitmq XercesC::XercesC rabbitmq-static tz rabbitmq-core ${PTHREAD_LIBRARYAA})
target_link_libraries(rabbitmq XercesC::XercesC rabbitmq-static tz rabbitmq-core )
   # target_link_libraries(rabbitmq XercesC::XercesC rabbitmq-static tz rabbitmq-core )
   
elseif(APPLE)
    target_link_libraries(rabbitmq XercesC::XercesC rabbitmq-static tz rabbitmq-core)
else()
    set(THREADS_PREFER_PTHREAD_FLAG ON)
    find_package(Threads REQUIRED)
    set(OPENSSL_ROOT_DIR "C:\\Strawberry\\c\\lib")
    set(OPENSSL_USE_STATIC_LIBS TRUE)
    find_package(OpenSSL 1.1.1 REQUIRED)
    target_link_libraries(rabbitmq XercesC::XercesC rabbitmq-static tz rabbitmq-core Threads::Threads OpenSSL::SSL OpenSSL::Crypto -static )
    #target_link_libraries(rabbitmq XercesC::XercesC rabbitmq-static tz rabbitmq-core)
endif()

set_target_properties(rabbitmq
        PROPERTIES POSITION_INDEPENDENT_CODE ON)
set_target_properties(rabbitmq PROPERTIES PREFIX "")



if (UNIX AND NOT APPLE)
    # for Linux, BSD, Solaris, Minix
    target_link_libraries(rabbitmq dl)
endif ()


#foreach (LETTER ${CMAKE_CXX_IMPLICIT_LINK_DIRECTORIES})
#
#    if (EXISTS "${LETTER}/libstdc++.a")
#           message (STATUS "${LETTER}")
#        set(STDCPP_LIBRARY "${LETTER}/libstdc++.a")
#    else ()
#        #  message (STATUS "${LETTER} ---- not these")
#    endif ()
#endforeach ()
#message(STATUS "Static linking with libstdc++.a in ${STDCPP_LIBRARY}")
#target_link_libraries(rabbitmqfmu ${STDCPP_LIBRARY})
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
message("Compiler is clang not adding -static-libgcc")
else ()
    string(APPEND CMAKE_SHARED_LINKER_FLAGS "-static-libgcc -static-libstdc++")
endif ()

if (WIN32)
    #windows
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libstdc++ -static-libgcc")
endif ()

#################################### TESTS ####################################

list(FILTER CPP_FILES EXCLUDE REGEX ".*src\\/main\\.cpp")

if (NOT ("${PROJECT_SOURCE_DIR}" STREQUAL "${PROJECT_BINARY_DIR}"))
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/modelDescription.xml
            DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/data.json
            DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/data_modeldescription.json
            DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/sample.xml
            DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
endif ()


if (ENABLE_RBMQ_FMU_TESTS)
    # ------------------------------- UNIT TESTS ----------------------------------
    file(GLOB CPP_TEST_FILES test/*.cpp)
    list(FILTER CPP_TEST_FILES EXCLUDE REGEX ".*_it.cpp")

    add_executable(unit-test-${PROJECT_NAME} ${C_FILES} ${CPP_FILES} ${CPP_TEST_FILES})
    target_link_libraries(unit-test-${PROJECT_NAME} gtest_main XercesC::XercesC rabbitmq-static tz rabbitmq-core)

    if (WIN32)
        add_test(NAME unit-test-${PROJECT_NAME} COMMAND unit-test-${PROJECT_NAME}   --gtest_output=xml:gtestresults.xml)
    else ()
        add_test(NAME unit-test-${PROJECT_NAME} COMMAND unit-test-${PROJECT_NAME}  --gtest_output=xml:gtestresults.xml)
    endif ()

    # ---------------------------- Integration TESTS ------------------------------

    file(GLOB CPP_TEST_FILES test/*.cpp)
    list(FILTER CPP_TEST_FILES INCLUDE REGEX ".*_it.cpp")

    add_executable(it-test-${PROJECT_NAME} ${C_FILES} ${CPP_FILES} ${CPP_TEST_FILES})
    target_link_libraries(it-test-${PROJECT_NAME} gtest_main XercesC::XercesC rabbitmq-static tz rabbitmq-core)

    #if (WIN32)
    #    add_test(it-test-${PROJECT_NAME} wine ${PROJECT_NAME} --gtest_output=xml:gtestresults.xml)
    #else ()
    #    add_test(it-test-${PROJECT_NAME} ${PROJECT_NAME} --gtest_output=xml:gtestresults.xml)
    #endif ()

    # ---------------------------- Small TESTS ------------------------------
    add_executable (BasicConsumer test/basic_consumer.cpp)
    target_link_libraries(BasicConsumer gtest_main XercesC::XercesC rabbitmq-static tz rabbitmq-core)
endif()

#################################### INSTALL targets ####################################
target_architecture(arch)
message(${arch})

set(FMU_OS "unknown")


if (UNIX)
    if (APPLE)
        set(FMU_OS "darwin")
    ELSE ()
        set(FMU_OS "linux")
    ENDIF ()
ELSEIF (WIN32)
    set(FMU_OS "win")
ENDIF ()


if (${arch} STREQUAL "x86_64")
    set(FMU_OS "${FMU_OS}64")
ELSE ()
    set(FMU_OS "${FMU_OS}32")
ENDIF ()


install(TARGETS rabbitmq DESTINATION "rabbitmqfmu/binaries/${FMU_OS}")
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/modelDescription.xml" DESTINATION "rabbitmqfmu/resources")
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/modelDescription.xml" DESTINATION "rabbitmqfmu")

#################################### Custom targets ####################################

add_custom_target(fmu

        COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/mkFmu.sh $<TARGET_FILE:rabbitmq> ${CMAKE_CURRENT_SOURCE_DIR}/modelDescription.xml
        COMMENT "Building fmu"
        WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
        )
