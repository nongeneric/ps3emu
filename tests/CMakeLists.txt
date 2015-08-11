cmake_minimum_required(VERSION 2.8.11)

project(tests)

set(SRC
    main.cpp
    PPUTests.cpp
    PPUDasmTests.cpp
)
add_executable(tests ${SRC})
target_link_libraries(tests disassm ps3emu)
