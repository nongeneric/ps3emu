cmake_minimum_required(VERSION 3.4)

project(root)
include_directories(${root_SOURCE_DIR})

set(CMAKE_CXX_STANDARD 14)

add_definitions(-DBOOST_ALL_DYN_LINK)
set(Boost_USE_STATIC_LIBS       OFF)
set(Boost_USE_MULTITHREADED      ON)
set(Boost_USE_STATIC_RUNTIME    OFF)
find_package(Boost COMPONENTS 
    filesystem regex thread system
    chrono program_options REQUIRED)
include_directories(SYSTEM ${Boost_INCLUDE_DIRS})
link_directories(${Boost_LIBRARY_DIRS})

find_package(PkgConfig REQUIRED)
pkg_search_module(GLFW3 REQUIRED glfw3)
pkg_search_module(SQLITE3 REQUIRED sqlite3)
pkg_search_module(CRYPTO REQUIRED libcrypto)
pkg_search_module(PNG REQUIRED libpng)
pkg_search_module(ZLIB REQUIRED zlib)
pkg_search_module(FREETYPE2 REQUIRED freetype2)

set(CMAKE_AR gcc-ar)
set(CMAKE_RANLIB gcc-ranlib)

include_directories(SYSTEM ps3emu/libs/graphics/GL/include)
#-fsanitize=undefined
#-fsanitize=thread

set(CMAKE_CXX_FLAGS "-Wno-unknown-pragmas ${CMAKE_CXX_FLAGS}")
set(CMAKE_CXX_FLAGS "-Wno-format-security ${CMAKE_CXX_FLAGS}")
#set(CMAKE_CXX_FLAGS "-Wno-unused-const-variable ${CMAKE_CXX_FLAGS}")
set(CMAKE_CXX_FLAGS "-Wno-missing-braces ${CMAKE_CXX_FLAGS}")
set(CMAKE_CXX_FLAGS "-Wno-comment ${CMAKE_CXX_FLAGS}")

add_definitions(-DDEBUG)
add_definitions(-DTESTS)
set(CMAKE_CXX_FLAGS "-ggdb3 -O0 -Wall -Werror ${CMAKE_CXX_FLAGS}")

#add_definitions(-DNDEBUG)
#set(CMAKE_CXX_FLAGS "-O3 -march=native -Wall -Werror ${CMAKE_CXX_FLAGS}")

include_directories(SYSTEM $ENV{PS3_HOST_INCLUDE})
include_directories(SYSTEM $ENV{PS3_HOST_CG_INCLUDE})
include_directories(SYSTEM $ENV{PS3_TARGET_COMMON_INCLUDE})
include_directories(SYSTEM thirdparty/cereal/include)
include_directories(SYSTEM ${FREETYPE2_INCLUDE_DIRS})

add_subdirectory(ps3emu)
add_subdirectory(dbg-gui)
add_subdirectory(ps3run)
add_subdirectory(ps3tool)
add_subdirectory(tests)
add_subdirectory(gcmviz)
