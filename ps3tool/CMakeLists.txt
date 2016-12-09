project(ps3tool)

set(SRC
    unsce.cpp
    shader_dasm.cpp
    restore_elf.cpp
    read_prx.cpp
    parse_spurs_trace.cpp
    rewrite.cpp
    prx_store.cpp
    ps3tool.h
    ps3tool.cpp
)
add_executable(ps3tool ${SRC})
target_link_libraries(ps3tool ps3emu ps3tool-core ${Boost_LIBRARIES} z)
