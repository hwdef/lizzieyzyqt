if(NOT DEFINED ROOT)
    message(FATAL_ERROR "ROOT is required")
endif()

if(NOT DEFINED TEST_BINARY_DIR)
    message(FATAL_ERROR "TEST_BINARY_DIR is required")
endif()

set(script_under_test "${ROOT}/cmake/LayerBoundaryCheck.cmake")
set(work_dir "${TEST_BINARY_DIR}/layer-boundary-check-script")
file(REMOVE_RECURSE "${work_dir}")
file(MAKE_DIRECTORY "${work_dir}")

function(write_text path text)
    get_filename_component(parent "${path}" DIRECTORY)
    file(MAKE_DIRECTORY "${parent}")
    file(WRITE "${path}" "${text}")
endfunction()

function(create_fake_root root_dir)
    file(REMOVE_RECURSE "${root_dir}")
    write_text("${root_dir}/src/core/Types.h" "#pragma once\nnamespace lizzie::core { struct Point { int x = 0; int y = 0; }; }\n")
    write_text("${root_dir}/src/core/Types.cpp" "#include \"Types.h\"\n")
    write_text("${root_dir}/src/core/CMakeLists.txt" "add_library(lizzie_core STATIC Types.cpp)\n")
    write_text("${root_dir}/src/engine/KataGoProcess.h" "#pragma once\n#include \"Types.h\"\n#include <QObject>\nnamespace lizzie::engine { class KataGoProcess final : public QObject { Q_OBJECT }; }\n")
    write_text("${root_dir}/src/engine/KataGoProcess.cpp" "#include \"KataGoProcess.h\"\n#include <QProcess>\n")
    write_text("${root_dir}/src/engine/ThreadedKataGoProcess.h" "#pragma once\nclass QThread;\nnamespace lizzie::engine { class ThreadedKataGoProcess { bool workerThreadIsCurrentThread() const; }; }\n")
    write_text("${root_dir}/src/engine/ThreadedAnalysisProcess.h" "#pragma once\nclass QThread;\nnamespace lizzie::engine { class ThreadedAnalysisProcess { bool workerThreadIsCurrentThread() const; }; }\n")
    write_text("${root_dir}/src/engine/CMakeLists.txt" "target_link_libraries(lizzie_engine PUBLIC lizzie_core Qt6::Core)\n")
    write_text("${root_dir}/src/app/WindowPlacement.h" "#pragma once\n#include <QRect>\n")
    write_text("${root_dir}/src/app/WindowPlacement.cpp" "#include \"WindowPlacement.h\"\n#include <QGuiApplication>\n#include <QScreen>\n")
    write_text("${root_dir}/src/app/CMakeLists.txt" "target_link_libraries(lizzie_app PUBLIC lizzie_core lizzie_engine Qt6::Core Qt6::Gui)\n")
    write_text("${root_dir}/src/main.cpp" "#include <QApplication>\n#include \"ui/MainWindow.h\"\nint main() { return 0; }\n")
    write_text("${root_dir}/src/ui/MainWindow.cpp" "#include \"MainWindow.h\"\n#include <QWidget>\n")
    write_text("${root_dir}/src/ui/MainWindow.h" "#pragma once\n")
endfunction()

set(clean_root "${work_dir}/clean")
create_fake_root("${clean_root}")
execute_process(
    COMMAND "${CMAKE_COMMAND}" "-DROOT=${clean_root}" -P "${script_under_test}"
    RESULT_VARIABLE clean_result
    OUTPUT_VARIABLE clean_output
    ERROR_VARIABLE clean_error
)
if(NOT clean_result EQUAL 0)
    message(FATAL_ERROR "Clean fake root should pass layer boundary scan: ${clean_output}${clean_error}")
endif()

set(core_qt_root "${work_dir}/core-qt")
create_fake_root("${core_qt_root}")
write_text("${core_qt_root}/src/core/Types.h" "#pragma once\n#include <QString>\nnamespace lizzie::core { struct Label { QString text; }; }\n")
execute_process(
    COMMAND "${CMAKE_COMMAND}" "-DROOT=${core_qt_root}" -P "${script_under_test}"
    RESULT_VARIABLE core_qt_result
    OUTPUT_VARIABLE core_qt_output
    ERROR_VARIABLE core_qt_error
)
if(core_qt_result EQUAL 0)
    message(FATAL_ERROR "Core Qt include should fail the layer boundary scan")
endif()
if(NOT "${core_qt_output}${core_qt_error}" MATCHES "src/core/Types[.]h")
    message(FATAL_ERROR "Core Qt include failure should name Types.h: ${core_qt_output}${core_qt_error}")
endif()

set(core_engine_root "${work_dir}/core-engine")
create_fake_root("${core_engine_root}")
write_text("${core_engine_root}/src/core/CMakeLists.txt" "target_link_libraries(lizzie_core PUBLIC lizzie_engine Qt6::Core)\n")
execute_process(
    COMMAND "${CMAKE_COMMAND}" "-DROOT=${core_engine_root}" -P "${script_under_test}"
    RESULT_VARIABLE core_engine_result
    OUTPUT_VARIABLE core_engine_output
    ERROR_VARIABLE core_engine_error
)
if(core_engine_result EQUAL 0)
    message(FATAL_ERROR "Core Qt/CMake dependency should fail the layer boundary scan")
endif()
if(NOT "${core_engine_output}${core_engine_error}" MATCHES "src/core/CMakeLists[.]txt")
    message(FATAL_ERROR "Core CMake dependency failure should name CMakeLists.txt: ${core_engine_output}${core_engine_error}")
endif()

set(engine_widgets_root "${work_dir}/engine-widgets")
create_fake_root("${engine_widgets_root}")
write_text("${engine_widgets_root}/src/engine/KataGoProcess.cpp" "#include \"KataGoProcess.h\"\n#include <QWidget>\n")
execute_process(
    COMMAND "${CMAKE_COMMAND}" "-DROOT=${engine_widgets_root}" -P "${script_under_test}"
    RESULT_VARIABLE engine_widgets_result
    OUTPUT_VARIABLE engine_widgets_output
    ERROR_VARIABLE engine_widgets_error
)
if(engine_widgets_result EQUAL 0)
    message(FATAL_ERROR "Engine Widgets include should fail the layer boundary scan")
endif()
if(NOT "${engine_widgets_output}${engine_widgets_error}" MATCHES "src/engine/KataGoProcess[.]cpp")
    message(FATAL_ERROR "Engine Widgets failure should name KataGoProcess.cpp: ${engine_widgets_output}${engine_widgets_error}")
endif()

set(engine_ui_root "${work_dir}/engine-ui")
create_fake_root("${engine_ui_root}")
write_text("${engine_ui_root}/src/engine/KataGoProcess.h" "#pragma once\n#include \"../ui/MainWindow.h\"\nnamespace lizzie::engine { struct BadUiBridge {}; }\n")
execute_process(
    COMMAND "${CMAKE_COMMAND}" "-DROOT=${engine_ui_root}" -P "${script_under_test}"
    RESULT_VARIABLE engine_ui_result
    OUTPUT_VARIABLE engine_ui_output
    ERROR_VARIABLE engine_ui_error
)
if(engine_ui_result EQUAL 0)
    message(FATAL_ERROR "Engine UI include should fail the layer boundary scan")
endif()
if(NOT "${engine_ui_output}${engine_ui_error}" MATCHES "src/engine/KataGoProcess[.]h")
    message(FATAL_ERROR "Engine UI include failure should name KataGoProcess.h: ${engine_ui_output}${engine_ui_error}")
endif()

set(app_widgets_root "${work_dir}/app-widgets")
create_fake_root("${app_widgets_root}")
write_text("${app_widgets_root}/src/app/SettingsRuntime.cpp" "#include \"SettingsRuntime.h\"\n#include <QWidget>\n")
execute_process(
    COMMAND "${CMAKE_COMMAND}" "-DROOT=${app_widgets_root}" -P "${script_under_test}"
    RESULT_VARIABLE app_widgets_result
    OUTPUT_VARIABLE app_widgets_output
    ERROR_VARIABLE app_widgets_error
)
if(app_widgets_result EQUAL 0)
    message(FATAL_ERROR "App Widgets include should fail the layer boundary scan")
endif()
if(NOT "${app_widgets_output}${app_widgets_error}" MATCHES "src/app/SettingsRuntime[.]cpp")
    message(FATAL_ERROR "App Widgets failure should name SettingsRuntime.cpp: ${app_widgets_output}${app_widgets_error}")
endif()

set(app_main_root "${work_dir}/app-main")
create_fake_root("${app_main_root}")
write_text("${app_main_root}/src/app/main.cpp" "#include <QApplication>\nint main() { return 0; }\n")
execute_process(
    COMMAND "${CMAKE_COMMAND}" "-DROOT=${app_main_root}" -P "${script_under_test}"
    RESULT_VARIABLE app_main_result
    OUTPUT_VARIABLE app_main_output
    ERROR_VARIABLE app_main_error
)
if(app_main_result EQUAL 0)
    message(FATAL_ERROR "Executable entry under app layer should fail the layer boundary scan")
endif()
if(NOT "${app_main_output}${app_main_error}" MATCHES "src/app/main[.]cpp")
    message(FATAL_ERROR "App-layer executable entry failure should name main.cpp: ${app_main_output}${app_main_error}")
endif()

set(app_ui_root "${work_dir}/app-ui")
create_fake_root("${app_ui_root}")
write_text("${app_ui_root}/src/app/EngineManager.h" "#pragma once\n#include \"../ui/MainWindow.h\"\nnamespace lizzie::app { struct BadUiBridge {}; }\n")
execute_process(
    COMMAND "${CMAKE_COMMAND}" "-DROOT=${app_ui_root}" -P "${script_under_test}"
    RESULT_VARIABLE app_ui_result
    OUTPUT_VARIABLE app_ui_output
    ERROR_VARIABLE app_ui_error
)
if(app_ui_result EQUAL 0)
    message(FATAL_ERROR "App UI include should fail the layer boundary scan")
endif()
if(NOT "${app_ui_output}${app_ui_error}" MATCHES "src/app/EngineManager[.]h")
    message(FATAL_ERROR "App UI include failure should name EngineManager.h: ${app_ui_output}${app_ui_error}")
endif()

set(app_quick_link_root "${work_dir}/app-quick-link")
create_fake_root("${app_quick_link_root}")
write_text("${app_quick_link_root}/src/app/CMakeLists.txt" "target_link_libraries(lizzie_app PUBLIC lizzie_core lizzie_engine Qt6::Core Qt6::Quick)\n")
execute_process(
    COMMAND "${CMAKE_COMMAND}" "-DROOT=${app_quick_link_root}" -P "${script_under_test}"
    RESULT_VARIABLE app_quick_link_result
    OUTPUT_VARIABLE app_quick_link_output
    ERROR_VARIABLE app_quick_link_error
)
if(app_quick_link_result EQUAL 0)
    message(FATAL_ERROR "App Qt Quick link should fail the layer boundary scan")
endif()
if(NOT "${app_quick_link_output}${app_quick_link_error}" MATCHES "src/app/CMakeLists[.]txt")
    message(FATAL_ERROR "App Qt Quick link failure should name CMakeLists.txt: ${app_quick_link_output}${app_quick_link_error}")
endif()

set(ui_qprocess_root "${work_dir}/ui-qprocess")
create_fake_root("${ui_qprocess_root}")
write_text("${ui_qprocess_root}/src/ui/MainWindow.cpp" "#include \"MainWindow.h\"\n#include <QProcess>\nvoid on_exit(QProcess::ExitStatus) {}\n")
execute_process(
    COMMAND "${CMAKE_COMMAND}" "-DROOT=${ui_qprocess_root}" -P "${script_under_test}"
    RESULT_VARIABLE ui_qprocess_result
    OUTPUT_VARIABLE ui_qprocess_output
    ERROR_VARIABLE ui_qprocess_error
)
if(ui_qprocess_result EQUAL 0)
    message(FATAL_ERROR "UI QProcess dependency should fail the layer boundary scan")
endif()
if(NOT "${ui_qprocess_output}${ui_qprocess_error}" MATCHES "src/ui/MainWindow[.]cpp")
    message(FATAL_ERROR "UI QProcess failure should name MainWindow.cpp: ${ui_qprocess_output}${ui_qprocess_error}")
endif()

set(app_main_qprocess_root "${work_dir}/app-main-qprocess")
create_fake_root("${app_main_qprocess_root}")
write_text("${app_main_qprocess_root}/src/main.cpp" "#include <QApplication>\n#include <QProcess>\nint main() { QProcess process; return 0; }\n")
execute_process(
    COMMAND "${CMAKE_COMMAND}" "-DROOT=${app_main_qprocess_root}" -P "${script_under_test}"
    RESULT_VARIABLE app_main_qprocess_result
    OUTPUT_VARIABLE app_main_qprocess_output
    ERROR_VARIABLE app_main_qprocess_error
)
if(app_main_qprocess_result EQUAL 0)
    message(FATAL_ERROR "App executable QProcess dependency should fail the layer boundary scan")
endif()
if(NOT "${app_main_qprocess_output}${app_main_qprocess_error}" MATCHES "src/main[.]cpp")
    message(FATAL_ERROR "App executable QProcess failure should name main.cpp: ${app_main_qprocess_output}${app_main_qprocess_error}")
endif()

set(ui_qthread_root "${work_dir}/ui-qthread")
create_fake_root("${ui_qthread_root}")
write_text("${ui_qthread_root}/src/ui/MainWindow.cpp" "#include \"MainWindow.h\"\n#include <QThread>\nvoid bad_thread() { QThread::currentThread(); }\n")
execute_process(
    COMMAND "${CMAKE_COMMAND}" "-DROOT=${ui_qthread_root}" -P "${script_under_test}"
    RESULT_VARIABLE ui_qthread_result
    OUTPUT_VARIABLE ui_qthread_output
    ERROR_VARIABLE ui_qthread_error
)
if(ui_qthread_result EQUAL 0)
    message(FATAL_ERROR "UI QThread dependency should fail the layer boundary scan")
endif()
if(NOT "${ui_qthread_output}${ui_qthread_error}" MATCHES "src/ui/MainWindow[.]cpp")
    message(FATAL_ERROR "UI QThread failure should name MainWindow.cpp: ${ui_qthread_output}${ui_qthread_error}")
endif()

set(engine_public_thread_root "${work_dir}/engine-public-thread")
create_fake_root("${engine_public_thread_root}")
write_text("${engine_public_thread_root}/src/engine/ThreadedKataGoProcess.h" "#pragma once\n#include <QThread>\nnamespace lizzie::engine { class ThreadedKataGoProcess { const QThread* workerThread() const; }; }\n")
execute_process(
    COMMAND "${CMAKE_COMMAND}" "-DROOT=${engine_public_thread_root}" -P "${script_under_test}"
    RESULT_VARIABLE engine_public_thread_result
    OUTPUT_VARIABLE engine_public_thread_output
    ERROR_VARIABLE engine_public_thread_error
)
if(engine_public_thread_result EQUAL 0)
    message(FATAL_ERROR "Engine public QThread API should fail the layer boundary scan")
endif()
if(NOT "${engine_public_thread_output}${engine_public_thread_error}" MATCHES "src/engine/ThreadedKataGoProcess[.]h")
    message(FATAL_ERROR "Engine public QThread failure should name ThreadedKataGoProcess.h: ${engine_public_thread_output}${engine_public_thread_error}")
endif()

set(app_private_engine_root "${work_dir}/app-private-engine")
create_fake_root("${app_private_engine_root}")
write_text("${app_private_engine_root}/src/app/EngineManager.cpp" "#include \"EngineManager.h\"\n#include \"KataGoProcess.h\"\n")
execute_process(
    COMMAND "${CMAKE_COMMAND}" "-DROOT=${app_private_engine_root}" -P "${script_under_test}"
    RESULT_VARIABLE app_private_engine_result
    OUTPUT_VARIABLE app_private_engine_output
    ERROR_VARIABLE app_private_engine_error
)
if(app_private_engine_result EQUAL 0)
    message(FATAL_ERROR "App engine-private process include should fail the layer boundary scan")
endif()
if(NOT "${app_private_engine_output}${app_private_engine_error}" MATCHES "src/app/EngineManager[.]cpp")
    message(FATAL_ERROR "App engine-private include failure should name EngineManager.cpp: ${app_private_engine_output}${app_private_engine_error}")
endif()

set(ui_private_engine_root "${work_dir}/ui-private-engine")
create_fake_root("${ui_private_engine_root}")
write_text("${ui_private_engine_root}/src/ui/MainWindow.cpp" "#include \"MainWindow.h\"\n#include \"ProcessDiagnostics.h\"\n")
execute_process(
    COMMAND "${CMAKE_COMMAND}" "-DROOT=${ui_private_engine_root}" -P "${script_under_test}"
    RESULT_VARIABLE ui_private_engine_result
    OUTPUT_VARIABLE ui_private_engine_output
    ERROR_VARIABLE ui_private_engine_error
)
if(ui_private_engine_result EQUAL 0)
    message(FATAL_ERROR "UI engine-private process include should fail the layer boundary scan")
endif()
if(NOT "${ui_private_engine_output}${ui_private_engine_error}" MATCHES "src/ui/MainWindow[.]cpp")
    message(FATAL_ERROR "UI engine-private include failure should name MainWindow.cpp: ${ui_private_engine_output}${ui_private_engine_error}")
endif()

message(STATUS "layer_boundary_check_script_tests passed")
