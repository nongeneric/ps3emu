project(tests)

find_package(Qt5Core)

FILE(COPY binaries DESTINATION .)

set(SRC
    main.cpp
    PPUTests.cpp
    PPUDasmTests.cpp
    ShDasmTests.cpp
    RunnerTests.cpp
    GraphicsTests.cpp
    ShaderRewriterTests.cpp
    SPUChannelsTests.cpp
    EnumTests.cpp
    TextureCacheTests.cpp
    RewriterTests.cpp
    TestUtils.cpp
)

add_executable(tests ${SRC})
target_link_libraries(tests ps3emu Qt5::Core)
