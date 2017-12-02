cmake_minimum_required(VERSION 3.8)

project(ps3emu-root)
include_directories(${ps3emu-root_SOURCE_DIR})

set(CMAKE_CXX_STANDARD 17)

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
pkg_search_module(PULSEAUDIO REQUIRED libpulse)
pkg_search_module(ICUUC REQUIRED icu-uc)

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
set(CMAKE_CXX_FLAGS "-Wall -Werror -march=native -msse2avx -rdynamic ${CMAKE_CXX_FLAGS}")
#set(CMAKE_CXX_FLAGS "-fno-strict-aliasing ${CMAKE_CXX_FLAGS}")

set(EMU_CONFIGURATION_DEBUG   1)
set(EMU_CONFIGURATION_TESTS   1)
set(EMU_CONFIGURATION_PAUSE   1)
set(EMU_CONFIGURATION_LOG     1)
set(EMU_CONFIGURATION_EXECMAP 0)

if(${EMU_CONFIGURATION_LOG})
    add_definitions(-DLOG_ENABLED)
endif()

if(${EMU_CONFIGURATION_EXECMAP})
    add_definitions(-DEXECMAP_ENABLED)
endif()

if(${EMU_CONFIGURATION_TESTS})
    # enable code required for running tests (small performance hit)
    add_definitions(-DTESTS)
endif()

if(${EMU_CONFIGURATION_DEBUG})
    add_definitions(-DDEBUG)

    # detect accesses to unallocated memory (massive performance hit)
    add_definitions(-DMEMORY_PROTECTION)
    
    set(CMAKE_CXX_FLAGS "-ggdb3 -O0 ${CMAKE_CXX_FLAGS}")
else()
    # disable all asserts (moderate performance hit)
    add_definitions(-DNDEBUG)
    
    set(CMAKE_CXX_FLAGS "-ggdb3 -O3 ${CMAKE_CXX_FLAGS}")
endif()

if(${EMU_CONFIGURATION_PAUSE})
    # enable thread pausing, required for the dbg-gui (moderate performance hit)
    add_definitions(-DDEBUGPAUSE)
endif()

include_directories(SYSTEM $ENV{PS3_HOST_INCLUDE})
include_directories(SYSTEM $ENV{PS3_HOST_CG_INCLUDE})
include_directories(SYSTEM $ENV{PS3_TARGET_COMMON_INCLUDE})
include_directories(SYSTEM thirdparty/cereal/include)
include_directories(SYSTEM thirdparty/xxHash)
include_directories(SYSTEM ${FREETYPE2_INCLUDE_DIRS})

add_subdirectory(ps3emu)
add_subdirectory(ps3tool-core)
add_subdirectory(ps3run)
add_subdirectory(ps3tool)
add_subdirectory(gcmviz)

if(${EMU_CONFIGURATION_PAUSE})
    add_subdirectory(dbg-gui)
endif()

if(${EMU_CONFIGURATION_TESTS})
    add_subdirectory(tests)
endif()
