project(dbg-gui)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt5Widgets)
find_package(Qt5Core)
set(SRC
    main.cpp
    MainWindow.h
    MainWindow.cpp
    MonospaceGrid.h
    MonospaceGrid.cpp
    DebuggerModel.h
    DebuggerModel.cpp
    CommandLineEdit.h
    CommandLineEdit.cpp
    DebugExpr.h
    DebugExpr.cpp
    Config.h
    Config.cpp
)
add_executable(dbg-gui ${SRC} ${QT_RESOURCES})
target_link_libraries(dbg-gui Qt5::Core Qt5::Widgets ps3tool-core ps3emu)
