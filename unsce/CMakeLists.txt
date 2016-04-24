project(unsce)

set(SRC
    unsce.cpp
)
add_executable(unsce ${SRC})
target_link_libraries(unsce ps3emu ${Boost_LIBRARIES} z)