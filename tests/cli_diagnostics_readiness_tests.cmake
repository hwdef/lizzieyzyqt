if(NOT DEFINED APP_EXECUTABLE)
    message(FATAL_ERROR "APP_EXECUTABLE is required")
endif()

if(NOT DEFINED TEST_BINARY_DIR)
    message(FATAL_ERROR "TEST_BINARY_DIR is required")
endif()

set(work_dir "${TEST_BINARY_DIR}/cli-diagnostics-readiness")
file(MAKE_DIRECTORY "${work_dir}")

function(require_output variable pattern description)
    if(NOT "${${variable}}" MATCHES "${pattern}")
        message(FATAL_ERROR "Diagnostics output did not ${description}: ${${variable}}")
    endif()
endfunction()

execute_process(
    COMMAND
        "${CMAKE_COMMAND}" -E env
        "--unset=LIZZIE_KATAGO_EXECUTABLE"
        "--unset=LIZZIE_KATAGO_MODEL"
        "--unset=LIZZIE_KATAGO_ANALYSIS_CONFIG"
        "--unset=LIZZIE_KATAGO_GTP_CONFIG"
        "QT_QPA_PLATFORM=offscreen"
        "${APP_EXECUTABLE}" --diagnostics
    WORKING_DIRECTORY "${work_dir}"
    RESULT_VARIABLE missing_result
    OUTPUT_VARIABLE missing_output
    ERROR_VARIABLE missing_error
)
set(missing_combined "${missing_output}${missing_error}")
if(NOT missing_result EQUAL 0)
    message(FATAL_ERROR "Missing-env diagnostics command failed: ${missing_combined}")
endif()

require_output(missing_combined "katago\\.env\\.complete: false" "report incomplete missing KataGo env")
require_output(missing_combined "katago\\.env\\.ready: false" "report unready missing KataGo env")
require_output(missing_combined "katago\\.env\\.status: missing" "report missing KataGo env status")
require_output(missing_combined "katago\\.env\\.missing\\.count: 4" "report all missing KataGo env paths")
require_output(missing_combined "katago\\.env\\.missing\\.0: executable" "name the missing executable")
require_output(missing_combined "katago\\.env\\.missing\\.1: model" "name the missing model")
require_output(missing_combined "katago\\.env\\.missing\\.2: analysisConfig" "name the missing analysis config")
require_output(missing_combined "katago\\.env\\.missing\\.3: gtpConfig" "name the missing GTP config")
require_output(missing_combined "katago\\.env\\.invalid\\.count: 0" "avoid invalid paths when env is missing")
require_output(missing_combined "katago\\.env\\.executable\\.hasText: false" "report unset executable as lacking path text")

set(invalid_executable "${work_dir}/not-executable")
set(valid_model "${work_dir}/model.bin.gz")
set(missing_analysis_config "${work_dir}/missing-analysis.cfg")
set(invalid_gtp_config "${work_dir}/gtp-config-directory")
file(WRITE "${invalid_executable}" "not executable\n")
file(WRITE "${valid_model}" "model fixture\n")
file(MAKE_DIRECTORY "${invalid_gtp_config}")

execute_process(
    COMMAND
        "${CMAKE_COMMAND}" -E env
        "QT_QPA_PLATFORM=offscreen"
        "LIZZIE_KATAGO_EXECUTABLE=${invalid_executable}"
        "LIZZIE_KATAGO_MODEL=${valid_model}"
        "LIZZIE_KATAGO_ANALYSIS_CONFIG=${missing_analysis_config}"
        "LIZZIE_KATAGO_GTP_CONFIG=${invalid_gtp_config}"
        "${APP_EXECUTABLE}" --diagnostics
    WORKING_DIRECTORY "${work_dir}"
    RESULT_VARIABLE invalid_result
    OUTPUT_VARIABLE invalid_output
    ERROR_VARIABLE invalid_error
)
set(invalid_combined "${invalid_output}${invalid_error}")
if(NOT invalid_result EQUAL 0)
    message(FATAL_ERROR "Invalid-env diagnostics command failed: ${invalid_combined}")
endif()

require_output(invalid_combined "katago\\.env\\.complete: true" "report complete invalid KataGo env")
require_output(invalid_combined "katago\\.env\\.ready: false" "report unready invalid KataGo env")
require_output(invalid_combined "katago\\.env\\.status: invalid" "report invalid KataGo env status")
require_output(invalid_combined "katago\\.env\\.missing\\.count: 0" "avoid missing paths when all env vars are set")
require_output(invalid_combined "katago\\.env\\.invalid\\.count: 3" "report invalid KataGo env paths")
require_output(invalid_combined "katago\\.env\\.invalid\\.0: executable" "name the invalid executable")
require_output(invalid_combined "katago\\.env\\.invalid\\.1: analysisConfig" "name the invalid analysis config")
require_output(invalid_combined "katago\\.env\\.invalid\\.2: gtpConfig" "name the invalid GTP config")
require_output(invalid_combined "katago\\.env\\.executable\\.hasText: true" "report invalid executable text presence")
require_output(invalid_combined "katago\\.env\\.model\\.hasText: true" "report valid model text presence")
require_output(invalid_combined "katago\\.env\\.executable\\.executable: false" "validate executable permissions")
require_output(invalid_combined "katago\\.env\\.analysisConfig\\.exists: false" "validate missing analysis config")
require_output(invalid_combined "katago\\.env\\.gtpConfig\\.file: false" "reject a directory GTP config")

set(blank_env_text "   ")
execute_process(
    COMMAND
        "${CMAKE_COMMAND}" -E env
        "QT_QPA_PLATFORM=offscreen"
        "LIZZIE_KATAGO_EXECUTABLE=${blank_env_text}"
        "LIZZIE_KATAGO_MODEL=${blank_env_text}"
        "LIZZIE_KATAGO_ANALYSIS_CONFIG=${blank_env_text}"
        "LIZZIE_KATAGO_GTP_CONFIG=${blank_env_text}"
        "${APP_EXECUTABLE}" --diagnostics
    WORKING_DIRECTORY "${work_dir}"
    RESULT_VARIABLE blank_result
    OUTPUT_VARIABLE blank_output
    ERROR_VARIABLE blank_error
)
set(blank_combined "${blank_output}${blank_error}")
if(NOT blank_result EQUAL 0)
    message(FATAL_ERROR "Blank-env diagnostics command failed: ${blank_combined}")
endif()

require_output(blank_combined "katago\\.env\\.complete: false" "treat blank KataGo env paths as incomplete")
require_output(blank_combined "katago\\.env\\.ready: false" "treat blank KataGo env paths as unready")
require_output(blank_combined "katago\\.env\\.status: missing" "report blank KataGo env paths as missing")
require_output(blank_combined "katago\\.env\\.missing\\.count: 4" "report all blank KataGo env paths as missing")
require_output(blank_combined "katago\\.env\\.invalid\\.count: 0" "avoid invalid paths when env vars are blank")
require_output(blank_combined "katago\\.env\\.executable\\.hasText: false" "report blank executable text status")
require_output(blank_combined "katago\\.env\\.model\\.hasText: false" "report blank model text status")
require_output(blank_combined "katago\\.env\\.analysisConfig\\.hasText: false" "report blank analysis config text status")
require_output(blank_combined "katago\\.env\\.gtpConfig\\.hasText: false" "report blank GTP config text status")
require_output(blank_combined "katago\\.env\\.executable\\.absolutePath: \\(blank\\)" "avoid resolving blank executable env paths")

set(missing_settings_file "${work_dir}/missing-settings.ini")
execute_process(
    COMMAND
        "${CMAKE_COMMAND}" -E env
        "--unset=LIZZIE_KATAGO_EXECUTABLE"
        "--unset=LIZZIE_KATAGO_MODEL"
        "--unset=LIZZIE_KATAGO_ANALYSIS_CONFIG"
        "--unset=LIZZIE_KATAGO_GTP_CONFIG"
        "QT_QPA_PLATFORM=offscreen"
        "LIZZIE_DIAGNOSTICS_SETTINGS_FILE=${missing_settings_file}"
        "${APP_EXECUTABLE}" --diagnostics
    WORKING_DIRECTORY "${work_dir}"
    RESULT_VARIABLE missing_settings_result
    OUTPUT_VARIABLE missing_settings_output
    ERROR_VARIABLE missing_settings_error
)
set(missing_settings_combined "${missing_settings_output}${missing_settings_error}")
if(NOT missing_settings_result EQUAL 0)
    message(FATAL_ERROR "Missing-settings diagnostics command failed: ${missing_settings_combined}")
endif()

foreach(settings_prefix engine compare)
    require_output(
        missing_settings_combined
        "qt\\.settings\\.${settings_prefix}\\.config\\.complete: false"
        "report incomplete missing ${settings_prefix} settings config")
    require_output(
        missing_settings_combined
        "qt\\.settings\\.${settings_prefix}\\.config\\.ready: false"
        "report unready missing ${settings_prefix} settings config")
    require_output(
        missing_settings_combined
        "qt\\.settings\\.${settings_prefix}\\.config\\.gtpReady: false"
        "report unready missing ${settings_prefix} GTP settings config")
    require_output(
        missing_settings_combined
        "qt\\.settings\\.${settings_prefix}\\.config\\.analysisReady: false"
        "report unready missing ${settings_prefix} analysis settings config")
    require_output(
        missing_settings_combined
        "qt\\.settings\\.${settings_prefix}\\.config\\.status: missing"
        "report missing ${settings_prefix} settings config status")
    require_output(
        missing_settings_combined
        "qt\\.settings\\.${settings_prefix}\\.config\\.missing\\.count: 4"
        "report all missing ${settings_prefix} settings config paths")
    require_output(
        missing_settings_combined
        "qt\\.settings\\.${settings_prefix}\\.config\\.invalid\\.count: 0"
        "avoid invalid ${settings_prefix} settings paths when config is missing")
endforeach()
require_output(
    missing_settings_combined
    "plan\\.acceptance\\.configuredStatus: needs-target-machine-and-katago-config"
    "summarize missing saved/env config acceptance status")
require_output(
    missing_settings_combined
    "plan\\.acceptance\\.configuredDualEngineConfigReady: false"
    "summarize missing saved/env dual-engine config readiness")
require_output(
    missing_settings_combined
    "plan\\.acceptance\\.configuredMainConfigSource: none"
    "report no missing saved/env main config source")
require_output(
    missing_settings_combined
    "plan\\.acceptance\\.configuredCompareConfigSource: none"
    "report no missing saved/env compare config source")
require_output(
    missing_settings_combined
    "plan\\.acceptance\\.configuredManualVerificationBlocker\\.1: kataGoConfig"
    "report missing main config manual-verification blocker")
require_output(
    missing_settings_combined
    "plan\\.acceptance\\.configuredDualEngineManualVerificationBlocker\\.1: mainKataGoConfig"
    "report missing dual-engine main config manual-verification blocker")
require_output(
    missing_settings_combined
    "plan\\.acceptance\\.configuredDualEngineManualVerificationBlocker\\.2: compareKataGoConfig"
    "report missing dual-engine compare config manual-verification blocker")
require_output(
    missing_settings_combined
    "plan\\.acceptance\\.configuredDualEngineTargetRunCandidate: false"
    "avoid missing saved/env dual-engine target-run readiness")
require_output(
    missing_settings_combined
    "plan\\.acceptance\\.configuredRealtimeGtpTargetRunCandidate: false"
    "avoid missing saved/env realtime GTP target-run readiness")
require_output(
    missing_settings_combined
    "plan\\.acceptance\\.configuredBatchAnalysisTargetRunCandidate: false"
    "avoid missing saved/env batch-analysis target-run readiness")
require_output(
    missing_settings_combined
    "plan\\.acceptance\\.configuredDualRealtimeGtpTargetRunCandidate: false"
    "avoid missing saved/env dual-engine realtime GTP target-run readiness")
require_output(
    missing_settings_combined
    "plan\\.acceptance\\.configuredDualBatchAnalysisTargetRunCandidate: false"
    "avoid missing saved/env dual-engine batch-analysis target-run readiness")
require_output(
    missing_settings_combined
    "plan\\.acceptance\\.finalAcceptanceBlocker\\.count: 10"
    "report missing saved/env final acceptance blocker count")
require_output(
    missing_settings_combined
    "plan\\.acceptance\\.finalAcceptanceBlocker\\.0: targetPlatform"
    "report missing saved/env target-platform final blocker")
require_output(
    missing_settings_combined
    "plan\\.acceptance\\.finalAcceptanceBlocker\\.1: mainRealtimeGtpConfig"
    "report missing saved/env main realtime GTP final blocker")
require_output(
    missing_settings_combined
    "plan\\.acceptance\\.finalAcceptanceBlocker\\.2: mainBatchAnalysisConfig"
    "report missing saved/env main batch-analysis final blocker")
require_output(
    missing_settings_combined
    "plan\\.acceptance\\.finalAcceptanceBlocker\\.3: compareRealtimeGtpConfig"
    "report missing saved/env compare realtime GTP final blocker")
require_output(
    missing_settings_combined
    "plan\\.acceptance\\.finalAcceptanceBlocker\\.4: compareBatchAnalysisConfig"
    "report missing saved/env compare batch-analysis final blocker")
require_output(
    missing_settings_combined
    "plan\\.acceptance\\.finalAcceptanceBlocker\\.5: targetDisplay"
    "report missing saved/env target-display final blocker")
require_output(
    missing_settings_combined
    "plan\\.acceptance\\.finalAcceptanceBlocker\\.6: multiDisplay"
    "report missing saved/env multi-display final blocker")
require_output(
    missing_settings_combined
    "plan\\.acceptance\\.finalAcceptanceBlocker\\.7: externalTargetVerification"
    "report missing saved/env external target final blocker")
require_output(
    missing_settings_combined
    "plan\\.acceptance\\.finalAcceptanceBlocker\\.8: manualRawEngineComparison"
    "report missing saved/env raw comparison final blocker")
require_output(
    missing_settings_combined
    "plan\\.acceptance\\.finalAcceptanceBlocker\\.9: manualUiInspection"
    "report missing saved/env manual UI final blocker")
require_output(
    missing_settings_combined
    "plan\\.acceptance\\.finalAcceptanceOutstandingBlocker\\.count: 10"
    "report missing saved/env final outstanding blocker count")
require_output(
    missing_settings_combined
    "plan\\.acceptance\\.finalAcceptanceOutstandingBlocker\\.1: mainRealtimeGtpConfig"
    "report missing saved/env main realtime GTP final outstanding blocker")
require_output(
    missing_settings_combined
    "plan\\.acceptance\\.finalAcceptanceOutstandingBlocker\\.4: compareBatchAnalysisConfig"
    "report missing saved/env compare batch-analysis final outstanding blocker")
require_output(
    missing_settings_combined
    "plan\\.acceptance\\.finalAcceptanceOutstandingBlocker\\.9: manualUiInspection"
    "report missing saved/env manual UI final outstanding blocker")

set(invalid_settings_file "${work_dir}/invalid-settings.ini")
file(WRITE "${invalid_settings_file}" "
[engine]
executable=${invalid_executable}
model=${valid_model}
gtpConfig=${invalid_gtp_config}
analysisConfig=${missing_analysis_config}

[compare]
executable=${APP_EXECUTABLE}
model=${valid_model}
gtpConfig=${missing_analysis_config}
analysisConfig=${valid_model}
")
execute_process(
    COMMAND
        "${CMAKE_COMMAND}" -E env
        "--unset=LIZZIE_KATAGO_EXECUTABLE"
        "--unset=LIZZIE_KATAGO_MODEL"
        "--unset=LIZZIE_KATAGO_ANALYSIS_CONFIG"
        "--unset=LIZZIE_KATAGO_GTP_CONFIG"
        "QT_QPA_PLATFORM=offscreen"
        "LIZZIE_DIAGNOSTICS_SETTINGS_FILE=${invalid_settings_file}"
        "${APP_EXECUTABLE}" --diagnostics
    WORKING_DIRECTORY "${work_dir}"
    RESULT_VARIABLE invalid_settings_result
    OUTPUT_VARIABLE invalid_settings_output
    ERROR_VARIABLE invalid_settings_error
)
set(invalid_settings_combined "${invalid_settings_output}${invalid_settings_error}")
if(NOT invalid_settings_result EQUAL 0)
    message(FATAL_ERROR "Invalid-settings diagnostics command failed: ${invalid_settings_combined}")
endif()

require_output(invalid_settings_combined "qt\\.settings\\.engine\\.config\\.complete: true" "report complete invalid main settings config")
require_output(invalid_settings_combined "qt\\.settings\\.engine\\.config\\.ready: false" "report unready invalid main settings config")
require_output(invalid_settings_combined "qt\\.settings\\.engine\\.config\\.gtpReady: false" "report invalid main GTP settings config")
require_output(
    invalid_settings_combined
    "qt\\.settings\\.engine\\.config\\.analysisReady: false"
    "report invalid main analysis settings config")
require_output(invalid_settings_combined "qt\\.settings\\.engine\\.config\\.status: invalid" "report invalid main settings status")
require_output(invalid_settings_combined "qt\\.settings\\.engine\\.config\\.missing\\.count: 0" "avoid missing main settings paths when all are set")
require_output(invalid_settings_combined "qt\\.settings\\.engine\\.config\\.invalid\\.count: 3" "report invalid main settings paths")
require_output(invalid_settings_combined "qt\\.settings\\.engine\\.config\\.invalid\\.0: executable" "name invalid main executable")
require_output(invalid_settings_combined "qt\\.settings\\.engine\\.config\\.invalid\\.1: gtpConfig" "name invalid main GTP config")
require_output(
    invalid_settings_combined
    "qt\\.settings\\.engine\\.config\\.invalid\\.2: analysisConfig"
    "name invalid main analysis config")
require_output(invalid_settings_combined "qt\\.settings\\.compare\\.config\\.complete: true" "report complete invalid compare settings config")
require_output(invalid_settings_combined "qt\\.settings\\.compare\\.config\\.ready: false" "report unready invalid compare settings config")
require_output(invalid_settings_combined "qt\\.settings\\.compare\\.config\\.gtpReady: false" "report invalid compare GTP settings config")
require_output(
    invalid_settings_combined
    "qt\\.settings\\.compare\\.config\\.analysisReady: true"
    "keep compare analysis settings ready when only GTP config is invalid")
require_output(invalid_settings_combined "qt\\.settings\\.compare\\.config\\.status: invalid" "report invalid compare settings status")
require_output(invalid_settings_combined "qt\\.settings\\.compare\\.config\\.missing\\.count: 0" "avoid missing compare settings paths when all are set")
require_output(invalid_settings_combined "qt\\.settings\\.compare\\.config\\.invalid\\.count: 1" "report one invalid compare settings path")
require_output(invalid_settings_combined "qt\\.settings\\.compare\\.config\\.invalid\\.0: gtpConfig" "name invalid compare GTP config")
require_output(
    invalid_settings_combined
    "plan\\.acceptance\\.configuredStatus: needs-target-machine-and-katago-config"
    "summarize invalid saved/env config acceptance status")
require_output(
    invalid_settings_combined
    "plan\\.acceptance\\.configuredDualEngineConfigReady: false"
    "summarize invalid saved/env dual-engine config readiness")
require_output(
    invalid_settings_combined
    "plan\\.acceptance\\.configuredMainConfigSource: none"
    "report no invalid saved/env main config source")
require_output(
    invalid_settings_combined
    "plan\\.acceptance\\.configuredCompareConfigSource: none"
    "report no invalid saved/env compare config source")
require_output(
    invalid_settings_combined
    "plan\\.acceptance\\.configuredManualVerificationBlocker\\.1: kataGoConfig"
    "report invalid main config manual-verification blocker")
require_output(
    invalid_settings_combined
    "plan\\.acceptance\\.configuredDualEngineManualVerificationBlocker\\.1: mainKataGoConfig"
    "report invalid dual-engine main config manual-verification blocker")
require_output(
    invalid_settings_combined
    "plan\\.acceptance\\.configuredDualEngineManualVerificationBlocker\\.2: compareKataGoConfig"
    "report invalid dual-engine compare config manual-verification blocker")
require_output(
    invalid_settings_combined
    "plan\\.acceptance\\.configuredDualEngineTargetRunCandidate: false"
    "avoid invalid saved/env dual-engine target-run readiness")
require_output(
    invalid_settings_combined
    "plan\\.acceptance\\.configuredRealtimeGtpTargetRunCandidate: false"
    "avoid invalid saved/env realtime GTP target-run readiness")
require_output(
    invalid_settings_combined
    "plan\\.acceptance\\.configuredBatchAnalysisTargetRunCandidate: false"
    "avoid invalid saved/env batch-analysis target-run readiness")
require_output(
    invalid_settings_combined
    "plan\\.acceptance\\.configuredDualRealtimeGtpTargetRunCandidate: false"
    "avoid invalid saved/env dual-engine realtime GTP target-run readiness")
require_output(
    invalid_settings_combined
    "plan\\.acceptance\\.configuredDualBatchAnalysisTargetRunCandidate: false"
    "avoid invalid saved/env dual-engine batch-analysis target-run readiness")
require_output(
    invalid_settings_combined
    "plan\\.acceptance\\.finalAcceptanceBlocker\\.count: 9"
    "report invalid saved/env final acceptance blocker count")
require_output(
    invalid_settings_combined
    "plan\\.acceptance\\.finalAcceptanceBlocker\\.0: targetPlatform"
    "report invalid saved/env target-platform final blocker")
require_output(
    invalid_settings_combined
    "plan\\.acceptance\\.finalAcceptanceBlocker\\.1: mainRealtimeGtpConfig"
    "report invalid saved/env main realtime GTP final blocker")
require_output(
    invalid_settings_combined
    "plan\\.acceptance\\.finalAcceptanceBlocker\\.2: mainBatchAnalysisConfig"
    "report invalid saved/env main batch-analysis final blocker")
require_output(
    invalid_settings_combined
    "plan\\.acceptance\\.finalAcceptanceBlocker\\.3: compareRealtimeGtpConfig"
    "report invalid saved/env compare realtime GTP final blocker")
require_output(
    invalid_settings_combined
    "plan\\.acceptance\\.finalAcceptanceBlocker\\.4: targetDisplay"
    "report invalid saved/env target-display final blocker")
require_output(
    invalid_settings_combined
    "plan\\.acceptance\\.finalAcceptanceBlocker\\.5: multiDisplay"
    "report invalid saved/env multi-display final blocker")
require_output(
    invalid_settings_combined
    "plan\\.acceptance\\.finalAcceptanceBlocker\\.6: externalTargetVerification"
    "report invalid saved/env external target final blocker")
require_output(
    invalid_settings_combined
    "plan\\.acceptance\\.finalAcceptanceBlocker\\.7: manualRawEngineComparison"
    "report invalid saved/env raw comparison final blocker")
require_output(
    invalid_settings_combined
    "plan\\.acceptance\\.finalAcceptanceBlocker\\.8: manualUiInspection"
    "report invalid saved/env manual UI final blocker")
require_output(
    invalid_settings_combined
    "plan\\.acceptance\\.finalAcceptanceOutstandingBlocker\\.count: 9"
    "report invalid saved/env final outstanding blocker count")
require_output(
    invalid_settings_combined
    "plan\\.acceptance\\.finalAcceptanceOutstandingBlocker\\.1: mainRealtimeGtpConfig"
    "report invalid saved/env main realtime GTP final outstanding blocker")
require_output(
    invalid_settings_combined
    "plan\\.acceptance\\.finalAcceptanceOutstandingBlocker\\.3: compareRealtimeGtpConfig"
    "report invalid saved/env compare realtime GTP final outstanding blocker")
require_output(
    invalid_settings_combined
    "plan\\.acceptance\\.finalAcceptanceOutstandingBlocker\\.8: manualUiInspection"
    "report invalid saved/env manual UI final outstanding blocker")
