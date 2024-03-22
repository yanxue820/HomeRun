cmake_minimum_required(VERSION 3.13)
cmake_policy(SET CMP0080 NEW)
project(HashingTables)

set(CMAKE_CXX_STANDARD 17)
set(HASHING_TABLES_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

if (${CMAKE_BUILD_TYPE} STREQUAL Release)
    message("Release mode")

endif ()

set(CMAKE_CXX_FLAGS "-Wall -Wextra")

# DEPENDENCIES
find_package(OpenSSL REQUIRED)

find_package(fmt QUIET)
if (fmt_FOUND)
    message(STATUS "Found fmt")
elseif (NOT fmt_FOUND AND NOT TARGET fmt::fmt)
    message("fmt was not found: add fmt subdirectory")
    if (NOT EXISTS "${PROJECT_SOURCE_DIR}/extern/fmt/CMakeLists.txt")
        find_package(Git REQUIRED)
        message("initialize Git submodule: extern/fmt")
        execute_process(COMMAND git submodule update --init extern/fmt
                WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")
    endif ()
    set(BUILD_SHARED_LIBS ON)
    set(FMT_TEST OFF CACHE BOOL "Disable tests for the fmt library" FORCE)
    add_subdirectory(extern/fmt EXCLUDE_FROM_ALL)
endif ()


# LIBRARY

add_library(HashingTables STATIC
        common/hash_table_entry.cpp
        common/hashing.cpp
        cuckoo_hashing/cuckoo_hashing.cpp
        simple_hashing/simple_hashing.cpp
        )

target_compile_definitions(HashingTables PRIVATE FMT_HEADER_ONLY)

target_include_directories(HashingTables PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/extern/fmt/include)

target_link_libraries(HashingTables PRIVATE
        OpenSSL::SSL
        OpenSSL::Crypto
        )

option(HASHING_TABLES_BUILD_TESTS OFF)
option(HASHING_TABLES_BUILD_EXAMPLES OFF)


# TESTS

if (HASHING_TABLES_BUILD_TESTS)
    if (NOT EXISTS "${PROJECT_SOURCE_DIR}/extern/googletest/CMakeLists.txt")
        find_package(Git REQUIRED)
        message("initialize Git submodule: extern/googletest")
        execute_process(COMMAND git submodule update --init extern/googletest
                WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")
    endif ()

    set(BUILD_GMOCK OFF)
    add_subdirectory(extern/googletest EXCLUDE_FROM_ALL)
    add_subdirectory(src/test)
endif (HASHING_TABLES_BUILD_TESTS)


# EXAMPLES

if (HASHING_TABLES_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif (HASHING_TABLES_BUILD_EXAMPLES)