if(NOT DEFINED ROOT)
    message(FATAL_ERROR "ROOT must point to the Qt source tree")
endif()

if(EXISTS "${ROOT}/src/app/main.cpp")
    message(FATAL_ERROR "app layer boundary violation found in src/app/main.cpp: executable entry point must live outside the app library layer")
endif()

function(check_layer_files layer_name files patterns)
    foreach(file IN LISTS ${files})
        file(READ "${file}" content)
        foreach(pattern IN LISTS ${patterns})
            if(content MATCHES "${pattern}")
                file(RELATIVE_PATH relative_file "${ROOT}" "${file}")
                message(FATAL_ERROR "${layer_name} layer boundary violation found in ${relative_file}")
            endif()
        endforeach()
    endforeach()
endfunction()

file(GLOB_RECURSE core_files
    LIST_DIRECTORIES false
    "${ROOT}/src/core/*.cpp"
    "${ROOT}/src/core/*.h"
    "${ROOT}/src/core/*.hpp"
)
list(APPEND core_files "${ROOT}/src/core/CMakeLists.txt")

file(GLOB_RECURSE engine_files
    LIST_DIRECTORIES false
    "${ROOT}/src/engine/*.cpp"
    "${ROOT}/src/engine/*.h"
    "${ROOT}/src/engine/*.hpp"
)
list(APPEND engine_files "${ROOT}/src/engine/CMakeLists.txt")

file(GLOB_RECURSE app_files
    LIST_DIRECTORIES false
    "${ROOT}/src/app/*.cpp"
    "${ROOT}/src/app/*.h"
    "${ROOT}/src/app/*.hpp"
)
list(APPEND app_files "${ROOT}/src/app/CMakeLists.txt")

file(GLOB_RECURSE ui_files
    LIST_DIRECTORIES false
    "${ROOT}/src/ui/*.cpp"
    "${ROOT}/src/ui/*.h"
    "${ROOT}/src/ui/*.hpp"
    "${ROOT}/src/ui/*.qml"
)

set(executable_entry_files)
if(EXISTS "${ROOT}/src/main.cpp")
    list(APPEND executable_entry_files "${ROOT}/src/main.cpp")
endif()

set(non_engine_process_files ${core_files} ${app_files} ${ui_files} ${executable_entry_files})
list(REMOVE_DUPLICATES non_engine_process_files)

set(app_ui_engine_private_api_files ${app_files} ${ui_files} ${executable_entry_files})
list(REMOVE_DUPLICATES app_ui_engine_private_api_files)

set(threaded_engine_public_headers
    "${ROOT}/src/engine/ThreadedAnalysisProcess.h"
    "${ROOT}/src/engine/ThreadedKataGoProcess.h"
)

set(core_forbidden_patterns
    "#[ \t]*include[ \t]*[<\"]Q[A-Za-z0-9_./-]*[>\"]"
    "Qt6::"
    "Q_OBJECT"
    "lizzie::(engine|app|ui)"
    "#[ \t]*include[ \t]*[<\"][.][.]/(engine|app|ui)/"
    "#[ \t]*include[ \t]*[<\"](engine|app|ui)/"
)

set(engine_forbidden_patterns
    "#[ \t]*include[ \t]*[<\"]Qt(Gui|Widgets|Quick|QuickWidgets|Qml)[^>\"]*[>\"]"
    "#[ \t]*include[ \t]*[<\"]Q(Application|GuiApplication|Widget|MainWindow|DockWidget|Action|Menu|MenuBar|ToolBar|Quick|Qml|Painter|Pixmap|Image|Screen|Window|OpenGL|SurfaceFormat|Palette|Font|Style|HeaderView|PushButton|TableWidget|TreeWidget)[A-Za-z0-9_./-]*[>\"]"
    "Qt6::(Gui|Widgets|Quick|QuickWidgets|Qml)"
    "lizzie::(app|ui)"
    "#[ \t]*include[ \t]*[<\"][.][.]/(app|ui)/"
    "#[ \t]*include[ \t]*[<\"](app|ui)/"
    "(MainWindow|BoardQuickItem|AnalysisGraphWidget|GtpConsoleWidget|EngineSettingsDialog)"
)

set(app_forbidden_patterns
    "#[ \t]*include[ \t]*[<\"]Qt(Widgets|Quick|QuickWidgets|Qml)[^>\"]*[>\"]"
    "#[ \t]*include[ \t]*[<\"]Q(Application|Widget|MainWindow|DockWidget|Quick|Qml|Menu|MenuBar|ToolBar|TableWidget|TreeWidget|HeaderView|FileDialog|PushButton|Style|Painter|Pixmap|OpenGL|SurfaceFormat|SGRendererInterface)[A-Za-z0-9_./-]*[>\"]"
    "Qt6::(Widgets|Quick|QuickWidgets|Qml)"
    "lizzie::ui"
    "#[ \t]*include[ \t]*[<\"][.][.]/ui/"
    "#[ \t]*include[ \t]*[<\"]ui/"
    "(MainWindow|BoardQuickItem|AnalysisGraphWidget|GtpConsoleWidget|EngineSettingsDialog)"
)

set(non_engine_process_thread_forbidden_patterns
    "#[ \t]*include[ \t]*[<\"]QProcess[>\"]"
    "QProcess::"
    "#[ \t]*include[ \t]*[<\"]QThread[>\"]"
    "QThread::"
    "#[ \t]*include[ \t]*[<\"]thread[>\"]"
    "std::thread"
    "QtConcurrent"
)

set(engine_public_thread_forbidden_patterns
    "#[ \t]*include[ \t]*[<\"]QThread[>\"]"
    "const[ \t]+QThread[ \t]*[*&]"
    "QThread[ \t]*[*&][ \t]*workerThread"
)

set(app_ui_engine_private_api_forbidden_patterns
    "#[ \t]*include[ \t]*[<\"]([A-Za-z0-9_./-]*/)?(AnalysisProcess|GtpCommandQueue|KataGoProcess|ProcessDiagnostics)[.]h[>\"]"
)

check_layer_files("core" core_files core_forbidden_patterns)
check_layer_files("engine" engine_files engine_forbidden_patterns)
check_layer_files("app" app_files app_forbidden_patterns)
check_layer_files("non-engine process/thread" non_engine_process_files non_engine_process_thread_forbidden_patterns)
check_layer_files("engine public thread API" threaded_engine_public_headers engine_public_thread_forbidden_patterns)
check_layer_files("app/UI engine private API" app_ui_engine_private_api_files app_ui_engine_private_api_forbidden_patterns)

message(STATUS "Core, engine, app, executable-entry, non-engine process/thread, and app/UI engine-private API boundaries passed under ${ROOT}/src")
