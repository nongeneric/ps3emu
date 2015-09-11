cmake_minimum_required(VERSION 2.8.11)

project(tests)

find_package(Qt5Core)

FILE(COPY binaries DESTINATION .)

set(SRC
    main.cpp
    PPUTests.cpp
    PPUDasmTests.cpp
    PPUSysCallsTests.cpp
    ShDasmTests.cpp
    RunnerTests.cpp
)

add_executable(tests ${SRC})
target_link_libraries(tests ps3emu Qt5::Core)
