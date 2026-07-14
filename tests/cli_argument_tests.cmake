if(NOT DEFINED APP_EXECUTABLE)
    message(FATAL_ERROR "APP_EXECUTABLE is required")
endif()

if(NOT DEFINED TEST_BINARY_DIR)
    message(FATAL_ERROR "TEST_BINARY_DIR is required")
endif()

set(work_dir "${TEST_BINARY_DIR}/cli-arguments")
file(MAKE_DIRECTORY "${work_dir}")

function(require_success name expected_pattern)
    execute_process(
        COMMAND "${APP_EXECUTABLE}" ${ARGN}
        WORKING_DIRECTORY "${work_dir}"
        TIMEOUT 10
        RESULT_VARIABLE result
        OUTPUT_VARIABLE output
        ERROR_VARIABLE error
    )
    set(combined "${output}${error}")
    if(NOT result EQUAL 0)
        message(FATAL_ERROR "${name} should succeed: ${combined}")
    endif()
    if(NOT "${combined}" MATCHES "${expected_pattern}")
        message(FATAL_ERROR "${name} did not report ${expected_pattern}: ${combined}")
    endif()
endfunction()

function(require_failure name expected_pattern)
    execute_process(
        COMMAND "${APP_EXECUTABLE}" ${ARGN}
        WORKING_DIRECTORY "${work_dir}"
        TIMEOUT 10
        RESULT_VARIABLE result
        OUTPUT_VARIABLE output
        ERROR_VARIABLE error
    )
    set(combined "${output}${error}")
    if(result EQUAL 0)
        message(FATAL_ERROR "${name} should fail: ${combined}")
    endif()
    if(NOT "${combined}" MATCHES "${expected_pattern}")
        message(FATAL_ERROR "${name} did not report ${expected_pattern}: ${combined}")
    endif()
endfunction()

require_success(
    "help"
    "Usage: lizzieyzy_qt .*--target-acceptance-record <ini>"
    --help)

require_success(
    "dash-prefixed target acceptance record value after option terminator"
    "\\[metadata\\]"
    --target-acceptance-record-template
    --target-acceptance-record
    --
    -target-acceptance-record.ini)

require_failure(
    "unknown long option"
    "Unknown option: --unknown-target-acceptance-option"
    --unknown-target-acceptance-option)

require_failure(
    "unknown long option after known option"
    "Unknown option: --unknown-target-acceptance-option"
    --diagnostics
    --unknown-target-acceptance-option)

require_failure(
    "missing target acceptance record value"
    "--target-acceptance-record requires an INI file path"
    --target-acceptance-record)

require_failure(
    "missing target acceptance record value before option"
    "--target-acceptance-record requires an INI file path"
    --target-acceptance-record
    --diagnostics)

require_failure(
    "missing target acceptance record value after option terminator"
    "--target-acceptance-record requires an INI file path"
    --target-acceptance-record
    --)

require_failure(
    "missing target acceptance record value before Qt option"
    "--target-acceptance-record requires an INI file path"
    --target-acceptance-record
    -platform
    offscreen)

require_failure(
    "empty target acceptance record equals value"
    "--target-acceptance-record requires an INI file path"
    --target-acceptance-record=)
