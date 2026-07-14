if(NOT DEFINED ROOT)
    message(FATAL_ERROR "ROOT is required")
endif()

if(NOT DEFINED TEST_BINARY_DIR)
    message(FATAL_ERROR "TEST_BINARY_DIR is required")
endif()

set(script_under_test "${ROOT}/cmake/LocalizationTextCheck.cmake")
set(work_dir "${TEST_BINARY_DIR}/localization-text-check-script")
file(REMOVE_RECURSE "${work_dir}")
file(MAKE_DIRECTORY "${work_dir}")

function(write_text path text)
    get_filename_component(parent "${path}" DIRECTORY)
    file(MAKE_DIRECTORY "${parent}")
    file(WRITE "${path}" "${text}")
endfunction()

function(create_fake_root root_dir)
    file(REMOVE_RECURSE "${root_dir}")
    write_text("${root_dir}/src/ui/MainWindow.cpp" "const char* text = \"需要 KataGo 可执行文件、模型和分析配置\";\n")
    write_text("${root_dir}/src/ui/MainWindow.h" "#pragma once\n")
    write_text("${root_dir}/tests/ui_tests.cpp" "int main() { return 0; }\n")
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
    message(FATAL_ERROR "Clean fake root should pass localization text check: ${clean_output}${clean_error}")
endif()

set(mixed_root "${work_dir}/mixed")
create_fake_root("${mixed_root}")
write_text("${mixed_root}/src/ui/MainWindow.cpp" "const char* text = \"需要 KataGo 可执行文件、模型和 analysis config\";\n")
execute_process(
    COMMAND "${CMAKE_COMMAND}" "-DROOT=${mixed_root}" -P "${script_under_test}"
    RESULT_VARIABLE mixed_result
    OUTPUT_VARIABLE mixed_output
    ERROR_VARIABLE mixed_error
)
if(mixed_result EQUAL 0)
    message(FATAL_ERROR "Mixed Chinese/English diagnostic should fail localization text check")
endif()
if(NOT "${mixed_output}${mixed_error}" MATCHES "MainWindow\\.cpp")
    message(FATAL_ERROR "Mixed diagnostic failure should name MainWindow.cpp: ${mixed_output}${mixed_error}")
endif()

set(mixed_gtp_root "${work_dir}/mixed-gtp")
create_fake_root("${mixed_gtp_root}")
write_text("${mixed_gtp_root}/src/ui/MainWindow.cpp" "const char* text = \"需要 KataGo 可执行文件、模型和 GTP config\";\n")
execute_process(
    COMMAND "${CMAKE_COMMAND}" "-DROOT=${mixed_gtp_root}" -P "${script_under_test}"
    RESULT_VARIABLE mixed_gtp_result
    OUTPUT_VARIABLE mixed_gtp_output
    ERROR_VARIABLE mixed_gtp_error
)
if(mixed_gtp_result EQUAL 0)
    message(FATAL_ERROR "Mixed Chinese/English GTP diagnostic should fail localization text check")
endif()
if(NOT "${mixed_gtp_output}${mixed_gtp_error}" MATCHES "MainWindow\\.cpp")
    message(FATAL_ERROR "Mixed GTP diagnostic failure should name MainWindow.cpp: ${mixed_gtp_output}${mixed_gtp_error}")
endif()

message(STATUS "localization_text_check_tests passed")
