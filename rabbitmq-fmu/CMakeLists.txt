cmake_minimum_required(VERSION 3.7.2)
project(rabbitmq-fmu CXX)

enable_testing()
include(CheckCXXCompilerFlag)

set(CMAKE_VERBOSE_MAKEFILE on)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)

if (COMPILER_SUPPORTS_CXX11)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

else ()
    message(STATUS "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support. Please use a different C++ compiler.")
endif ()


#CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
#CHECK_CXX_COMPILER_FLAG("-std=c++0x" COMPILER_SUPPORTS_CXX0X)
#if (COMPILER_SUPPORTS_CXX11)
#    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
#elseif (COMPILER_SUPPORTS_CXX0X)
#    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
#else ()
#    message(STATUS "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support. Please use a different C++ compiler.")
#endif ()


#add_definitions(-DSOURCE_ROOT="${CMAKE_CURRENT_SOURCE_DIR}")

#set(CMAKE_C_FLAGS ${CMAKE_C_FLAGS} "-pthread")


#get_cmake_property(_variableNames VARIABLES)
#list (SORT _variableNames)
#foreach (_variableName ${_variableNames})
#    message(STATUS "${_variableName}=${${_variableName}}")
#endforeach()

#add_dependencies(rapidjson)
#include_directories(${libshmfmu_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../thirdparty/fmi/include)
include_directories(${RapidJSON_SOURCE_DIR}/include)
include_directories(${gtest_SOURCE_DIR}/include)
#include_directories(${LIBRABBITMQ_INCLUDE_DIRS})
#target_link_libraries(${PROJECT_NAME} libshmfmu_static)
#target_link_libraries(${PROJECT_NAME} ${libshmfmu_LIBS})


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
endif()


file(GLOB_RECURSE CPP_FILES src/*.cpp)
file(GLOB C_FILES src/*.c)

add_executable(${PROJECT_NAME} ${C_FILES} ${CPP_FILES})
target_link_libraries(${PROJECT_NAME} XercesC::XercesC rabbitmq-static)



add_library(${PROJECT_NAME}-fmu SHARED ${C_FILES} ${CPP_FILES})
target_link_libraries(${PROJECT_NAME}-fmu XercesC::XercesC rabbitmq-static)
set_target_properties(${PROJECT_NAME}-fmu
        PROPERTIES POSITION_INDEPENDENT_CODE ON)

if(UNIX AND NOT APPLE)
    # for Linux, BSD, Solaris, Minix
    target_link_libraries (${PROJECT_NAME}-fmu dl)
endif()


foreach(LETTER ${CMAKE_CXX_IMPLICIT_LINK_DIRECTORIES})

    if(EXISTS "${LETTER}/libstdc++.a")
        #   message (STATUS "${LETTER}")
        set(STDCPP_LIBRARY "${LETTER}/libstdc++.a")
    else()
        #  message (STATUS "${LETTER} ---- not these")
    endif()
endforeach()
message(STATUS "Static linking with libstdc++.a in ${STDCPP_LIBRARY}")
target_link_libraries(${PROJECT_NAME}-fmu ${STDCPP_LIBRARY})

if (WIN32)
    #windows
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libstdc++ -static-libgcc")
endif ()

#################################### TESTS ####################################

list(FILTER CPP_FILES EXCLUDE REGEX ".*src\\/main\\.cpp")

if(NOT ("${PROJECT_SOURCE_DIR}" STREQUAL "${PROJECT_BINARY_DIR}"))
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/modelDescription.xml
            DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/data.json
            DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/data_modeldescription.json
            DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/sample.xml
            DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
endif()


# ------------------------------- UNIT TESTS ----------------------------------
file(GLOB CPP_TEST_FILES test/*.cpp)
list(FILTER CPP_TEST_FILES EXCLUDE REGEX ".*_it.cpp")

add_executable(unit-test-${PROJECT_NAME} ${C_FILES} ${CPP_FILES} ${CPP_TEST_FILES})
target_link_libraries(unit-test-${PROJECT_NAME} gtest_main XercesC::XercesC rabbitmq-static)

if (WIN32)
    add_test(unit-test-${PROJECT_NAME} wine ${PROJECT_NAME} --gtest_output=xml:gtestresults.xml)
else ()
    add_test(unit-test-${PROJECT_NAME} ${PROJECT_NAME} --gtest_output=xml:gtestresults.xml)
endif ()

# ---------------------------- Integration TESTS ------------------------------

file(GLOB CPP_TEST_FILES test/*.cpp)
list(FILTER CPP_TEST_FILES INCLUDE REGEX ".*_it.cpp")

add_executable(it-test-${PROJECT_NAME} ${C_FILES} ${CPP_FILES} ${CPP_TEST_FILES})
target_link_libraries(it-test-${PROJECT_NAME} gtest_main XercesC::XercesC rabbitmq-static)

#if (WIN32)
#    add_test(it-test-${PROJECT_NAME} wine ${PROJECT_NAME} --gtest_output=xml:gtestresults.xml)
#else ()
#    add_test(it-test-${PROJECT_NAME} ${PROJECT_NAME} --gtest_output=xml:gtestresults.xml)
#endif ()


#################################### Custom targets ####################################

add_custom_target(fmu

        COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/mkFmu.sh $<TARGET_FILE:${PROJECT_NAME}-fmu> ${CMAKE_CURRENT_SOURCE_DIR}/modelDescription.xml
        COMMENT "Building fmu"
        WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
        )