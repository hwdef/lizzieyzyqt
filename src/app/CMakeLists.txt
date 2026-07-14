add_library(lizzie_app STATIC
    AnalysisStore.cpp
    AnalysisStreamGate.cpp
    AppSettings.cpp
    BatchAnalysisRun.cpp
    BatchAnalysisTracker.cpp
    EngineAutomation.cpp
    EngineCommandDispatcher.cpp
    EngineCommandPlanner.cpp
    EngineManager.cpp
    FileWrite.cpp
    FirstRun.cpp
    GameDocumentSession.cpp
    GameEditor.cpp
    GameSetup.cpp
    LegacyConfigImport.cpp
    PositionRequestGuard.cpp
    SessionSettings.cpp
    SettingsRuntime.cpp
    WindowPlacement.cpp
)

target_include_directories(lizzie_app
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(lizzie_app
    PUBLIC
        lizzie_core
        lizzie_engine
        Qt6::Core
        Qt6::Gui
)

target_compile_features(lizzie_app PUBLIC cxx_std_20)
