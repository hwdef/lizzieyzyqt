if(NOT DEFINED ROOT)
    message(FATAL_ERROR "ROOT is required")
endif()

if(NOT DEFINED TEST_BINARY_DIR)
    message(FATAL_ERROR "TEST_BINARY_DIR is required")
endif()

set(script_under_test "${ROOT}/cmake/NoDirectUiGtpCommands.cmake")
set(work_dir "${TEST_BINARY_DIR}/no-direct-ui-gtp-commands-script")
file(REMOVE_RECURSE "${work_dir}")
file(MAKE_DIRECTORY "${work_dir}")

function(write_text path text)
    get_filename_component(parent "${path}" DIRECTORY)
    file(MAKE_DIRECTORY "${parent}")
    file(WRITE "${path}" "${text}")
endfunction()

function(create_fake_root root_dir)
    file(REMOVE_RECURSE "${root_dir}")
    write_text("${root_dir}/src/ui/MainWindow.cpp" "#include \"MainWindow.h\"\nvoid ui_dispatch() { dispatchConsoleCommand(engine, command); }\n")
    write_text("${root_dir}/src/ui/MainWindow.h" "#pragma once\n")
    write_text("${root_dir}/src/app/Planner.cpp" "void app_plans() { lizzie::engine::GtpCommand{std::nullopt, \"kata-stop\", {}}; }\n")
    write_text("${root_dir}/src/main.cpp" "#include \"ui/MainWindow.h\"\nint main() { return 0; }\n")
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
    message(FATAL_ERROR "Clean fake root should pass direct UI GTP command scan: ${clean_output}${clean_error}")
endif()

set(ui_command_root "${work_dir}/ui-command")
create_fake_root("${ui_command_root}")
write_text("${ui_command_root}/src/ui/MainWindow.cpp" "void bad_ui() { lizzie::engine::GtpCommand{std::nullopt, \"clear_board\", {}}; }\n")
execute_process(
    COMMAND "${CMAKE_COMMAND}" "-DROOT=${ui_command_root}" -P "${script_under_test}"
    RESULT_VARIABLE ui_command_result
    OUTPUT_VARIABLE ui_command_output
    ERROR_VARIABLE ui_command_error
)
if(ui_command_result EQUAL 0)
    message(FATAL_ERROR "UI GtpCommand construction should fail the architecture scan")
endif()
if(NOT "${ui_command_output}${ui_command_error}" MATCHES "MainWindow[.]cpp")
    message(FATAL_ERROR "UI GtpCommand failure should name MainWindow.cpp: ${ui_command_output}${ui_command_error}")
endif()

set(ui_send_root "${work_dir}/ui-send")
create_fake_root("${ui_send_root}")
write_text("${ui_send_root}/src/ui/MainWindow.cpp" "void bad_ui() { engine.sendCommand(command); }\n")
execute_process(
    COMMAND "${CMAKE_COMMAND}" "-DROOT=${ui_send_root}" -P "${script_under_test}"
    RESULT_VARIABLE ui_send_result
    OUTPUT_VARIABLE ui_send_output
    ERROR_VARIABLE ui_send_error
)
if(ui_send_result EQUAL 0)
    message(FATAL_ERROR "UI direct sendCommand should fail the architecture scan")
endif()
if(NOT "${ui_send_output}${ui_send_error}" MATCHES "MainWindow[.]cpp")
    message(FATAL_ERROR "UI sendCommand failure should name MainWindow.cpp: ${ui_send_output}${ui_send_error}")
endif()

set(app_main_command_root "${work_dir}/app-main-command")
create_fake_root("${app_main_command_root}")
write_text("${app_main_command_root}/src/main.cpp" "int main() { lizzie::engine::GtpCommand{std::nullopt, \"clear_board\", {}}; return 0; }\n")
execute_process(
    COMMAND "${CMAKE_COMMAND}" "-DROOT=${app_main_command_root}" -P "${script_under_test}"
    RESULT_VARIABLE app_main_command_result
    OUTPUT_VARIABLE app_main_command_output
    ERROR_VARIABLE app_main_command_error
)
if(app_main_command_result EQUAL 0)
    message(FATAL_ERROR "App executable GtpCommand construction should fail the architecture scan")
endif()
if(NOT "${app_main_command_output}${app_main_command_error}" MATCHES "main[.]cpp")
    message(FATAL_ERROR "App executable GtpCommand failure should name main.cpp: ${app_main_command_output}${app_main_command_error}")
endif()

set(app_main_send_root "${work_dir}/app-main-send")
create_fake_root("${app_main_send_root}")
write_text("${app_main_send_root}/src/main.cpp" "int main() { return engine.sendCommand(command); }\n")
execute_process(
    COMMAND "${CMAKE_COMMAND}" "-DROOT=${app_main_send_root}" -P "${script_under_test}"
    RESULT_VARIABLE app_main_send_result
    OUTPUT_VARIABLE app_main_send_output
    ERROR_VARIABLE app_main_send_error
)
if(app_main_send_result EQUAL 0)
    message(FATAL_ERROR "App executable direct sendCommand should fail the architecture scan")
endif()
if(NOT "${app_main_send_output}${app_main_send_error}" MATCHES "main[.]cpp")
    message(FATAL_ERROR "App executable sendCommand failure should name main.cpp: ${app_main_send_output}${app_main_send_error}")
endif()

message(STATUS "no_direct_ui_gtp_commands_script_tests passed")
