set_source_files_properties(
    ThreadedAnalysisProcess.h
    ThreadedKataGoProcess.h
    PROPERTIES SKIP_AUTOMOC ON
)

qt6_wrap_cpp(lizzie_engine_threaded_moc_sources
    ThreadedAnalysisProcess.h
    ThreadedKataGoProcess.h
)

add_library(lizzie_engine STATIC
    AnalysisJson.cpp
    AnalysisProcess.cpp
    BatchAnalysis.cpp
    GtpCommandQueue.cpp
    GtpProtocol.cpp
    KataGoConfig.cpp
    KataGoProcess.cpp
    KataGoRules.cpp
    PositionSync.cpp
    ProcessEnvironment.cpp
    ProcessDiagnostics.cpp
    RealtimeAnalysis.cpp
    ThreadedAnalysisProcess.cpp
    ThreadedAnalysisProcess.h
    ThreadedKataGoProcess.cpp
    ThreadedKataGoProcess.h
    ${lizzie_engine_threaded_moc_sources}
)

target_include_directories(lizzie_engine
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(lizzie_engine
    PUBLIC
        lizzie_core
        Qt6::Core
)

target_compile_features(lizzie_engine PUBLIC cxx_std_20)
