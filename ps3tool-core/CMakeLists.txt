project(ps3tool-core)

set(SRC
    Rewriter.cpp
)

add_library(ps3tool-core SHARED ${SRC})
target_link_libraries(ps3tool-core ps3emu ${Boost_LIBRARIES} z)
