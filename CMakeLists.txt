cmake_minimum_required(VERSION 2.8.11)

include_directories(${pdfshrink_SOURCE_DIR})

add_definitions(-DBOOST_LOG_DYN_LINK)
set(Boost_USE_STATIC_LIBS       OFF)
set(Boost_USE_MULTITHREADED      ON)
set(Boost_USE_STATIC_RUNTIME    OFF)
find_package(Boost COMPONENTS filesystem regex thread system log chrono REQUIRED)
include_directories(SYSTEM ${Boost_INCLUDE_DIRS})
link_directories(${Boost_LIBRARY_DIRS})

# glad --profile core --api gl=4.5 --generator=c --spec gl --no-loader --out-path GL --extensions=""
include_directories(SYSTEM libs/graphics/GL/include)

#-fsanitize=undefined
add_definitions(-DDEBUG)
set(CMAKE_CXX_FLAGS "-ggdb -O0 -Wall -Werror -std=c++14 ${CMAKE_CXX_FLAGS}")
#add_definitions(-DNDEBUG)
#set(CMAKE_CXX_FLAGS "-O2 -Wall -Werror -std=c++14 ${CMAKE_CXX_FLAGS}")

include_directories(SYSTEM $ENV{PS3_HOST_INCLUDE})
include_directories(SYSTEM $ENV{PS3_HOST_CG_INCLUDE})
include_directories(SYSTEM $ENV{PS3_TARGET_COMMON_INCLUDE})

add_subdirectory(ps3emu)
add_subdirectory(dbg-gui)
add_subdirectory(libs)
add_subdirectory(ps3run)
add_subdirectory(tests)