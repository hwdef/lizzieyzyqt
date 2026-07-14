if(NOT DEFINED KATAGO_INTEGRATION_TEST)
    message(FATAL_ERROR "KATAGO_INTEGRATION_TEST is required")
endif()

if(NOT DEFINED TEST_BINARY_DIR)
    message(FATAL_ERROR "TEST_BINARY_DIR is required")
endif()

set(work_dir "${TEST_BINARY_DIR}/katago-integration-preflight")
file(MAKE_DIRECTORY "${work_dir}")

function(require_failed_preflight name expected_pattern)
    execute_process(
        COMMAND
            "${CMAKE_COMMAND}" -E env
            ${ARGN}
            "${KATAGO_INTEGRATION_TEST}"
        WORKING_DIRECTORY "${work_dir}"
        RESULT_VARIABLE result
        OUTPUT_VARIABLE output
        ERROR_VARIABLE error
    )
    set(combined "${output}${error}")
    if(result EQUAL 0)
        message(FATAL_ERROR "${name} preflight should fail: ${combined}")
    endif()
    if(NOT "${combined}" MATCHES "${expected_pattern}")
        message(FATAL_ERROR "${name} preflight did not report ${expected_pattern}: ${combined}")
    endif()
endfunction()

set(valid_model "${work_dir}/model.bin.gz")
set(valid_analysis_config "${work_dir}/analysis.cfg")
set(valid_gtp_config "${work_dir}/gtp.cfg")
set(invalid_model_dir "${work_dir}/model-directory")
set(invalid_gtp_dir "${work_dir}/gtp-directory")
set(missing_executable "${work_dir}/missing-katago")
set(missing_analysis_config "${work_dir}/missing-analysis.cfg")

file(WRITE "${valid_model}" "model fixture\n")
file(WRITE "${valid_analysis_config}" "analysis config fixture\n")
file(WRITE "${valid_gtp_config}" "gtp config fixture\n")
file(MAKE_DIRECTORY "${invalid_model_dir}")
file(MAKE_DIRECTORY "${invalid_gtp_dir}")

execute_process(
    COMMAND
        "${CMAKE_COMMAND}" -E env
        "--unset=LIZZIE_KATAGO_EXECUTABLE"
        "--unset=LIZZIE_KATAGO_MODEL"
        "--unset=LIZZIE_KATAGO_ANALYSIS_CONFIG"
        "--unset=LIZZIE_KATAGO_GTP_CONFIG"
        "${KATAGO_INTEGRATION_TEST}"
    WORKING_DIRECTORY "${work_dir}"
    RESULT_VARIABLE skipped_result
    OUTPUT_VARIABLE skipped_output
    ERROR_VARIABLE skipped_error
)
set(skipped_combined "${skipped_output}${skipped_error}")
if(NOT skipped_result EQUAL 0)
    message(FATAL_ERROR "all-unset integration preflight should skip successfully: ${skipped_combined}")
endif()
foreach(name IN ITEMS
        LIZZIE_KATAGO_EXECUTABLE
        LIZZIE_KATAGO_MODEL
        LIZZIE_KATAGO_ANALYSIS_CONFIG
        LIZZIE_KATAGO_GTP_CONFIG)
    if(NOT "${skipped_combined}" MATCHES "katago\\.integration\\.env\\.${name}\\.set: false")
        message(FATAL_ERROR "all-unset integration preflight should report ${name} unset: ${skipped_combined}")
    endif()
    if(NOT "${skipped_combined}" MATCHES "katago\\.integration\\.env\\.${name}\\.hasText: false")
        message(FATAL_ERROR "all-unset integration preflight should report ${name} has no text: ${skipped_combined}")
    endif()
    if(NOT "${skipped_combined}" MATCHES "katago\\.integration\\.env\\.${name}\\.absolutePath: \\(blank\\)")
        message(FATAL_ERROR "all-unset integration preflight should report ${name} blank path: ${skipped_combined}")
    endif()
    if(NOT "${skipped_combined}" MATCHES "katago\\.integration\\.env\\.${name}\\.usable: false")
        message(FATAL_ERROR "all-unset integration preflight should report ${name} unusable: ${skipped_combined}")
    endif()
endforeach()
if(NOT "${skipped_combined}" MATCHES "katago\\.integration\\.env\\.LIZZIE_KATAGO_EXECUTABLE\\.executable: false")
    message(FATAL_ERROR "all-unset integration preflight should report executable status: ${skipped_combined}")
endif()

require_failed_preflight(
    "missing executable"
    "LIZZIE_KATAGO_EXECUTABLE is invalid: .*hasText=true.*exists=false.*file=false.*readable=false.*executable=false"
    "LIZZIE_KATAGO_EXECUTABLE=${missing_executable}"
    "LIZZIE_KATAGO_MODEL=${valid_model}"
    "LIZZIE_KATAGO_ANALYSIS_CONFIG=${valid_analysis_config}"
)

require_failed_preflight(
    "empty executable"
    "LIZZIE_KATAGO_EXECUTABLE is invalid: .*hasText=false.*absolutePath=\\(blank\\).*exists=false.*file=false.*readable=false.*executable=false"
    "LIZZIE_KATAGO_EXECUTABLE="
    "LIZZIE_KATAGO_MODEL=${valid_model}"
    "LIZZIE_KATAGO_ANALYSIS_CONFIG=${valid_analysis_config}"
)

require_failed_preflight(
    "directory model"
    "LIZZIE_KATAGO_MODEL is invalid: .*hasText=true.*exists=true.*file=false.*readable=false"
    "LIZZIE_KATAGO_EXECUTABLE=${KATAGO_INTEGRATION_TEST}"
    "LIZZIE_KATAGO_MODEL=${invalid_model_dir}"
    "LIZZIE_KATAGO_ANALYSIS_CONFIG=${valid_analysis_config}"
)

require_failed_preflight(
    "blank model"
    "LIZZIE_KATAGO_MODEL is invalid: .*hasText=false.*absolutePath=\\(blank\\).*exists=false.*file=false.*readable=false"
    "LIZZIE_KATAGO_EXECUTABLE=${KATAGO_INTEGRATION_TEST}"
    "LIZZIE_KATAGO_MODEL=   "
    "LIZZIE_KATAGO_ANALYSIS_CONFIG=${valid_analysis_config}"
)

require_failed_preflight(
    "missing analysis config"
    "LIZZIE_KATAGO_ANALYSIS_CONFIG is invalid: .*hasText=true.*exists=false.*file=false.*readable=false"
    "LIZZIE_KATAGO_EXECUTABLE=${KATAGO_INTEGRATION_TEST}"
    "LIZZIE_KATAGO_MODEL=${valid_model}"
    "LIZZIE_KATAGO_ANALYSIS_CONFIG=${missing_analysis_config}"
)

require_failed_preflight(
    "blank analysis config"
    "LIZZIE_KATAGO_ANALYSIS_CONFIG is invalid: .*hasText=false.*absolutePath=\\(blank\\).*exists=false.*file=false.*readable=false"
    "LIZZIE_KATAGO_EXECUTABLE=${KATAGO_INTEGRATION_TEST}"
    "LIZZIE_KATAGO_MODEL=${valid_model}"
    "LIZZIE_KATAGO_ANALYSIS_CONFIG=   "
)

require_failed_preflight(
    "directory GTP config"
    "LIZZIE_KATAGO_GTP_CONFIG is invalid: .*hasText=true.*exists=true.*file=false.*readable=false"
    "LIZZIE_KATAGO_EXECUTABLE=${KATAGO_INTEGRATION_TEST}"
    "LIZZIE_KATAGO_MODEL=${valid_model}"
    "LIZZIE_KATAGO_GTP_CONFIG=${invalid_gtp_dir}"
)

require_failed_preflight(
    "blank GTP config"
    "LIZZIE_KATAGO_GTP_CONFIG is invalid: .*hasText=false.*absolutePath=\\(blank\\).*exists=false.*file=false.*readable=false"
    "LIZZIE_KATAGO_EXECUTABLE=${KATAGO_INTEGRATION_TEST}"
    "LIZZIE_KATAGO_MODEL=${valid_model}"
    "LIZZIE_KATAGO_GTP_CONFIG=   "
)
