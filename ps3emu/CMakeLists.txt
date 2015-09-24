cmake_minimum_required(VERSION 2.8.11)

project(ps3emu)

set(SRC
    ELFLoader.cpp
    ELFLoader.h
    PPU.cpp
    PPU.h
    ppu_dasm.cpp
    ppu_dasm.h
    ppu_dasm_forms.h
    ncall.cpp
    utils.h
    shader_dasm.h
    shader_dasm.cpp
    Rsx.h
    Rsx.cpp
    LocalMemory.h
    LocalMemory.cpp
)
add_library(ps3emu ${SRC})
target_link_libraries(ps3emu libs ${Boost_LIBRARIES} glfw pthread)
