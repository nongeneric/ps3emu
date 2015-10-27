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
    shaders/shader_dasm.h
    shaders/shader_dasm.cpp
    shaders/ShaderRewriter.h
    shaders/ShaderRewriter.cpp
    shaders/ShaderGenerator.h
    shaders/ShaderGenerator.cpp
    shaders/AST.h
    shaders/AST.cpp
    Rsx.h
    Rsx.cpp
)
add_library(ps3emu ${SRC})
target_link_libraries(ps3emu libs ${Boost_LIBRARIES} glfw pthread)