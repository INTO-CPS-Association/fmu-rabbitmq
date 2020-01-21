cmake_minimum_required(VERSION 3.7.2)

if (WIN32)
    # fix for mingw
    set(USE_SYSTEM_TZ_DB ON CACHE BOOL "" FORCE)
endif ()

add_subdirectory(date)
