cmake_minimum_required(VERSION 2.8.11)

project(ps3emu)

set(SRC
    ELFLoader.cpp
    ELFLoader.h
    ppu/ppu_dasm.cpp
    ppu/ppu_dasm.h
    ppu/ppu_dasm_forms.h
    ncall.cpp
    utils.h
    utils.cpp
    MainMemory.h
    MainMemory.cpp
    Process.h
    Process.cpp
    ppu/PPUThread.h
    ppu/PPUThread.cpp
    ppu/InterruptPPUThread.h
    ppu/InterruptPPUThread.cpp
    ppu/CallbackThread.h
    ppu/CallbackThread.cpp
    ContentManager.h
    ContentManager.cpp
    InternalMemoryManager.h
    InternalMemoryManager.cpp
    spu/SPUThread.cpp
    spu/SPUDasm.cpp
    shaders/shader_dasm.h
    shaders/shader_dasm.cpp
    shaders/ShaderRewriter.h
    shaders/ShaderRewriter.cpp
    shaders/ShaderGenerator.h
    shaders/ShaderGenerator.cpp
    shaders/AST.h
    shaders/AST.cpp
    rsx/Rsx.h
    rsx/Rsx.cpp
    rsx/RsxLoop.cpp
    rsx/GLTexture.cpp
    rsx/GLFramebuffer.cpp
    rsx/GLBuffer.cpp
    gcmviz/GcmDatabase.cpp
)
add_library(ps3emu ${SRC})
target_link_libraries(ps3emu libs ${Boost_LIBRARIES} glfw pthread crypto sqlite3)