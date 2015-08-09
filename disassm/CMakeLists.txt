cmake_minimum_required(VERSION 2.8.11)

project(disassm)

set(SRC
disasm.cpp
disasm.h
ppu_dasm.cpp
ppu_dasm.h
)
add_library(disassm ${SRC})
