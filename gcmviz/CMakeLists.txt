project(gcmviz)

find_package(Qt5Widgets)
find_package(Qt5Core)

set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

qt5_wrap_ui(MainWindow_ui_moc ${PROJECT_SOURCE_DIR}/MainWindow.ui)
qt5_wrap_ui(ImageView_ui_moc ${PROJECT_SOURCE_DIR}/ImageView.ui)
qt5_wrap_ui(OpenGLPreview_ui_moc ${PROJECT_SOURCE_DIR}/OpenGLPreview.ui)

set(SRC
    main.cpp
    ContextTables.cpp
    MainWindowModel.cpp
    OpenGLPreview.cpp
    OpenGLPreviewWidget.cpp
    ${MainWindow_ui_moc}
    ${ImageView_ui_moc}
    ${OpenGLPreview_ui_moc}
)
add_executable(gcmviz ${SRC} ${QT_RESOURCES})
target_link_libraries(gcmviz Qt5::Core Qt5::Widgets ps3emu dl)
