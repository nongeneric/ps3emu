project(ps3tool)

set(SRC
    unsce.cpp
    shader_dasm.cpp
    restore_elf.cpp
    read_prx.cpp
    ps3tool.h
    ps3tool.cpp
)
add_executable(ps3tool ${SRC})
target_link_libraries(ps3tool ps3emu ${Boost_LIBRARIES} z)
