cmake_minimum_required(VERSION 2.8.11)

project(ps3emu)

set(SRC
main.cpp
ELFLoader.cpp
ELFLoader.h
PPU.cpp
PPU.h
)
add_executable(ps3emu ${SRC})
target_link_libraries(ps3emu disassm)
