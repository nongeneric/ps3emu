cmake_minimum_required(VERSION 2.8.11)

project(libs)

# glad --profile core --api gl=4.5 --generator=c --spec gl --no-loader --out-path GL --extensions=""
include_directories(SYSTEM graphics/GL/include)

set(SRC
    sys.cpp
    sys.h
    graphics/gcm.cpp
    graphics/gcm_buffers.cpp
    graphics/sysutil_sysparam.cpp
    graphics/graphics.cpp
    graphics/GL/src/glad.c
)
add_library(libs ${SRC})