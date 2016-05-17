project(libs)

set(SRC
    sys.cpp
    fs.cpp
    Controller.cpp
    libsysutil.cpp
    cellGame.cpp
    sceNp2.cpp
    sceNp.cpp
    cellSpurs.cpp
    sysSpu.cpp
    heap.cpp
    libpngdec.cpp
    sync/lwmutex.cpp
    sync/mutex.cpp
    sync/cond.cpp
    sync/lwcond.cpp
    sync/rwlock.cpp
    sync/queue.cpp
    graphics/gcm.cpp
    graphics/gcm_buffers.cpp
    graphics/sysutil_sysparam.cpp
    graphics/graphics.cpp
    graphics/GL/src/glad.c
)

add_library(libs ${SRC})
target_link_libraries(libs ps3emu)