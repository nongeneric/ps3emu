cmake_minimum_required(VERSION 2.8.11)

project(shader_dasm)

set(SRC
    shader_dasm.cpp
)
add_executable(shader_dasm ${SRC})
target_link_libraries(shader_dasm ps3emu)