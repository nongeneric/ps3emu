project(ps3run)

set(SRC
    ps3run.cpp
)
add_executable(ps3run ${SRC})
target_link_libraries(ps3run ps3emu)