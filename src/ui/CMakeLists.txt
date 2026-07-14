add_library(lizzie_ui STATIC
    AnalysisGraphWidget.cpp
    BoardQuickItem.cpp
    EngineSettingsDialog.cpp
    GtpConsoleWidget.cpp
    MainWindow.cpp
)

target_include_directories(lizzie_ui
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(lizzie_ui
    PUBLIC
        lizzie_core
        lizzie_engine
        lizzie_app
        Qt6::Core
        Qt6::Widgets
        Qt6::Quick
        Qt6::QuickWidgets
)

target_compile_features(lizzie_ui PUBLIC cxx_std_20)
