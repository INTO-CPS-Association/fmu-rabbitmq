cmake_minimum_required(VERSION 3.7.2)

set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(BUILD_STATIC_LIBS ON CACHE BOOL "" FORCE)
set(BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(BUILD_TOOLS OFF CACHE BOOL "" FORCE)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(ENABLE_SSL_SUPPORT ON CACHE BOOL "" FORCE)
set(BUILD_TOOLS_DOCS OFF CACHE BOOL "" FORCE)

if (WIN32)
    # fix for mingw
    set(HAVE_SELECT ON CACHE BOOL "" FORCE)
    set(HAVE_POLL OFF CACHE BOOL "" FORCE)
endif ()

#execute_process(COMMAND sed -i. "'s/target_link_libraries(rabbitmq-static $\\{RMQ_LIBRARIES\\}).*/target_link_libraries(rabbitmq-static $\\{RMQ_LIBRARIES\\}) target_include_directories(rabbitmq-static PUBLIC \$\\{CMAKE_CURRENT_SOURCE_DIR\\}))/g'" ${CMAKE_CURRENT_SOURCE_DIR}/rabbitmw-c/CMakeLists.txt)

add_subdirectory(rabbitmq-c)

#target_include_directories(rabbitmq-static PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/rabbitmq-c/librabbitmq)

target_include_directories(rabbitmq-static PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/rabbitmq-c/librabbitmq>)
set_target_properties(rabbitmq-static
        PROPERTIES POSITION_INDEPENDENT_CODE ON)