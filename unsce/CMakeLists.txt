cmake_minimum_required(VERSION 2.8.11)

project(unsce)

set(SRC
    unsce.cpp
)
add_executable(unsce ${SRC})
target_link_libraries(unsce ps3emu ${Boost_LIBRARIES} crypto)