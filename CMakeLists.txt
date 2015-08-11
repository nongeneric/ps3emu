cmake_minimum_required(VERSION 2.8.11)

project(pdfshrink)

include_directories(${pdfshrink_SOURCE_DIR})

# set up boost
set(Boost_USE_STATIC_LIBS       OFF)
set(Boost_USE_MULTITHREADED      ON)
set(Boost_USE_STATIC_RUNTIME    OFF)
find_package(Boost COMPONENTS system filesystem regex thread REQUIRED)
include_directories(SYSTEM ${Boost_INCLUDE_DIRS})
link_directories(${Boost_LIBRARY_DIRS})

add_definitions(-DDEBUG)
set(CMAKE_CXX_FLAGS "-ggdb -O0 -pedantic -Wall -Werror -std=c++1y ${CMAKE_CXX_FLAGS}")

add_subdirectory(disassm)
add_subdirectory(ps3emu)
add_subdirectory(dbg-gui)
add_subdirectory(tests)