project(ps3tool)

find_package(Qt5Widgets)
find_package(Qt5Core)

set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

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
    rsx_dasm.cpp
    find_spu_elfs.cpp
    dump_instrdb.cpp
    print-gcmviz-parse.cpp
    trace_viz.cpp
    dasm.cpp
    unpack_trp.cpp
    split_log.cpp
)
add_executable(ps3tool ${SRC})
target_link_libraries(ps3tool ps3emu ps3tool-core ${Boost_LIBRARIES} z Qt5::Core Qt5::Widgets)
