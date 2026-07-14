if(NOT DEFINED ROOT)
    message(FATAL_ERROR "ROOT is required")
endif()

if(NOT DEFINED TEST_BINARY_DIR)
    message(FATAL_ERROR "TEST_BINARY_DIR is required")
endif()

set(script_under_test "${ROOT}/cmake/FirstReleaseScopeCheck.cmake")
set(work_dir "${TEST_BINARY_DIR}/first-release-scope-check-script")
file(REMOVE_RECURSE "${work_dir}")
file(MAKE_DIRECTORY "${work_dir}")

function(write_text path text)
    get_filename_component(parent "${path}" DIRECTORY)
    file(MAKE_DIRECTORY "${parent}")
    file(WRITE "${path}" "${text}")
endfunction()

function(create_fake_root root_dir)
    file(REMOVE_RECURSE "${root_dir}")
    write_text("${root_dir}/src/CMakeLists.txt" "add_library(fake_app app/Main.cpp)\n")
    write_text("${root_dir}/src/app/Main.cpp" "void configure_katago() {}\n")
    write_text(
        "${root_dir}/src/app/LegacyConfigImport.cpp"
        "const char* note = \"Skipped Java SSH engine entry\";\nbool useJavaSSH = false;\n")
    write_text("${root_dir}/src/ui/MainWindow.cpp" "void show_main_window() {}\n")
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
    message(FATAL_ERROR "Clean fake root should pass first-release scope scan: ${clean_output}${clean_error}")
endif()

set(remote_ssh_path_root "${work_dir}/remote-ssh-path")
create_fake_root("${remote_ssh_path_root}")
write_text("${remote_ssh_path_root}/src/engine/RemoteSshEngine.cpp" "void configure_katago() {}\n")
execute_process(
    COMMAND "${CMAKE_COMMAND}" "-DROOT=${remote_ssh_path_root}" -P "${script_under_test}"
    RESULT_VARIABLE remote_ssh_path_result
    OUTPUT_VARIABLE remote_ssh_path_output
    ERROR_VARIABLE remote_ssh_path_error
)
if(remote_ssh_path_result EQUAL 0)
    message(FATAL_ERROR "Remote SSH implementation path should fail the first-release scope scan")
endif()
if(NOT "${remote_ssh_path_output}${remote_ssh_path_error}" MATCHES "RemoteSshEngine\\.cpp")
    message(FATAL_ERROR "remote-ssh path failure should name RemoteSshEngine.cpp: ${remote_ssh_path_output}${remote_ssh_path_error}")
endif()

set(online_board_declaration_root "${work_dir}/online-board-declaration")
create_fake_root("${online_board_declaration_root}")
write_text("${online_board_declaration_root}/src/ui/MainWindow.cpp" "class FoxOnlineBoardPanel {};\n")
execute_process(
    COMMAND "${CMAKE_COMMAND}" "-DROOT=${online_board_declaration_root}" -P "${script_under_test}"
    RESULT_VARIABLE online_board_declaration_result
    OUTPUT_VARIABLE online_board_declaration_output
    ERROR_VARIABLE online_board_declaration_error
)
if(online_board_declaration_result EQUAL 0)
    message(FATAL_ERROR "Online board implementation marker should fail the first-release scope scan")
endif()
if(NOT "${online_board_declaration_output}${online_board_declaration_error}" MATCHES "MainWindow\\.cpp")
    message(FATAL_ERROR "online-board declaration failure should name MainWindow.cpp: ${online_board_declaration_output}${online_board_declaration_error}")
endif()

set(training_worker_root "${work_dir}/training-worker")
create_fake_root("${training_worker_root}")
write_text("${training_worker_root}/src/app/TrainingWorker.cpp" "void run_distributed_training() {}\n")
execute_process(
    COMMAND "${CMAKE_COMMAND}" "-DROOT=${training_worker_root}" -P "${script_under_test}"
    RESULT_VARIABLE training_worker_result
    OUTPUT_VARIABLE training_worker_output
    ERROR_VARIABLE training_worker_error
)
if(training_worker_result EQUAL 0)
    message(FATAL_ERROR "Distributed-training implementation marker should fail the first-release scope scan")
endif()
if(NOT "${training_worker_output}${training_worker_error}" MATCHES "TrainingWorker\\.cpp")
    message(FATAL_ERROR "training-worker failure should name TrainingWorker.cpp: ${training_worker_output}${training_worker_error}")
endif()

message(STATUS "first_release_scope_check_script_tests passed")
