if(NOT DEFINED ROOT)
    message(FATAL_ERROR "ROOT is required")
endif()

if(NOT DEFINED TEST_BINARY_DIR)
    message(FATAL_ERROR "TEST_BINARY_DIR is required")
endif()

set(script_under_test "${ROOT}/cmake/NoJavaUiDeps.cmake")
set(work_dir "${TEST_BINARY_DIR}/no-java-ui-deps-script")
file(REMOVE_RECURSE "${work_dir}")
file(MAKE_DIRECTORY "${work_dir}")

function(write_text path text)
    get_filename_component(parent "${path}" DIRECTORY)
    file(MAKE_DIRECTORY "${parent}")
    file(WRITE "${path}" "${text}")
endfunction()

function(create_fake_root root_dir)
    file(REMOVE_RECURSE "${root_dir}")
    write_text("${root_dir}/CMakeLists.txt" "cmake_minimum_required(VERSION 3.24)\nproject(FakeQt)\n")
    write_text("${root_dir}/src/CMakeLists.txt" "add_library(fake_core fake.cpp)\n")
    write_text("${root_dir}/src/fake.cpp" "int fake_core() { return 0; }\n")
    write_text("${root_dir}/src/fake.h" "#pragma once\n")
    write_text("${root_dir}/tests/CMakeLists.txt" "add_executable(fake_tests fake_tests.cpp)\n")
    write_text("${root_dir}/tests/fake_tests.cpp" "int main() { return 0; }\n")
    write_text("${root_dir}/cmake/Helper.cmake" "set(fake_helper TRUE)\n")
    write_text("${root_dir}/cmake/NoJavaUiDeps.cmake" "set(forbidden_marker \"javax.swing\")\n")
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
    message(FATAL_ERROR "Clean fake root should pass Java UI dependency scan: ${clean_output}${clean_error}")
endif()

set(root_cmake_root "${work_dir}/root-cmake")
create_fake_root("${root_cmake_root}")
write_text("${root_cmake_root}/CMakeLists.txt" "set(old_ui \"javax.swing.JFrame\")\n")
execute_process(
    COMMAND "${CMAKE_COMMAND}" "-DROOT=${root_cmake_root}" -P "${script_under_test}"
    RESULT_VARIABLE root_cmake_result
    OUTPUT_VARIABLE root_cmake_output
    ERROR_VARIABLE root_cmake_error
)
if(root_cmake_result EQUAL 0)
    message(FATAL_ERROR "Root CMakeLists Java UI marker should fail the dependency scan")
endif()
if(NOT "${root_cmake_output}${root_cmake_error}" MATCHES "CMakeLists\\.txt")
    message(FATAL_ERROR "Root-CMake failure should name CMakeLists.txt: ${root_cmake_output}${root_cmake_error}")
endif()

set(cmake_helper_root "${work_dir}/cmake-helper")
create_fake_root("${cmake_helper_root}")
write_text("${cmake_helper_root}/cmake/Helper.cmake" "set(old_ui \"java.awt.Frame\")\n")
execute_process(
    COMMAND "${CMAKE_COMMAND}" "-DROOT=${cmake_helper_root}" -P "${script_under_test}"
    RESULT_VARIABLE cmake_helper_result
    OUTPUT_VARIABLE cmake_helper_output
    ERROR_VARIABLE cmake_helper_error
)
if(cmake_helper_result EQUAL 0)
    message(FATAL_ERROR "cmake helper Java UI marker should fail the dependency scan")
endif()
if(NOT "${cmake_helper_output}${cmake_helper_error}" MATCHES "Helper\\.cmake")
    message(FATAL_ERROR "cmake-helper failure should name Helper.cmake: ${cmake_helper_output}${cmake_helper_error}")
endif()

set(global_state_root "${work_dir}/global-state")
create_fake_root("${global_state_root}")
write_text("${global_state_root}/src/ui/MainWindow.cpp" "void old_state() { auto old_board = Lizzie.board; }\n")
execute_process(
    COMMAND "${CMAKE_COMMAND}" "-DROOT=${global_state_root}" -P "${script_under_test}"
    RESULT_VARIABLE global_state_result
    OUTPUT_VARIABLE global_state_output
    ERROR_VARIABLE global_state_error
)
if(global_state_result EQUAL 0)
    message(FATAL_ERROR "Legacy Lizzie global-state marker should fail the dependency scan")
endif()
if(NOT "${global_state_output}${global_state_error}" MATCHES "MainWindow\\.cpp")
    message(FATAL_ERROR "global-state failure should name MainWindow.cpp: ${global_state_output}${global_state_error}")
endif()

message(STATUS "no_java_ui_deps_script_tests passed")
