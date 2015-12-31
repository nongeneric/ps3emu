cmake_minimum_required(VERSION 2.8.11)

project(libs)

set(SRC
    sys.cpp
    fs.cpp
    ConcurrentQueue.cpp
    Controller.cpp
    libsysutil.cpp
    cellGame.cpp
    graphics/gcm.cpp
    graphics/gcm_buffers.cpp
    graphics/sysutil_sysparam.cpp
    graphics/graphics.cpp
    graphics/GL/src/glad.c
)

add_library(libs ${SRC})
target_link_libraries(libs ps3emu)