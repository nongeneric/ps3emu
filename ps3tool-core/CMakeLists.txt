project(ps3tool-core)

set(SRC
    Rewriter.cpp
    GraphTools.cpp
    NinjaScript.cpp
    Rewriter.h
    GraphTools.h
    NinjaScript.h
)

add_library(ps3tool-core SHARED ${SRC})
target_link_libraries(ps3tool-core ps3emu ${Boost_LIBRARIES} z metis)
