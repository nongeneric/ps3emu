cmake_minimum_required(VERSION 3.14)

project(ps3emu-root)
include_directories(${ps3emu-root_SOURCE_DIR})

set(CMAKE_CXX_STANDARD 17)

add_definitions(-DBOOST_ALL_DYN_LINK)
set(Boost_USE_STATIC_LIBS       OFF)
set(Boost_USE_MULTITHREADED      ON)
set(Boost_USE_STATIC_RUNTIME    OFF)
find_package(Boost COMPONENTS 
    thread system
    chrono program_options context REQUIRED)
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
pkg_search_module(TBB REQUIRED tbb)
pkg_search_module(SDL2 REQUIRED sdl2)

set(CMAKE_AR gcc-ar)
set(CMAKE_RANLIB gcc-ranlib)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(EMU_CONFIGURATION_ADDRESS_SANITIZER "address sanitizer")
option(EMU_CONFIGURATION_UNDEFINED_SANITIZER "undefined sanitizer")
option(EMU_CONFIGURATION_THREAD_SANITIZER "thread sanitizer")
option(EMU_CONFIGURATION_DEBUG "debug")
option(EMU_CONFIGURATION_TESTS "tests")
option(EMU_CONFIGURATION_PAUSE "pause")
option(EMU_CONFIGURATION_LOG "log")
option(EMU_CONFIGURATION_EXECMAP "execmap")
option(EMU_CONFIGURATION_VTUNE "vtune")
option(EMU_CONFIGURATION_GLDEBUG "gldebug")
set(EMU_CONFIGURATION_BUILD_NAME "" CACHE STRING "build name used in rewriter binary names")

include_directories(SYSTEM ps3emu/libs/graphics/GL/include)

set(CMAKE_CXX_FLAGS "-Wno-unknown-pragmas ${CMAKE_CXX_FLAGS}")
set(CMAKE_CXX_FLAGS "-Wno-format-security ${CMAKE_CXX_FLAGS}")
set(CMAKE_CXX_FLAGS "-Wno-missing-braces ${CMAKE_CXX_FLAGS}")
set(CMAKE_CXX_FLAGS "-Wno-comment ${CMAKE_CXX_FLAGS}")
set(CMAKE_CXX_FLAGS "-Wall -Werror -march=native ${CMAKE_CXX_FLAGS}")

#set(CMAKE_CXX_FLAGS "-Wno-unused-const-variable ${CMAKE_CXX_FLAGS}")
#set(CMAKE_CXX_FLAGS "-fno-strict-aliasing ${CMAKE_CXX_FLAGS}")

if(${EMU_CONFIGURATION_ADDRESS_SANITIZER})
    set(CMAKE_CXX_FLAGS "-fsanitize=address ${CMAKE_CXX_FLAGS}")
endif()

if(${EMU_CONFIGURATION_UNDEFINED_SANITIZER})
    set(CMAKE_CXX_FLAGS "-fsanitize=undefined ${CMAKE_CXX_FLAGS}")
endif()

if(${EMU_CONFIGURATION_THREAD_SANITIZER})
    set(CMAKE_CXX_FLAGS "-fsanitize=thread ${CMAKE_CXX_FLAGS}")
endif()

if(${EMU_CONFIGURATION_VTUNE})
    include_directories(/opt/intel/vtune_amplifier/include)
    add_definitions(-DVTUNE_ENABLED)
else()
    add_definitions(-DINTEL_NO_ITTNOTIFY_API)
endif()

if(${EMU_CONFIGURATION_GLDEBUG})
    add_definitions(-DGL_DEBUG_ENABLED)
endif()

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
    #add_definitions(-DMEMORY_PROTECTION)
    
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

include_directories("${CMAKE_BINARY_DIR}")
include_directories("${CMAKE_SOURCE_DIR}")
include_directories(SYSTEM ${FREETYPE2_INCLUDE_DIRS})
include_directories(SYSTEM thirdparty/cereal/include)
include_directories(SYSTEM thirdparty/xxHash)
include_directories(SYSTEM thirdparty/range-v3/include)
include_directories(SYSTEM thirdparty/Catch2/single_include)

configure_file (
    "${CMAKE_SOURCE_DIR}/ps3emu/build-config.h.in"
    "${CMAKE_BINARY_DIR}/ps3emu/build-config.h"
)

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
