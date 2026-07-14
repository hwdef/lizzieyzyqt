if(NOT DEFINED APP_EXECUTABLE)
    message(FATAL_ERROR "APP_EXECUTABLE is required")
endif()

if(NOT DEFINED TEST_BINARY_DIR)
    message(FATAL_ERROR "TEST_BINARY_DIR is required")
endif()

if(NOT DEFINED IMAGE_FIXTURE_WRITER)
    message(FATAL_ERROR "IMAGE_FIXTURE_WRITER is required")
endif()

set(work_dir "${TEST_BINARY_DIR}/cli-target-acceptance-report")
file(MAKE_DIRECTORY "${work_dir}")

function(require_output variable pattern description)
    if(NOT "${${variable}}" MATCHES "${pattern}")
        message(FATAL_ERROR "Target acceptance report output did not ${description}: ${${variable}}")
    endif()
endfunction()

function(require_no_output variable pattern description)
    if("${${variable}}" MATCHES "${pattern}")
        message(FATAL_ERROR "Target acceptance report output unexpectedly ${description}: ${${variable}}")
    endif()
endfunction()

function(require_output_match_count variable pattern expected_count description)
    string(REGEX MATCHALL "${pattern}" matches "${${variable}}")
    list(LENGTH matches match_count)
    if(NOT match_count EQUAL expected_count)
        message(
            FATAL_ERROR
            "Target acceptance report output did not ${description}; expected ${expected_count}, got ${match_count}: ${${variable}}")
    endif()
endfunction()

function(extract_record_template_value source_var key output_var)
    string(REGEX MATCH "(^|\n)${key}=([^\n]*)" template_match "${${source_var}}")
    if(NOT template_match)
        message(FATAL_ERROR "Target acceptance record template probe did not include ${key}: ${${source_var}}")
    endif()
    set("${output_var}" "${CMAKE_MATCH_2}" PARENT_SCOPE)
endfunction()

set(model_path "${work_dir}/report-model.bin.gz")
set(analysis_config_path "${work_dir}/report-analysis.cfg")
set(gtp_config_path "${work_dir}/report-gtp.cfg")
set(compare_model_path "${work_dir}/report-compare-model.bin.gz")
set(compare_analysis_config_path "${work_dir}/report-compare-analysis.cfg")
set(compare_gtp_config_path "${work_dir}/report-compare-gtp.cfg")
set(engine_working_dir "${work_dir}/engine-working-dir")
set(compare_working_dir "${work_dir}/compare-working-dir")
set(settings_file "${work_dir}/target-acceptance-report-settings.ini")
set(mode_blocker_settings_file "${work_dir}/target-acceptance-report-mode-blocker-settings.ini")
set(acceptance_record_file "${work_dir}/target-acceptance-record.ini")
set(acceptance_record_failed_manual_file "${work_dir}/target-acceptance-record-failed-manual.ini")
set(acceptance_record_invalid_manual_file "${work_dir}/target-acceptance-record-invalid-manual.ini")
set(acceptance_record_failed_checklist_file "${work_dir}/target-acceptance-record-failed-checklist.ini")
set(acceptance_record_invalid_checklist_file "${work_dir}/target-acceptance-record-invalid-checklist.ini")
set(acceptance_record_failed_target_component_file "${work_dir}/target-acceptance-record-failed-target-component.ini")
set(acceptance_record_invalid_target_component_file "${work_dir}/target-acceptance-record-invalid-target-component.ini")
set(acceptance_record_missing_evidence_file "${work_dir}/target-acceptance-record-missing-evidence.ini")
set(acceptance_record_empty_evidence_file "${work_dir}/target-acceptance-record-empty-evidence.ini")
set(acceptance_record_duplicate_evidence_file "${work_dir}/target-acceptance-record-duplicate-evidence.ini")
set(acceptance_record_uniform_screenshot_file "${work_dir}/target-acceptance-record-uniform-screenshot.ini")
set(acceptance_record_4k_screenshot_file "${work_dir}/target-acceptance-record-4k-screenshot.ini")
set(acceptance_record_bad_evidence_content_file "${work_dir}/target-acceptance-record-bad-evidence-content.ini")
set(acceptance_record_missing_diagnostics_record_sha_file "${work_dir}/target-acceptance-record-missing-diagnostics-record-sha.ini")
set(acceptance_record_missing_diagnostics_final_blockers_file "${work_dir}/target-acceptance-record-missing-diagnostics-final-blockers.ini")
set(acceptance_record_mismatched_diagnostics_record_path_file "${work_dir}/target-acceptance-record-mismatched-diagnostics-record-path.ini")
set(acceptance_record_mismatched_diagnostics_app_file "${work_dir}/target-acceptance-record-mismatched-diagnostics-app.ini")
set(acceptance_record_mismatched_diagnostics_executable_file "${work_dir}/target-acceptance-record-mismatched-diagnostics-executable.ini")
set(acceptance_record_bad_report_content_file "${work_dir}/target-acceptance-record-bad-report-content.ini")
set(acceptance_record_missing_report_record_sha_file "${work_dir}/target-acceptance-record-missing-report-record-sha.ini")
set(acceptance_record_missing_report_final_blockers_file "${work_dir}/target-acceptance-record-missing-report-final-blockers.ini")
set(acceptance_record_mismatched_report_app_file "${work_dir}/target-acceptance-record-mismatched-report-app.ini")
set(acceptance_record_mismatched_report_executable_file "${work_dir}/target-acceptance-record-mismatched-report-executable.ini")
set(acceptance_record_mismatched_report_record_path_file "${work_dir}/target-acceptance-record-mismatched-report-record-path.ini")
set(acceptance_record_mismatched_engine_log_version_file "${work_dir}/target-acceptance-record-mismatched-engine-log-version.ini")
set(acceptance_record_missing_engine_gpu_stderr_file "${work_dir}/target-acceptance-record-missing-engine-gpu-stderr.ini")
set(acceptance_record_mismatched_raw_log_version_file "${work_dir}/target-acceptance-record-mismatched-raw-log-version.ini")
set(acceptance_record_bad_raw_log_content_file "${work_dir}/target-acceptance-record-bad-raw-log-content.ini")
set(acceptance_record_bad_manual_ui_log_content_file "${work_dir}/target-acceptance-record-bad-manual-ui-log-content.ini")
set(acceptance_record_bad_external_verification_log_content_file "${work_dir}/target-acceptance-record-bad-external-verification-log-content.ini")
set(acceptance_record_incomplete_raw_log_content_file "${work_dir}/target-acceptance-record-incomplete-raw-log-content.ini")
set(acceptance_record_missing_raw_log_winrate_file "${work_dir}/target-acceptance-record-missing-raw-log-winrate.ini")
set(acceptance_record_missing_raw_log_score_mean_file "${work_dir}/target-acceptance-record-missing-raw-log-score-mean.ini")
set(acceptance_record_missing_raw_log_score_stdev_file "${work_dir}/target-acceptance-record-missing-raw-log-score-stdev.ini")
set(acceptance_record_missing_raw_log_policy_file "${work_dir}/target-acceptance-record-missing-raw-log-policy.ini")
set(acceptance_record_missing_raw_log_visits_file "${work_dir}/target-acceptance-record-missing-raw-log-visits.ini")
set(acceptance_record_missing_raw_log_pv_file "${work_dir}/target-acceptance-record-missing-raw-log-pv.ini")
set(acceptance_record_missing_raw_log_pv_visits_file "${work_dir}/target-acceptance-record-missing-raw-log-pv-visits.ini")
set(acceptance_record_missing_raw_log_ownership_file "${work_dir}/target-acceptance-record-missing-raw-log-ownership.ini")
set(acceptance_record_missing_metadata_file "${work_dir}/target-acceptance-record-missing-metadata.ini")
set(acceptance_record_invalid_metadata_file "${work_dir}/target-acceptance-record-invalid-metadata.ini")
set(acceptance_record_future_metadata_file "${work_dir}/target-acceptance-record-future-metadata.ini")
set(acceptance_record_wrong_app_version_file "${work_dir}/target-acceptance-record-wrong-app-version.ini")
set(acceptance_record_wrong_app_executable_file "${work_dir}/target-acceptance-record-wrong-app-executable.ini")
set(acceptance_record_wrong_os_metadata_file "${work_dir}/target-acceptance-record-wrong-os-metadata.ini")
set(acceptance_record_wrong_runtime_metadata_file "${work_dir}/target-acceptance-record-wrong-runtime-metadata.ini")
set(acceptance_record_wrong_abi_metadata_file "${work_dir}/target-acceptance-record-wrong-abi-metadata.ini")
set(acceptance_record_wrong_cpu_metadata_file "${work_dir}/target-acceptance-record-wrong-cpu-metadata.ini")
set(acceptance_record_wrong_target_machine_file "${work_dir}/target-acceptance-record-wrong-target-machine.ini")
set(acceptance_record_wrong_gpu_driver_file "${work_dir}/target-acceptance-record-wrong-gpu-driver.ini")
set(acceptance_record_wrong_katago_version_file "${work_dir}/target-acceptance-record-wrong-katago-version.ini")
set(acceptance_record_hash_mismatch_file "${work_dir}/target-acceptance-record-hash-mismatch.ini")
set(acceptance_record_stale_evidence_file "${work_dir}/target-acceptance-record-stale-evidence.ini")
set(relative_acceptance_record_dir "${work_dir}/relative-record")
set(relative_acceptance_record_evidence_dir "${relative_acceptance_record_dir}/evidence")
set(relative_acceptance_record_file "${relative_acceptance_record_dir}/target-acceptance-record-relative.ini")
set(record_diagnostics_path "${work_dir}/target-diagnostics.txt")
set(record_report_path "${work_dir}/target-report.md")
set(record_engine_log_path "${work_dir}/target-engine.log")
set(record_raw_log_path "${work_dir}/target-raw-katago.log")
set(record_manual_ui_log_path "${work_dir}/target-manual-ui-inspection.log")
set(record_external_verification_log_path "${work_dir}/target-external-verification.log")
set(record_screenshot_path "${work_dir}/target-screenshot.ppm")
set(uniform_record_screenshot_path "${work_dir}/uniform-target-screenshot.ppm")
set(record_4k_screenshot_path "${work_dir}/target-screenshot-4k.png")
set(empty_record_diagnostics_path "${work_dir}/empty-target-diagnostics.txt")
set(bad_record_diagnostics_path "${work_dir}/bad-target-diagnostics.txt")
set(missing_record_sha_record_diagnostics_path "${work_dir}/missing-record-sha-target-diagnostics.txt")
set(missing_final_blockers_record_diagnostics_path "${work_dir}/missing-final-blockers-target-diagnostics.txt")
set(mismatched_record_diagnostics_path "${work_dir}/mismatched-target-diagnostics.txt")
set(mismatched_record_diagnostics_app_path "${work_dir}/mismatched-target-diagnostics-app.txt")
set(mismatched_record_diagnostics_executable_path "${work_dir}/mismatched-target-diagnostics-executable.txt")
set(bad_record_report_path "${work_dir}/bad-target-report.md")
set(missing_record_sha_record_report_path "${work_dir}/missing-record-sha-target-report.md")
set(missing_final_blockers_record_report_path "${work_dir}/missing-final-blockers-target-report.md")
set(mismatched_record_report_app_path "${work_dir}/mismatched-target-report-app.md")
set(mismatched_record_report_executable_path "${work_dir}/mismatched-target-report-executable.md")
set(mismatched_record_report_path "${work_dir}/mismatched-target-report.md")
set(mismatched_record_engine_log_path "${work_dir}/mismatched-target-engine.log")
set(missing_gpu_record_engine_log_path "${work_dir}/missing-gpu-target-engine.log")
set(mismatched_record_raw_log_path "${work_dir}/mismatched-target-raw-katago.log")
set(bad_record_raw_log_path "${work_dir}/bad-target-raw-katago.log")
set(incomplete_record_raw_log_path "${work_dir}/incomplete-target-raw-katago.log")
set(missing_winrate_record_raw_log_path "${work_dir}/missing-winrate-target-raw-katago.log")
set(missing_score_mean_record_raw_log_path "${work_dir}/missing-score-mean-target-raw-katago.log")
set(missing_score_stdev_record_raw_log_path "${work_dir}/missing-score-stdev-target-raw-katago.log")
set(missing_policy_record_raw_log_path "${work_dir}/missing-policy-target-raw-katago.log")
set(missing_visits_record_raw_log_path "${work_dir}/missing-visits-target-raw-katago.log")
set(missing_pv_record_raw_log_path "${work_dir}/missing-pv-target-raw-katago.log")
set(missing_pv_visits_record_raw_log_path "${work_dir}/missing-pv-visits-target-raw-katago.log")
set(missing_ownership_record_raw_log_path "${work_dir}/missing-ownership-target-raw-katago.log")
set(bad_record_manual_ui_log_path "${work_dir}/bad-target-manual-ui-inspection.log")
set(bad_record_external_verification_log_path "${work_dir}/bad-target-external-verification.log")
set(expected_app_version "0.1.0")
set(mismatched_record_app_executable_path "${work_dir}/other-lizzieyzy_qt")
set(record_evidence_bound_paths
    "${acceptance_record_file}\n${acceptance_record_failed_manual_file}\n${acceptance_record_invalid_manual_file}\n${acceptance_record_failed_checklist_file}\n${acceptance_record_invalid_checklist_file}\n${acceptance_record_failed_target_component_file}\n${acceptance_record_invalid_target_component_file}\n${acceptance_record_empty_evidence_file}\n${acceptance_record_duplicate_evidence_file}\n${acceptance_record_uniform_screenshot_file}\n${acceptance_record_4k_screenshot_file}\n${acceptance_record_bad_report_content_file}\n${acceptance_record_missing_report_record_sha_file}\n${acceptance_record_missing_report_final_blockers_file}\n${acceptance_record_mismatched_report_app_file}\n${acceptance_record_mismatched_report_executable_file}\n${acceptance_record_mismatched_report_record_path_file}\n${acceptance_record_mismatched_engine_log_version_file}\n${acceptance_record_missing_engine_gpu_stderr_file}\n${acceptance_record_mismatched_raw_log_version_file}\n${acceptance_record_bad_raw_log_content_file}\n${acceptance_record_bad_manual_ui_log_content_file}\n${acceptance_record_bad_external_verification_log_content_file}\n${acceptance_record_incomplete_raw_log_content_file}\n${acceptance_record_missing_raw_log_winrate_file}\n${acceptance_record_missing_raw_log_score_mean_file}\n${acceptance_record_missing_raw_log_score_stdev_file}\n${acceptance_record_missing_raw_log_policy_file}\n${acceptance_record_missing_raw_log_visits_file}\n${acceptance_record_missing_raw_log_pv_file}\n${acceptance_record_missing_raw_log_pv_visits_file}\n${acceptance_record_missing_raw_log_ownership_file}\n${acceptance_record_missing_metadata_file}\n${acceptance_record_invalid_metadata_file}\n${acceptance_record_future_metadata_file}\n${acceptance_record_wrong_app_version_file}\n${acceptance_record_wrong_app_executable_file}\n${acceptance_record_wrong_os_metadata_file}\n${acceptance_record_wrong_runtime_metadata_file}\n${acceptance_record_wrong_abi_metadata_file}\n${acceptance_record_wrong_cpu_metadata_file}\n${acceptance_record_wrong_target_machine_file}\n${acceptance_record_wrong_gpu_driver_file}\n${acceptance_record_wrong_katago_version_file}\n${acceptance_record_hash_mismatch_file}\n${acceptance_record_stale_evidence_file}\n")
set(record_report_bound_paths
    "${acceptance_record_file}\n${acceptance_record_failed_manual_file}\n${acceptance_record_invalid_manual_file}\n${acceptance_record_missing_evidence_file}\n${acceptance_record_empty_evidence_file}\n${acceptance_record_duplicate_evidence_file}\n${acceptance_record_uniform_screenshot_file}\n${acceptance_record_4k_screenshot_file}\n${acceptance_record_bad_evidence_content_file}\n${acceptance_record_missing_diagnostics_record_sha_file}\n${acceptance_record_missing_diagnostics_final_blockers_file}\n${acceptance_record_mismatched_diagnostics_record_path_file}\n${acceptance_record_mismatched_diagnostics_app_file}\n${acceptance_record_mismatched_diagnostics_executable_file}\n${acceptance_record_mismatched_engine_log_version_file}\n${acceptance_record_missing_engine_gpu_stderr_file}\n${acceptance_record_mismatched_raw_log_version_file}\n${acceptance_record_bad_raw_log_content_file}\n${acceptance_record_bad_manual_ui_log_content_file}\n${acceptance_record_bad_external_verification_log_content_file}\n${acceptance_record_incomplete_raw_log_content_file}\n${acceptance_record_missing_raw_log_winrate_file}\n${acceptance_record_missing_raw_log_score_mean_file}\n${acceptance_record_missing_raw_log_score_stdev_file}\n${acceptance_record_missing_raw_log_policy_file}\n${acceptance_record_missing_raw_log_visits_file}\n${acceptance_record_missing_raw_log_pv_file}\n${acceptance_record_missing_raw_log_pv_visits_file}\n${acceptance_record_missing_raw_log_ownership_file}\n${acceptance_record_missing_metadata_file}\n${acceptance_record_invalid_metadata_file}\n${acceptance_record_future_metadata_file}\n${acceptance_record_wrong_app_version_file}\n${acceptance_record_wrong_app_executable_file}\n${acceptance_record_wrong_os_metadata_file}\n${acceptance_record_wrong_runtime_metadata_file}\n${acceptance_record_wrong_abi_metadata_file}\n${acceptance_record_wrong_cpu_metadata_file}\n${acceptance_record_wrong_target_machine_file}\n${acceptance_record_wrong_gpu_driver_file}\n${acceptance_record_wrong_katago_version_file}\n${acceptance_record_hash_mismatch_file}\n${acceptance_record_stale_evidence_file}\n")
set(record_diagnostics_sha_marker "plan.acceptance.recordFile.sha256: fixture\n")
set(record_diagnostics_final_markers "plan.acceptance.finalAcceptanceStatus: fixture\nplan.acceptance.finalAcceptanceBlocker.count: 0\nplan.acceptance.finalAcceptanceOutstandingBlocker.count: 0\n")
set(record_diagnostics_content "LizzieYzy Qt Diagnostics\napp.version: ${expected_app_version}\napp.executable: ${APP_EXECUTABLE}\nplan.acceptance.recordFile.canonicalPath: ${acceptance_record_file}\n${record_diagnostics_sha_marker}${record_diagnostics_final_markers}${record_evidence_bound_paths}")
set(record_report_content "# Target Acceptance Report\napp.version: ${expected_app_version}\napp.executable: ${APP_EXECUTABLE}\n`plan.acceptance.recordFile.canonicalPath`: ${acceptance_record_file}\n`plan.acceptance.recordFile.sha256`: fixture\n`plan.acceptance.finalAcceptanceStatus`: fixture\n`plan.acceptance.finalAcceptanceBlocker`: fixture\n`plan.acceptance.finalAcceptanceOutstandingBlocker`: fixture\n${record_report_bound_paths}")
set(record_engine_log_content "target KataGo engine log evidence fixture\nKataGo 1.15.3\nstderr: CUDA fake backend ready\n")
set(mismatched_record_engine_log_content "target KataGo engine log evidence fixture\nKataGo 9.99.9\nstderr: CUDA fake backend ready\n")
set(missing_gpu_record_engine_log_content "target KataGo engine log evidence fixture\nKataGo 1.15.3\nengine startup captured\n")
set(raw_katago_comparison_checklist_content "analysisJsonRawResponse\nrealtimeGtpRawLine\ncandidateTableRendering\npvPreview\nwinrateScorePerspective\nownershipDisplay\ngenmove\nhumanVsAi\nselfPlay\nengineVsEngine\n")
set(record_raw_log_content "target raw KataGo evidence fixture\nKataGo 1.15.3\nkata-analyze raw line\n{\"rootInfo\":{\"winrate\":0.5,\"scoreMean\":0.0,\"ownership\":[0.1,-0.1]},\"moveInfos\":[{\"move\":\"D4\",\"visits\":10,\"winrate\":0.5,\"scoreMean\":0.0,\"scoreStdev\":4.2,\"policy\":0.12,\"pv\":[\"D4\",\"Q16\"],\"pvVisits\":[10,5],\"ownership\":[0.1,-0.1]}]}\n${raw_katago_comparison_checklist_content}")
set(manual_ui_inspection_log_content "target manual UI inspection evidence fixture\nmainWindow4KHighDpiLayout\nboardLinesCoordinatesAndStarPoints\nstoneRenderingAndCandidateLabels\ncandidateTableColumns\npvPreviewStones\nownershipMainBoardOverlay\nownershipMiniBoardDock\nwinrateScoreGraph\ntoolbarDockAndMenuLayout\nbilingualTextFit\nsavedWindowRestore\nmultiDisplayPlacement\n")
set(external_verification_log_content "target external verification evidence fixture\nlinuxKdeWaylandNvidiaRealtimeKataGo\nwindowsNvidiaRealtimeKataGo\nphysical4KHighDpiMultiDisplayUi\nrawKataGoUiComparison\n")
set(bad_record_manual_ui_log_content "target manual UI inspection evidence fixture without checklist item markers\n")
set(bad_record_external_verification_log_content "target external verification evidence fixture without checklist item markers\n")
set(mismatched_record_raw_log_content "target raw KataGo evidence fixture\nKataGo 9.99.9\nkata-analyze raw line\n{\"rootInfo\":{\"winrate\":0.5,\"scoreMean\":0.0,\"ownership\":[0.1,-0.1]},\"moveInfos\":[{\"move\":\"D4\",\"visits\":10,\"winrate\":0.5,\"scoreMean\":0.0,\"scoreStdev\":4.2,\"policy\":0.12,\"pv\":[\"D4\",\"Q16\"],\"pvVisits\":[10,5],\"ownership\":[0.1,-0.1]}]}\n${raw_katago_comparison_checklist_content}")
set(record_screenshot_content "P3\n2 1\n255\n255 255 255 0 0 0\n")
set(uniform_record_screenshot_content "P3\n2 1\n255\n255 255 255 255 255 255\n")
set(bad_record_diagnostics_content "not diagnostics evidence fixture\n")
set(missing_record_sha_record_diagnostics_content "LizzieYzy Qt Diagnostics\napp.version: ${expected_app_version}\napp.executable: ${APP_EXECUTABLE}\nplan.acceptance.recordFile.canonicalPath: ${acceptance_record_missing_diagnostics_record_sha_file}\n${record_diagnostics_final_markers}")
set(missing_final_blockers_record_diagnostics_content "LizzieYzy Qt Diagnostics\napp.version: ${expected_app_version}\napp.executable: ${APP_EXECUTABLE}\nplan.acceptance.recordFile.canonicalPath: ${acceptance_record_missing_diagnostics_final_blockers_file}\n${record_diagnostics_sha_marker}")
set(mismatched_record_diagnostics_content "LizzieYzy Qt Diagnostics\napp.version: ${expected_app_version}\napp.executable: ${APP_EXECUTABLE}\nplan.acceptance.recordFile.canonicalPath: ${work_dir}/other-target-acceptance-record.ini\n${record_diagnostics_sha_marker}${record_diagnostics_final_markers}")
set(mismatched_record_diagnostics_app_content "LizzieYzy Qt Diagnostics\napp.version: 9.9.9\napp.executable: ${APP_EXECUTABLE}\nplan.acceptance.recordFile.canonicalPath: ${acceptance_record_mismatched_diagnostics_app_file}\n${record_diagnostics_sha_marker}${record_diagnostics_final_markers}${acceptance_record_mismatched_diagnostics_app_file}\n")
set(mismatched_record_diagnostics_executable_content "LizzieYzy Qt Diagnostics\napp.version: ${expected_app_version}\napp.executable: ${mismatched_record_app_executable_path}\nplan.acceptance.recordFile.canonicalPath: ${acceptance_record_mismatched_diagnostics_executable_file}\n${record_diagnostics_sha_marker}${record_diagnostics_final_markers}${acceptance_record_mismatched_diagnostics_executable_file}\n")
set(bad_record_report_content "not target acceptance report evidence fixture\n")
set(missing_record_sha_record_report_content "# Target Acceptance Report\napp.version: ${expected_app_version}\napp.executable: ${APP_EXECUTABLE}\n`plan.acceptance.recordFile.canonicalPath`: ${acceptance_record_missing_report_record_sha_file}\n`plan.acceptance.finalAcceptanceStatus`: fixture\n`plan.acceptance.finalAcceptanceBlocker`: fixture\n`plan.acceptance.finalAcceptanceOutstandingBlocker`: fixture\n")
set(missing_final_blockers_record_report_content "# Target Acceptance Report\napp.version: ${expected_app_version}\napp.executable: ${APP_EXECUTABLE}\n`plan.acceptance.recordFile.canonicalPath`: ${acceptance_record_missing_report_final_blockers_file}\n`plan.acceptance.recordFile.sha256`: fixture\n`plan.acceptance.finalAcceptanceStatus`: fixture\n")
set(mismatched_record_report_app_content "# Target Acceptance Report\napp.version: 9.9.9\napp.executable: ${APP_EXECUTABLE}\n`plan.acceptance.recordFile.canonicalPath`: ${acceptance_record_mismatched_report_app_file}\n`plan.acceptance.recordFile.sha256`: fixture\n`plan.acceptance.finalAcceptanceStatus`: fixture\n`plan.acceptance.finalAcceptanceBlocker`: fixture\n`plan.acceptance.finalAcceptanceOutstandingBlocker`: fixture\n")
set(mismatched_record_report_executable_content "# Target Acceptance Report\napp.version: ${expected_app_version}\napp.executable: ${mismatched_record_app_executable_path}\n`plan.acceptance.recordFile.canonicalPath`: ${acceptance_record_mismatched_report_executable_file}\n`plan.acceptance.recordFile.sha256`: fixture\n`plan.acceptance.finalAcceptanceStatus`: fixture\n`plan.acceptance.finalAcceptanceBlocker`: fixture\n`plan.acceptance.finalAcceptanceOutstandingBlocker`: fixture\n")
set(mismatched_record_report_content "# Target Acceptance Report\n`plan.acceptance.recordFile.canonicalPath`: ${work_dir}/other-target-acceptance-record.ini\n`plan.acceptance.recordFile.sha256`: fixture\n`plan.acceptance.finalAcceptanceStatus`: fixture\n")
set(bad_record_raw_log_content "not raw comparison evidence fixture\n")
set(incomplete_record_raw_log_content "KataGo raw comparison evidence without required raw protocol markers\n")
set(missing_winrate_record_raw_log_content "target raw KataGo evidence fixture\nKataGo 1.15.3\nkata-analyze raw line\n{\"rootInfo\":{\"scoreMean\":0.0,\"ownership\":[0.1,-0.1]},\"moveInfos\":[{\"move\":\"D4\",\"visits\":10,\"scoreMean\":0.0,\"scoreStdev\":4.2,\"policy\":0.12,\"pv\":[\"D4\",\"Q16\"],\"pvVisits\":[10,5],\"ownership\":[0.1,-0.1]}]}\n${raw_katago_comparison_checklist_content}")
set(missing_score_mean_record_raw_log_content "target raw KataGo evidence fixture\nKataGo 1.15.3\nkata-analyze raw line\n{\"rootInfo\":{\"winrate\":0.5,\"ownership\":[0.1,-0.1]},\"moveInfos\":[{\"move\":\"D4\",\"visits\":10,\"winrate\":0.5,\"scoreStdev\":4.2,\"policy\":0.12,\"pv\":[\"D4\",\"Q16\"],\"pvVisits\":[10,5],\"ownership\":[0.1,-0.1]}]}\n${raw_katago_comparison_checklist_content}")
set(missing_score_stdev_record_raw_log_content "target raw KataGo evidence fixture\nKataGo 1.15.3\nkata-analyze raw line\n{\"rootInfo\":{\"winrate\":0.5,\"scoreMean\":0.0,\"ownership\":[0.1,-0.1]},\"moveInfos\":[{\"move\":\"D4\",\"visits\":10,\"winrate\":0.5,\"scoreMean\":0.0,\"policy\":0.12,\"pv\":[\"D4\",\"Q16\"],\"pvVisits\":[10,5],\"ownership\":[0.1,-0.1]}]}\n${raw_katago_comparison_checklist_content}")
set(missing_policy_record_raw_log_content "target raw KataGo evidence fixture\nKataGo 1.15.3\nkata-analyze raw line\n{\"rootInfo\":{\"winrate\":0.5,\"scoreMean\":0.0,\"ownership\":[0.1,-0.1]},\"moveInfos\":[{\"move\":\"D4\",\"visits\":10,\"winrate\":0.5,\"scoreMean\":0.0,\"scoreStdev\":4.2,\"pv\":[\"D4\",\"Q16\"],\"pvVisits\":[10,5],\"ownership\":[0.1,-0.1]}]}\n${raw_katago_comparison_checklist_content}")
set(missing_visits_record_raw_log_content "target raw KataGo evidence fixture\nKataGo 1.15.3\nkata-analyze raw line\n{\"rootInfo\":{\"winrate\":0.5,\"scoreMean\":0.0,\"ownership\":[0.1,-0.1]},\"moveInfos\":[{\"move\":\"D4\",\"winrate\":0.5,\"scoreMean\":0.0,\"scoreStdev\":4.2,\"policy\":0.12,\"pv\":[\"D4\",\"Q16\"],\"pvVisits\":[10,5],\"ownership\":[0.1,-0.1]}]}\n${raw_katago_comparison_checklist_content}")
set(missing_pv_record_raw_log_content "target raw KataGo evidence fixture\nKataGo 1.15.3\nkata-analyze raw line\n{\"rootInfo\":{\"winrate\":0.5,\"scoreMean\":0.0,\"ownership\":[0.1,-0.1]},\"moveInfos\":[{\"move\":\"D4\",\"visits\":10,\"winrate\":0.5,\"scoreMean\":0.0,\"scoreStdev\":4.2,\"policy\":0.12,\"pvVisits\":[10,5],\"ownership\":[0.1,-0.1]}]}\n${raw_katago_comparison_checklist_content}")
set(missing_pv_visits_record_raw_log_content "target raw KataGo evidence fixture\nKataGo 1.15.3\nkata-analyze raw line\n{\"rootInfo\":{\"winrate\":0.5,\"scoreMean\":0.0,\"ownership\":[0.1,-0.1]},\"moveInfos\":[{\"move\":\"D4\",\"visits\":10,\"winrate\":0.5,\"scoreMean\":0.0,\"scoreStdev\":4.2,\"policy\":0.12,\"pv\":[\"D4\",\"Q16\"],\"ownership\":[0.1,-0.1]}]}\n${raw_katago_comparison_checklist_content}")
set(missing_ownership_record_raw_log_content "target raw KataGo evidence fixture\nKataGo 1.15.3\nkata-analyze raw line\n{\"rootInfo\":{\"winrate\":0.5,\"scoreMean\":0.0},\"moveInfos\":[{\"move\":\"D4\",\"visits\":10,\"winrate\":0.5,\"scoreMean\":0.0,\"scoreStdev\":4.2,\"policy\":0.12,\"pv\":[\"D4\",\"Q16\"],\"pvVisits\":[10,5]}]}\n${raw_katago_comparison_checklist_content}")
file(SHA256 "${APP_EXECUTABLE}" expected_app_executable_sha256)
execute_process(
    COMMAND
        "${CMAKE_COMMAND}" -E env
        "QT_QPA_PLATFORM=missing-qpa-for-template-probe"
        "${APP_EXECUTABLE}" --target-acceptance-record-template
    WORKING_DIRECTORY "${work_dir}"
    RESULT_VARIABLE expected_metadata_template_result
    OUTPUT_VARIABLE expected_metadata_template_output
    ERROR_VARIABLE expected_metadata_template_error
)
set(expected_metadata_template_combined "${expected_metadata_template_output}${expected_metadata_template_error}")
if(NOT expected_metadata_template_result EQUAL 0)
    message(FATAL_ERROR "Target acceptance record template probe failed: ${expected_metadata_template_combined}")
endif()
extract_record_template_value(expected_metadata_template_combined "osPrettyName" expected_os_pretty_name)
extract_record_template_value(expected_metadata_template_combined "osKernelType" expected_os_kernel_type)
extract_record_template_value(expected_metadata_template_combined "osKernelVersion" expected_os_kernel_version)
extract_record_template_value(expected_metadata_template_combined "qtRuntimeVersion" expected_qt_runtime_version)
extract_record_template_value(expected_metadata_template_combined "qtBuildAbi" expected_qt_build_abi)
extract_record_template_value(expected_metadata_template_combined "cpuArchitecture" expected_cpu_architecture)
extract_record_template_value(expected_metadata_template_combined "targetMachine" expected_target_machine)
set(relative_record_diagnostics_content "LizzieYzy Qt Diagnostics\napp.version: ${expected_app_version}\napp.executable: ${APP_EXECUTABLE}\nplan.acceptance.recordFile.canonicalPath: ${relative_acceptance_record_file}\n${record_diagnostics_sha_marker}${record_diagnostics_final_markers}${relative_acceptance_record_file}\n")
set(relative_record_report_content "# Target Acceptance Report\napp.version: ${expected_app_version}\napp.executable: ${APP_EXECUTABLE}\n`plan.acceptance.recordFile.canonicalPath`: ${relative_acceptance_record_file}\n`plan.acceptance.recordFile.sha256`: relative fixture\n`plan.acceptance.finalAcceptanceStatus`: relative fixture\n`plan.acceptance.finalAcceptanceBlocker`: relative fixture\n`plan.acceptance.finalAcceptanceOutstandingBlocker`: relative fixture\n")
set(relative_record_engine_log_content "relative KataGo engine log evidence fixture\nKataGo 1.15.3\nstderr: CUDA fake backend ready\n")
set(relative_record_raw_log_content "relative raw KataGo evidence fixture\nKataGo 1.15.3\nkata-analyze raw line\n{\"rootInfo\":{\"winrate\":0.5,\"scoreMean\":0.0,\"ownership\":[0.1,-0.1]},\"moveInfos\":[{\"move\":\"D4\",\"visits\":10,\"winrate\":0.5,\"scoreMean\":0.0,\"scoreStdev\":4.2,\"policy\":0.12,\"pv\":[\"D4\",\"Q16\"],\"pvVisits\":[10,5],\"ownership\":[0.1,-0.1]}]}\n${raw_katago_comparison_checklist_content}")
set(relative_record_screenshot_content "P3\n2 1\n255\n255 255 255 0 0 0\n")
execute_process(
    COMMAND "${IMAGE_FIXTURE_WRITER}" "${record_4k_screenshot_path}"
    RESULT_VARIABLE image_fixture_result
    OUTPUT_VARIABLE image_fixture_output
    ERROR_VARIABLE image_fixture_error
)
if(NOT image_fixture_result EQUAL 0)
    message(FATAL_ERROR "Could not create 4K screenshot fixture: ${image_fixture_output}${image_fixture_error}")
endif()
string(SHA256 record_diagnostics_sha256 "${record_diagnostics_content}")
string(SHA256 record_report_sha256 "${record_report_content}")
string(SHA256 record_engine_log_sha256 "${record_engine_log_content}")
string(SHA256 mismatched_record_engine_log_sha256 "${mismatched_record_engine_log_content}")
string(SHA256 missing_gpu_record_engine_log_sha256 "${missing_gpu_record_engine_log_content}")
string(SHA256 record_raw_log_sha256 "${record_raw_log_content}")
string(SHA256 record_manual_ui_log_sha256 "${manual_ui_inspection_log_content}")
string(SHA256 record_external_verification_log_sha256 "${external_verification_log_content}")
string(SHA256 bad_record_manual_ui_log_sha256 "${bad_record_manual_ui_log_content}")
string(SHA256 bad_record_external_verification_log_sha256 "${bad_record_external_verification_log_content}")
string(SHA256 mismatched_record_raw_log_sha256 "${mismatched_record_raw_log_content}")
string(SHA256 record_screenshot_sha256 "${record_screenshot_content}")
string(SHA256 uniform_record_screenshot_sha256 "${uniform_record_screenshot_content}")
file(SHA256 "${record_4k_screenshot_path}" record_4k_screenshot_sha256)
string(SHA256 bad_record_diagnostics_sha256 "${bad_record_diagnostics_content}")
string(SHA256 missing_record_sha_record_diagnostics_sha256 "${missing_record_sha_record_diagnostics_content}")
string(SHA256 missing_final_blockers_record_diagnostics_sha256 "${missing_final_blockers_record_diagnostics_content}")
string(SHA256 mismatched_record_diagnostics_sha256 "${mismatched_record_diagnostics_content}")
string(SHA256 mismatched_record_diagnostics_app_sha256 "${mismatched_record_diagnostics_app_content}")
string(SHA256 mismatched_record_diagnostics_executable_sha256 "${mismatched_record_diagnostics_executable_content}")
string(SHA256 bad_record_report_sha256 "${bad_record_report_content}")
string(SHA256 missing_record_sha_record_report_sha256 "${missing_record_sha_record_report_content}")
string(SHA256 missing_final_blockers_record_report_sha256 "${missing_final_blockers_record_report_content}")
string(SHA256 mismatched_record_report_app_sha256 "${mismatched_record_report_app_content}")
string(SHA256 mismatched_record_report_executable_sha256 "${mismatched_record_report_executable_content}")
string(SHA256 mismatched_record_report_sha256 "${mismatched_record_report_content}")
string(SHA256 bad_record_raw_log_sha256 "${bad_record_raw_log_content}")
string(SHA256 incomplete_record_raw_log_sha256 "${incomplete_record_raw_log_content}")
string(SHA256 missing_winrate_record_raw_log_sha256 "${missing_winrate_record_raw_log_content}")
string(SHA256 missing_score_mean_record_raw_log_sha256 "${missing_score_mean_record_raw_log_content}")
string(SHA256 missing_score_stdev_record_raw_log_sha256 "${missing_score_stdev_record_raw_log_content}")
string(SHA256 missing_policy_record_raw_log_sha256 "${missing_policy_record_raw_log_content}")
string(SHA256 missing_visits_record_raw_log_sha256 "${missing_visits_record_raw_log_content}")
string(SHA256 missing_pv_record_raw_log_sha256 "${missing_pv_record_raw_log_content}")
string(SHA256 missing_pv_visits_record_raw_log_sha256 "${missing_pv_visits_record_raw_log_content}")
string(SHA256 missing_ownership_record_raw_log_sha256 "${missing_ownership_record_raw_log_content}")
string(SHA256 relative_record_diagnostics_sha256 "${relative_record_diagnostics_content}")
string(SHA256 relative_record_report_sha256 "${relative_record_report_content}")
string(SHA256 relative_record_engine_log_sha256 "${relative_record_engine_log_content}")
string(SHA256 relative_record_raw_log_sha256 "${relative_record_raw_log_content}")
string(SHA256 relative_record_screenshot_sha256 "${relative_record_screenshot_content}")
file(WRITE "${model_path}" "target acceptance report model fixture\n")
file(WRITE "${analysis_config_path}" "target acceptance report analysis config fixture\n")
file(WRITE "${gtp_config_path}" "target acceptance report gtp config fixture\n")
file(WRITE "${compare_model_path}" "target acceptance report compare model fixture\n")
file(WRITE "${compare_analysis_config_path}" "target acceptance report compare analysis config fixture\n")
file(WRITE "${compare_gtp_config_path}" "target acceptance report compare gtp config fixture\n")
file(WRITE "${record_diagnostics_path}" "${record_diagnostics_content}")
file(WRITE "${record_report_path}" "${record_report_content}")
file(WRITE "${record_engine_log_path}" "${record_engine_log_content}")
file(WRITE "${mismatched_record_engine_log_path}" "${mismatched_record_engine_log_content}")
file(WRITE "${missing_gpu_record_engine_log_path}" "${missing_gpu_record_engine_log_content}")
file(WRITE "${record_raw_log_path}" "${record_raw_log_content}")
file(WRITE "${record_manual_ui_log_path}" "${manual_ui_inspection_log_content}")
file(WRITE "${record_external_verification_log_path}" "${external_verification_log_content}")
file(WRITE "${bad_record_manual_ui_log_path}" "${bad_record_manual_ui_log_content}")
file(WRITE "${bad_record_external_verification_log_path}" "${bad_record_external_verification_log_content}")
file(WRITE "${mismatched_record_raw_log_path}" "${mismatched_record_raw_log_content}")
file(WRITE "${record_screenshot_path}" "${record_screenshot_content}")
file(WRITE "${uniform_record_screenshot_path}" "${uniform_record_screenshot_content}")
file(WRITE "${empty_record_diagnostics_path}" "")
file(WRITE "${bad_record_diagnostics_path}" "${bad_record_diagnostics_content}")
file(WRITE "${missing_record_sha_record_diagnostics_path}" "${missing_record_sha_record_diagnostics_content}")
file(WRITE "${missing_final_blockers_record_diagnostics_path}" "${missing_final_blockers_record_diagnostics_content}")
file(WRITE "${mismatched_record_diagnostics_path}" "${mismatched_record_diagnostics_content}")
file(WRITE "${mismatched_record_diagnostics_app_path}" "${mismatched_record_diagnostics_app_content}")
file(WRITE "${mismatched_record_diagnostics_executable_path}" "${mismatched_record_diagnostics_executable_content}")
file(WRITE "${bad_record_report_path}" "${bad_record_report_content}")
file(WRITE "${missing_record_sha_record_report_path}" "${missing_record_sha_record_report_content}")
file(WRITE "${missing_final_blockers_record_report_path}" "${missing_final_blockers_record_report_content}")
file(WRITE "${mismatched_record_report_app_path}" "${mismatched_record_report_app_content}")
file(WRITE "${mismatched_record_report_executable_path}" "${mismatched_record_report_executable_content}")
file(WRITE "${mismatched_record_report_path}" "${mismatched_record_report_content}")
file(WRITE "${bad_record_raw_log_path}" "${bad_record_raw_log_content}")
file(WRITE "${incomplete_record_raw_log_path}" "${incomplete_record_raw_log_content}")
file(WRITE "${missing_winrate_record_raw_log_path}" "${missing_winrate_record_raw_log_content}")
file(WRITE "${missing_score_mean_record_raw_log_path}" "${missing_score_mean_record_raw_log_content}")
file(WRITE "${missing_score_stdev_record_raw_log_path}" "${missing_score_stdev_record_raw_log_content}")
file(WRITE "${missing_policy_record_raw_log_path}" "${missing_policy_record_raw_log_content}")
file(WRITE "${missing_visits_record_raw_log_path}" "${missing_visits_record_raw_log_content}")
file(WRITE "${missing_pv_record_raw_log_path}" "${missing_pv_record_raw_log_content}")
file(WRITE "${missing_pv_visits_record_raw_log_path}" "${missing_pv_visits_record_raw_log_content}")
file(WRITE "${missing_ownership_record_raw_log_path}" "${missing_ownership_record_raw_log_content}")
file(MAKE_DIRECTORY "${relative_acceptance_record_evidence_dir}")
file(WRITE "${relative_acceptance_record_evidence_dir}/target-diagnostics.txt" "${relative_record_diagnostics_content}")
file(WRITE "${relative_acceptance_record_evidence_dir}/target-report.md" "${relative_record_report_content}")
file(WRITE "${relative_acceptance_record_evidence_dir}/target-engine.log" "${relative_record_engine_log_content}")
file(WRITE "${relative_acceptance_record_evidence_dir}/target-raw-katago.log" "${relative_record_raw_log_content}")
file(WRITE "${relative_acceptance_record_evidence_dir}/target-screenshot.ppm" "${relative_record_screenshot_content}")
string(TIMESTAMP acceptance_completed_utc "%Y-%m-%dT%H:%M:%SZ" UTC)
file(MAKE_DIRECTORY "${engine_working_dir}")
file(MAKE_DIRECTORY "${compare_working_dir}")
file(WRITE "${settings_file}" "
[engine]
executable=${APP_EXECUTABLE}
model=${model_path}
gtpConfig=${gtp_config_path}
analysisConfig=${analysis_config_path}
workingDirectory=${engine_working_dir}
extraArgs=--target-report-main \\\"spaced value\\\" \\\"\\\"

[compare]
executable=${APP_EXECUTABLE}
model=${compare_model_path}
gtpConfig=${compare_gtp_config_path}
analysisConfig=${compare_analysis_config_path}
workingDirectory=${compare_working_dir}
extraArgs=--target-report-compare \\\"compare spaced\\\" \\\"\\\"
")
file(WRITE "${mode_blocker_settings_file}" "
[engine]
executable=${APP_EXECUTABLE}
model=${model_path}
gtpConfig=${gtp_config_path}
workingDirectory=${engine_working_dir}
")
file(WRITE "${acceptance_record_file}" "
[metadata]
completedUtc=${acceptance_completed_utc}
appVersion=${expected_app_version}
appExecutableSha256=${expected_app_executable_sha256}
osPrettyName=${expected_os_pretty_name}
osKernelType=${expected_os_kernel_type}
osKernelVersion=${expected_os_kernel_version}
qtRuntimeVersion=${expected_qt_runtime_version}
qtBuildAbi=${expected_qt_build_abi}
cpuArchitecture=${expected_cpu_architecture}
reviewer=target tester
targetMachine=${expected_target_machine}
gpuDriver=NVIDIA 555.55
kataGoVersion=KataGo 1.15.3
notes=all target evidence attached

[evidence]
diagnostics=${record_diagnostics_path}
targetAcceptanceReport=${record_report_path}
engineLog=${record_engine_log_path}
rawKataGoComparisonLog=${record_raw_log_path}
manualUiInspectionLog=${record_manual_ui_log_path}
externalTargetVerificationLog=${record_external_verification_log_path}
screenshot=${record_screenshot_path}

[evidenceSha256]
diagnostics=${record_diagnostics_sha256}
targetAcceptanceReport=${record_report_sha256}
engineLog=${record_engine_log_sha256}
rawKataGoComparisonLog=${record_raw_log_sha256}
manualUiInspectionLog=${record_manual_ui_log_sha256}
externalTargetVerificationLog=${record_external_verification_log_sha256}
screenshot=${record_screenshot_sha256}

[evidenceContentMarkers]
diagnostics=LizzieYzy Qt Diagnostics; app.version; <record appVersion>; app.executable; <current app.executable>; plan.acceptance.recordFile.canonicalPath; <record canonical path>; plan.acceptance.recordFile.sha256; plan.acceptance.finalAcceptanceStatus; plan.acceptance.finalAcceptanceBlocker; plan.acceptance.finalAcceptanceOutstandingBlocker
targetAcceptanceReport=# Target Acceptance Report; app.version; <record appVersion>; app.executable; <current app.executable>; plan.acceptance.recordFile.canonicalPath; <record canonical path>; plan.acceptance.recordFile.sha256; plan.acceptance.finalAcceptanceStatus; plan.acceptance.finalAcceptanceBlocker; plan.acceptance.finalAcceptanceOutstandingBlocker
engineLog=KataGo; <record kataGoVersion>
engineLog.gpuOrStderrAny=CUDA; OpenCL; TensorRT; GPU; gpu; backend; Backend; stderr:; Stderr:
rawKataGoComparisonLog=KataGo; <record kataGoVersion>; kata-analyze; "moveInfos"; "rootInfo"; "winrate"; "scoreMean"; "scoreStdev"; "visits"; "policy"; "pv"; "pvVisits"; "ownership"; analysisJsonRawResponse; realtimeGtpRawLine; candidateTableRendering; pvPreview; winrateScorePerspective; ownershipDisplay; genmove; humanVsAi; selfPlay; engineVsEngine
	manualUiInspectionLog=mainWindow4KHighDpiLayout; boardLinesCoordinatesAndStarPoints; stoneRenderingAndCandidateLabels; candidateTableColumns; pvPreviewStones; ownershipMainBoardOverlay; ownershipMiniBoardDock; winrateScoreGraph; toolbarDockAndMenuLayout; bilingualTextFit; savedWindowRestore; multiDisplayPlacement
	externalTargetVerificationLog=linuxKdeWaylandNvidiaRealtimeKataGo; windowsNvidiaRealtimeKataGo; physical4KHighDpiMultiDisplayUi; rawKataGoUiComparison
	
	[recordHints]
	passValues=pass; passed; accepted; true; yes
	failValues=fail; failed; rejected; false; no
	metadataKeysRequired=completedUtc; appVersion; appExecutableSha256; osPrettyName; osKernelType; osKernelVersion; qtRuntimeVersion; qtBuildAbi; cpuArchitecture; reviewer; targetMachine; gpuDriver; kataGoVersion
	metadataExactMatchKeys=appVersion; appExecutableSha256; osPrettyName; osKernelType; osKernelVersion; qtRuntimeVersion; qtBuildAbi; cpuArchitecture; targetMachine
	metadataValueCheckedKeys=completedUtc; reviewer; gpuDriver; kataGoVersion
	completedUtcRequired=ISO-UTC-not-future
	aggregateAcceptanceKeysRequired=pass
	checklistItemsRequired=pass
	evidencePathsRequired=readable-non-empty-distinct
	evidenceSha256Required=true
	evidenceContentMarkersRequired=true
	recordAndEvidenceTimestampsMustNotAfterCompletedUtc=true
	
	[acceptance]
	linuxKdeWaylandNvidia=pass
	windowsNvidia=accepted
	physicalDisplay=yes
	externalTargetVerification=pass
	rawKataGoComparison=true
	manualUiInspection=passed

[checklist.linuxKdeWaylandNvidia]
linuxOs=pass
branchResync=pass

[checklist.externalTargetVerification]
linuxKdeWaylandNvidiaRealtimeKataGo=pass
windowsNvidiaRealtimeKataGo=pass
physical4KHighDpiMultiDisplayUi=pass
rawKataGoUiComparison=pass
")
file(WRITE "${acceptance_record_failed_manual_file}" "
[metadata]
completedUtc=${acceptance_completed_utc}
appVersion=${expected_app_version}
appExecutableSha256=${expected_app_executable_sha256}
osPrettyName=${expected_os_pretty_name}
osKernelType=${expected_os_kernel_type}
osKernelVersion=${expected_os_kernel_version}
qtRuntimeVersion=${expected_qt_runtime_version}
qtBuildAbi=${expected_qt_build_abi}
cpuArchitecture=${expected_cpu_architecture}
reviewer=target tester
targetMachine=${expected_target_machine}
gpuDriver=NVIDIA 555.55
kataGoVersion=KataGo 1.15.3

[evidence]
diagnostics=${record_diagnostics_path}
targetAcceptanceReport=${record_report_path}
engineLog=${record_engine_log_path}
rawKataGoComparisonLog=${record_raw_log_path}
manualUiInspectionLog=${record_manual_ui_log_path}
externalTargetVerificationLog=${record_external_verification_log_path}
screenshot=${record_screenshot_path}

[evidenceSha256]
diagnostics=${record_diagnostics_sha256}
targetAcceptanceReport=${record_report_sha256}
engineLog=${record_engine_log_sha256}
rawKataGoComparisonLog=${record_raw_log_sha256}
manualUiInspectionLog=${record_manual_ui_log_sha256}
externalTargetVerificationLog=${record_external_verification_log_sha256}
screenshot=${record_screenshot_sha256}

[acceptance]
linuxKdeWaylandNvidia=pass
windowsNvidia=pass
physicalDisplay=pass
externalTargetVerification=pass
rawKataGoComparison=fail
manualUiInspection=pass

[checklist.linuxKdeWaylandNvidia]
linuxOs=pass
branchResync=pass

[checklist.externalTargetVerification]
linuxKdeWaylandNvidiaRealtimeKataGo=pass
windowsNvidiaRealtimeKataGo=pass
physical4KHighDpiMultiDisplayUi=pass
rawKataGoUiComparison=pass
")
file(WRITE "${acceptance_record_invalid_manual_file}" "
[metadata]
completedUtc=${acceptance_completed_utc}
appVersion=${expected_app_version}
appExecutableSha256=${expected_app_executable_sha256}
osPrettyName=${expected_os_pretty_name}
osKernelType=${expected_os_kernel_type}
osKernelVersion=${expected_os_kernel_version}
qtRuntimeVersion=${expected_qt_runtime_version}
qtBuildAbi=${expected_qt_build_abi}
cpuArchitecture=${expected_cpu_architecture}
reviewer=target tester
targetMachine=${expected_target_machine}
gpuDriver=NVIDIA 555.55
kataGoVersion=KataGo 1.15.3

[evidence]
diagnostics=${record_diagnostics_path}
targetAcceptanceReport=${record_report_path}
engineLog=${record_engine_log_path}
rawKataGoComparisonLog=${record_raw_log_path}
manualUiInspectionLog=${record_manual_ui_log_path}
externalTargetVerificationLog=${record_external_verification_log_path}
screenshot=${record_screenshot_path}

[evidenceSha256]
diagnostics=${record_diagnostics_sha256}
targetAcceptanceReport=${record_report_sha256}
engineLog=${record_engine_log_sha256}
rawKataGoComparisonLog=${record_raw_log_sha256}
manualUiInspectionLog=${record_manual_ui_log_sha256}
externalTargetVerificationLog=${record_external_verification_log_sha256}
screenshot=${record_screenshot_sha256}

[acceptance]
linuxKdeWaylandNvidia=pass
windowsNvidia=pass
physicalDisplay=pass
externalTargetVerification=pass
rawKataGoComparison=pass
manualUiInspection=maybe

[checklist.linuxKdeWaylandNvidia]
linuxOs=pass
branchResync=pass

[checklist.externalTargetVerification]
linuxKdeWaylandNvidiaRealtimeKataGo=pass
windowsNvidiaRealtimeKataGo=pass
physical4KHighDpiMultiDisplayUi=pass
rawKataGoUiComparison=pass
")
file(WRITE "${acceptance_record_failed_checklist_file}" "
[acceptance]
externalTargetVerification=pass
rawKataGoComparison=pass
manualUiInspection=pass

[checklist.externalTargetVerification]
rawKataGoUiComparison=fail
")
file(WRITE "${acceptance_record_invalid_checklist_file}" "
[acceptance]
externalTargetVerification=pass
rawKataGoComparison=pass
manualUiInspection=pass

[checklist.externalTargetVerification]
rawKataGoUiComparison=maybe
")
file(WRITE "${acceptance_record_failed_target_component_file}" "
[acceptance]
externalTargetVerification=pass
linuxKdeWaylandNvidia=pass
windowsNvidia=fail
physicalDisplay=pass
rawKataGoComparison=pass
manualUiInspection=pass
")
file(WRITE "${acceptance_record_invalid_target_component_file}" "
[acceptance]
externalTargetVerification=pass
linuxKdeWaylandNvidia=pass
windowsNvidia=maybe
physicalDisplay=pass
rawKataGoComparison=pass
manualUiInspection=pass
")
file(WRITE "${acceptance_record_missing_evidence_file}" "
[metadata]
completedUtc=${acceptance_completed_utc}
appVersion=${expected_app_version}
appExecutableSha256=${expected_app_executable_sha256}
osPrettyName=${expected_os_pretty_name}
osKernelType=${expected_os_kernel_type}
osKernelVersion=${expected_os_kernel_version}
qtRuntimeVersion=${expected_qt_runtime_version}
qtBuildAbi=${expected_qt_build_abi}
cpuArchitecture=${expected_cpu_architecture}
reviewer=target tester
targetMachine=${expected_target_machine}
gpuDriver=NVIDIA 555.55
kataGoVersion=KataGo 1.15.3

[acceptance]
linuxKdeWaylandNvidia=pass
windowsNvidia=pass
physicalDisplay=pass
rawKataGoComparison=pass
manualUiInspection=pass
")
file(WRITE "${acceptance_record_missing_metadata_file}" "
[evidence]
diagnostics=${record_diagnostics_path}
targetAcceptanceReport=${record_report_path}
engineLog=${record_engine_log_path}
rawKataGoComparisonLog=${record_raw_log_path}
manualUiInspectionLog=${record_manual_ui_log_path}
externalTargetVerificationLog=${record_external_verification_log_path}
screenshot=${record_screenshot_path}

[evidenceSha256]
diagnostics=${record_diagnostics_sha256}
targetAcceptanceReport=${record_report_sha256}
engineLog=${record_engine_log_sha256}
rawKataGoComparisonLog=${record_raw_log_sha256}
manualUiInspectionLog=${record_manual_ui_log_sha256}
externalTargetVerificationLog=${record_external_verification_log_sha256}
screenshot=${record_screenshot_sha256}

[acceptance]
linuxKdeWaylandNvidia=pass
windowsNvidia=pass
physicalDisplay=pass
externalTargetVerification=pass
rawKataGoComparison=pass
manualUiInspection=pass
")
file(WRITE "${acceptance_record_invalid_metadata_file}" "
[metadata]
completedUtc=2026-07-02 12:34:56
appVersion=${expected_app_version}
appExecutableSha256=${expected_app_executable_sha256}
osPrettyName=${expected_os_pretty_name}
osKernelType=${expected_os_kernel_type}
osKernelVersion=${expected_os_kernel_version}
qtRuntimeVersion=${expected_qt_runtime_version}
qtBuildAbi=${expected_qt_build_abi}
cpuArchitecture=${expected_cpu_architecture}
reviewer=target tester
targetMachine=${expected_target_machine}
gpuDriver=NVIDIA 555.55
kataGoVersion=KataGo 1.15.3

[evidence]
diagnostics=${record_diagnostics_path}
targetAcceptanceReport=${record_report_path}
engineLog=${record_engine_log_path}
rawKataGoComparisonLog=${record_raw_log_path}
manualUiInspectionLog=${record_manual_ui_log_path}
externalTargetVerificationLog=${record_external_verification_log_path}
screenshot=${record_screenshot_path}

[evidenceSha256]
diagnostics=${record_diagnostics_sha256}
targetAcceptanceReport=${record_report_sha256}
engineLog=${record_engine_log_sha256}
rawKataGoComparisonLog=${record_raw_log_sha256}
manualUiInspectionLog=${record_manual_ui_log_sha256}
externalTargetVerificationLog=${record_external_verification_log_sha256}
screenshot=${record_screenshot_sha256}

[acceptance]
linuxKdeWaylandNvidia=pass
windowsNvidia=pass
physicalDisplay=pass
externalTargetVerification=pass
rawKataGoComparison=pass
manualUiInspection=pass
")
file(WRITE "${acceptance_record_future_metadata_file}" "
[metadata]
completedUtc=2999-01-01T00:00:00Z
appVersion=${expected_app_version}
appExecutableSha256=${expected_app_executable_sha256}
osPrettyName=${expected_os_pretty_name}
osKernelType=${expected_os_kernel_type}
osKernelVersion=${expected_os_kernel_version}
qtRuntimeVersion=${expected_qt_runtime_version}
qtBuildAbi=${expected_qt_build_abi}
cpuArchitecture=${expected_cpu_architecture}
reviewer=target tester
targetMachine=${expected_target_machine}
gpuDriver=NVIDIA 555.55
kataGoVersion=KataGo 1.15.3

[evidence]
diagnostics=${record_diagnostics_path}
targetAcceptanceReport=${record_report_path}
engineLog=${record_engine_log_path}
rawKataGoComparisonLog=${record_raw_log_path}
manualUiInspectionLog=${record_manual_ui_log_path}
externalTargetVerificationLog=${record_external_verification_log_path}
screenshot=${record_screenshot_path}

[evidenceSha256]
diagnostics=${record_diagnostics_sha256}
targetAcceptanceReport=${record_report_sha256}
engineLog=${record_engine_log_sha256}
rawKataGoComparisonLog=${record_raw_log_sha256}
manualUiInspectionLog=${record_manual_ui_log_sha256}
externalTargetVerificationLog=${record_external_verification_log_sha256}
screenshot=${record_screenshot_sha256}

[acceptance]
linuxKdeWaylandNvidia=pass
windowsNvidia=pass
physicalDisplay=pass
externalTargetVerification=pass
rawKataGoComparison=pass
manualUiInspection=pass
")
file(WRITE "${acceptance_record_empty_evidence_file}" "
[metadata]
completedUtc=${acceptance_completed_utc}
appVersion=${expected_app_version}
appExecutableSha256=${expected_app_executable_sha256}
osPrettyName=${expected_os_pretty_name}
osKernelType=${expected_os_kernel_type}
osKernelVersion=${expected_os_kernel_version}
qtRuntimeVersion=${expected_qt_runtime_version}
qtBuildAbi=${expected_qt_build_abi}
cpuArchitecture=${expected_cpu_architecture}
reviewer=target tester
targetMachine=${expected_target_machine}
gpuDriver=NVIDIA 555.55
kataGoVersion=KataGo 1.15.3

[evidence]
diagnostics=${empty_record_diagnostics_path}
targetAcceptanceReport=${record_report_path}
engineLog=${record_engine_log_path}
rawKataGoComparisonLog=${record_raw_log_path}
manualUiInspectionLog=${record_manual_ui_log_path}
externalTargetVerificationLog=${record_external_verification_log_path}
screenshot=${record_screenshot_path}

[acceptance]
linuxKdeWaylandNvidia=pass
windowsNvidia=pass
physicalDisplay=pass
externalTargetVerification=pass
rawKataGoComparison=pass
manualUiInspection=pass
")
file(WRITE "${acceptance_record_duplicate_evidence_file}" "
[metadata]
completedUtc=${acceptance_completed_utc}
appVersion=${expected_app_version}
appExecutableSha256=${expected_app_executable_sha256}
osPrettyName=${expected_os_pretty_name}
osKernelType=${expected_os_kernel_type}
osKernelVersion=${expected_os_kernel_version}
qtRuntimeVersion=${expected_qt_runtime_version}
qtBuildAbi=${expected_qt_build_abi}
cpuArchitecture=${expected_cpu_architecture}
reviewer=target tester
targetMachine=${expected_target_machine}
gpuDriver=NVIDIA 555.55
kataGoVersion=KataGo 1.15.3

[evidence]
diagnostics=${record_diagnostics_path}
targetAcceptanceReport=${record_report_path}
engineLog=${record_engine_log_path}
rawKataGoComparisonLog=${record_engine_log_path}
manualUiInspectionLog=${record_manual_ui_log_path}
externalTargetVerificationLog=${record_external_verification_log_path}
screenshot=${record_screenshot_path}

[evidenceSha256]
diagnostics=${record_diagnostics_sha256}
targetAcceptanceReport=${record_report_sha256}
engineLog=${record_engine_log_sha256}
rawKataGoComparisonLog=${record_engine_log_sha256}
manualUiInspectionLog=${record_manual_ui_log_sha256}
externalTargetVerificationLog=${record_external_verification_log_sha256}
screenshot=${record_screenshot_sha256}

[acceptance]
linuxKdeWaylandNvidia=pass
windowsNvidia=pass
physicalDisplay=pass
externalTargetVerification=pass
rawKataGoComparison=pass
manualUiInspection=pass
")
file(WRITE "${acceptance_record_uniform_screenshot_file}" "
[metadata]
completedUtc=${acceptance_completed_utc}
appVersion=${expected_app_version}
appExecutableSha256=${expected_app_executable_sha256}
osPrettyName=${expected_os_pretty_name}
osKernelType=${expected_os_kernel_type}
osKernelVersion=${expected_os_kernel_version}
qtRuntimeVersion=${expected_qt_runtime_version}
qtBuildAbi=${expected_qt_build_abi}
cpuArchitecture=${expected_cpu_architecture}
reviewer=target tester
targetMachine=${expected_target_machine}
gpuDriver=NVIDIA 555.55
kataGoVersion=KataGo 1.15.3

[evidence]
diagnostics=${record_diagnostics_path}
targetAcceptanceReport=${record_report_path}
engineLog=${record_engine_log_path}
rawKataGoComparisonLog=${record_raw_log_path}
manualUiInspectionLog=${record_manual_ui_log_path}
externalTargetVerificationLog=${record_external_verification_log_path}
screenshot=${uniform_record_screenshot_path}

[evidenceSha256]
diagnostics=${record_diagnostics_sha256}
targetAcceptanceReport=${record_report_sha256}
engineLog=${record_engine_log_sha256}
rawKataGoComparisonLog=${record_raw_log_sha256}
manualUiInspectionLog=${record_manual_ui_log_sha256}
externalTargetVerificationLog=${record_external_verification_log_sha256}
screenshot=${uniform_record_screenshot_sha256}

[acceptance]
linuxKdeWaylandNvidia=pass
windowsNvidia=pass
physicalDisplay=pass
externalTargetVerification=pass
rawKataGoComparison=pass
manualUiInspection=pass
")
file(WRITE "${acceptance_record_4k_screenshot_file}" "
[metadata]
completedUtc=${acceptance_completed_utc}
appVersion=${expected_app_version}
appExecutableSha256=${expected_app_executable_sha256}
osPrettyName=${expected_os_pretty_name}
osKernelType=${expected_os_kernel_type}
osKernelVersion=${expected_os_kernel_version}
qtRuntimeVersion=${expected_qt_runtime_version}
qtBuildAbi=${expected_qt_build_abi}
cpuArchitecture=${expected_cpu_architecture}
reviewer=target tester
targetMachine=${expected_target_machine}
gpuDriver=NVIDIA 555.55
kataGoVersion=KataGo 1.15.3

[evidence]
diagnostics=${record_diagnostics_path}
targetAcceptanceReport=${record_report_path}
engineLog=${record_engine_log_path}
rawKataGoComparisonLog=${record_raw_log_path}
manualUiInspectionLog=${record_manual_ui_log_path}
externalTargetVerificationLog=${record_external_verification_log_path}
screenshot=${record_4k_screenshot_path}

[evidenceSha256]
diagnostics=${record_diagnostics_sha256}
targetAcceptanceReport=${record_report_sha256}
engineLog=${record_engine_log_sha256}
rawKataGoComparisonLog=${record_raw_log_sha256}
manualUiInspectionLog=${record_manual_ui_log_sha256}
externalTargetVerificationLog=${record_external_verification_log_sha256}
screenshot=${record_4k_screenshot_sha256}

[acceptance]
linuxKdeWaylandNvidia=pass
windowsNvidia=pass
physicalDisplay=pass
externalTargetVerification=pass
rawKataGoComparison=pass
manualUiInspection=pass
")
file(WRITE "${acceptance_record_bad_evidence_content_file}" "
[metadata]
completedUtc=${acceptance_completed_utc}
appVersion=${expected_app_version}
appExecutableSha256=${expected_app_executable_sha256}
osPrettyName=${expected_os_pretty_name}
osKernelType=${expected_os_kernel_type}
osKernelVersion=${expected_os_kernel_version}
qtRuntimeVersion=${expected_qt_runtime_version}
qtBuildAbi=${expected_qt_build_abi}
cpuArchitecture=${expected_cpu_architecture}
reviewer=target tester
targetMachine=${expected_target_machine}
gpuDriver=NVIDIA 555.55
kataGoVersion=KataGo 1.15.3

[evidence]
diagnostics=${bad_record_diagnostics_path}
targetAcceptanceReport=${record_report_path}
engineLog=${record_engine_log_path}
rawKataGoComparisonLog=${record_raw_log_path}
manualUiInspectionLog=${record_manual_ui_log_path}
externalTargetVerificationLog=${record_external_verification_log_path}
screenshot=${record_screenshot_path}

[evidenceSha256]
diagnostics=${bad_record_diagnostics_sha256}
targetAcceptanceReport=${record_report_sha256}
engineLog=${record_engine_log_sha256}
rawKataGoComparisonLog=${record_raw_log_sha256}
manualUiInspectionLog=${record_manual_ui_log_sha256}
externalTargetVerificationLog=${record_external_verification_log_sha256}
screenshot=${record_screenshot_sha256}

[acceptance]
linuxKdeWaylandNvidia=pass
windowsNvidia=pass
physicalDisplay=pass
externalTargetVerification=pass
rawKataGoComparison=pass
manualUiInspection=pass
")
file(WRITE "${acceptance_record_missing_diagnostics_record_sha_file}" "
[metadata]
completedUtc=${acceptance_completed_utc}
appVersion=${expected_app_version}
appExecutableSha256=${expected_app_executable_sha256}
osPrettyName=${expected_os_pretty_name}
osKernelType=${expected_os_kernel_type}
osKernelVersion=${expected_os_kernel_version}
qtRuntimeVersion=${expected_qt_runtime_version}
qtBuildAbi=${expected_qt_build_abi}
cpuArchitecture=${expected_cpu_architecture}
reviewer=target tester
targetMachine=${expected_target_machine}
gpuDriver=NVIDIA 555.55
kataGoVersion=KataGo 1.15.3

[evidence]
diagnostics=${missing_record_sha_record_diagnostics_path}
targetAcceptanceReport=${record_report_path}
engineLog=${record_engine_log_path}
rawKataGoComparisonLog=${record_raw_log_path}
manualUiInspectionLog=${record_manual_ui_log_path}
externalTargetVerificationLog=${record_external_verification_log_path}
screenshot=${record_screenshot_path}

[evidenceSha256]
diagnostics=${missing_record_sha_record_diagnostics_sha256}
targetAcceptanceReport=${record_report_sha256}
engineLog=${record_engine_log_sha256}
rawKataGoComparisonLog=${record_raw_log_sha256}
manualUiInspectionLog=${record_manual_ui_log_sha256}
externalTargetVerificationLog=${record_external_verification_log_sha256}
screenshot=${record_screenshot_sha256}

[acceptance]
linuxKdeWaylandNvidia=pass
windowsNvidia=pass
physicalDisplay=pass
externalTargetVerification=pass
rawKataGoComparison=pass
manualUiInspection=pass
")
file(WRITE "${acceptance_record_missing_diagnostics_final_blockers_file}" "
[metadata]
completedUtc=${acceptance_completed_utc}
appVersion=${expected_app_version}
appExecutableSha256=${expected_app_executable_sha256}
osPrettyName=${expected_os_pretty_name}
osKernelType=${expected_os_kernel_type}
osKernelVersion=${expected_os_kernel_version}
qtRuntimeVersion=${expected_qt_runtime_version}
qtBuildAbi=${expected_qt_build_abi}
cpuArchitecture=${expected_cpu_architecture}
reviewer=target tester
targetMachine=${expected_target_machine}
gpuDriver=NVIDIA 555.55
kataGoVersion=KataGo 1.15.3

[evidence]
diagnostics=${missing_final_blockers_record_diagnostics_path}
targetAcceptanceReport=${record_report_path}
engineLog=${record_engine_log_path}
rawKataGoComparisonLog=${record_raw_log_path}
manualUiInspectionLog=${record_manual_ui_log_path}
externalTargetVerificationLog=${record_external_verification_log_path}
screenshot=${record_screenshot_path}

[evidenceSha256]
diagnostics=${missing_final_blockers_record_diagnostics_sha256}
targetAcceptanceReport=${record_report_sha256}
engineLog=${record_engine_log_sha256}
rawKataGoComparisonLog=${record_raw_log_sha256}
manualUiInspectionLog=${record_manual_ui_log_sha256}
externalTargetVerificationLog=${record_external_verification_log_sha256}
screenshot=${record_screenshot_sha256}

[acceptance]
linuxKdeWaylandNvidia=pass
windowsNvidia=pass
physicalDisplay=pass
externalTargetVerification=pass
rawKataGoComparison=pass
manualUiInspection=pass
")
file(WRITE "${acceptance_record_mismatched_diagnostics_record_path_file}" "
[metadata]
completedUtc=${acceptance_completed_utc}
appVersion=${expected_app_version}
appExecutableSha256=${expected_app_executable_sha256}
osPrettyName=${expected_os_pretty_name}
osKernelType=${expected_os_kernel_type}
osKernelVersion=${expected_os_kernel_version}
qtRuntimeVersion=${expected_qt_runtime_version}
qtBuildAbi=${expected_qt_build_abi}
cpuArchitecture=${expected_cpu_architecture}
reviewer=target tester
targetMachine=${expected_target_machine}
gpuDriver=NVIDIA 555.55
kataGoVersion=KataGo 1.15.3

[evidence]
diagnostics=${mismatched_record_diagnostics_path}
targetAcceptanceReport=${record_report_path}
engineLog=${record_engine_log_path}
rawKataGoComparisonLog=${record_raw_log_path}
manualUiInspectionLog=${record_manual_ui_log_path}
externalTargetVerificationLog=${record_external_verification_log_path}
screenshot=${record_screenshot_path}

[evidenceSha256]
diagnostics=${mismatched_record_diagnostics_sha256}
targetAcceptanceReport=${record_report_sha256}
engineLog=${record_engine_log_sha256}
rawKataGoComparisonLog=${record_raw_log_sha256}
manualUiInspectionLog=${record_manual_ui_log_sha256}
externalTargetVerificationLog=${record_external_verification_log_sha256}
screenshot=${record_screenshot_sha256}

[acceptance]
linuxKdeWaylandNvidia=pass
windowsNvidia=pass
physicalDisplay=pass
externalTargetVerification=pass
rawKataGoComparison=pass
manualUiInspection=pass
")
file(WRITE "${acceptance_record_mismatched_diagnostics_app_file}" "
[metadata]
completedUtc=${acceptance_completed_utc}
appVersion=${expected_app_version}
appExecutableSha256=${expected_app_executable_sha256}
osPrettyName=${expected_os_pretty_name}
osKernelType=${expected_os_kernel_type}
osKernelVersion=${expected_os_kernel_version}
qtRuntimeVersion=${expected_qt_runtime_version}
qtBuildAbi=${expected_qt_build_abi}
cpuArchitecture=${expected_cpu_architecture}
reviewer=target tester
targetMachine=${expected_target_machine}
gpuDriver=NVIDIA 555.55
kataGoVersion=KataGo 1.15.3

[evidence]
diagnostics=${mismatched_record_diagnostics_app_path}
targetAcceptanceReport=${record_report_path}
engineLog=${record_engine_log_path}
rawKataGoComparisonLog=${record_raw_log_path}
manualUiInspectionLog=${record_manual_ui_log_path}
externalTargetVerificationLog=${record_external_verification_log_path}
screenshot=${record_screenshot_path}

[evidenceSha256]
diagnostics=${mismatched_record_diagnostics_app_sha256}
targetAcceptanceReport=${record_report_sha256}
engineLog=${record_engine_log_sha256}
rawKataGoComparisonLog=${record_raw_log_sha256}
manualUiInspectionLog=${record_manual_ui_log_sha256}
externalTargetVerificationLog=${record_external_verification_log_sha256}
screenshot=${record_screenshot_sha256}

[acceptance]
linuxKdeWaylandNvidia=pass
windowsNvidia=pass
physicalDisplay=pass
externalTargetVerification=pass
rawKataGoComparison=pass
manualUiInspection=pass
")
file(WRITE "${acceptance_record_mismatched_diagnostics_executable_file}" "
[metadata]
completedUtc=${acceptance_completed_utc}
appVersion=${expected_app_version}
appExecutableSha256=${expected_app_executable_sha256}
osPrettyName=${expected_os_pretty_name}
osKernelType=${expected_os_kernel_type}
osKernelVersion=${expected_os_kernel_version}
qtRuntimeVersion=${expected_qt_runtime_version}
qtBuildAbi=${expected_qt_build_abi}
cpuArchitecture=${expected_cpu_architecture}
reviewer=target tester
targetMachine=${expected_target_machine}
gpuDriver=NVIDIA 555.55
kataGoVersion=KataGo 1.15.3

[evidence]
diagnostics=${mismatched_record_diagnostics_executable_path}
targetAcceptanceReport=${record_report_path}
engineLog=${record_engine_log_path}
rawKataGoComparisonLog=${record_raw_log_path}
manualUiInspectionLog=${record_manual_ui_log_path}
externalTargetVerificationLog=${record_external_verification_log_path}
screenshot=${record_screenshot_path}

[evidenceSha256]
diagnostics=${mismatched_record_diagnostics_executable_sha256}
targetAcceptanceReport=${record_report_sha256}
engineLog=${record_engine_log_sha256}
rawKataGoComparisonLog=${record_raw_log_sha256}
manualUiInspectionLog=${record_manual_ui_log_sha256}
externalTargetVerificationLog=${record_external_verification_log_sha256}
screenshot=${record_screenshot_sha256}

[acceptance]
linuxKdeWaylandNvidia=pass
windowsNvidia=pass
physicalDisplay=pass
externalTargetVerification=pass
rawKataGoComparison=pass
manualUiInspection=pass
")
file(WRITE "${acceptance_record_bad_report_content_file}" "
[metadata]
completedUtc=${acceptance_completed_utc}
appVersion=${expected_app_version}
appExecutableSha256=${expected_app_executable_sha256}
osPrettyName=${expected_os_pretty_name}
osKernelType=${expected_os_kernel_type}
osKernelVersion=${expected_os_kernel_version}
qtRuntimeVersion=${expected_qt_runtime_version}
qtBuildAbi=${expected_qt_build_abi}
cpuArchitecture=${expected_cpu_architecture}
reviewer=target tester
targetMachine=${expected_target_machine}
gpuDriver=NVIDIA 555.55
kataGoVersion=KataGo 1.15.3

[evidence]
diagnostics=${record_diagnostics_path}
targetAcceptanceReport=${bad_record_report_path}
engineLog=${record_engine_log_path}
rawKataGoComparisonLog=${record_raw_log_path}
manualUiInspectionLog=${record_manual_ui_log_path}
externalTargetVerificationLog=${record_external_verification_log_path}
screenshot=${record_screenshot_path}

[evidenceSha256]
diagnostics=${record_diagnostics_sha256}
targetAcceptanceReport=${bad_record_report_sha256}
engineLog=${record_engine_log_sha256}
rawKataGoComparisonLog=${record_raw_log_sha256}
manualUiInspectionLog=${record_manual_ui_log_sha256}
externalTargetVerificationLog=${record_external_verification_log_sha256}
screenshot=${record_screenshot_sha256}

[acceptance]
linuxKdeWaylandNvidia=pass
windowsNvidia=pass
physicalDisplay=pass
externalTargetVerification=pass
rawKataGoComparison=pass
manualUiInspection=pass
")
file(WRITE "${acceptance_record_missing_report_record_sha_file}" "
[metadata]
completedUtc=${acceptance_completed_utc}
appVersion=${expected_app_version}
appExecutableSha256=${expected_app_executable_sha256}
osPrettyName=${expected_os_pretty_name}
osKernelType=${expected_os_kernel_type}
osKernelVersion=${expected_os_kernel_version}
qtRuntimeVersion=${expected_qt_runtime_version}
qtBuildAbi=${expected_qt_build_abi}
cpuArchitecture=${expected_cpu_architecture}
reviewer=target tester
targetMachine=${expected_target_machine}
gpuDriver=NVIDIA 555.55
kataGoVersion=KataGo 1.15.3

[evidence]
diagnostics=${record_diagnostics_path}
targetAcceptanceReport=${missing_record_sha_record_report_path}
engineLog=${record_engine_log_path}
rawKataGoComparisonLog=${record_raw_log_path}
manualUiInspectionLog=${record_manual_ui_log_path}
externalTargetVerificationLog=${record_external_verification_log_path}
screenshot=${record_screenshot_path}

[evidenceSha256]
diagnostics=${record_diagnostics_sha256}
targetAcceptanceReport=${missing_record_sha_record_report_sha256}
engineLog=${record_engine_log_sha256}
rawKataGoComparisonLog=${record_raw_log_sha256}
manualUiInspectionLog=${record_manual_ui_log_sha256}
externalTargetVerificationLog=${record_external_verification_log_sha256}
screenshot=${record_screenshot_sha256}

[acceptance]
linuxKdeWaylandNvidia=pass
windowsNvidia=pass
physicalDisplay=pass
externalTargetVerification=pass
rawKataGoComparison=pass
manualUiInspection=pass
")
file(WRITE "${acceptance_record_missing_report_final_blockers_file}" "
[metadata]
completedUtc=${acceptance_completed_utc}
appVersion=${expected_app_version}
appExecutableSha256=${expected_app_executable_sha256}
osPrettyName=${expected_os_pretty_name}
osKernelType=${expected_os_kernel_type}
osKernelVersion=${expected_os_kernel_version}
qtRuntimeVersion=${expected_qt_runtime_version}
qtBuildAbi=${expected_qt_build_abi}
cpuArchitecture=${expected_cpu_architecture}
reviewer=target tester
targetMachine=${expected_target_machine}
gpuDriver=NVIDIA 555.55
kataGoVersion=KataGo 1.15.3

[evidence]
diagnostics=${record_diagnostics_path}
targetAcceptanceReport=${missing_final_blockers_record_report_path}
engineLog=${record_engine_log_path}
rawKataGoComparisonLog=${record_raw_log_path}
manualUiInspectionLog=${record_manual_ui_log_path}
externalTargetVerificationLog=${record_external_verification_log_path}
screenshot=${record_screenshot_path}

[evidenceSha256]
diagnostics=${record_diagnostics_sha256}
targetAcceptanceReport=${missing_final_blockers_record_report_sha256}
engineLog=${record_engine_log_sha256}
rawKataGoComparisonLog=${record_raw_log_sha256}
manualUiInspectionLog=${record_manual_ui_log_sha256}
externalTargetVerificationLog=${record_external_verification_log_sha256}
screenshot=${record_screenshot_sha256}

[acceptance]
linuxKdeWaylandNvidia=pass
windowsNvidia=pass
physicalDisplay=pass
externalTargetVerification=pass
rawKataGoComparison=pass
manualUiInspection=pass
")
file(WRITE "${acceptance_record_mismatched_report_app_file}" "
[metadata]
completedUtc=${acceptance_completed_utc}
appVersion=${expected_app_version}
appExecutableSha256=${expected_app_executable_sha256}
osPrettyName=${expected_os_pretty_name}
osKernelType=${expected_os_kernel_type}
osKernelVersion=${expected_os_kernel_version}
qtRuntimeVersion=${expected_qt_runtime_version}
qtBuildAbi=${expected_qt_build_abi}
cpuArchitecture=${expected_cpu_architecture}
reviewer=target tester
targetMachine=${expected_target_machine}
gpuDriver=NVIDIA 555.55
kataGoVersion=KataGo 1.15.3

[evidence]
diagnostics=${record_diagnostics_path}
targetAcceptanceReport=${mismatched_record_report_app_path}
engineLog=${record_engine_log_path}
rawKataGoComparisonLog=${record_raw_log_path}
manualUiInspectionLog=${record_manual_ui_log_path}
externalTargetVerificationLog=${record_external_verification_log_path}
screenshot=${record_screenshot_path}

[evidenceSha256]
diagnostics=${record_diagnostics_sha256}
targetAcceptanceReport=${mismatched_record_report_app_sha256}
engineLog=${record_engine_log_sha256}
rawKataGoComparisonLog=${record_raw_log_sha256}
manualUiInspectionLog=${record_manual_ui_log_sha256}
externalTargetVerificationLog=${record_external_verification_log_sha256}
screenshot=${record_screenshot_sha256}

[acceptance]
linuxKdeWaylandNvidia=pass
windowsNvidia=pass
physicalDisplay=pass
externalTargetVerification=pass
rawKataGoComparison=pass
manualUiInspection=pass
")
file(WRITE "${acceptance_record_mismatched_report_executable_file}" "
[metadata]
completedUtc=${acceptance_completed_utc}
appVersion=${expected_app_version}
appExecutableSha256=${expected_app_executable_sha256}
osPrettyName=${expected_os_pretty_name}
osKernelType=${expected_os_kernel_type}
osKernelVersion=${expected_os_kernel_version}
qtRuntimeVersion=${expected_qt_runtime_version}
qtBuildAbi=${expected_qt_build_abi}
cpuArchitecture=${expected_cpu_architecture}
reviewer=target tester
targetMachine=${expected_target_machine}
gpuDriver=NVIDIA 555.55
kataGoVersion=KataGo 1.15.3

[evidence]
diagnostics=${record_diagnostics_path}
targetAcceptanceReport=${mismatched_record_report_executable_path}
engineLog=${record_engine_log_path}
rawKataGoComparisonLog=${record_raw_log_path}
manualUiInspectionLog=${record_manual_ui_log_path}
externalTargetVerificationLog=${record_external_verification_log_path}
screenshot=${record_screenshot_path}

[evidenceSha256]
diagnostics=${record_diagnostics_sha256}
targetAcceptanceReport=${mismatched_record_report_executable_sha256}
engineLog=${record_engine_log_sha256}
rawKataGoComparisonLog=${record_raw_log_sha256}
manualUiInspectionLog=${record_manual_ui_log_sha256}
externalTargetVerificationLog=${record_external_verification_log_sha256}
screenshot=${record_screenshot_sha256}

[acceptance]
linuxKdeWaylandNvidia=pass
windowsNvidia=pass
physicalDisplay=pass
externalTargetVerification=pass
rawKataGoComparison=pass
manualUiInspection=pass
")
file(WRITE "${acceptance_record_mismatched_report_record_path_file}" "
[metadata]
completedUtc=${acceptance_completed_utc}
appVersion=${expected_app_version}
appExecutableSha256=${expected_app_executable_sha256}
osPrettyName=${expected_os_pretty_name}
osKernelType=${expected_os_kernel_type}
osKernelVersion=${expected_os_kernel_version}
qtRuntimeVersion=${expected_qt_runtime_version}
qtBuildAbi=${expected_qt_build_abi}
cpuArchitecture=${expected_cpu_architecture}
reviewer=target tester
targetMachine=${expected_target_machine}
gpuDriver=NVIDIA 555.55
kataGoVersion=KataGo 1.15.3

[evidence]
diagnostics=${record_diagnostics_path}
targetAcceptanceReport=${mismatched_record_report_path}
engineLog=${record_engine_log_path}
rawKataGoComparisonLog=${record_raw_log_path}
manualUiInspectionLog=${record_manual_ui_log_path}
externalTargetVerificationLog=${record_external_verification_log_path}
screenshot=${record_screenshot_path}

[evidenceSha256]
diagnostics=${record_diagnostics_sha256}
targetAcceptanceReport=${mismatched_record_report_sha256}
engineLog=${record_engine_log_sha256}
rawKataGoComparisonLog=${record_raw_log_sha256}
manualUiInspectionLog=${record_manual_ui_log_sha256}
externalTargetVerificationLog=${record_external_verification_log_sha256}
screenshot=${record_screenshot_sha256}

[acceptance]
linuxKdeWaylandNvidia=pass
windowsNvidia=pass
physicalDisplay=pass
externalTargetVerification=pass
rawKataGoComparison=pass
manualUiInspection=pass
")
file(WRITE "${acceptance_record_mismatched_engine_log_version_file}" "
[metadata]
completedUtc=${acceptance_completed_utc}
appVersion=${expected_app_version}
appExecutableSha256=${expected_app_executable_sha256}
osPrettyName=${expected_os_pretty_name}
osKernelType=${expected_os_kernel_type}
osKernelVersion=${expected_os_kernel_version}
qtRuntimeVersion=${expected_qt_runtime_version}
qtBuildAbi=${expected_qt_build_abi}
cpuArchitecture=${expected_cpu_architecture}
reviewer=target tester
targetMachine=${expected_target_machine}
gpuDriver=NVIDIA 555.55
kataGoVersion=KataGo 1.15.3

[evidence]
diagnostics=${record_diagnostics_path}
targetAcceptanceReport=${record_report_path}
engineLog=${mismatched_record_engine_log_path}
rawKataGoComparisonLog=${record_raw_log_path}
manualUiInspectionLog=${record_manual_ui_log_path}
externalTargetVerificationLog=${record_external_verification_log_path}
screenshot=${record_screenshot_path}

[evidenceSha256]
diagnostics=${record_diagnostics_sha256}
targetAcceptanceReport=${record_report_sha256}
engineLog=${mismatched_record_engine_log_sha256}
rawKataGoComparisonLog=${record_raw_log_sha256}
manualUiInspectionLog=${record_manual_ui_log_sha256}
externalTargetVerificationLog=${record_external_verification_log_sha256}
screenshot=${record_screenshot_sha256}

[acceptance]
linuxKdeWaylandNvidia=pass
windowsNvidia=pass
physicalDisplay=pass
externalTargetVerification=pass
rawKataGoComparison=pass
manualUiInspection=pass
")
file(WRITE "${acceptance_record_missing_engine_gpu_stderr_file}" "
[metadata]
completedUtc=${acceptance_completed_utc}
appVersion=${expected_app_version}
appExecutableSha256=${expected_app_executable_sha256}
osPrettyName=${expected_os_pretty_name}
osKernelType=${expected_os_kernel_type}
osKernelVersion=${expected_os_kernel_version}
qtRuntimeVersion=${expected_qt_runtime_version}
qtBuildAbi=${expected_qt_build_abi}
cpuArchitecture=${expected_cpu_architecture}
reviewer=target tester
targetMachine=${expected_target_machine}
gpuDriver=NVIDIA 555.55
kataGoVersion=KataGo 1.15.3

[evidence]
diagnostics=${record_diagnostics_path}
targetAcceptanceReport=${record_report_path}
engineLog=${missing_gpu_record_engine_log_path}
rawKataGoComparisonLog=${record_raw_log_path}
manualUiInspectionLog=${record_manual_ui_log_path}
externalTargetVerificationLog=${record_external_verification_log_path}
screenshot=${record_screenshot_path}

[evidenceSha256]
diagnostics=${record_diagnostics_sha256}
targetAcceptanceReport=${record_report_sha256}
engineLog=${missing_gpu_record_engine_log_sha256}
rawKataGoComparisonLog=${record_raw_log_sha256}
manualUiInspectionLog=${record_manual_ui_log_sha256}
externalTargetVerificationLog=${record_external_verification_log_sha256}
screenshot=${record_screenshot_sha256}

[acceptance]
linuxKdeWaylandNvidia=pass
windowsNvidia=pass
physicalDisplay=pass
externalTargetVerification=pass
rawKataGoComparison=pass
manualUiInspection=pass
")
file(WRITE "${acceptance_record_mismatched_raw_log_version_file}" "
[metadata]
completedUtc=${acceptance_completed_utc}
appVersion=${expected_app_version}
appExecutableSha256=${expected_app_executable_sha256}
osPrettyName=${expected_os_pretty_name}
osKernelType=${expected_os_kernel_type}
osKernelVersion=${expected_os_kernel_version}
qtRuntimeVersion=${expected_qt_runtime_version}
qtBuildAbi=${expected_qt_build_abi}
cpuArchitecture=${expected_cpu_architecture}
reviewer=target tester
targetMachine=${expected_target_machine}
gpuDriver=NVIDIA 555.55
kataGoVersion=KataGo 1.15.3

[evidence]
diagnostics=${record_diagnostics_path}
targetAcceptanceReport=${record_report_path}
engineLog=${record_engine_log_path}
rawKataGoComparisonLog=${mismatched_record_raw_log_path}
manualUiInspectionLog=${record_manual_ui_log_path}
externalTargetVerificationLog=${record_external_verification_log_path}
screenshot=${record_screenshot_path}

[evidenceSha256]
diagnostics=${record_diagnostics_sha256}
targetAcceptanceReport=${record_report_sha256}
engineLog=${record_engine_log_sha256}
rawKataGoComparisonLog=${mismatched_record_raw_log_sha256}
manualUiInspectionLog=${record_manual_ui_log_sha256}
externalTargetVerificationLog=${record_external_verification_log_sha256}
screenshot=${record_screenshot_sha256}

[acceptance]
linuxKdeWaylandNvidia=pass
windowsNvidia=pass
physicalDisplay=pass
externalTargetVerification=pass
rawKataGoComparison=pass
manualUiInspection=pass
")
file(WRITE "${acceptance_record_bad_raw_log_content_file}" "
[metadata]
completedUtc=${acceptance_completed_utc}
appVersion=${expected_app_version}
appExecutableSha256=${expected_app_executable_sha256}
osPrettyName=${expected_os_pretty_name}
osKernelType=${expected_os_kernel_type}
osKernelVersion=${expected_os_kernel_version}
qtRuntimeVersion=${expected_qt_runtime_version}
qtBuildAbi=${expected_qt_build_abi}
cpuArchitecture=${expected_cpu_architecture}
reviewer=target tester
targetMachine=${expected_target_machine}
gpuDriver=NVIDIA 555.55
kataGoVersion=KataGo 1.15.3

[evidence]
diagnostics=${record_diagnostics_path}
targetAcceptanceReport=${record_report_path}
engineLog=${record_engine_log_path}
rawKataGoComparisonLog=${bad_record_raw_log_path}
manualUiInspectionLog=${record_manual_ui_log_path}
externalTargetVerificationLog=${record_external_verification_log_path}
screenshot=${record_screenshot_path}

[evidenceSha256]
diagnostics=${record_diagnostics_sha256}
targetAcceptanceReport=${record_report_sha256}
engineLog=${record_engine_log_sha256}
rawKataGoComparisonLog=${bad_record_raw_log_sha256}
manualUiInspectionLog=${record_manual_ui_log_sha256}
externalTargetVerificationLog=${record_external_verification_log_sha256}
screenshot=${record_screenshot_sha256}

[acceptance]
linuxKdeWaylandNvidia=pass
windowsNvidia=pass
physicalDisplay=pass
externalTargetVerification=pass
rawKataGoComparison=pass
manualUiInspection=pass
")
file(WRITE "${acceptance_record_bad_manual_ui_log_content_file}" "
[metadata]
completedUtc=${acceptance_completed_utc}
appVersion=${expected_app_version}
appExecutableSha256=${expected_app_executable_sha256}
osPrettyName=${expected_os_pretty_name}
osKernelType=${expected_os_kernel_type}
osKernelVersion=${expected_os_kernel_version}
qtRuntimeVersion=${expected_qt_runtime_version}
qtBuildAbi=${expected_qt_build_abi}
cpuArchitecture=${expected_cpu_architecture}
reviewer=target tester
targetMachine=${expected_target_machine}
gpuDriver=NVIDIA 555.55
kataGoVersion=KataGo 1.15.3

[evidence]
diagnostics=${record_diagnostics_path}
targetAcceptanceReport=${record_report_path}
engineLog=${record_engine_log_path}
rawKataGoComparisonLog=${record_raw_log_path}
manualUiInspectionLog=${bad_record_manual_ui_log_path}
externalTargetVerificationLog=${record_external_verification_log_path}
screenshot=${record_screenshot_path}

[evidenceSha256]
diagnostics=${record_diagnostics_sha256}
targetAcceptanceReport=${record_report_sha256}
engineLog=${record_engine_log_sha256}
rawKataGoComparisonLog=${record_raw_log_sha256}
manualUiInspectionLog=${bad_record_manual_ui_log_sha256}
externalTargetVerificationLog=${record_external_verification_log_sha256}
screenshot=${record_screenshot_sha256}

[acceptance]
linuxKdeWaylandNvidia=pass
windowsNvidia=pass
physicalDisplay=pass
externalTargetVerification=pass
rawKataGoComparison=pass
manualUiInspection=pass
")
file(WRITE "${acceptance_record_bad_external_verification_log_content_file}" "
[metadata]
completedUtc=${acceptance_completed_utc}
appVersion=${expected_app_version}
appExecutableSha256=${expected_app_executable_sha256}
osPrettyName=${expected_os_pretty_name}
osKernelType=${expected_os_kernel_type}
osKernelVersion=${expected_os_kernel_version}
qtRuntimeVersion=${expected_qt_runtime_version}
qtBuildAbi=${expected_qt_build_abi}
cpuArchitecture=${expected_cpu_architecture}
reviewer=target tester
targetMachine=${expected_target_machine}
gpuDriver=NVIDIA 555.55
kataGoVersion=KataGo 1.15.3

[evidence]
diagnostics=${record_diagnostics_path}
targetAcceptanceReport=${record_report_path}
engineLog=${record_engine_log_path}
rawKataGoComparisonLog=${record_raw_log_path}
manualUiInspectionLog=${record_manual_ui_log_path}
externalTargetVerificationLog=${bad_record_external_verification_log_path}
screenshot=${record_screenshot_path}

[evidenceSha256]
diagnostics=${record_diagnostics_sha256}
targetAcceptanceReport=${record_report_sha256}
engineLog=${record_engine_log_sha256}
rawKataGoComparisonLog=${record_raw_log_sha256}
manualUiInspectionLog=${record_manual_ui_log_sha256}
externalTargetVerificationLog=${bad_record_external_verification_log_sha256}
screenshot=${record_screenshot_sha256}

[acceptance]
linuxKdeWaylandNvidia=pass
windowsNvidia=pass
physicalDisplay=pass
externalTargetVerification=pass
rawKataGoComparison=pass
manualUiInspection=pass
")
file(WRITE "${acceptance_record_incomplete_raw_log_content_file}" "
[metadata]
completedUtc=${acceptance_completed_utc}
appVersion=${expected_app_version}
appExecutableSha256=${expected_app_executable_sha256}
osPrettyName=${expected_os_pretty_name}
osKernelType=${expected_os_kernel_type}
osKernelVersion=${expected_os_kernel_version}
qtRuntimeVersion=${expected_qt_runtime_version}
qtBuildAbi=${expected_qt_build_abi}
cpuArchitecture=${expected_cpu_architecture}
reviewer=target tester
targetMachine=${expected_target_machine}
gpuDriver=NVIDIA 555.55
kataGoVersion=KataGo 1.15.3

[evidence]
diagnostics=${record_diagnostics_path}
targetAcceptanceReport=${record_report_path}
engineLog=${record_engine_log_path}
rawKataGoComparisonLog=${incomplete_record_raw_log_path}
manualUiInspectionLog=${record_manual_ui_log_path}
externalTargetVerificationLog=${record_external_verification_log_path}
screenshot=${record_screenshot_path}

[evidenceSha256]
diagnostics=${record_diagnostics_sha256}
targetAcceptanceReport=${record_report_sha256}
engineLog=${record_engine_log_sha256}
rawKataGoComparisonLog=${incomplete_record_raw_log_sha256}
manualUiInspectionLog=${record_manual_ui_log_sha256}
externalTargetVerificationLog=${record_external_verification_log_sha256}
screenshot=${record_screenshot_sha256}

[acceptance]
linuxKdeWaylandNvidia=pass
windowsNvidia=pass
physicalDisplay=pass
externalTargetVerification=pass
rawKataGoComparison=pass
manualUiInspection=pass
")
file(WRITE "${acceptance_record_missing_raw_log_winrate_file}" "
[metadata]
completedUtc=${acceptance_completed_utc}
appVersion=${expected_app_version}
appExecutableSha256=${expected_app_executable_sha256}
osPrettyName=${expected_os_pretty_name}
osKernelType=${expected_os_kernel_type}
osKernelVersion=${expected_os_kernel_version}
qtRuntimeVersion=${expected_qt_runtime_version}
qtBuildAbi=${expected_qt_build_abi}
cpuArchitecture=${expected_cpu_architecture}
reviewer=target tester
targetMachine=${expected_target_machine}
gpuDriver=NVIDIA 555.55
kataGoVersion=KataGo 1.15.3

[evidence]
diagnostics=${record_diagnostics_path}
targetAcceptanceReport=${record_report_path}
engineLog=${record_engine_log_path}
rawKataGoComparisonLog=${missing_winrate_record_raw_log_path}
manualUiInspectionLog=${record_manual_ui_log_path}
externalTargetVerificationLog=${record_external_verification_log_path}
screenshot=${record_screenshot_path}

[evidenceSha256]
diagnostics=${record_diagnostics_sha256}
targetAcceptanceReport=${record_report_sha256}
engineLog=${record_engine_log_sha256}
rawKataGoComparisonLog=${missing_winrate_record_raw_log_sha256}
manualUiInspectionLog=${record_manual_ui_log_sha256}
externalTargetVerificationLog=${record_external_verification_log_sha256}
screenshot=${record_screenshot_sha256}

[acceptance]
linuxKdeWaylandNvidia=pass
windowsNvidia=pass
physicalDisplay=pass
externalTargetVerification=pass
rawKataGoComparison=pass
manualUiInspection=pass
")
file(WRITE "${acceptance_record_missing_raw_log_score_mean_file}" "
[metadata]
completedUtc=${acceptance_completed_utc}
appVersion=${expected_app_version}
appExecutableSha256=${expected_app_executable_sha256}
osPrettyName=${expected_os_pretty_name}
osKernelType=${expected_os_kernel_type}
osKernelVersion=${expected_os_kernel_version}
qtRuntimeVersion=${expected_qt_runtime_version}
qtBuildAbi=${expected_qt_build_abi}
cpuArchitecture=${expected_cpu_architecture}
reviewer=target tester
targetMachine=${expected_target_machine}
gpuDriver=NVIDIA 555.55
kataGoVersion=KataGo 1.15.3

[evidence]
diagnostics=${record_diagnostics_path}
targetAcceptanceReport=${record_report_path}
engineLog=${record_engine_log_path}
rawKataGoComparisonLog=${missing_score_mean_record_raw_log_path}
manualUiInspectionLog=${record_manual_ui_log_path}
externalTargetVerificationLog=${record_external_verification_log_path}
screenshot=${record_screenshot_path}

[evidenceSha256]
diagnostics=${record_diagnostics_sha256}
targetAcceptanceReport=${record_report_sha256}
engineLog=${record_engine_log_sha256}
rawKataGoComparisonLog=${missing_score_mean_record_raw_log_sha256}
manualUiInspectionLog=${record_manual_ui_log_sha256}
externalTargetVerificationLog=${record_external_verification_log_sha256}
screenshot=${record_screenshot_sha256}

[acceptance]
linuxKdeWaylandNvidia=pass
windowsNvidia=pass
physicalDisplay=pass
externalTargetVerification=pass
rawKataGoComparison=pass
manualUiInspection=pass
")
file(WRITE "${acceptance_record_missing_raw_log_score_stdev_file}" "
[metadata]
completedUtc=${acceptance_completed_utc}
appVersion=${expected_app_version}
appExecutableSha256=${expected_app_executable_sha256}
osPrettyName=${expected_os_pretty_name}
osKernelType=${expected_os_kernel_type}
osKernelVersion=${expected_os_kernel_version}
qtRuntimeVersion=${expected_qt_runtime_version}
qtBuildAbi=${expected_qt_build_abi}
cpuArchitecture=${expected_cpu_architecture}
reviewer=target tester
targetMachine=${expected_target_machine}
gpuDriver=NVIDIA 555.55
kataGoVersion=KataGo 1.15.3

[evidence]
diagnostics=${record_diagnostics_path}
targetAcceptanceReport=${record_report_path}
engineLog=${record_engine_log_path}
rawKataGoComparisonLog=${missing_score_stdev_record_raw_log_path}
manualUiInspectionLog=${record_manual_ui_log_path}
externalTargetVerificationLog=${record_external_verification_log_path}
screenshot=${record_screenshot_path}

[evidenceSha256]
diagnostics=${record_diagnostics_sha256}
targetAcceptanceReport=${record_report_sha256}
engineLog=${record_engine_log_sha256}
rawKataGoComparisonLog=${missing_score_stdev_record_raw_log_sha256}
manualUiInspectionLog=${record_manual_ui_log_sha256}
externalTargetVerificationLog=${record_external_verification_log_sha256}
screenshot=${record_screenshot_sha256}

[acceptance]
linuxKdeWaylandNvidia=pass
windowsNvidia=pass
physicalDisplay=pass
externalTargetVerification=pass
rawKataGoComparison=pass
manualUiInspection=pass
")
file(WRITE "${acceptance_record_missing_raw_log_policy_file}" "
[metadata]
completedUtc=${acceptance_completed_utc}
appVersion=${expected_app_version}
appExecutableSha256=${expected_app_executable_sha256}
osPrettyName=${expected_os_pretty_name}
osKernelType=${expected_os_kernel_type}
osKernelVersion=${expected_os_kernel_version}
qtRuntimeVersion=${expected_qt_runtime_version}
qtBuildAbi=${expected_qt_build_abi}
cpuArchitecture=${expected_cpu_architecture}
reviewer=target tester
targetMachine=${expected_target_machine}
gpuDriver=NVIDIA 555.55
kataGoVersion=KataGo 1.15.3

[evidence]
diagnostics=${record_diagnostics_path}
targetAcceptanceReport=${record_report_path}
engineLog=${record_engine_log_path}
rawKataGoComparisonLog=${missing_policy_record_raw_log_path}
manualUiInspectionLog=${record_manual_ui_log_path}
externalTargetVerificationLog=${record_external_verification_log_path}
screenshot=${record_screenshot_path}

[evidenceSha256]
diagnostics=${record_diagnostics_sha256}
targetAcceptanceReport=${record_report_sha256}
engineLog=${record_engine_log_sha256}
rawKataGoComparisonLog=${missing_policy_record_raw_log_sha256}
manualUiInspectionLog=${record_manual_ui_log_sha256}
externalTargetVerificationLog=${record_external_verification_log_sha256}
screenshot=${record_screenshot_sha256}

[acceptance]
linuxKdeWaylandNvidia=pass
windowsNvidia=pass
physicalDisplay=pass
externalTargetVerification=pass
rawKataGoComparison=pass
manualUiInspection=pass
")
file(WRITE "${acceptance_record_missing_raw_log_visits_file}" "
[metadata]
completedUtc=${acceptance_completed_utc}
appVersion=${expected_app_version}
appExecutableSha256=${expected_app_executable_sha256}
osPrettyName=${expected_os_pretty_name}
osKernelType=${expected_os_kernel_type}
osKernelVersion=${expected_os_kernel_version}
qtRuntimeVersion=${expected_qt_runtime_version}
qtBuildAbi=${expected_qt_build_abi}
cpuArchitecture=${expected_cpu_architecture}
reviewer=target tester
targetMachine=${expected_target_machine}
gpuDriver=NVIDIA 555.55
kataGoVersion=KataGo 1.15.3

[evidence]
diagnostics=${record_diagnostics_path}
targetAcceptanceReport=${record_report_path}
engineLog=${record_engine_log_path}
rawKataGoComparisonLog=${missing_visits_record_raw_log_path}
manualUiInspectionLog=${record_manual_ui_log_path}
externalTargetVerificationLog=${record_external_verification_log_path}
screenshot=${record_screenshot_path}

[evidenceSha256]
diagnostics=${record_diagnostics_sha256}
targetAcceptanceReport=${record_report_sha256}
engineLog=${record_engine_log_sha256}
rawKataGoComparisonLog=${missing_visits_record_raw_log_sha256}
manualUiInspectionLog=${record_manual_ui_log_sha256}
externalTargetVerificationLog=${record_external_verification_log_sha256}
screenshot=${record_screenshot_sha256}

[acceptance]
linuxKdeWaylandNvidia=pass
windowsNvidia=pass
physicalDisplay=pass
externalTargetVerification=pass
rawKataGoComparison=pass
manualUiInspection=pass
")
file(WRITE "${acceptance_record_missing_raw_log_pv_file}" "
[metadata]
completedUtc=${acceptance_completed_utc}
appVersion=${expected_app_version}
appExecutableSha256=${expected_app_executable_sha256}
osPrettyName=${expected_os_pretty_name}
osKernelType=${expected_os_kernel_type}
osKernelVersion=${expected_os_kernel_version}
qtRuntimeVersion=${expected_qt_runtime_version}
qtBuildAbi=${expected_qt_build_abi}
cpuArchitecture=${expected_cpu_architecture}
reviewer=target tester
targetMachine=${expected_target_machine}
gpuDriver=NVIDIA 555.55
kataGoVersion=KataGo 1.15.3

[evidence]
diagnostics=${record_diagnostics_path}
targetAcceptanceReport=${record_report_path}
engineLog=${record_engine_log_path}
rawKataGoComparisonLog=${missing_pv_record_raw_log_path}
manualUiInspectionLog=${record_manual_ui_log_path}
externalTargetVerificationLog=${record_external_verification_log_path}
screenshot=${record_screenshot_path}

[evidenceSha256]
diagnostics=${record_diagnostics_sha256}
targetAcceptanceReport=${record_report_sha256}
engineLog=${record_engine_log_sha256}
rawKataGoComparisonLog=${missing_pv_record_raw_log_sha256}
manualUiInspectionLog=${record_manual_ui_log_sha256}
externalTargetVerificationLog=${record_external_verification_log_sha256}
screenshot=${record_screenshot_sha256}

[acceptance]
linuxKdeWaylandNvidia=pass
windowsNvidia=pass
physicalDisplay=pass
externalTargetVerification=pass
rawKataGoComparison=pass
manualUiInspection=pass
")
file(WRITE "${acceptance_record_missing_raw_log_pv_visits_file}" "
[metadata]
completedUtc=${acceptance_completed_utc}
appVersion=${expected_app_version}
appExecutableSha256=${expected_app_executable_sha256}
osPrettyName=${expected_os_pretty_name}
osKernelType=${expected_os_kernel_type}
osKernelVersion=${expected_os_kernel_version}
qtRuntimeVersion=${expected_qt_runtime_version}
qtBuildAbi=${expected_qt_build_abi}
cpuArchitecture=${expected_cpu_architecture}
reviewer=target tester
targetMachine=${expected_target_machine}
gpuDriver=NVIDIA 555.55
kataGoVersion=KataGo 1.15.3

[evidence]
diagnostics=${record_diagnostics_path}
targetAcceptanceReport=${record_report_path}
engineLog=${record_engine_log_path}
rawKataGoComparisonLog=${missing_pv_visits_record_raw_log_path}
manualUiInspectionLog=${record_manual_ui_log_path}
externalTargetVerificationLog=${record_external_verification_log_path}
screenshot=${record_screenshot_path}

[evidenceSha256]
diagnostics=${record_diagnostics_sha256}
targetAcceptanceReport=${record_report_sha256}
engineLog=${record_engine_log_sha256}
rawKataGoComparisonLog=${missing_pv_visits_record_raw_log_sha256}
manualUiInspectionLog=${record_manual_ui_log_sha256}
externalTargetVerificationLog=${record_external_verification_log_sha256}
screenshot=${record_screenshot_sha256}

[acceptance]
linuxKdeWaylandNvidia=pass
windowsNvidia=pass
physicalDisplay=pass
externalTargetVerification=pass
rawKataGoComparison=pass
manualUiInspection=pass
")
file(WRITE "${acceptance_record_missing_raw_log_ownership_file}" "
[metadata]
completedUtc=${acceptance_completed_utc}
appVersion=${expected_app_version}
appExecutableSha256=${expected_app_executable_sha256}
osPrettyName=${expected_os_pretty_name}
osKernelType=${expected_os_kernel_type}
osKernelVersion=${expected_os_kernel_version}
qtRuntimeVersion=${expected_qt_runtime_version}
qtBuildAbi=${expected_qt_build_abi}
cpuArchitecture=${expected_cpu_architecture}
reviewer=target tester
targetMachine=${expected_target_machine}
gpuDriver=NVIDIA 555.55
kataGoVersion=KataGo 1.15.3

[evidence]
diagnostics=${record_diagnostics_path}
targetAcceptanceReport=${record_report_path}
engineLog=${record_engine_log_path}
rawKataGoComparisonLog=${missing_ownership_record_raw_log_path}
manualUiInspectionLog=${record_manual_ui_log_path}
externalTargetVerificationLog=${record_external_verification_log_path}
screenshot=${record_screenshot_path}

[evidenceSha256]
diagnostics=${record_diagnostics_sha256}
targetAcceptanceReport=${record_report_sha256}
engineLog=${record_engine_log_sha256}
rawKataGoComparisonLog=${missing_ownership_record_raw_log_sha256}
manualUiInspectionLog=${record_manual_ui_log_sha256}
externalTargetVerificationLog=${record_external_verification_log_sha256}
screenshot=${record_screenshot_sha256}

[acceptance]
linuxKdeWaylandNvidia=pass
windowsNvidia=pass
physicalDisplay=pass
externalTargetVerification=pass
rawKataGoComparison=pass
manualUiInspection=pass
")
file(WRITE "${acceptance_record_wrong_app_version_file}" "
[metadata]
completedUtc=${acceptance_completed_utc}
appVersion=0.0.0
appExecutableSha256=${expected_app_executable_sha256}
osPrettyName=${expected_os_pretty_name}
osKernelType=${expected_os_kernel_type}
osKernelVersion=${expected_os_kernel_version}
qtRuntimeVersion=${expected_qt_runtime_version}
qtBuildAbi=${expected_qt_build_abi}
cpuArchitecture=${expected_cpu_architecture}
reviewer=target tester
targetMachine=${expected_target_machine}
gpuDriver=NVIDIA 555.55
kataGoVersion=KataGo 1.15.3

[evidence]
diagnostics=${record_diagnostics_path}
targetAcceptanceReport=${record_report_path}
engineLog=${record_engine_log_path}
rawKataGoComparisonLog=${record_raw_log_path}
manualUiInspectionLog=${record_manual_ui_log_path}
externalTargetVerificationLog=${record_external_verification_log_path}
screenshot=${record_screenshot_path}

[evidenceSha256]
diagnostics=${record_diagnostics_sha256}
targetAcceptanceReport=${record_report_sha256}
engineLog=${record_engine_log_sha256}
rawKataGoComparisonLog=${record_raw_log_sha256}
manualUiInspectionLog=${record_manual_ui_log_sha256}
externalTargetVerificationLog=${record_external_verification_log_sha256}
screenshot=${record_screenshot_sha256}

[acceptance]
linuxKdeWaylandNvidia=pass
windowsNvidia=pass
physicalDisplay=pass
externalTargetVerification=pass
rawKataGoComparison=pass
manualUiInspection=pass
")
file(WRITE "${acceptance_record_wrong_app_executable_file}" "
[metadata]
completedUtc=${acceptance_completed_utc}
appVersion=${expected_app_version}
appExecutableSha256=0000000000000000000000000000000000000000000000000000000000000000
osPrettyName=${expected_os_pretty_name}
osKernelType=${expected_os_kernel_type}
osKernelVersion=${expected_os_kernel_version}
qtRuntimeVersion=${expected_qt_runtime_version}
qtBuildAbi=${expected_qt_build_abi}
cpuArchitecture=${expected_cpu_architecture}
reviewer=target tester
targetMachine=${expected_target_machine}
gpuDriver=NVIDIA 555.55
kataGoVersion=KataGo 1.15.3

[evidence]
diagnostics=${record_diagnostics_path}
targetAcceptanceReport=${record_report_path}
engineLog=${record_engine_log_path}
rawKataGoComparisonLog=${record_raw_log_path}
manualUiInspectionLog=${record_manual_ui_log_path}
externalTargetVerificationLog=${record_external_verification_log_path}
screenshot=${record_screenshot_path}

[evidenceSha256]
diagnostics=${record_diagnostics_sha256}
targetAcceptanceReport=${record_report_sha256}
engineLog=${record_engine_log_sha256}
rawKataGoComparisonLog=${record_raw_log_sha256}
manualUiInspectionLog=${record_manual_ui_log_sha256}
externalTargetVerificationLog=${record_external_verification_log_sha256}
screenshot=${record_screenshot_sha256}

[acceptance]
linuxKdeWaylandNvidia=pass
windowsNvidia=pass
physicalDisplay=pass
externalTargetVerification=pass
rawKataGoComparison=pass
manualUiInspection=pass
")
file(WRITE "${acceptance_record_wrong_os_metadata_file}" "
[metadata]
completedUtc=${acceptance_completed_utc}
appVersion=${expected_app_version}
appExecutableSha256=${expected_app_executable_sha256}
osPrettyName=${expected_os_pretty_name}
osKernelType=not-the-current-kernel
osKernelVersion=${expected_os_kernel_version}
qtRuntimeVersion=${expected_qt_runtime_version}
qtBuildAbi=${expected_qt_build_abi}
cpuArchitecture=${expected_cpu_architecture}
reviewer=target tester
targetMachine=${expected_target_machine}
gpuDriver=NVIDIA 555.55
kataGoVersion=KataGo 1.15.3

[evidence]
diagnostics=${record_diagnostics_path}
targetAcceptanceReport=${record_report_path}
engineLog=${record_engine_log_path}
rawKataGoComparisonLog=${record_raw_log_path}
manualUiInspectionLog=${record_manual_ui_log_path}
externalTargetVerificationLog=${record_external_verification_log_path}
screenshot=${record_screenshot_path}

[evidenceSha256]
diagnostics=${record_diagnostics_sha256}
targetAcceptanceReport=${record_report_sha256}
engineLog=${record_engine_log_sha256}
rawKataGoComparisonLog=${record_raw_log_sha256}
manualUiInspectionLog=${record_manual_ui_log_sha256}
externalTargetVerificationLog=${record_external_verification_log_sha256}
screenshot=${record_screenshot_sha256}

[acceptance]
linuxKdeWaylandNvidia=pass
windowsNvidia=pass
physicalDisplay=pass
externalTargetVerification=pass
rawKataGoComparison=pass
manualUiInspection=pass
")
file(WRITE "${acceptance_record_wrong_runtime_metadata_file}" "
[metadata]
completedUtc=${acceptance_completed_utc}
appVersion=${expected_app_version}
appExecutableSha256=${expected_app_executable_sha256}
osPrettyName=${expected_os_pretty_name}
osKernelType=${expected_os_kernel_type}
osKernelVersion=${expected_os_kernel_version}
qtRuntimeVersion=0.0.0-not-the-current-runtime
qtBuildAbi=${expected_qt_build_abi}
cpuArchitecture=${expected_cpu_architecture}
reviewer=target tester
targetMachine=${expected_target_machine}
gpuDriver=NVIDIA 555.55
kataGoVersion=KataGo 1.15.3

[evidence]
diagnostics=${record_diagnostics_path}
targetAcceptanceReport=${record_report_path}
engineLog=${record_engine_log_path}
rawKataGoComparisonLog=${record_raw_log_path}
manualUiInspectionLog=${record_manual_ui_log_path}
externalTargetVerificationLog=${record_external_verification_log_path}
screenshot=${record_screenshot_path}

[evidenceSha256]
diagnostics=${record_diagnostics_sha256}
targetAcceptanceReport=${record_report_sha256}
engineLog=${record_engine_log_sha256}
rawKataGoComparisonLog=${record_raw_log_sha256}
manualUiInspectionLog=${record_manual_ui_log_sha256}
externalTargetVerificationLog=${record_external_verification_log_sha256}
screenshot=${record_screenshot_sha256}

[acceptance]
linuxKdeWaylandNvidia=pass
windowsNvidia=pass
physicalDisplay=pass
externalTargetVerification=pass
rawKataGoComparison=pass
manualUiInspection=pass
")
file(WRITE "${acceptance_record_wrong_abi_metadata_file}" "
[metadata]
completedUtc=${acceptance_completed_utc}
appVersion=${expected_app_version}
appExecutableSha256=${expected_app_executable_sha256}
osPrettyName=${expected_os_pretty_name}
osKernelType=${expected_os_kernel_type}
osKernelVersion=${expected_os_kernel_version}
qtRuntimeVersion=${expected_qt_runtime_version}
qtBuildAbi=not-the-current-build-abi
cpuArchitecture=${expected_cpu_architecture}
reviewer=target tester
targetMachine=${expected_target_machine}
gpuDriver=NVIDIA 555.55
kataGoVersion=KataGo 1.15.3

[evidence]
diagnostics=${record_diagnostics_path}
targetAcceptanceReport=${record_report_path}
engineLog=${record_engine_log_path}
rawKataGoComparisonLog=${record_raw_log_path}
manualUiInspectionLog=${record_manual_ui_log_path}
externalTargetVerificationLog=${record_external_verification_log_path}
screenshot=${record_screenshot_path}

[evidenceSha256]
diagnostics=${record_diagnostics_sha256}
targetAcceptanceReport=${record_report_sha256}
engineLog=${record_engine_log_sha256}
rawKataGoComparisonLog=${record_raw_log_sha256}
manualUiInspectionLog=${record_manual_ui_log_sha256}
externalTargetVerificationLog=${record_external_verification_log_sha256}
screenshot=${record_screenshot_sha256}

[acceptance]
linuxKdeWaylandNvidia=pass
windowsNvidia=pass
physicalDisplay=pass
externalTargetVerification=pass
rawKataGoComparison=pass
manualUiInspection=pass
")
file(WRITE "${acceptance_record_wrong_cpu_metadata_file}" "
[metadata]
completedUtc=${acceptance_completed_utc}
appVersion=${expected_app_version}
appExecutableSha256=${expected_app_executable_sha256}
osPrettyName=${expected_os_pretty_name}
osKernelType=${expected_os_kernel_type}
osKernelVersion=${expected_os_kernel_version}
qtRuntimeVersion=${expected_qt_runtime_version}
qtBuildAbi=${expected_qt_build_abi}
cpuArchitecture=not-the-current-cpu
reviewer=target tester
targetMachine=${expected_target_machine}
gpuDriver=NVIDIA 555.55
kataGoVersion=KataGo 1.15.3

[evidence]
diagnostics=${record_diagnostics_path}
targetAcceptanceReport=${record_report_path}
engineLog=${record_engine_log_path}
rawKataGoComparisonLog=${record_raw_log_path}
manualUiInspectionLog=${record_manual_ui_log_path}
externalTargetVerificationLog=${record_external_verification_log_path}
screenshot=${record_screenshot_path}

[evidenceSha256]
diagnostics=${record_diagnostics_sha256}
targetAcceptanceReport=${record_report_sha256}
engineLog=${record_engine_log_sha256}
rawKataGoComparisonLog=${record_raw_log_sha256}
manualUiInspectionLog=${record_manual_ui_log_sha256}
externalTargetVerificationLog=${record_external_verification_log_sha256}
screenshot=${record_screenshot_sha256}

[acceptance]
linuxKdeWaylandNvidia=pass
windowsNvidia=pass
physicalDisplay=pass
externalTargetVerification=pass
rawKataGoComparison=pass
manualUiInspection=pass
")
file(WRITE "${acceptance_record_wrong_target_machine_file}" "
[metadata]
completedUtc=${acceptance_completed_utc}
appVersion=${expected_app_version}
appExecutableSha256=${expected_app_executable_sha256}
osPrettyName=${expected_os_pretty_name}
osKernelType=${expected_os_kernel_type}
osKernelVersion=${expected_os_kernel_version}
qtRuntimeVersion=${expected_qt_runtime_version}
qtBuildAbi=${expected_qt_build_abi}
cpuArchitecture=${expected_cpu_architecture}
reviewer=target tester
targetMachine=not-the-current-target-machine
gpuDriver=NVIDIA 555.55
kataGoVersion=KataGo 1.15.3

[evidence]
diagnostics=${record_diagnostics_path}
targetAcceptanceReport=${record_report_path}
engineLog=${record_engine_log_path}
rawKataGoComparisonLog=${record_raw_log_path}
manualUiInspectionLog=${record_manual_ui_log_path}
externalTargetVerificationLog=${record_external_verification_log_path}
screenshot=${record_screenshot_path}

[evidenceSha256]
diagnostics=${record_diagnostics_sha256}
targetAcceptanceReport=${record_report_sha256}
engineLog=${record_engine_log_sha256}
rawKataGoComparisonLog=${record_raw_log_sha256}
manualUiInspectionLog=${record_manual_ui_log_sha256}
externalTargetVerificationLog=${record_external_verification_log_sha256}
screenshot=${record_screenshot_sha256}

[acceptance]
linuxKdeWaylandNvidia=pass
windowsNvidia=pass
physicalDisplay=pass
externalTargetVerification=pass
rawKataGoComparison=pass
manualUiInspection=pass
")
file(WRITE "${acceptance_record_wrong_gpu_driver_file}" "
[metadata]
completedUtc=${acceptance_completed_utc}
appVersion=${expected_app_version}
appExecutableSha256=${expected_app_executable_sha256}
osPrettyName=${expected_os_pretty_name}
osKernelType=${expected_os_kernel_type}
osKernelVersion=${expected_os_kernel_version}
qtRuntimeVersion=${expected_qt_runtime_version}
qtBuildAbi=${expected_qt_build_abi}
cpuArchitecture=${expected_cpu_architecture}
reviewer=target tester
targetMachine=${expected_target_machine}
gpuDriver=Mesa llvmpipe
kataGoVersion=KataGo 1.15.3

[evidence]
diagnostics=${record_diagnostics_path}
targetAcceptanceReport=${record_report_path}
engineLog=${record_engine_log_path}
rawKataGoComparisonLog=${record_raw_log_path}
manualUiInspectionLog=${record_manual_ui_log_path}
externalTargetVerificationLog=${record_external_verification_log_path}
screenshot=${record_screenshot_path}

[evidenceSha256]
diagnostics=${record_diagnostics_sha256}
targetAcceptanceReport=${record_report_sha256}
engineLog=${record_engine_log_sha256}
rawKataGoComparisonLog=${record_raw_log_sha256}
manualUiInspectionLog=${record_manual_ui_log_sha256}
externalTargetVerificationLog=${record_external_verification_log_sha256}
screenshot=${record_screenshot_sha256}

[acceptance]
linuxKdeWaylandNvidia=pass
windowsNvidia=pass
physicalDisplay=pass
externalTargetVerification=pass
rawKataGoComparison=pass
manualUiInspection=pass
")
file(WRITE "${acceptance_record_wrong_katago_version_file}" "
[metadata]
completedUtc=${acceptance_completed_utc}
appVersion=${expected_app_version}
appExecutableSha256=${expected_app_executable_sha256}
osPrettyName=${expected_os_pretty_name}
osKernelType=${expected_os_kernel_type}
osKernelVersion=${expected_os_kernel_version}
qtRuntimeVersion=${expected_qt_runtime_version}
qtBuildAbi=${expected_qt_build_abi}
cpuArchitecture=${expected_cpu_architecture}
reviewer=target tester
targetMachine=${expected_target_machine}
gpuDriver=NVIDIA 555.55
kataGoVersion=Leela Zero 0.17

[evidence]
diagnostics=${record_diagnostics_path}
targetAcceptanceReport=${record_report_path}
engineLog=${record_engine_log_path}
rawKataGoComparisonLog=${record_raw_log_path}
manualUiInspectionLog=${record_manual_ui_log_path}
externalTargetVerificationLog=${record_external_verification_log_path}
screenshot=${record_screenshot_path}

[evidenceSha256]
diagnostics=${record_diagnostics_sha256}
targetAcceptanceReport=${record_report_sha256}
engineLog=${record_engine_log_sha256}
rawKataGoComparisonLog=${record_raw_log_sha256}
manualUiInspectionLog=${record_manual_ui_log_sha256}
externalTargetVerificationLog=${record_external_verification_log_sha256}
screenshot=${record_screenshot_sha256}

[acceptance]
linuxKdeWaylandNvidia=pass
windowsNvidia=pass
physicalDisplay=pass
externalTargetVerification=pass
rawKataGoComparison=pass
manualUiInspection=pass
")
file(WRITE "${acceptance_record_hash_mismatch_file}" "
[metadata]
completedUtc=${acceptance_completed_utc}
appVersion=${expected_app_version}
appExecutableSha256=${expected_app_executable_sha256}
osPrettyName=${expected_os_pretty_name}
osKernelType=${expected_os_kernel_type}
osKernelVersion=${expected_os_kernel_version}
qtRuntimeVersion=${expected_qt_runtime_version}
qtBuildAbi=${expected_qt_build_abi}
cpuArchitecture=${expected_cpu_architecture}
reviewer=target tester
targetMachine=${expected_target_machine}
gpuDriver=NVIDIA 555.55
kataGoVersion=KataGo 1.15.3

[evidence]
diagnostics=${record_diagnostics_path}
targetAcceptanceReport=${record_report_path}
engineLog=${record_engine_log_path}
rawKataGoComparisonLog=${record_raw_log_path}
manualUiInspectionLog=${record_manual_ui_log_path}
externalTargetVerificationLog=${record_external_verification_log_path}
screenshot=${record_screenshot_path}

[evidenceSha256]
diagnostics=0000000000000000000000000000000000000000000000000000000000000000
targetAcceptanceReport=${record_report_sha256}
engineLog=${record_engine_log_sha256}
rawKataGoComparisonLog=${record_raw_log_sha256}
manualUiInspectionLog=${record_manual_ui_log_sha256}
externalTargetVerificationLog=${record_external_verification_log_sha256}
screenshot=${record_screenshot_sha256}

[acceptance]
linuxKdeWaylandNvidia=pass
windowsNvidia=pass
physicalDisplay=pass
externalTargetVerification=pass
rawKataGoComparison=pass
manualUiInspection=pass
")
file(WRITE "${acceptance_record_stale_evidence_file}" "
[metadata]
completedUtc=2000-01-01T00:00:00Z
appVersion=${expected_app_version}
appExecutableSha256=${expected_app_executable_sha256}
osPrettyName=${expected_os_pretty_name}
osKernelType=${expected_os_kernel_type}
osKernelVersion=${expected_os_kernel_version}
qtRuntimeVersion=${expected_qt_runtime_version}
qtBuildAbi=${expected_qt_build_abi}
cpuArchitecture=${expected_cpu_architecture}
reviewer=target tester
targetMachine=${expected_target_machine}
gpuDriver=NVIDIA 555.55
kataGoVersion=KataGo 1.15.3

[evidence]
diagnostics=${record_diagnostics_path}
targetAcceptanceReport=${record_report_path}
engineLog=${record_engine_log_path}
rawKataGoComparisonLog=${record_raw_log_path}
manualUiInspectionLog=${record_manual_ui_log_path}
externalTargetVerificationLog=${record_external_verification_log_path}
screenshot=${record_screenshot_path}

[evidenceSha256]
diagnostics=${record_diagnostics_sha256}
targetAcceptanceReport=${record_report_sha256}
engineLog=${record_engine_log_sha256}
rawKataGoComparisonLog=${record_raw_log_sha256}
manualUiInspectionLog=${record_manual_ui_log_sha256}
externalTargetVerificationLog=${record_external_verification_log_sha256}
screenshot=${record_screenshot_sha256}

[acceptance]
linuxKdeWaylandNvidia=pass
windowsNvidia=pass
physicalDisplay=pass
externalTargetVerification=pass
rawKataGoComparison=pass
manualUiInspection=pass
")
file(WRITE "${relative_acceptance_record_file}" "
[metadata]
completedUtc=${acceptance_completed_utc}
appVersion=${expected_app_version}
appExecutableSha256=${expected_app_executable_sha256}
osPrettyName=${expected_os_pretty_name}
osKernelType=${expected_os_kernel_type}
osKernelVersion=${expected_os_kernel_version}
qtRuntimeVersion=${expected_qt_runtime_version}
qtBuildAbi=${expected_qt_build_abi}
cpuArchitecture=${expected_cpu_architecture}
reviewer=target tester
targetMachine=${expected_target_machine}
gpuDriver=NVIDIA 555.55
kataGoVersion=KataGo 1.15.3

[evidence]
diagnostics=evidence/target-diagnostics.txt
targetAcceptanceReport=evidence/target-report.md
engineLog=evidence/target-engine.log
rawKataGoComparisonLog=evidence/target-raw-katago.log
manualUiInspectionLog=${record_manual_ui_log_path}
externalTargetVerificationLog=${record_external_verification_log_path}
screenshot=evidence/target-screenshot.ppm

[evidenceSha256]
diagnostics=${relative_record_diagnostics_sha256}
targetAcceptanceReport=${relative_record_report_sha256}
engineLog=${relative_record_engine_log_sha256}
rawKataGoComparisonLog=${relative_record_raw_log_sha256}
manualUiInspectionLog=${record_manual_ui_log_sha256}
externalTargetVerificationLog=${record_external_verification_log_sha256}
screenshot=${relative_record_screenshot_sha256}

[acceptance]
linuxKdeWaylandNvidia=pass
windowsNvidia=pass
physicalDisplay=pass
externalTargetVerification=pass
rawKataGoComparison=pass
manualUiInspection=pass
")
file(SHA256 "${acceptance_record_file}" acceptance_record_sha256)
file(SHA256 "${relative_acceptance_record_file}" relative_acceptance_record_sha256)

execute_process(
    COMMAND
        "${CMAKE_COMMAND}" -E env
        "QT_QPA_PLATFORM=missing-qpa-for-template-test"
        "${APP_EXECUTABLE}" --target-acceptance-record-template
    WORKING_DIRECTORY "${work_dir}"
    RESULT_VARIABLE record_template_result
    OUTPUT_VARIABLE record_template_output
    ERROR_VARIABLE record_template_error
)
set(record_template_combined "${record_template_output}${record_template_error}")
if(NOT record_template_result EQUAL 0)
    message(FATAL_ERROR "Target acceptance record template command failed: ${record_template_combined}")
endif()

require_output(record_template_combined "^\\[metadata\\]" "start the record template with metadata")
require_output(
    record_template_combined
    "completedUtc=[0-9][0-9][0-9][0-9]-[0-9][0-9]-[0-9][0-9]T[0-9][0-9]:[0-9][0-9]:[0-9][0-9]Z"
    "include a generated UTC completion timestamp")
require_output(record_template_combined "appVersion=${expected_app_version}\n" "include current app version metadata key")
require_output(
    record_template_combined
    "appExecutableSha256=${expected_app_executable_sha256}\n"
    "include current app executable SHA-256 metadata key")
require_output(record_template_combined "osPrettyName=[^\n]+\n" "include current OS pretty-name metadata key")
require_output(record_template_combined "osKernelType=[^\n]+\n" "include current OS kernel-type metadata key")
require_output(record_template_combined "osKernelVersion=[^\n]+\n" "include current OS kernel-version metadata key")
require_output(record_template_combined "qtRuntimeVersion=[^\n]+\n" "include current Qt runtime metadata key")
require_output(record_template_combined "qtBuildAbi=[^\n]+\n" "include current Qt build ABI metadata key")
require_output(record_template_combined "cpuArchitecture=[^\n]+\n" "include current CPU architecture metadata key")
require_output(record_template_combined "reviewer=\n" "include target acceptance reviewer metadata key")
require_output(record_template_combined "targetMachine=[^\n]+\n" "include current target machine metadata key")
require_output(record_template_combined "gpuDriver=\n" "include GPU driver metadata key")
require_output(record_template_combined "kataGoVersion=\n" "include KataGo version metadata key")
require_output(record_template_combined "\\[evidence\\]" "include evidence section")
require_output(record_template_combined "diagnostics=\n" "include diagnostics evidence key")
require_output(record_template_combined "targetAcceptanceReport=\n" "include target report evidence key")
require_output(record_template_combined "engineLog=\n" "include engine log evidence key")
require_output(record_template_combined "rawKataGoComparisonLog=\n" "include raw KataGo evidence key")
require_output(record_template_combined "manualUiInspectionLog=\n" "include manual UI inspection evidence key")
require_output(record_template_combined "externalTargetVerificationLog=\n" "include external target verification evidence key")
require_output(record_template_combined "screenshot=\n" "include screenshot evidence key")
require_output(record_template_combined "\\[evidenceSha256\\]" "include evidence SHA-256 pinning section")
require_output(record_template_combined "diagnostics=\n" "include diagnostics SHA-256 pinning key")
require_output(record_template_combined "\\[evidenceContentMarkers\\]" "include evidence content marker guidance section")
require_output(
    record_template_combined
    "diagnostics=LizzieYzy Qt Diagnostics.*app\\.version.*<record appVersion>.*app\\.executable.*<current app\\.executable>.*plan\\.acceptance\\.recordFile\\.sha256.*plan\\.acceptance\\.finalAcceptanceOutstandingBlocker"
    "include diagnostics evidence content markers in the record template")
require_output(
    record_template_combined
    "targetAcceptanceReport=# Target Acceptance Report.*app\\.version.*<record appVersion>.*app\\.executable.*<current app\\.executable>.*plan\\.acceptance\\.recordFile\\.sha256.*plan\\.acceptance\\.finalAcceptanceOutstandingBlocker"
    "include target acceptance report evidence content markers in the record template")
require_output(
    record_template_combined
    "engineLog=KataGo; <record kataGoVersion>"
    "include engine log evidence content markers in the record template")
require_output(
    record_template_combined
    "engineLog\\.gpuOrStderrAny=.*CUDA.*OpenCL.*TensorRT.*stderr:"
    "include engine log GPU/stderr marker choices in the record template")
require_output(
    record_template_combined
    "rawKataGoComparisonLog=.*kata-analyze.*\"moveInfos\".*\"rootInfo\".*analysisJsonRawResponse.*engineVsEngine"
    "include raw KataGo comparison evidence content markers in the record template")
require_output(
    record_template_combined
    "manualUiInspectionLog=.*mainWindow4KHighDpiLayout.*multiDisplayPlacement"
    "include manual UI evidence content markers in the record template")
    require_output(
        record_template_combined
        "externalTargetVerificationLog=.*linuxKdeWaylandNvidiaRealtimeKataGo.*rawKataGoUiComparison"
        "include external target verification evidence content markers in the record template")
    require_output(record_template_combined "\\[recordHints\\]" "include record hint guidance section")
    require_output(
        record_template_combined
        "passValues=pass; passed; accepted; true; yes"
        "include accepted pass values in the record template")
    require_output(
        record_template_combined
        "failValues=fail; failed; rejected; false; no"
        "include accepted fail values in the record template")
    require_output(
        record_template_combined
        "metadataKeysRequired=completedUtc; appVersion; appExecutableSha256; osPrettyName; osKernelType; osKernelVersion; qtRuntimeVersion; qtBuildAbi; cpuArchitecture; reviewer; targetMachine; gpuDriver; kataGoVersion"
        "include required metadata key hints in the record template")
    require_output(
        record_template_combined
        "metadataExactMatchKeys=appVersion; appExecutableSha256; osPrettyName; osKernelType; osKernelVersion; qtRuntimeVersion; qtBuildAbi; cpuArchitecture; targetMachine"
        "include exact-match metadata key hints in the record template")
    require_output(
        record_template_combined
        "metadataValueCheckedKeys=completedUtc; reviewer; gpuDriver; kataGoVersion"
        "include value-checked metadata key hints in the record template")
    require_output(
        record_template_combined
        "completedUtcRequired=ISO-UTC-not-future"
        "include completedUtc requirement hint in the record template")
    require_output(
        record_template_combined
        "aggregateAcceptanceKeysRequired=pass"
        "include aggregate acceptance pass requirement hint in the record template")
    require_output(
        record_template_combined
        "checklistItemsRequired=pass"
        "include checklist pass requirement hint in the record template")
    require_output(
        record_template_combined
        "evidencePathsRequired=readable-non-empty-distinct"
        "include evidence path requirement hint in the record template")
    require_output(
        record_template_combined
        "evidenceSha256Required=true"
        "include evidence SHA-256 requirement hint in the record template")
    require_output(
        record_template_combined
        "evidenceContentMarkersRequired=true"
        "include evidence content marker requirement hint in the record template")
    require_output(
        record_template_combined
        "recordAndEvidenceTimestampsMustNotAfterCompletedUtc=true"
        "include record/evidence timestamp requirement hint in the record template")
    require_output(record_template_combined "\\[acceptance\\]" "include top-level acceptance section")
require_output(record_template_combined "linuxKdeWaylandNvidia=\n" "include Linux target acceptance result key")
require_output(record_template_combined "windowsNvidia=\n" "include Windows target acceptance result key")
require_output(record_template_combined "physicalDisplay=\n" "include physical display acceptance result key")
require_output(record_template_combined "externalTargetVerification=\n" "include external target verification result key")
require_output(record_template_combined "rawKataGoComparison=\n" "include raw KataGo comparison result key")
require_output(record_template_combined "manualUiInspection=\n" "include manual UI inspection result key")
require_output(record_template_combined "\\[checklist\\.linuxKdeWaylandNvidia\\]" "include Linux target checklist")
require_output(record_template_combined "linuxOs=\n" "include Linux OS checklist key")
require_output(record_template_combined "kdeSession=\n" "include KDE session checklist key")
require_output(record_template_combined "waylandSession=\n" "include Wayland session checklist key")
require_output(record_template_combined "qwaylandPlatformPlugin=\n" "include Linux Wayland plugin checklist key")
require_output(record_template_combined "nvidiaEnvironmentHint=\n" "include NVIDIA environment checklist key")
require_output(record_template_combined "packageStarts=\n" "include package startup checklist key")
require_output(record_template_combined "configureKataGoPaths=\n" "include KataGo path configuration checklist key")
require_output(record_template_combined "realtimeAnalyzeCurrentPosition=\n" "include realtime analysis checklist key")
require_output(record_template_combined "branchResync=\n" "include branch resync checklist key")
require_output(record_template_combined "gpuStderrCaptured=\n" "include GPU stderr checklist key")
require_output(record_template_combined "\\[checklist\\.windowsNvidia\\]" "include Windows target checklist")
require_output(record_template_combined "windowsOs=\n" "include Windows OS checklist key")
require_output(record_template_combined "qwindowsPlatformPlugin=\n" "include Windows platform plugin checklist key")
require_output(record_template_combined "packageExtracts=\n" "include Windows package extraction checklist key")
require_output(record_template_combined "appLocalQtRuntime=\n" "include Windows app-local Qt runtime checklist key")
require_output(record_template_combined "nativeSettingsPathDialog=\n" "include Windows native settings path-dialog checklist key")
require_output(record_template_combined "engineDiagnosticsCaptured=\n" "include Windows engine diagnostics checklist key")
require_output(record_template_combined "\\[checklist\\.physicalDisplay\\]" "include physical display checklist")
require_output(record_template_combined "physical4KPanel=\n" "include physical 4K panel checklist key")
require_output(record_template_combined "highDpiScale150Or200=\n" "include high-DPI scale checklist key")
require_output(record_template_combined "multiDisplayLayout=\n" "include multi-display checklist key")
require_output(record_template_combined "boardTextSharpness=\n" "include board text sharpness checklist key")
require_output(record_template_combined "candidateLabelsNoOverlap=\n" "include candidate label overlap checklist key")
require_output(record_template_combined "pvPreviewNoOverlap=\n" "include PV preview overlap checklist key")
require_output(record_template_combined "ownershipOverlayReadable=\n" "include ownership overlay readability checklist key")
require_output(record_template_combined "winrateScoreGraphReadable=\n" "include graph readability checklist key")
require_output(record_template_combined "restoredWindowVisible=\n" "include restored window visibility checklist key")
require_output(record_template_combined "\\[checklist\\.rawKataGoComparison\\]" "include raw KataGo comparison checklist")
require_output(record_template_combined "analysisJsonRawResponse=\n" "include Analysis JSON raw comparison checklist key")
require_output(record_template_combined "realtimeGtpRawLine=\n" "include realtime GTP raw comparison checklist key")
require_output(record_template_combined "candidateTableRendering=\n" "include candidate table raw comparison checklist key")
require_output(record_template_combined "pvPreview=\n" "include PV preview raw comparison checklist key")
require_output(record_template_combined "winrateScorePerspective=\n" "include winrate and score raw comparison checklist key")
require_output(record_template_combined "ownershipDisplay=\n" "include ownership display raw comparison checklist key")
require_output(record_template_combined "genmove=\n" "include genmove raw comparison checklist key")
require_output(record_template_combined "humanVsAi=\n" "include Human-vs-AI raw comparison checklist key")
require_output(record_template_combined "selfPlay=\n" "include self-play raw comparison checklist key")
require_output(record_template_combined "engineVsEngine=\n" "include engine-vs-engine raw comparison checklist key")
require_output(record_template_combined "\\[checklist\\.externalTargetVerification\\]" "include external verification checklist")
require_output(
    record_template_combined
    "linuxKdeWaylandNvidiaRealtimeKataGo=\n"
    "include Linux target realtime KataGo external verification checklist key")
require_output(
    record_template_combined
    "windowsNvidiaRealtimeKataGo=\n"
    "include Windows target realtime KataGo external verification checklist key")
require_output(
    record_template_combined
    "physical4KHighDpiMultiDisplayUi=\n"
    "include physical 4K high-DPI multi-display external verification checklist key")
require_output(record_template_combined "rawKataGoUiComparison=\n" "include raw KataGo UI comparison checklist key")
require_output(record_template_combined "\\[checklist\\.manualUiInspection\\]" "include manual UI checklist")
require_output(record_template_combined "mainWindow4KHighDpiLayout=\n" "include high-DPI UI inspection checklist key")
require_output(
    record_template_combined
    "boardLinesCoordinatesAndStarPoints=\n"
    "include board line, coordinate, and star-point UI inspection checklist key")
require_output(
    record_template_combined
    "stoneRenderingAndCandidateLabels=\n"
    "include stone rendering and candidate-label UI inspection checklist key")
require_output(record_template_combined "candidateTableColumns=\n" "include candidate-table UI inspection checklist key")
require_output(record_template_combined "pvPreviewStones=\n" "include PV preview stones UI inspection checklist key")
require_output(
    record_template_combined
    "ownershipMainBoardOverlay=\n"
    "include main-board ownership UI inspection checklist key")
require_output(
    record_template_combined
    "ownershipMiniBoardDock=\n"
    "include mini-board ownership UI inspection checklist key")
require_output(record_template_combined "winrateScoreGraph=\n" "include winrate and score graph UI inspection checklist key")
require_output(record_template_combined "toolbarDockAndMenuLayout=\n" "include toolbar, dock, and menu UI inspection checklist key")
require_output(record_template_combined "bilingualTextFit=\n" "include bilingual text-fit UI inspection checklist key")
require_output(record_template_combined "savedWindowRestore=\n" "include saved-window restore UI inspection checklist key")
require_output(record_template_combined "multiDisplayPlacement=\n" "include multi-display placement UI inspection checklist key")
require_output_match_count(
    record_template_combined
    "nvidiaEnvironmentHint=\n"
    2
    "include NVIDIA environment checklist key in both Linux and Windows sections")
require_output_match_count(
    record_template_combined
    "configureKataGoPaths=\n"
    2
    "include KataGo path configuration checklist key in both Linux and Windows sections")
require_output_match_count(
    record_template_combined
    "realtimeAnalyzeCurrentPosition=\n"
    2
    "include realtime analysis checklist key in both Linux and Windows sections")

execute_process(
    COMMAND
        "${CMAKE_COMMAND}" -E env
        "QT_QPA_PLATFORM=offscreen"
        "LIZZIE_KATAGO_EXECUTABLE=${APP_EXECUTABLE}"
        "LIZZIE_KATAGO_MODEL=${model_path}"
        "LIZZIE_KATAGO_ANALYSIS_CONFIG=${analysis_config_path}"
        "LIZZIE_KATAGO_GTP_CONFIG=${gtp_config_path}"
        "LIZZIE_DIAGNOSTICS_SETTINGS_FILE=${settings_file}"
        "${APP_EXECUTABLE}" --target-acceptance-report
    WORKING_DIRECTORY "${work_dir}"
    RESULT_VARIABLE report_result
    OUTPUT_VARIABLE report_output
    ERROR_VARIABLE report_error
)
set(report_combined "${report_output}${report_error}")
if(NOT report_result EQUAL 0)
    message(FATAL_ERROR "Target acceptance report command failed: ${report_combined}")
endif()

require_output(report_combined "^# Target Acceptance Report" "print the report title")
require_output(report_combined "lizzieyzy_qt --target-acceptance-report" "name the report CLI")
require_output(report_combined "`app\\.version`: " "report the app version")
require_output(report_combined "`generatedUtc`: " "report the generation timestamp")
require_output(report_combined "`app\\.executable`: " "report the app executable path")
require_output(report_combined "`app\\.dir`: " "report the app directory")
require_output(report_combined "`process\\.currentWorkingDirectory`: " "report the process working directory")
require_output(report_combined "`qt\\.version`: " "report the Qt runtime version")
require_output(report_combined "`qt\\.platform`: offscreen" "report the Qt platform")
require_output(report_combined "`os\\.prettyName`: " "report the OS pretty name")
require_output(report_combined "`os\\.kernelType`: " "report the OS kernel type")
require_output(report_combined "`os\\.kernelVersion`: " "report the OS kernel version")
require_output(report_combined "`plan\\.target\\.osFamily`: " "report the target OS family")
require_output(report_combined "## Configured Paths" "include configured path summary section")
require_output(
    report_combined
    "`katago\\.env\\.executable`: .*lizzieyzy_qt"
    "report target-report KataGo env executable path")
require_output(
    report_combined
    "`katago\\.env\\.model`: .*report-model\\.bin\\.gz"
    "report target-report KataGo env model path")
require_output(
    report_combined
    "`katago\\.env\\.gtpConfig`: .*report-gtp\\.cfg"
    "report target-report KataGo env GTP config path")
require_output(
    report_combined
    "`katago\\.env\\.analysisConfig`: .*report-analysis\\.cfg"
    "report target-report KataGo env analysis config path")
require_output(
    report_combined
    "`qt\\.settings\\.diagnosticsOverrideFile`: .*target-acceptance-report-settings\\.ini"
    "report target-report diagnostics settings file")
require_output(
    report_combined
    "`qt\\.settings\\.fileName`: .*target-acceptance-report-settings\\.ini"
    "report target-report QSettings file")
require_output(
    report_combined
    "`qt\\.settings\\.engine\\.executable\\.value`: .*lizzieyzy_qt"
    "report target-report saved main executable path")
require_output(
    report_combined
    "`qt\\.settings\\.engine\\.model\\.value`: .*report-model\\.bin\\.gz"
    "report target-report saved main model path")
require_output(
    report_combined
    "`qt\\.settings\\.engine\\.gtpConfig\\.value`: .*report-gtp\\.cfg"
    "report target-report saved main GTP config path")
require_output(
    report_combined
    "`qt\\.settings\\.engine\\.analysisConfig\\.value`: .*report-analysis\\.cfg"
    "report target-report saved main analysis config path")
require_output(
    report_combined
    "`qt\\.settings\\.engine\\.workingDirectory\\.value`: .*engine-working-dir"
    "report target-report saved main working directory")
require_output(
    report_combined
    "`qt\\.settings\\.engine\\.extraArgs\\.value`: --target-report-main \\\"spaced value\\\" \\\"\\\""
    "report target-report saved main extra args")
require_output(
    report_combined
    "`qt\\.settings\\.engine\\.extraArgs\\.parsed\\.count`: 3"
    "report target-report parsed main extra arg count")
require_output(
    report_combined
    "`qt\\.settings\\.engine\\.extraArgs\\.parsed\\.0`: --target-report-main"
    "report target-report first parsed main extra arg")
require_output(
    report_combined
    "`qt\\.settings\\.engine\\.extraArgs\\.parsed\\.1`: spaced value"
    "report target-report spaced parsed main extra arg")
require_output(
    report_combined
    "`qt\\.settings\\.engine\\.extraArgs\\.parsed\\.2`: \\(empty\\)"
    "report target-report empty parsed main extra arg")
require_output(
    report_combined
    "`qt\\.settings\\.compare\\.model\\.value`: .*report-compare-model\\.bin\\.gz"
    "report target-report saved compare model path")
require_output(
    report_combined
    "`qt\\.settings\\.compare\\.gtpConfig\\.value`: .*report-compare-gtp\\.cfg"
    "report target-report saved compare GTP config path")
require_output(
    report_combined
    "`qt\\.settings\\.compare\\.analysisConfig\\.value`: .*report-compare-analysis\\.cfg"
    "report target-report saved compare analysis config path")
require_output(
    report_combined
    "`qt\\.settings\\.compare\\.workingDirectory\\.value`: .*compare-working-dir"
    "report target-report saved compare working directory")
require_output(
    report_combined
    "`qt\\.settings\\.compare\\.extraArgs\\.value`: --target-report-compare \\\"compare spaced\\\" \\\"\\\""
    "report target-report saved compare extra args")
require_output(
    report_combined
    "`qt\\.settings\\.compare\\.extraArgs\\.parsed\\.count`: 3"
    "report target-report parsed compare extra arg count")
require_output(
    report_combined
    "`qt\\.settings\\.compare\\.extraArgs\\.parsed\\.0`: --target-report-compare"
    "report target-report first parsed compare extra arg")
require_output(
    report_combined
    "`qt\\.settings\\.compare\\.extraArgs\\.parsed\\.1`: compare spaced"
    "report target-report spaced parsed compare extra arg")
require_output(
    report_combined
    "`qt\\.settings\\.compare\\.extraArgs\\.parsed\\.2`: \\(empty\\)"
    "report target-report empty parsed compare extra arg")
require_output(report_combined "`katago\\.env\\.status`: ready" "report ready KataGo env fixture status")
require_output(
    report_combined
    "`plan\\.acceptance\\.recordFile`: \\(unset\\)"
    "report unset target acceptance record file")
require_output(
    report_combined
    "`plan\\.acceptance\\.recordFile\\.set`: false"
    "report unset target acceptance record file set flag")
require_output(
    report_combined
    "`plan\\.acceptance\\.recordMetadata\\.ready`: false"
    "report incomplete target acceptance metadata without a record file")
require_output(
    report_combined
    "`plan\\.acceptance\\.recordMetadata\\.blocker`: .*completedUtc"
    "report missing target acceptance completion metadata")
require_output(
    report_combined
    "`plan\\.acceptance\\.recordHint\\.passValues`: pass; passed; accepted; true; yes"
    "report target acceptance accepted pass value hints")
require_output(
    report_combined
    "`plan\\.acceptance\\.recordHint\\.failValues`: fail; failed; rejected; false; no"
    "report target acceptance accepted fail value hints")
require_output(
    report_combined
    "`plan\\.acceptance\\.recordHint\\.metadataKeysRequired`: completedUtc; appVersion; appExecutableSha256; osPrettyName; osKernelType; osKernelVersion; qtRuntimeVersion; qtBuildAbi; cpuArchitecture; reviewer; targetMachine; gpuDriver; kataGoVersion"
    "report target acceptance required metadata key hints")
require_output(
    report_combined
    "`plan\\.acceptance\\.recordHint\\.metadataExactMatchKeys`: appVersion; appExecutableSha256; osPrettyName; osKernelType; osKernelVersion; qtRuntimeVersion; qtBuildAbi; cpuArchitecture; targetMachine"
    "report target acceptance exact-match metadata key hints")
require_output(
    report_combined
    "`plan\\.acceptance\\.recordHint\\.metadataValueCheckedKeys`: completedUtc; reviewer; gpuDriver; kataGoVersion"
    "report target acceptance value-checked metadata key hints")
require_output(
    report_combined
    "`plan\\.acceptance\\.recordHint\\.completedUtcRequired`: ISO-UTC-not-future"
    "report target acceptance completedUtc requirement hint")
require_output(
    report_combined
    "`plan\\.acceptance\\.recordHint\\.aggregateAcceptanceKeysRequired`: pass"
    "report target acceptance aggregate pass requirement hint")
require_output(
    report_combined
    "`plan\\.acceptance\\.recordHint\\.checklistItemsRequired`: pass"
    "report target acceptance checklist pass requirement hint")
require_output(
    report_combined
    "`plan\\.acceptance\\.recordHint\\.evidencePathsRequired`: readable-non-empty-distinct"
    "report target acceptance evidence path requirement hint")
require_output(
    report_combined
    "`plan\\.acceptance\\.recordHint\\.evidenceSha256Required`: true"
    "report target acceptance evidence SHA-256 requirement hint")
require_output(
    report_combined
    "`plan\\.acceptance\\.recordHint\\.evidenceContentMarkersRequired`: true"
    "report target acceptance evidence content marker requirement hint")
require_output(
    report_combined
    "`plan\\.acceptance\\.recordHint\\.recordAndEvidenceTimestampsMustNotAfterCompletedUtc`: true"
    "report target acceptance timestamp requirement hint")
require_output(
    report_combined
    "`plan\\.acceptance\\.linuxKdeWaylandNvidiaManualResult`: manual-record-required"
    "report missing Linux target manual result")
require_output(
    report_combined
    "`plan\\.acceptance\\.windowsNvidiaManualResult`: manual-record-required"
    "report missing Windows target manual result")
require_output(
    report_combined
    "`plan\\.acceptance\\.physicalDisplayManualResult`: manual-record-required"
    "report missing physical display manual result")
require_output(report_combined "`plan\\.target\\.inFirstReleaseScope`: true" "report first-release OS scope")
require_output(
    report_combined
    "`plan\\.acceptance\\.configuredStatus`: katago-config-ready-needs-target-machine"
    "summarize configured target-machine acceptance status")
require_output(
    report_combined
    "`plan\\.acceptance\\.targetPlatformCandidate`: false"
    "report offscreen target platform candidate")
require_output(report_combined "`plan\\.acceptance\\.realKataGoEnvReady`: true" "report ready KataGo env fixture flag")
require_output(report_combined "`plan\\.acceptance\\.realKataGoEnvStatus`: ready" "summarize real KataGo env readiness")
require_output(report_combined "`plan\\.acceptance\\.savedMainConfigReady`: true" "report ready saved main config flag")
require_output(report_combined "`plan\\.acceptance\\.savedMainGtpReady`: true" "report ready saved main GTP flag")
require_output(
    report_combined
    "`plan\\.acceptance\\.savedMainAnalysisReady`: true"
    "report ready saved main analysis flag")
require_output(report_combined "`plan\\.acceptance\\.savedMainConfigStatus`: ready" "report ready saved main config")
require_output(report_combined "`plan\\.acceptance\\.savedCompareConfigReady`: true" "report ready saved compare config flag")
require_output(report_combined "`plan\\.acceptance\\.savedCompareGtpReady`: true" "report ready saved compare GTP flag")
require_output(
    report_combined
    "`plan\\.acceptance\\.savedCompareAnalysisReady`: true"
    "report ready saved compare analysis flag")
require_output(report_combined "`plan\\.acceptance\\.savedCompareConfigStatus`: ready" "report ready saved compare config")
require_output(
    report_combined
    "`plan\\.acceptance\\.envOrSavedMainConfigReady`: true"
    "report ready env-or-saved main config flag")
require_output(
    report_combined
    "`plan\\.acceptance\\.envOrSavedMainGtpReady`: true"
    "report ready env-or-saved main GTP flag")
require_output(
    report_combined
    "`plan\\.acceptance\\.envOrSavedMainAnalysisReady`: true"
    "report ready env-or-saved main analysis flag")
require_output(
    report_combined
    "`plan\\.acceptance\\.configuredMainConfigSource`: env-and-saved"
    "report env and saved main config source")
require_output(
    report_combined
    "`plan\\.acceptance\\.configuredMainGtpSource`: env-and-saved"
    "report env and saved main GTP source")
require_output(
    report_combined
    "`plan\\.acceptance\\.configuredMainAnalysisSource`: env-and-saved"
    "report env and saved main analysis source")
require_output(
    report_combined
    "`plan\\.acceptance\\.configuredCompareConfigSource`: saved"
    "report saved compare config source")
require_output(
    report_combined
    "`plan\\.acceptance\\.configuredCompareGtpSource`: saved"
    "report saved compare GTP source")
require_output(
    report_combined
    "`plan\\.acceptance\\.configuredCompareAnalysisSource`: saved"
    "report saved compare analysis source")
require_output(
    report_combined
    "`plan\\.acceptance\\.configuredDualEngineConfigReady`: true"
    "report ready dual-engine config")
require_output(
    report_combined
    "`plan\\.acceptance\\.configuredDualEngineGtpReady`: true"
    "report ready dual-engine GTP config")
require_output(
    report_combined
    "`plan\\.acceptance\\.configuredDualEngineAnalysisReady`: true"
    "report ready dual-engine analysis config")
require_output(
    report_combined
    "`plan\\.acceptance\\.realKataGoTargetRunCandidate`: false"
    "report offscreen real KataGo target-run candidate")
require_output(
    report_combined
    "`plan\\.acceptance\\.realKataGoManualVerificationCandidate`: false"
    "report offscreen real KataGo manual verification candidate")
require_output(
    report_combined
    "`plan\\.acceptance\\.configuredKataGoTargetRunCandidate`: false"
    "report offscreen configured KataGo target-run candidate")
require_output(
    report_combined
    "`plan\\.acceptance\\.configuredManualVerificationCandidate`: false"
    "report offscreen configured manual verification candidate")
require_output(
    report_combined
    "`plan\\.acceptance\\.configuredRealtimeGtpTargetRunCandidate`: false"
    "report offscreen realtime GTP target-run candidate")
require_output(
    report_combined
    "`plan\\.acceptance\\.configuredBatchAnalysisTargetRunCandidate`: false"
    "report offscreen batch-analysis target-run candidate")
require_output(
    report_combined
    "`plan\\.acceptance\\.configuredRealtimeGtpManualVerificationCandidate`: false"
    "report offscreen realtime GTP manual verification candidate")
require_output(
    report_combined
    "`plan\\.acceptance\\.configuredBatchAnalysisManualVerificationCandidate`: false"
    "report offscreen batch-analysis manual verification candidate")
require_output(
    report_combined
    "`plan\\.acceptance\\.configuredDualEngineTargetRunCandidate`: false"
    "report offscreen dual-engine target-run candidate")
require_output(
    report_combined
    "`plan\\.acceptance\\.configuredDualEngineManualVerificationCandidate`: false"
    "report offscreen dual-engine manual verification candidate")
require_output(
    report_combined
    "`plan\\.acceptance\\.configuredDualRealtimeGtpTargetRunCandidate`: false"
    "report offscreen dual realtime GTP target-run candidate")
require_output(
    report_combined
    "`plan\\.acceptance\\.configuredDualBatchAnalysisTargetRunCandidate`: false"
    "report offscreen dual batch-analysis target-run candidate")
require_output(
    report_combined
    "`plan\\.acceptance\\.configuredDualRealtimeGtpManualVerificationCandidate`: false"
    "report offscreen dual realtime GTP manual verification candidate")
require_output(
    report_combined
    "`plan\\.acceptance\\.configuredDualBatchAnalysisManualVerificationCandidate`: false"
    "report offscreen dual batch-analysis manual verification candidate")
require_output(
    report_combined
    "`plan\\.target\\.acceptance\\.linuxKdeWaylandNvidiaCandidate`: false"
    "report offscreen Linux KDE Wayland NVIDIA target candidate")
require_output(
    report_combined
    "`plan\\.target\\.linuxKdeWayland\\.detected`: false"
    "report offscreen Linux KDE Wayland target detection")
require_output(
    report_combined
    "`plan\\.target\\.acceptance\\.windowsNvidiaCandidate`: false"
    "report offscreen Windows NVIDIA target candidate")
require_output(
    report_combined
    "`plan\\.target\\.windows\\.detected`: false"
    "report offscreen Windows target detection")
require_output(
    report_combined
    "`plan\\.target\\.acceptance\\.firstReleaseNvidiaPlatformCandidate`: false"
    "report offscreen first-release NVIDIA platform candidate")
require_output(
    report_combined
    "`plan\\.target\\.acceptance\\.firstReleaseNvidiaPlatformBlocker`: linuxKdeWaylandNvidia, windowsNvidia"
    "report offscreen first-release NVIDIA platform blockers")
require_output(
    report_combined
    "`plan\\.target\\.acceptance\\.targetDisplayBlocker`: display4K, highDpiScale"
    "report offscreen target display blockers")
require_output(
    report_combined
    "`plan\\.target\\.acceptance\\.multiDisplayBlocker`: multiDisplay"
    "report offscreen target multi-display blocker")
require_output(
    report_combined
    "`plan\\.acceptance\\.targetDisplayBlocker`: display4K, highDpiScale"
    "report offscreen acceptance target display blockers")
require_output(
    report_combined
    "`plan\\.acceptance\\.multiDisplayBlocker`: multiDisplay"
    "report offscreen acceptance multi-display blocker")
require_output(
    report_combined
    "`plan\\.target\\.display\\.primaryDevicePixelsAtLeast4K`: false"
    "report offscreen primary 4K display candidate")
require_output(
    report_combined
    "`plan\\.target\\.display\\.primaryScaleAtLeast1_5`: false"
    "report offscreen primary high-DPI scale candidate")
require_output(
    report_combined
    "`plan\\.target\\.display\\.primaryTargetScreenCandidate`: false"
    "report offscreen primary target display candidate")
require_output(
    report_combined
    "`plan\\.acceptance\\.display4KCandidate`: false"
    "report offscreen PLAN acceptance 4K display candidate")
require_output(
    report_combined
    "`plan\\.acceptance\\.highDpiCandidate`: false"
    "report offscreen PLAN acceptance high-DPI candidate")
require_output(
    report_combined
    "`plan\\.acceptance\\.sameScreenTargetDisplayCandidate`: false"
    "report offscreen same-screen target display candidate")
require_output(
    report_combined
    "`plan\\.acceptance\\.status`: katago-env-ready-needs-target-machine"
    "summarize target-machine acceptance status")
require_output(
    report_combined
    "`plan\\.acceptance\\.externalTargetVerificationRequired`: true"
    "report external target verification requirement")
require_output(
    report_combined
    "`plan\\.acceptance\\.externalTargetVerificationStatus`: manual-record-required"
    "report external target verification status")
require_output(
    report_combined
    "`plan\\.acceptance\\.manualRawEngineComparisonRequired`: true"
    "report manual raw engine comparison requirement")
require_output(
    report_combined
    "`plan\\.acceptance\\.rawKataGoComparisonStatus`: manual-record-required"
    "report raw KataGo comparison status")
require_output(
    report_combined
    "`plan\\.acceptance\\.manualUiInspectionRequired`: true"
    "report manual UI inspection requirement")
require_output(
    report_combined
    "`plan\\.acceptance\\.manualUiInspectionStatus`: manual-record-required"
    "report manual UI inspection status")
require_output(
    report_combined
    "`plan\\.acceptance\\.finalAcceptanceStatus`: needs-readiness-and-final-manual-acceptance"
    "report final acceptance status")
require_output(
    report_combined
    "`plan\\.acceptance\\.finalAcceptanceBlocker`: targetPlatform, targetDisplay, multiDisplay, externalTargetVerification, manualRawEngineComparison, manualUiInspection"
    "report final acceptance blockers for offscreen target-report smoke")
require_output(
    report_combined
    "`plan\\.acceptance\\.finalAcceptanceOutstandingBlocker`: targetPlatform, targetDisplay, multiDisplay, externalTargetVerification, manualRawEngineComparison, manualUiInspection"
    "report final acceptance outstanding blockers in the final section")
require_no_output(
    report_combined
    "- Outstanding blockers:"
    "kept the empty final acceptance outstanding-blocker prompt")
require_output(
    report_combined
    "`plan\\.acceptance\\.linuxKdeWaylandNvidiaVerificationStatus`: katago-gtp-config-ready-needs-linux-kde-wayland-nvidia"
    "report Linux KDE Wayland NVIDIA verification status")
require_output(
    report_combined
    "`plan\\.target\\.acceptance\\.linuxKdeWaylandNvidiaBlocker`: .*qtWaylandPlatform.*nvidiaEnvironmentHint"
    "report Linux KDE Wayland NVIDIA platform blockers")
require_output(
    report_combined
    "`plan\\.acceptance\\.windowsNvidiaVerificationStatus`: katago-gtp-config-ready-needs-windows-nvidia"
    "report Windows NVIDIA verification status")
require_output(
    report_combined
    "`plan\\.target\\.acceptance\\.windowsNvidiaBlocker`: .*windowsPlatformPlugin.*nvidiaEnvironmentHint"
    "report Windows NVIDIA platform blockers")
require_output(
    report_combined
    "`plan\\.acceptance\\.physicalDisplayVerificationStatus`: needs-target-display-and-multi-display"
    "report physical display verification status")
require_output(
    report_combined
    "`plan\\.acceptance\\.rawKataGoComparisonChecklist`: .*analysisJsonRawResponse"
    "include the raw KataGo comparison checklist field")
require_output(
    report_combined
    "`plan\\.acceptance\\.externalTargetVerificationChecklist`: .*linuxKdeWaylandNvidiaRealtimeKataGo"
    "include the external target verification checklist field")
require_output(
    report_combined
    "`plan\\.acceptance\\.manualUiInspectionChecklist`: .*mainWindow4KHighDpiLayout"
    "include the manual UI inspection checklist field")
require_output(report_combined "## Linux KDE Wayland \\+ NVIDIA" "include the Linux target section")
require_output(report_combined "## Windows \\+ NVIDIA" "include the Windows target section")
require_output(report_combined "## High DPI And Multi-Display" "include the display target section")
require_output(report_combined "## Raw KataGo Comparison" "include the raw KataGo comparison section")
require_output(report_combined "## External Target Verification" "include the external target verification section")
require_output(report_combined "## Manual UI Inspection" "include the manual UI inspection section")
require_output(report_combined "## Final Acceptance" "include the final acceptance section")
require_output_match_count(
    report_combined
    "- Result: pass / fail"
    6
    "include manual result placeholders for each manual acceptance section")
require_output_match_count(
    report_combined
    "- Notes:"
    6
    "include manual notes placeholders for each manual acceptance section")

execute_process(
    COMMAND
        "${CMAKE_COMMAND}" -E env
        "--unset=LIZZIE_TARGET_ACCEPTANCE_RECORD_FILE"
        "QT_QPA_PLATFORM=offscreen"
        "LIZZIE_KATAGO_EXECUTABLE=${APP_EXECUTABLE}"
        "LIZZIE_KATAGO_MODEL=${model_path}"
        "LIZZIE_KATAGO_ANALYSIS_CONFIG=${analysis_config_path}"
        "LIZZIE_KATAGO_GTP_CONFIG=${gtp_config_path}"
        "LIZZIE_DIAGNOSTICS_SETTINGS_FILE=${settings_file}"
        "${APP_EXECUTABLE}" --target-acceptance-report --target-acceptance-record "${acceptance_record_file}"
    WORKING_DIRECTORY "${work_dir}"
    RESULT_VARIABLE recorded_report_result
    OUTPUT_VARIABLE recorded_report_output
    ERROR_VARIABLE recorded_report_error
)
set(recorded_report_combined "${recorded_report_output}${recorded_report_error}")
if(NOT recorded_report_result EQUAL 0)
    message(FATAL_ERROR "Target acceptance recorded-result report command failed: ${recorded_report_combined}")
endif()

require_output(
    recorded_report_combined
    "`plan\\.acceptance\\.recordFile`: .*target-acceptance-record\\.ini"
    "report target acceptance record file path")
require_output(
    recorded_report_combined
    "`plan\\.acceptance\\.recordFile\\.set`: true"
    "report target acceptance record file set flag")
require_output(
    recorded_report_combined
    "`plan\\.acceptance\\.recordFile\\.exists`: true"
    "report target acceptance record file exists")
require_output(
    recorded_report_combined
    "`plan\\.acceptance\\.recordFile\\.readable`: true"
    "report target acceptance record file readable")
require_output(
    recorded_report_combined
    "`plan\\.acceptance\\.recordFile\\.canonicalPath`: .*target-acceptance-record\\.ini"
    "report target acceptance record canonical path")
require_output(
    recorded_report_combined
    "`plan\\.acceptance\\.recordFile\\.size`: [1-9][0-9]*"
    "report target acceptance record file size")
require_output(
    recorded_report_combined
    "`plan\\.acceptance\\.recordFile\\.sha256`: ${acceptance_record_sha256}"
    "report target acceptance record SHA-256")
require_output(
    recorded_report_combined
    "`plan\\.acceptance\\.recordFile\\.lastModifiedUtc`: [0-9]"
    "report target acceptance record modification time")
require_output(
    recorded_report_combined
    "`plan\\.acceptance\\.recordFile\\.timestampStatus`: not-after-completed-utc"
    "report target acceptance record timestamp status")
require_output(
    recorded_report_combined
    "`plan\\.acceptance\\.record\\.completedUtc`: ${acceptance_completed_utc}"
    "report target acceptance completion time")
require_output(
    recorded_report_combined
    "`plan\\.acceptance\\.record\\.completedUtcStatus`: ready"
    "report ready target acceptance completion time status")
require_output(
    recorded_report_combined
    "`plan\\.acceptance\\.record\\.appVersion`: ${expected_app_version}"
    "report target acceptance app version")
require_output(
    recorded_report_combined
    "`plan\\.acceptance\\.record\\.appVersionStatus`: match"
    "report matching target acceptance app version")
require_output(
    recorded_report_combined
    "`plan\\.acceptance\\.record\\.appExecutableSha256`: ${expected_app_executable_sha256}"
    "report target acceptance app executable SHA-256")
require_output(
    recorded_report_combined
    "`plan\\.acceptance\\.record\\.appExecutableSha256Status`: match"
    "report matching target acceptance app executable SHA-256")
require_output(
    recorded_report_combined
    "`plan\\.acceptance\\.record\\.osPrettyName`: [^\n]+"
    "report target acceptance OS pretty name")
require_output(
    recorded_report_combined
    "`plan\\.acceptance\\.record\\.osPrettyNameStatus`: match"
    "report matching target acceptance OS pretty name")
require_output(
    recorded_report_combined
    "`plan\\.acceptance\\.record\\.osKernelType`: [^\n]+"
    "report target acceptance OS kernel type")
require_output(
    recorded_report_combined
    "`plan\\.acceptance\\.record\\.osKernelTypeStatus`: match"
    "report matching target acceptance OS kernel type")
require_output(
    recorded_report_combined
    "`plan\\.acceptance\\.record\\.osKernelVersion`: [^\n]+"
    "report target acceptance OS kernel version")
require_output(
    recorded_report_combined
    "`plan\\.acceptance\\.record\\.osKernelVersionStatus`: match"
    "report matching target acceptance OS kernel version")
require_output(
    recorded_report_combined
    "`plan\\.acceptance\\.record\\.qtRuntimeVersion`: [^\n]+"
    "report target acceptance Qt runtime version")
require_output(
    recorded_report_combined
    "`plan\\.acceptance\\.record\\.qtRuntimeVersionStatus`: match"
    "report matching target acceptance Qt runtime version")
require_output(
    recorded_report_combined
    "`plan\\.acceptance\\.record\\.qtBuildAbi`: [^\n]+"
    "report target acceptance Qt build ABI")
require_output(
    recorded_report_combined
    "`plan\\.acceptance\\.record\\.qtBuildAbiStatus`: match"
    "report matching target acceptance Qt build ABI")
require_output(
    recorded_report_combined
    "`plan\\.acceptance\\.record\\.cpuArchitecture`: [^\n]+"
    "report target acceptance CPU architecture")
require_output(
    recorded_report_combined
    "`plan\\.acceptance\\.record\\.cpuArchitectureStatus`: match"
    "report matching target acceptance CPU architecture")
require_output(
    recorded_report_combined
    "`plan\\.acceptance\\.record\\.reviewer`: target tester"
    "report target acceptance reviewer")
require_output(
    recorded_report_combined
    "`plan\\.acceptance\\.record\\.targetMachine`: ${expected_target_machine}"
    "report target acceptance machine")
require_output(
    recorded_report_combined
    "`plan\\.acceptance\\.record\\.targetMachineStatus`: match"
    "report matching target acceptance machine")
require_output(
    recorded_report_combined
    "`plan\\.acceptance\\.record\\.gpuDriver`: NVIDIA 555\\.55"
    "report target acceptance GPU driver")
require_output(
    recorded_report_combined
    "`plan\\.acceptance\\.record\\.gpuDriverStatus`: match"
    "report matching target acceptance GPU driver status")
require_output(
    recorded_report_combined
    "`plan\\.acceptance\\.record\\.kataGoVersion`: KataGo 1\\.15\\.3"
    "report target acceptance KataGo version")
require_output(
    recorded_report_combined
    "`plan\\.acceptance\\.record\\.kataGoVersionStatus`: match"
    "report matching target acceptance KataGo version status")
require_output(
    recorded_report_combined
    "`plan\\.acceptance\\.record\\.notes`: all target evidence attached"
    "report target acceptance notes")
require_output(
    recorded_report_combined
    "`plan\\.acceptance\\.recordMetadata\\.ready`: true"
    "report complete target acceptance metadata readiness")
require_output(
    recorded_report_combined
    "`plan\\.acceptance\\.recordMetadata\\.blocker`: \\(none\\)"
    "report no target acceptance metadata blockers")
require_output(
    recorded_report_combined
    "`plan\\.acceptance\\.evidence\\.diagnostics\\.value`: .*target-diagnostics\\.txt"
    "report diagnostics evidence path")
require_output(
    recorded_report_combined
    "`plan\\.acceptance\\.evidence\\.diagnostics\\.exists`: true"
    "report diagnostics evidence exists")
require_output(
    recorded_report_combined
    "`plan\\.acceptance\\.evidence\\.diagnostics\\.sha256`: ${record_diagnostics_sha256}"
    "report diagnostics evidence SHA-256")
require_output(
    recorded_report_combined
    "`plan\\.acceptance\\.evidence\\.diagnostics\\.lastModifiedUtc`: [0-9]"
    "report diagnostics evidence modification time")
require_output(
    recorded_report_combined
    "`plan\\.acceptance\\.evidence\\.diagnostics\\.expectedSha256`: ${record_diagnostics_sha256}"
    "report diagnostics evidence expected SHA-256")
require_output(
    recorded_report_combined
    "`plan\\.acceptance\\.evidence\\.diagnostics\\.sha256Status`: match"
    "report diagnostics evidence SHA-256 match status")
require_output(
    recorded_report_combined
    "`plan\\.acceptance\\.evidence\\.diagnostics\\.timestampStatus`: not-after-completed-utc"
    "report diagnostics evidence timestamp status")
require_output(
    recorded_report_combined
    "`plan\\.acceptance\\.evidence\\.diagnostics\\.contentStatus`: match"
    "report diagnostics evidence content status")
require_output(
    recorded_report_combined
    "`plan\\.acceptance\\.evidence\\.targetAcceptanceReport\\.readable`: true"
    "report target report evidence readable")
require_output(
    recorded_report_combined
    "`plan\\.acceptance\\.evidence\\.targetAcceptanceReport\\.contentStatus`: match"
    "report target acceptance report evidence content status")
require_output(
    recorded_report_combined
    "`plan\\.acceptance\\.evidence\\.engineLog\\.exists`: true"
    "report engine log evidence exists")
require_output(
    recorded_report_combined
    "`plan\\.acceptance\\.evidence\\.engineLog\\.contentStatus`: match"
    "report engine log evidence content status")
require_output(
    recorded_report_combined
    "`plan\\.acceptance\\.evidence\\.engineLog\\.gpuOrStderrContentStatus`: match"
    "report engine log GPU/stderr evidence content status")
require_output(
    recorded_report_combined
    "`plan\\.acceptance\\.evidence\\.engineLog\\.missingGpuOrStderrContentMarker`: \\(none\\)"
    "report no missing engine log GPU/stderr evidence content markers")
require_output(
    recorded_report_combined
    "`plan\\.acceptance\\.evidence\\.rawKataGoComparisonLog\\.exists`: true"
    "report raw KataGo evidence exists")
require_output(
    recorded_report_combined
    "`plan\\.acceptance\\.evidence\\.rawKataGoComparisonLog\\.contentStatus`: match"
    "report raw KataGo comparison log evidence content status")
require_output(
    recorded_report_combined
    "`plan\\.acceptance\\.evidence\\.rawKataGoComparisonLog\\.missingContentMarker`: \\(none\\)"
    "report no missing raw KataGo comparison log evidence content markers")
require_output(
    recorded_report_combined
    "`plan\\.acceptance\\.evidence\\.manualUiInspectionLog\\.exists`: true"
    "report manual UI inspection evidence exists")
require_output(
    recorded_report_combined
    "`plan\\.acceptance\\.evidence\\.manualUiInspectionLog\\.contentStatus`: match"
    "report manual UI inspection evidence content status")
require_output(
    recorded_report_combined
    "`plan\\.acceptance\\.evidence\\.manualUiInspectionLog\\.missingContentMarker`: \\(none\\)"
    "report no missing manual UI inspection evidence content markers")
require_output(
    recorded_report_combined
    "`plan\\.acceptance\\.evidence\\.externalTargetVerificationLog\\.exists`: true"
    "report external target verification evidence exists")
require_output(
    recorded_report_combined
    "`plan\\.acceptance\\.evidence\\.externalTargetVerificationLog\\.contentStatus`: match"
    "report external target verification evidence content status")
require_output(
    recorded_report_combined
    "`plan\\.acceptance\\.evidence\\.externalTargetVerificationLog\\.missingContentMarker`: \\(none\\)"
    "report no missing external target verification evidence content markers")
require_output(
    recorded_report_combined
    "`plan\\.acceptance\\.evidence\\.screenshot\\.exists`: true"
    "report screenshot evidence exists")
require_output(
    recorded_report_combined
    "`plan\\.acceptance\\.evidence\\.screenshot\\.imageReadable`: true"
    "report readable screenshot image evidence")
require_output(
    recorded_report_combined
    "`plan\\.acceptance\\.evidence\\.screenshot\\.imageFormat`: ppm"
    "report screenshot image format")
require_output(
    recorded_report_combined
    "`plan\\.acceptance\\.evidence\\.screenshot\\.imageWidth`: 2"
    "report screenshot image width")
require_output(
    recorded_report_combined
    "`plan\\.acceptance\\.evidence\\.screenshot\\.imageHeight`: 1"
    "report screenshot image height")
require_output(
    recorded_report_combined
    "`plan\\.acceptance\\.evidence\\.screenshot\\.imagePixelsAtLeast4K`: false"
    "report screenshot 4K pixel candidate")
require_output(
    recorded_report_combined
    "`plan\\.acceptance\\.evidence\\.screenshot\\.imageHasPixelVariation`: true"
    "report screenshot pixel variation")
require_output(
    recorded_report_combined
    "`plan\\.acceptance\\.evidence\\.screenshot\\.readyFor4KAcceptance`: false"
    "report screenshot 4K acceptance readiness")
require_output(
    recorded_report_combined
    "`plan\\.acceptance\\.evidence\\.ready`: true"
    "report complete acceptance evidence readiness")
require_output(
    recorded_report_combined
    "`plan\\.acceptance\\.evidence\\.blocker`: \\(none\\)"
    "report no acceptance evidence blockers")
require_output(
    recorded_report_combined
    "`plan\\.acceptance\\.evidenceSha256\\.ready`: true"
    "report acceptance evidence SHA-256 readiness")
require_output(
    recorded_report_combined
    "`plan\\.acceptance\\.evidenceSha256\\.blocker`: \\(none\\)"
    "report no acceptance evidence SHA-256 blockers")
require_output(
    recorded_report_combined
    "`plan\\.acceptance\\.recordTimestamp\\.ready`: true"
    "report target acceptance record timestamp readiness")
require_output(
    recorded_report_combined
    "`plan\\.acceptance\\.recordTimestamp\\.blocker`: \\(none\\)"
    "report no target acceptance record timestamp blockers")
require_output(
    recorded_report_combined
    "`plan\\.acceptance\\.evidenceTimestamp\\.ready`: true"
    "report acceptance evidence timestamp readiness")
require_output(
    recorded_report_combined
    "`plan\\.acceptance\\.evidenceTimestamp\\.blocker`: \\(none\\)"
    "report no acceptance evidence timestamp blockers")
require_output(
    recorded_report_combined
    "`plan\\.acceptance\\.linuxKdeWaylandNvidiaManualResult`: pass"
    "normalize Linux target manual acceptance result")
require_output(
    recorded_report_combined
    "`plan\\.acceptance\\.windowsNvidiaManualResult`: pass"
    "normalize Windows target manual acceptance result")
require_output(
    recorded_report_combined
    "`plan\\.acceptance\\.physicalDisplayManualResult`: pass"
    "normalize physical display manual acceptance result")
require_output(
    recorded_report_combined
    "`plan\\.acceptance\\.externalTargetVerificationManualResult`: pass"
    "derive external target verification checklist manual result")
require_output(
    recorded_report_combined
    "`plan\\.acceptance\\.checklistPassedRecord`: .*linuxKdeWaylandNvidia\\.linuxOs"
    "report passed Linux checklist item record")
require_output(
    recorded_report_combined
    "`plan\\.acceptance\\.checklistPassedRecord`: .*externalTargetVerification\\.rawKataGoUiComparison"
    "report passed external checklist item record")
require_output(
    recorded_report_combined
    "`plan\\.acceptance\\.checklistMissingRecord`: .*rawKataGoComparison\\.analysisJsonRawResponse"
    "report missing raw KataGo checklist item record")
require_output(
    recorded_report_combined
    "`plan\\.acceptance\\.checklistMissingRecord`: .*manualUiInspection\\.mainWindow4KHighDpiLayout"
    "report missing manual UI checklist item record")
require_output(
    recorded_report_combined
    "`plan\\.acceptance\\.manualFailedRecord`: \\(none\\)"
    "report no failed aggregate manual records")
require_output(
    recorded_report_combined
    "`plan\\.acceptance\\.manualInvalidRecord`: \\(none\\)"
    "report no invalid aggregate manual records")
require_output(
    recorded_report_combined
    "`plan\\.acceptance\\.checklist\\.ready`: false"
    "report incomplete target acceptance checklist readiness")
require_output(
    recorded_report_combined
    "`plan\\.acceptance\\.checklist\\.blocker`: .*rawKataGoComparison\\.analysisJsonRawResponse"
    "report incomplete raw KataGo checklist blocker")
require_output(
    recorded_report_combined
    "`plan\\.acceptance\\.checklist\\.blocker`: .*manualUiInspection\\.mainWindow4KHighDpiLayout"
    "report incomplete manual UI checklist blocker")
require_output(
    recorded_report_combined
    "- \\[x\\] linuxOs"
    "mark passed Linux checklist items")
require_output(
    recorded_report_combined
    "- \\[x\\] rawKataGoUiComparison"
    "mark passed external checklist items")
require_output(
    recorded_report_combined
    "`plan\\.acceptance\\.externalTargetVerificationStatus`: pass"
    "derive external target verification pass status")
require_output(
    recorded_report_combined
    "`plan\\.acceptance\\.rawKataGoComparisonStatus`: pass"
    "normalize raw KataGo comparison pass status")
require_output(
    recorded_report_combined
    "`plan\\.acceptance\\.manualUiInspectionStatus`: pass"
    "normalize manual UI inspection pass status")
require_output(
    recorded_report_combined
    "`plan\\.acceptance\\.finalAcceptanceBlocker`: targetPlatform, targetDisplay, multiDisplay, acceptanceChecklist, screenshotEvidence4K"
    "remove satisfied manual acceptance blockers while keeping incomplete checklist and weak screenshot evidence blockers")
require_no_output(
    recorded_report_combined
    "`plan\\.acceptance\\.finalAcceptanceBlocker`: [^\n]*(externalTargetVerification|manualRawEngineComparison|manualUiInspection)"
    "kept satisfied manual acceptance blockers")

execute_process(
    COMMAND
        "${CMAKE_COMMAND}" -E env
        "QT_QPA_PLATFORM=offscreen"
        "LIZZIE_KATAGO_EXECUTABLE=${APP_EXECUTABLE}"
        "LIZZIE_KATAGO_MODEL=${model_path}"
        "LIZZIE_KATAGO_ANALYSIS_CONFIG=${analysis_config_path}"
        "LIZZIE_KATAGO_GTP_CONFIG=${gtp_config_path}"
        "LIZZIE_DIAGNOSTICS_SETTINGS_FILE=${settings_file}"
        "LIZZIE_TARGET_ACCEPTANCE_RECORD_FILE=${acceptance_record_4k_screenshot_file}"
        "${APP_EXECUTABLE}" --target-acceptance-report
    WORKING_DIRECTORY "${work_dir}"
    RESULT_VARIABLE screenshot_4k_report_result
    OUTPUT_VARIABLE screenshot_4k_report_output
    ERROR_VARIABLE screenshot_4k_report_error
)
set(screenshot_4k_report_combined "${screenshot_4k_report_output}${screenshot_4k_report_error}")
if(NOT screenshot_4k_report_result EQUAL 0)
    message(FATAL_ERROR "Target acceptance 4K-screenshot report command failed: ${screenshot_4k_report_combined}")
endif()

require_output(
    screenshot_4k_report_combined
    "`plan\\.acceptance\\.evidence\\.screenshot\\.sha256Status`: match"
    "keep 4K screenshot evidence SHA-256 match")
require_output(
    screenshot_4k_report_combined
    "`plan\\.acceptance\\.evidence\\.screenshot\\.imageWidth`: 3840"
    "report 4K screenshot image width")
require_output(
    screenshot_4k_report_combined
    "`plan\\.acceptance\\.evidence\\.screenshot\\.imageHeight`: 2160"
    "report 4K screenshot image height")
require_output(
    screenshot_4k_report_combined
    "`plan\\.acceptance\\.evidence\\.screenshot\\.imagePixelsAtLeast4K`: true"
    "accept 4K screenshot pixel envelope")
require_output(
    screenshot_4k_report_combined
    "`plan\\.acceptance\\.evidence\\.screenshot\\.imageHasPixelVariation`: true"
    "accept non-uniform 4K screenshot pixel variation")
require_output(
    screenshot_4k_report_combined
    "`plan\\.acceptance\\.evidence\\.screenshot\\.readyFor4KAcceptance`: true"
    "accept complete 4K screenshot evidence")
require_output(
    screenshot_4k_report_combined
    "`plan\\.acceptance\\.finalAcceptanceBlocker`: targetPlatform, targetDisplay, multiDisplay, acceptanceChecklist"
    "remove weak screenshot evidence blocker for complete 4K screenshots")
require_no_output(
    screenshot_4k_report_combined
    "`plan\\.acceptance\\.finalAcceptanceBlocker`: [^\n]*screenshotEvidence4K"
    "kept weak screenshot evidence blocker for complete 4K screenshots")

execute_process(
    COMMAND
        "${CMAKE_COMMAND}" -E env
        "QT_QPA_PLATFORM=offscreen"
        "LIZZIE_KATAGO_EXECUTABLE=${APP_EXECUTABLE}"
        "LIZZIE_KATAGO_MODEL=${model_path}"
        "LIZZIE_KATAGO_ANALYSIS_CONFIG=${analysis_config_path}"
        "LIZZIE_KATAGO_GTP_CONFIG=${gtp_config_path}"
        "LIZZIE_DIAGNOSTICS_SETTINGS_FILE=${settings_file}"
        "LIZZIE_TARGET_ACCEPTANCE_RECORD_FILE=${acceptance_record_failed_manual_file}"
        "${APP_EXECUTABLE}" --target-acceptance-report
    WORKING_DIRECTORY "${work_dir}"
    RESULT_VARIABLE failed_manual_report_result
    OUTPUT_VARIABLE failed_manual_report_output
    ERROR_VARIABLE failed_manual_report_error
)
set(failed_manual_report_combined "${failed_manual_report_output}${failed_manual_report_error}")
if(NOT failed_manual_report_result EQUAL 0)
    message(FATAL_ERROR "Target acceptance failed-manual report command failed: ${failed_manual_report_combined}")
endif()

require_output(
    failed_manual_report_combined
    "`plan\\.acceptance\\.rawKataGoComparisonStatus`: fail"
    "report failed raw KataGo manual result")
require_output(
    failed_manual_report_combined
    "`plan\\.acceptance\\.manualFailedRecord`: .*rawKataGoComparison"
    "list failed raw KataGo aggregate manual result")
require_output(
    failed_manual_report_combined
    "`plan\\.acceptance\\.finalAcceptanceBlocker`: [^\n]*manualRawEngineComparison"
    "keep failed raw KataGo manual blocker")
require_output(
    failed_manual_report_combined
    "`plan\\.acceptance\\.finalAcceptanceStatus`: manual-acceptance-failed"
    "prioritize failed manual acceptance records over readiness blockers")

execute_process(
    COMMAND
        "${CMAKE_COMMAND}" -E env
        "QT_QPA_PLATFORM=offscreen"
        "LIZZIE_KATAGO_EXECUTABLE=${APP_EXECUTABLE}"
        "LIZZIE_KATAGO_MODEL=${model_path}"
        "LIZZIE_KATAGO_ANALYSIS_CONFIG=${analysis_config_path}"
        "LIZZIE_KATAGO_GTP_CONFIG=${gtp_config_path}"
        "LIZZIE_DIAGNOSTICS_SETTINGS_FILE=${settings_file}"
        "LIZZIE_TARGET_ACCEPTANCE_RECORD_FILE=${acceptance_record_invalid_manual_file}"
        "${APP_EXECUTABLE}" --target-acceptance-report
    WORKING_DIRECTORY "${work_dir}"
    RESULT_VARIABLE invalid_manual_report_result
    OUTPUT_VARIABLE invalid_manual_report_output
    ERROR_VARIABLE invalid_manual_report_error
)
set(invalid_manual_report_combined "${invalid_manual_report_output}${invalid_manual_report_error}")
if(NOT invalid_manual_report_result EQUAL 0)
    message(FATAL_ERROR "Target acceptance invalid-manual report command failed: ${invalid_manual_report_combined}")
endif()

require_output(
    invalid_manual_report_combined
    "`plan\\.acceptance\\.manualUiInspectionStatus`: invalid-manual-record"
    "report invalid manual UI inspection result")
require_output(
    invalid_manual_report_combined
    "`plan\\.acceptance\\.manualInvalidRecord`: .*manualUiInspection"
    "list invalid manual UI aggregate result")
require_output(
    invalid_manual_report_combined
    "`plan\\.acceptance\\.finalAcceptanceBlocker`: [^\n]*manualUiInspection"
    "keep invalid manual UI inspection blocker")
require_output(
    invalid_manual_report_combined
    "`plan\\.acceptance\\.finalAcceptanceStatus`: invalid-manual-acceptance-record"
    "prioritize invalid manual acceptance records over readiness blockers")

execute_process(
    COMMAND
        "${CMAKE_COMMAND}" -E env
        "QT_QPA_PLATFORM=offscreen"
        "LIZZIE_KATAGO_EXECUTABLE=${APP_EXECUTABLE}"
        "LIZZIE_KATAGO_MODEL=${model_path}"
        "LIZZIE_KATAGO_ANALYSIS_CONFIG=${analysis_config_path}"
        "LIZZIE_KATAGO_GTP_CONFIG=${gtp_config_path}"
        "LIZZIE_DIAGNOSTICS_SETTINGS_FILE=${settings_file}"
        "LIZZIE_TARGET_ACCEPTANCE_RECORD_FILE=${acceptance_record_failed_checklist_file}"
        "${APP_EXECUTABLE}" --target-acceptance-report
    WORKING_DIRECTORY "${work_dir}"
    RESULT_VARIABLE failed_checklist_report_result
    OUTPUT_VARIABLE failed_checklist_report_output
    ERROR_VARIABLE failed_checklist_report_error
)
set(failed_checklist_report_combined "${failed_checklist_report_output}${failed_checklist_report_error}")
if(NOT failed_checklist_report_result EQUAL 0)
    message(FATAL_ERROR "Target acceptance failed-checklist report command failed: ${failed_checklist_report_combined}")
endif()

require_output(
    failed_checklist_report_combined
    "`plan\\.acceptance\\.externalTargetVerificationStatus`: pass"
    "keep aggregate external target verification pass status")
require_output(
    failed_checklist_report_combined
    "`plan\\.acceptance\\.checklistFailedRecord`: .*externalTargetVerification\\.rawKataGoUiComparison"
    "report failed checklist item record")
require_output(
    failed_checklist_report_combined
    "`plan\\.acceptance\\.finalAcceptanceStatus`: manual-acceptance-failed"
    "prioritize failed checklist records over readiness blockers")

execute_process(
    COMMAND
        "${CMAKE_COMMAND}" -E env
        "QT_QPA_PLATFORM=offscreen"
        "LIZZIE_KATAGO_EXECUTABLE=${APP_EXECUTABLE}"
        "LIZZIE_KATAGO_MODEL=${model_path}"
        "LIZZIE_KATAGO_ANALYSIS_CONFIG=${analysis_config_path}"
        "LIZZIE_KATAGO_GTP_CONFIG=${gtp_config_path}"
        "LIZZIE_DIAGNOSTICS_SETTINGS_FILE=${settings_file}"
        "LIZZIE_TARGET_ACCEPTANCE_RECORD_FILE=${acceptance_record_invalid_checklist_file}"
        "${APP_EXECUTABLE}" --target-acceptance-report
    WORKING_DIRECTORY "${work_dir}"
    RESULT_VARIABLE invalid_checklist_report_result
    OUTPUT_VARIABLE invalid_checklist_report_output
    ERROR_VARIABLE invalid_checklist_report_error
)
set(invalid_checklist_report_combined "${invalid_checklist_report_output}${invalid_checklist_report_error}")
if(NOT invalid_checklist_report_result EQUAL 0)
    message(FATAL_ERROR "Target acceptance invalid-checklist report command failed: ${invalid_checklist_report_combined}")
endif()

require_output(
    invalid_checklist_report_combined
    "`plan\\.acceptance\\.externalTargetVerificationStatus`: pass"
    "keep aggregate external target verification pass status with invalid checklist item")
require_output(
    invalid_checklist_report_combined
    "`plan\\.acceptance\\.checklistInvalidRecord`: .*externalTargetVerification\\.rawKataGoUiComparison"
    "report invalid checklist item record")
require_output(
    invalid_checklist_report_combined
    "`plan\\.acceptance\\.finalAcceptanceStatus`: invalid-manual-acceptance-record"
    "prioritize invalid checklist records over readiness blockers")

execute_process(
    COMMAND
        "${CMAKE_COMMAND}" -E env
        "QT_QPA_PLATFORM=offscreen"
        "LIZZIE_KATAGO_EXECUTABLE=${APP_EXECUTABLE}"
        "LIZZIE_KATAGO_MODEL=${model_path}"
        "LIZZIE_KATAGO_ANALYSIS_CONFIG=${analysis_config_path}"
        "LIZZIE_KATAGO_GTP_CONFIG=${gtp_config_path}"
        "LIZZIE_DIAGNOSTICS_SETTINGS_FILE=${settings_file}"
        "LIZZIE_TARGET_ACCEPTANCE_RECORD_FILE=${acceptance_record_failed_target_component_file}"
        "${APP_EXECUTABLE}" --target-acceptance-report
    WORKING_DIRECTORY "${work_dir}"
    RESULT_VARIABLE failed_target_component_report_result
    OUTPUT_VARIABLE failed_target_component_report_output
    ERROR_VARIABLE failed_target_component_report_error
)
set(failed_target_component_report_combined "${failed_target_component_report_output}${failed_target_component_report_error}")
if(NOT failed_target_component_report_result EQUAL 0)
    message(FATAL_ERROR "Target acceptance failed-target-component report command failed: ${failed_target_component_report_combined}")
endif()

require_output(
    failed_target_component_report_combined
    "`plan\\.acceptance\\.externalTargetVerificationStatus`: pass"
    "keep explicit aggregate external target verification pass status")
require_output(
    failed_target_component_report_combined
    "`plan\\.acceptance\\.windowsNvidiaManualResult`: fail"
    "report failed Windows target component result")
require_output(
    failed_target_component_report_combined
    "`plan\\.acceptance\\.manualFailedRecord`: .*windowsNvidia"
    "list failed Windows target component aggregate manual result")
require_output(
    failed_target_component_report_combined
    "`plan\\.acceptance\\.finalAcceptanceStatus`: manual-acceptance-failed"
    "prioritize failed target component records over readiness blockers")

execute_process(
    COMMAND
        "${CMAKE_COMMAND}" -E env
        "QT_QPA_PLATFORM=offscreen"
        "LIZZIE_KATAGO_EXECUTABLE=${APP_EXECUTABLE}"
        "LIZZIE_KATAGO_MODEL=${model_path}"
        "LIZZIE_KATAGO_ANALYSIS_CONFIG=${analysis_config_path}"
        "LIZZIE_KATAGO_GTP_CONFIG=${gtp_config_path}"
        "LIZZIE_DIAGNOSTICS_SETTINGS_FILE=${settings_file}"
        "LIZZIE_TARGET_ACCEPTANCE_RECORD_FILE=${acceptance_record_invalid_target_component_file}"
        "${APP_EXECUTABLE}" --target-acceptance-report
    WORKING_DIRECTORY "${work_dir}"
    RESULT_VARIABLE invalid_target_component_report_result
    OUTPUT_VARIABLE invalid_target_component_report_output
    ERROR_VARIABLE invalid_target_component_report_error
)
set(invalid_target_component_report_combined "${invalid_target_component_report_output}${invalid_target_component_report_error}")
if(NOT invalid_target_component_report_result EQUAL 0)
    message(FATAL_ERROR "Target acceptance invalid-target-component report command failed: ${invalid_target_component_report_combined}")
endif()

require_output(
    invalid_target_component_report_combined
    "`plan\\.acceptance\\.externalTargetVerificationStatus`: pass"
    "keep explicit aggregate external target verification pass status with invalid component")
require_output(
    invalid_target_component_report_combined
    "`plan\\.acceptance\\.windowsNvidiaManualResult`: invalid-manual-record"
    "report invalid Windows target component result")
require_output(
    invalid_target_component_report_combined
    "`plan\\.acceptance\\.manualInvalidRecord`: .*windowsNvidia"
    "list invalid Windows target component aggregate manual result")
require_output(
    invalid_target_component_report_combined
    "`plan\\.acceptance\\.finalAcceptanceStatus`: invalid-manual-acceptance-record"
    "prioritize invalid target component records over readiness blockers")

execute_process(
    COMMAND
        "${CMAKE_COMMAND}" -E env
        "QT_QPA_PLATFORM=offscreen"
        "LIZZIE_KATAGO_EXECUTABLE=${APP_EXECUTABLE}"
        "LIZZIE_KATAGO_MODEL=${model_path}"
        "LIZZIE_KATAGO_ANALYSIS_CONFIG=${analysis_config_path}"
        "LIZZIE_KATAGO_GTP_CONFIG=${gtp_config_path}"
        "LIZZIE_DIAGNOSTICS_SETTINGS_FILE=${settings_file}"
        "LIZZIE_TARGET_ACCEPTANCE_RECORD_FILE=${acceptance_record_hash_mismatch_file}"
        "${APP_EXECUTABLE}" --target-acceptance-report
    WORKING_DIRECTORY "${work_dir}"
    RESULT_VARIABLE hash_mismatch_report_result
    OUTPUT_VARIABLE hash_mismatch_report_output
    ERROR_VARIABLE hash_mismatch_report_error
)
set(hash_mismatch_report_combined "${hash_mismatch_report_output}${hash_mismatch_report_error}")
if(NOT hash_mismatch_report_result EQUAL 0)
    message(FATAL_ERROR "Target acceptance hash-mismatch report command failed: ${hash_mismatch_report_combined}")
endif()

require_output(
    hash_mismatch_report_combined
    "`plan\\.acceptance\\.evidence\\.diagnostics\\.sha256Status`: mismatch"
    "report diagnostics evidence SHA-256 mismatch")
require_output(
    hash_mismatch_report_combined
    "`plan\\.acceptance\\.evidenceSha256\\.ready`: false"
    "report failed evidence SHA-256 readiness")
require_output(
    hash_mismatch_report_combined
    "`plan\\.acceptance\\.evidenceSha256\\.blocker`: diagnosticsEvidenceSha256"
    "report diagnostics evidence SHA-256 blocker")
require_output(
    hash_mismatch_report_combined
    "`plan\\.acceptance\\.finalAcceptanceBlocker`: targetPlatform, targetDisplay, multiDisplay, acceptanceChecklist, acceptanceEvidenceSha256, screenshotEvidence4K"
    "require matching evidence SHA-256 before final acceptance")

execute_process(
    COMMAND
        "${CMAKE_COMMAND}" -E env
        "QT_QPA_PLATFORM=offscreen"
        "LIZZIE_KATAGO_EXECUTABLE=${APP_EXECUTABLE}"
        "LIZZIE_KATAGO_MODEL=${model_path}"
        "LIZZIE_KATAGO_ANALYSIS_CONFIG=${analysis_config_path}"
        "LIZZIE_KATAGO_GTP_CONFIG=${gtp_config_path}"
        "LIZZIE_DIAGNOSTICS_SETTINGS_FILE=${settings_file}"
        "LIZZIE_TARGET_ACCEPTANCE_RECORD_FILE=${acceptance_record_stale_evidence_file}"
        "${APP_EXECUTABLE}" --target-acceptance-report
    WORKING_DIRECTORY "${work_dir}"
    RESULT_VARIABLE stale_evidence_report_result
    OUTPUT_VARIABLE stale_evidence_report_output
    ERROR_VARIABLE stale_evidence_report_error
)
set(stale_evidence_report_combined "${stale_evidence_report_output}${stale_evidence_report_error}")
if(NOT stale_evidence_report_result EQUAL 0)
    message(FATAL_ERROR "Target acceptance stale-evidence report command failed: ${stale_evidence_report_combined}")
endif()

require_output(
    stale_evidence_report_combined
    "`plan\\.acceptance\\.evidence\\.diagnostics\\.timestampStatus`: after-completed-utc"
    "report diagnostics evidence modified after completion")
require_output(
    stale_evidence_report_combined
    "`plan\\.acceptance\\.recordFile\\.timestampStatus`: after-completed-utc"
    "report target acceptance record modified after completion")
require_output(
    stale_evidence_report_combined
    "`plan\\.acceptance\\.recordTimestamp\\.ready`: false"
    "report failed target acceptance record timestamp readiness")
require_output(
    stale_evidence_report_combined
    "`plan\\.acceptance\\.recordTimestamp\\.blocker`: recordFileTimestamp"
    "report target acceptance record timestamp blocker")
require_output(
    stale_evidence_report_combined
    "`plan\\.acceptance\\.evidenceTimestamp\\.ready`: false"
    "report failed evidence timestamp readiness")
require_output(
    stale_evidence_report_combined
    "`plan\\.acceptance\\.evidenceTimestamp\\.blocker`: diagnosticsEvidenceTimestamp"
    "report diagnostics evidence timestamp blocker")
require_output(
    stale_evidence_report_combined
    "`plan\\.acceptance\\.finalAcceptanceBlocker`: targetPlatform, targetDisplay, multiDisplay, acceptanceChecklist, acceptanceRecordTimestamp, acceptanceEvidenceTimestamp"
    "require record and evidence timestamps not newer than completedUtc before final acceptance")

execute_process(
    COMMAND
        "${CMAKE_COMMAND}" -E env
        "QT_QPA_PLATFORM=offscreen"
        "LIZZIE_KATAGO_EXECUTABLE=${APP_EXECUTABLE}"
        "LIZZIE_KATAGO_MODEL=${model_path}"
        "LIZZIE_KATAGO_ANALYSIS_CONFIG=${analysis_config_path}"
        "LIZZIE_KATAGO_GTP_CONFIG=${gtp_config_path}"
        "LIZZIE_DIAGNOSTICS_SETTINGS_FILE=${settings_file}"
        "LIZZIE_TARGET_ACCEPTANCE_RECORD_FILE=${relative_acceptance_record_file}"
        "${APP_EXECUTABLE}" --target-acceptance-report
    WORKING_DIRECTORY "${TEST_BINARY_DIR}"
    RESULT_VARIABLE relative_evidence_report_result
    OUTPUT_VARIABLE relative_evidence_report_output
    ERROR_VARIABLE relative_evidence_report_error
)
set(relative_evidence_report_combined "${relative_evidence_report_output}${relative_evidence_report_error}")
if(NOT relative_evidence_report_result EQUAL 0)
    message(FATAL_ERROR "Target acceptance relative-evidence report command failed: ${relative_evidence_report_combined}")
endif()

require_output(
    relative_evidence_report_combined
    "`plan\\.acceptance\\.evidence\\.diagnostics\\.value`: evidence/target-diagnostics\\.txt"
    "preserve relative diagnostics evidence path display")
require_output(
    relative_evidence_report_combined
    "`plan\\.acceptance\\.recordFile\\.sha256`: ${relative_acceptance_record_sha256}"
    "report relative target acceptance record SHA-256")
require_output(
    relative_evidence_report_combined
    "`plan\\.acceptance\\.evidence\\.diagnostics\\.exists`: true"
    "resolve diagnostics evidence relative to the record file")
require_output(
    relative_evidence_report_combined
    "`plan\\.acceptance\\.evidence\\.diagnostics\\.lastModifiedUtc`: [0-9]"
    "report relative diagnostics evidence modification time")
require_output(
    relative_evidence_report_combined
    "`plan\\.acceptance\\.evidence\\.diagnostics\\.timestampStatus`: not-after-completed-utc"
    "report relative diagnostics evidence timestamp status")
require_output(
    relative_evidence_report_combined
    "`plan\\.acceptance\\.evidence\\.ready`: true"
    "accept complete relative evidence paths")
require_output(
    relative_evidence_report_combined
    "`plan\\.acceptance\\.recordMetadata\\.ready`: true"
    "accept complete metadata with relative evidence paths")
require_output(
    relative_evidence_report_combined
    "`plan\\.acceptance\\.evidence\\.blocker`: \\(none\\)"
    "report no relative evidence blockers")
require_output(
    relative_evidence_report_combined
    "`plan\\.acceptance\\.evidenceTimestamp\\.ready`: true"
    "report relative evidence timestamp readiness")
require_output(
    relative_evidence_report_combined
    "`plan\\.acceptance\\.finalAcceptanceBlocker`: targetPlatform, targetDisplay, multiDisplay, acceptanceChecklist, screenshotEvidence4K"
    "avoid final acceptance evidence blocker for complete relative evidence while keeping incomplete checklist and weak screenshot evidence blockers")
require_no_output(
    relative_evidence_report_combined
    "`plan\\.acceptance\\.finalAcceptanceBlocker`: [^\n]*acceptanceEvidence"
    "reintroduced evidence blocker for complete relative evidence")
require_no_output(
    relative_evidence_report_combined
    "`plan\\.acceptance\\.finalAcceptanceBlocker`: [^\n]*acceptanceMetadata"
    "reintroduced metadata blocker for complete relative evidence")

execute_process(
    COMMAND
        "${CMAKE_COMMAND}" -E env
        "QT_QPA_PLATFORM=offscreen"
        "LIZZIE_KATAGO_EXECUTABLE=${APP_EXECUTABLE}"
        "LIZZIE_KATAGO_MODEL=${model_path}"
        "LIZZIE_KATAGO_ANALYSIS_CONFIG=${analysis_config_path}"
        "LIZZIE_KATAGO_GTP_CONFIG=${gtp_config_path}"
        "LIZZIE_DIAGNOSTICS_SETTINGS_FILE=${settings_file}"
        "LIZZIE_TARGET_ACCEPTANCE_RECORD_FILE=${acceptance_record_wrong_app_version_file}"
        "${APP_EXECUTABLE}" --target-acceptance-report
    WORKING_DIRECTORY "${work_dir}"
    RESULT_VARIABLE wrong_app_version_report_result
    OUTPUT_VARIABLE wrong_app_version_report_output
    ERROR_VARIABLE wrong_app_version_report_error
)
set(wrong_app_version_report_combined "${wrong_app_version_report_output}${wrong_app_version_report_error}")
if(NOT wrong_app_version_report_result EQUAL 0)
    message(FATAL_ERROR "Target acceptance wrong-app-version report command failed: ${wrong_app_version_report_combined}")
endif()

require_output(
    wrong_app_version_report_combined
    "`plan\\.acceptance\\.record\\.appVersion`: 0\\.0\\.0"
    "report mismatched target acceptance app version")
require_output(
    wrong_app_version_report_combined
    "`plan\\.acceptance\\.record\\.appVersionStatus`: mismatch"
    "report mismatched target acceptance app version status")
require_output(
    wrong_app_version_report_combined
    "`plan\\.acceptance\\.recordMetadata\\.ready`: false"
    "reject mismatched app version metadata")
require_output(
    wrong_app_version_report_combined
    "`plan\\.acceptance\\.recordMetadata\\.blocker`: appVersion"
    "report app version metadata blocker")
require_output(
    wrong_app_version_report_combined
    "`plan\\.acceptance\\.finalAcceptanceBlocker`: targetPlatform, targetDisplay, multiDisplay, acceptanceChecklist, acceptanceMetadata"
    "require matching app version metadata before final acceptance")

execute_process(
    COMMAND
        "${CMAKE_COMMAND}" -E env
        "QT_QPA_PLATFORM=offscreen"
        "LIZZIE_KATAGO_EXECUTABLE=${APP_EXECUTABLE}"
        "LIZZIE_KATAGO_MODEL=${model_path}"
        "LIZZIE_KATAGO_ANALYSIS_CONFIG=${analysis_config_path}"
        "LIZZIE_KATAGO_GTP_CONFIG=${gtp_config_path}"
        "LIZZIE_DIAGNOSTICS_SETTINGS_FILE=${settings_file}"
        "LIZZIE_TARGET_ACCEPTANCE_RECORD_FILE=${acceptance_record_wrong_app_executable_file}"
        "${APP_EXECUTABLE}" --target-acceptance-report
    WORKING_DIRECTORY "${work_dir}"
    RESULT_VARIABLE wrong_app_executable_report_result
    OUTPUT_VARIABLE wrong_app_executable_report_output
    ERROR_VARIABLE wrong_app_executable_report_error
)
set(wrong_app_executable_report_combined "${wrong_app_executable_report_output}${wrong_app_executable_report_error}")
if(NOT wrong_app_executable_report_result EQUAL 0)
    message(FATAL_ERROR "Target acceptance wrong-app-executable report command failed: ${wrong_app_executable_report_combined}")
endif()

require_output(
    wrong_app_executable_report_combined
    "`plan\\.acceptance\\.record\\.appExecutableSha256`: 0000000000000000000000000000000000000000000000000000000000000000"
    "report mismatched target acceptance app executable SHA-256")
require_output(
    wrong_app_executable_report_combined
    "`plan\\.acceptance\\.record\\.appExecutableSha256Status`: mismatch"
    "report mismatched target acceptance app executable SHA-256 status")
require_output(
    wrong_app_executable_report_combined
    "`plan\\.acceptance\\.recordMetadata\\.ready`: false"
    "reject mismatched app executable metadata")
require_output(
    wrong_app_executable_report_combined
    "`plan\\.acceptance\\.recordMetadata\\.blocker`: appExecutableSha256"
    "report app executable metadata blocker")
require_output(
    wrong_app_executable_report_combined
    "`plan\\.acceptance\\.finalAcceptanceBlocker`: targetPlatform, targetDisplay, multiDisplay, acceptanceChecklist, acceptanceMetadata"
    "require matching app executable metadata before final acceptance")

execute_process(
    COMMAND
        "${CMAKE_COMMAND}" -E env
        "QT_QPA_PLATFORM=offscreen"
        "LIZZIE_KATAGO_EXECUTABLE=${APP_EXECUTABLE}"
        "LIZZIE_KATAGO_MODEL=${model_path}"
        "LIZZIE_KATAGO_ANALYSIS_CONFIG=${analysis_config_path}"
        "LIZZIE_KATAGO_GTP_CONFIG=${gtp_config_path}"
        "LIZZIE_DIAGNOSTICS_SETTINGS_FILE=${settings_file}"
        "LIZZIE_TARGET_ACCEPTANCE_RECORD_FILE=${acceptance_record_wrong_os_metadata_file}"
        "${APP_EXECUTABLE}" --target-acceptance-report
    WORKING_DIRECTORY "${work_dir}"
    RESULT_VARIABLE wrong_os_metadata_report_result
    OUTPUT_VARIABLE wrong_os_metadata_report_output
    ERROR_VARIABLE wrong_os_metadata_report_error
)
set(wrong_os_metadata_report_combined "${wrong_os_metadata_report_output}${wrong_os_metadata_report_error}")
if(NOT wrong_os_metadata_report_result EQUAL 0)
    message(FATAL_ERROR "Target acceptance wrong-OS-metadata report command failed: ${wrong_os_metadata_report_combined}")
endif()

require_output(
    wrong_os_metadata_report_combined
    "`plan\\.acceptance\\.record\\.osKernelType`: not-the-current-kernel"
    "report mismatched target acceptance OS kernel type")
require_output(
    wrong_os_metadata_report_combined
    "`plan\\.acceptance\\.record\\.osKernelTypeStatus`: mismatch"
    "report mismatched target acceptance OS kernel type status")
require_output(
    wrong_os_metadata_report_combined
    "`plan\\.acceptance\\.recordMetadata\\.ready`: false"
    "reject mismatched OS metadata")
require_output(
    wrong_os_metadata_report_combined
    "`plan\\.acceptance\\.recordMetadata\\.blocker`: osKernelType"
    "report OS kernel metadata blocker")
require_output(
    wrong_os_metadata_report_combined
    "`plan\\.acceptance\\.finalAcceptanceBlocker`: targetPlatform, targetDisplay, multiDisplay, acceptanceChecklist, acceptanceMetadata"
    "require matching OS metadata before final acceptance")

execute_process(
    COMMAND
        "${CMAKE_COMMAND}" -E env
        "QT_QPA_PLATFORM=offscreen"
        "LIZZIE_KATAGO_EXECUTABLE=${APP_EXECUTABLE}"
        "LIZZIE_KATAGO_MODEL=${model_path}"
        "LIZZIE_KATAGO_ANALYSIS_CONFIG=${analysis_config_path}"
        "LIZZIE_KATAGO_GTP_CONFIG=${gtp_config_path}"
        "LIZZIE_DIAGNOSTICS_SETTINGS_FILE=${settings_file}"
        "LIZZIE_TARGET_ACCEPTANCE_RECORD_FILE=${acceptance_record_wrong_runtime_metadata_file}"
        "${APP_EXECUTABLE}" --target-acceptance-report
    WORKING_DIRECTORY "${work_dir}"
    RESULT_VARIABLE wrong_runtime_metadata_report_result
    OUTPUT_VARIABLE wrong_runtime_metadata_report_output
    ERROR_VARIABLE wrong_runtime_metadata_report_error
)
set(
    wrong_runtime_metadata_report_combined
    "${wrong_runtime_metadata_report_output}${wrong_runtime_metadata_report_error}")
if(NOT wrong_runtime_metadata_report_result EQUAL 0)
    message(
        FATAL_ERROR
        "Target acceptance wrong-runtime-metadata report command failed: ${wrong_runtime_metadata_report_combined}")
endif()

require_output(
    wrong_runtime_metadata_report_combined
    "`plan\\.acceptance\\.record\\.qtRuntimeVersion`: 0\\.0\\.0-not-the-current-runtime"
    "report mismatched target acceptance Qt runtime version")
require_output(
    wrong_runtime_metadata_report_combined
    "`plan\\.acceptance\\.record\\.qtRuntimeVersionStatus`: mismatch"
    "report mismatched target acceptance Qt runtime version status")
require_output(
    wrong_runtime_metadata_report_combined
    "`plan\\.acceptance\\.recordMetadata\\.ready`: false"
    "reject mismatched runtime metadata")
require_output(
    wrong_runtime_metadata_report_combined
    "`plan\\.acceptance\\.recordMetadata\\.blocker`: qtRuntimeVersion"
    "report Qt runtime metadata blocker")
require_output(
    wrong_runtime_metadata_report_combined
    "`plan\\.acceptance\\.finalAcceptanceBlocker`: targetPlatform, targetDisplay, multiDisplay, acceptanceChecklist, acceptanceMetadata"
    "require matching runtime metadata before final acceptance")

execute_process(
    COMMAND
        "${CMAKE_COMMAND}" -E env
        "QT_QPA_PLATFORM=offscreen"
        "LIZZIE_KATAGO_EXECUTABLE=${APP_EXECUTABLE}"
        "LIZZIE_KATAGO_MODEL=${model_path}"
        "LIZZIE_KATAGO_ANALYSIS_CONFIG=${analysis_config_path}"
        "LIZZIE_KATAGO_GTP_CONFIG=${gtp_config_path}"
        "LIZZIE_DIAGNOSTICS_SETTINGS_FILE=${settings_file}"
        "LIZZIE_TARGET_ACCEPTANCE_RECORD_FILE=${acceptance_record_wrong_abi_metadata_file}"
        "${APP_EXECUTABLE}" --target-acceptance-report
    WORKING_DIRECTORY "${work_dir}"
    RESULT_VARIABLE wrong_abi_metadata_report_result
    OUTPUT_VARIABLE wrong_abi_metadata_report_output
    ERROR_VARIABLE wrong_abi_metadata_report_error
)
set(wrong_abi_metadata_report_combined "${wrong_abi_metadata_report_output}${wrong_abi_metadata_report_error}")
if(NOT wrong_abi_metadata_report_result EQUAL 0)
    message(FATAL_ERROR "Target acceptance wrong-ABI-metadata report command failed: ${wrong_abi_metadata_report_combined}")
endif()

require_output(
    wrong_abi_metadata_report_combined
    "`plan\\.acceptance\\.record\\.qtBuildAbi`: not-the-current-build-abi"
    "report mismatched target acceptance Qt build ABI")
require_output(
    wrong_abi_metadata_report_combined
    "`plan\\.acceptance\\.record\\.qtBuildAbiStatus`: mismatch"
    "report mismatched target acceptance Qt build ABI status")
require_output(
    wrong_abi_metadata_report_combined
    "`plan\\.acceptance\\.recordMetadata\\.ready`: false"
    "reject mismatched ABI metadata")
require_output(
    wrong_abi_metadata_report_combined
    "`plan\\.acceptance\\.recordMetadata\\.blocker`: qtBuildAbi"
    "report Qt build ABI metadata blocker")
require_output(
    wrong_abi_metadata_report_combined
    "`plan\\.acceptance\\.finalAcceptanceBlocker`: targetPlatform, targetDisplay, multiDisplay, acceptanceChecklist, acceptanceMetadata"
    "require matching ABI metadata before final acceptance")

execute_process(
    COMMAND
        "${CMAKE_COMMAND}" -E env
        "QT_QPA_PLATFORM=offscreen"
        "LIZZIE_KATAGO_EXECUTABLE=${APP_EXECUTABLE}"
        "LIZZIE_KATAGO_MODEL=${model_path}"
        "LIZZIE_KATAGO_ANALYSIS_CONFIG=${analysis_config_path}"
        "LIZZIE_KATAGO_GTP_CONFIG=${gtp_config_path}"
        "LIZZIE_DIAGNOSTICS_SETTINGS_FILE=${settings_file}"
        "LIZZIE_TARGET_ACCEPTANCE_RECORD_FILE=${acceptance_record_wrong_cpu_metadata_file}"
        "${APP_EXECUTABLE}" --target-acceptance-report
    WORKING_DIRECTORY "${work_dir}"
    RESULT_VARIABLE wrong_cpu_metadata_report_result
    OUTPUT_VARIABLE wrong_cpu_metadata_report_output
    ERROR_VARIABLE wrong_cpu_metadata_report_error
)
set(wrong_cpu_metadata_report_combined "${wrong_cpu_metadata_report_output}${wrong_cpu_metadata_report_error}")
if(NOT wrong_cpu_metadata_report_result EQUAL 0)
    message(FATAL_ERROR "Target acceptance wrong-CPU-metadata report command failed: ${wrong_cpu_metadata_report_combined}")
endif()

require_output(
    wrong_cpu_metadata_report_combined
    "`plan\\.acceptance\\.record\\.cpuArchitecture`: not-the-current-cpu"
    "report mismatched target acceptance CPU architecture")
require_output(
    wrong_cpu_metadata_report_combined
    "`plan\\.acceptance\\.record\\.cpuArchitectureStatus`: mismatch"
    "report mismatched target acceptance CPU architecture status")
require_output(
    wrong_cpu_metadata_report_combined
    "`plan\\.acceptance\\.recordMetadata\\.ready`: false"
    "reject mismatched CPU metadata")
require_output(
    wrong_cpu_metadata_report_combined
    "`plan\\.acceptance\\.recordMetadata\\.blocker`: cpuArchitecture"
    "report CPU architecture metadata blocker")
require_output(
    wrong_cpu_metadata_report_combined
    "`plan\\.acceptance\\.finalAcceptanceBlocker`: targetPlatform, targetDisplay, multiDisplay, acceptanceChecklist, acceptanceMetadata"
    "require matching CPU metadata before final acceptance")

execute_process(
    COMMAND
        "${CMAKE_COMMAND}" -E env
        "QT_QPA_PLATFORM=offscreen"
        "LIZZIE_KATAGO_EXECUTABLE=${APP_EXECUTABLE}"
        "LIZZIE_KATAGO_MODEL=${model_path}"
        "LIZZIE_KATAGO_ANALYSIS_CONFIG=${analysis_config_path}"
        "LIZZIE_KATAGO_GTP_CONFIG=${gtp_config_path}"
        "LIZZIE_DIAGNOSTICS_SETTINGS_FILE=${settings_file}"
        "LIZZIE_TARGET_ACCEPTANCE_RECORD_FILE=${acceptance_record_wrong_target_machine_file}"
        "${APP_EXECUTABLE}" --target-acceptance-report
    WORKING_DIRECTORY "${work_dir}"
    RESULT_VARIABLE wrong_target_machine_report_result
    OUTPUT_VARIABLE wrong_target_machine_report_output
    ERROR_VARIABLE wrong_target_machine_report_error
)
set(
    wrong_target_machine_report_combined
    "${wrong_target_machine_report_output}${wrong_target_machine_report_error}")
if(NOT wrong_target_machine_report_result EQUAL 0)
    message(
        FATAL_ERROR
        "Target acceptance wrong-target-machine report command failed: ${wrong_target_machine_report_combined}")
endif()

require_output(
    wrong_target_machine_report_combined
    "`plan\\.acceptance\\.record\\.targetMachine`: not-the-current-target-machine"
    "report mismatched target acceptance machine")
require_output(
    wrong_target_machine_report_combined
    "`plan\\.acceptance\\.record\\.targetMachineStatus`: mismatch"
    "report mismatched target acceptance machine status")
require_output(
    wrong_target_machine_report_combined
    "`plan\\.acceptance\\.recordMetadata\\.ready`: false"
    "reject mismatched target machine metadata")
require_output(
    wrong_target_machine_report_combined
    "`plan\\.acceptance\\.recordMetadata\\.blocker`: targetMachine"
    "report target machine metadata blocker")
require_output(
    wrong_target_machine_report_combined
    "`plan\\.acceptance\\.finalAcceptanceBlocker`: targetPlatform, targetDisplay, multiDisplay, acceptanceChecklist, acceptanceMetadata"
    "require matching target machine metadata before final acceptance")

execute_process(
    COMMAND
        "${CMAKE_COMMAND}" -E env
        "QT_QPA_PLATFORM=offscreen"
        "LIZZIE_KATAGO_EXECUTABLE=${APP_EXECUTABLE}"
        "LIZZIE_KATAGO_MODEL=${model_path}"
        "LIZZIE_KATAGO_ANALYSIS_CONFIG=${analysis_config_path}"
        "LIZZIE_KATAGO_GTP_CONFIG=${gtp_config_path}"
        "LIZZIE_DIAGNOSTICS_SETTINGS_FILE=${settings_file}"
        "LIZZIE_TARGET_ACCEPTANCE_RECORD_FILE=${acceptance_record_wrong_gpu_driver_file}"
        "${APP_EXECUTABLE}" --target-acceptance-report
    WORKING_DIRECTORY "${work_dir}"
    RESULT_VARIABLE wrong_gpu_driver_report_result
    OUTPUT_VARIABLE wrong_gpu_driver_report_output
    ERROR_VARIABLE wrong_gpu_driver_report_error
)
set(wrong_gpu_driver_report_combined "${wrong_gpu_driver_report_output}${wrong_gpu_driver_report_error}")
if(NOT wrong_gpu_driver_report_result EQUAL 0)
    message(FATAL_ERROR "Target acceptance wrong-GPU-driver report command failed: ${wrong_gpu_driver_report_combined}")
endif()

require_output(
    wrong_gpu_driver_report_combined
    "`plan\\.acceptance\\.record\\.gpuDriver`: Mesa llvmpipe"
    "report non-NVIDIA target acceptance GPU driver")
require_output(
    wrong_gpu_driver_report_combined
    "`plan\\.acceptance\\.record\\.gpuDriverStatus`: mismatch"
    "report non-NVIDIA target acceptance GPU driver status")
require_output(
    wrong_gpu_driver_report_combined
    "`plan\\.acceptance\\.recordMetadata\\.ready`: false"
    "reject non-NVIDIA target acceptance metadata")
require_output(
    wrong_gpu_driver_report_combined
    "`plan\\.acceptance\\.recordMetadata\\.blocker`: gpuDriver"
    "report GPU driver metadata blocker")
require_output(
    wrong_gpu_driver_report_combined
    "`plan\\.acceptance\\.finalAcceptanceBlocker`: targetPlatform, targetDisplay, multiDisplay, acceptanceChecklist, acceptanceMetadata"
    "require NVIDIA GPU driver metadata before final acceptance")

execute_process(
    COMMAND
        "${CMAKE_COMMAND}" -E env
        "QT_QPA_PLATFORM=offscreen"
        "LIZZIE_KATAGO_EXECUTABLE=${APP_EXECUTABLE}"
        "LIZZIE_KATAGO_MODEL=${model_path}"
        "LIZZIE_KATAGO_ANALYSIS_CONFIG=${analysis_config_path}"
        "LIZZIE_KATAGO_GTP_CONFIG=${gtp_config_path}"
        "LIZZIE_DIAGNOSTICS_SETTINGS_FILE=${settings_file}"
        "LIZZIE_TARGET_ACCEPTANCE_RECORD_FILE=${acceptance_record_wrong_katago_version_file}"
        "${APP_EXECUTABLE}" --target-acceptance-report
    WORKING_DIRECTORY "${work_dir}"
    RESULT_VARIABLE wrong_katago_version_report_result
    OUTPUT_VARIABLE wrong_katago_version_report_output
    ERROR_VARIABLE wrong_katago_version_report_error
)
set(
    wrong_katago_version_report_combined
    "${wrong_katago_version_report_output}${wrong_katago_version_report_error}")
if(NOT wrong_katago_version_report_result EQUAL 0)
    message(FATAL_ERROR "Target acceptance wrong-KataGo-version report command failed: ${wrong_katago_version_report_combined}")
endif()

require_output(
    wrong_katago_version_report_combined
    "`plan\\.acceptance\\.record\\.kataGoVersion`: Leela Zero 0\\.17"
    "report non-KataGo target acceptance engine version")
require_output(
    wrong_katago_version_report_combined
    "`plan\\.acceptance\\.record\\.kataGoVersionStatus`: mismatch"
    "report non-KataGo target acceptance engine version status")
require_output(
    wrong_katago_version_report_combined
    "`plan\\.acceptance\\.recordMetadata\\.ready`: false"
    "reject non-KataGo target acceptance metadata")
require_output(
    wrong_katago_version_report_combined
    "`plan\\.acceptance\\.recordMetadata\\.blocker`: kataGoVersion"
    "report KataGo version metadata blocker")
require_output(
    wrong_katago_version_report_combined
    "`plan\\.acceptance\\.finalAcceptanceBlocker`: targetPlatform, targetDisplay, multiDisplay, acceptanceChecklist, acceptanceMetadata"
    "require KataGo version metadata before final acceptance")

execute_process(
    COMMAND
        "${CMAKE_COMMAND}" -E env
        "QT_QPA_PLATFORM=offscreen"
        "LIZZIE_KATAGO_EXECUTABLE=${APP_EXECUTABLE}"
        "LIZZIE_KATAGO_MODEL=${model_path}"
        "LIZZIE_KATAGO_ANALYSIS_CONFIG=${analysis_config_path}"
        "LIZZIE_KATAGO_GTP_CONFIG=${gtp_config_path}"
        "LIZZIE_DIAGNOSTICS_SETTINGS_FILE=${settings_file}"
        "LIZZIE_TARGET_ACCEPTANCE_RECORD_FILE=${acceptance_record_missing_metadata_file}"
        "${APP_EXECUTABLE}" --target-acceptance-report
    WORKING_DIRECTORY "${work_dir}"
    RESULT_VARIABLE missing_metadata_report_result
    OUTPUT_VARIABLE missing_metadata_report_output
    ERROR_VARIABLE missing_metadata_report_error
)
set(missing_metadata_report_combined "${missing_metadata_report_output}${missing_metadata_report_error}")
if(NOT missing_metadata_report_result EQUAL 0)
    message(FATAL_ERROR "Target acceptance missing-metadata report command failed: ${missing_metadata_report_combined}")
endif()

require_output(
    missing_metadata_report_combined
    "`plan\\.acceptance\\.recordMetadata\\.ready`: false"
    "report incomplete metadata readiness for pass records without metadata")
require_output(
    missing_metadata_report_combined
    "`plan\\.acceptance\\.recordMetadata\\.blocker`: .*completedUtc"
    "report missing completion metadata blocker")
require_output(
    missing_metadata_report_combined
    "`plan\\.acceptance\\.evidence\\.ready`: true"
    "keep complete evidence readiness when metadata is missing")
require_output(
    missing_metadata_report_combined
    "`plan\\.acceptance\\.finalAcceptanceBlocker`: targetPlatform, targetDisplay, multiDisplay, acceptanceChecklist, acceptanceMetadata"
    "require metadata before final acceptance")

execute_process(
    COMMAND
        "${CMAKE_COMMAND}" -E env
        "QT_QPA_PLATFORM=offscreen"
        "LIZZIE_KATAGO_EXECUTABLE=${APP_EXECUTABLE}"
        "LIZZIE_KATAGO_MODEL=${model_path}"
        "LIZZIE_KATAGO_ANALYSIS_CONFIG=${analysis_config_path}"
        "LIZZIE_KATAGO_GTP_CONFIG=${gtp_config_path}"
        "LIZZIE_DIAGNOSTICS_SETTINGS_FILE=${settings_file}"
        "LIZZIE_TARGET_ACCEPTANCE_RECORD_FILE=${acceptance_record_invalid_metadata_file}"
        "${APP_EXECUTABLE}" --target-acceptance-report
    WORKING_DIRECTORY "${work_dir}"
    RESULT_VARIABLE invalid_metadata_report_result
    OUTPUT_VARIABLE invalid_metadata_report_output
    ERROR_VARIABLE invalid_metadata_report_error
)
set(invalid_metadata_report_combined "${invalid_metadata_report_output}${invalid_metadata_report_error}")
if(NOT invalid_metadata_report_result EQUAL 0)
    message(FATAL_ERROR "Target acceptance invalid-metadata report command failed: ${invalid_metadata_report_combined}")
endif()

require_output(
    invalid_metadata_report_combined
    "`plan\\.acceptance\\.record\\.completedUtc`: 2026-07-02 12:34:56"
    "report invalid target acceptance completion timestamp verbatim")
require_output(
    invalid_metadata_report_combined
    "`plan\\.acceptance\\.record\\.completedUtcStatus`: invalid-format"
    "report invalid target acceptance completion timestamp status")
require_output(
    invalid_metadata_report_combined
    "`plan\\.acceptance\\.recordMetadata\\.ready`: false"
    "reject non-UTC target acceptance completion timestamp")
require_output(
    invalid_metadata_report_combined
    "`plan\\.acceptance\\.recordMetadata\\.blocker`: completedUtc"
    "report invalid completion timestamp metadata blocker")
require_output(
    invalid_metadata_report_combined
    "`plan\\.acceptance\\.finalAcceptanceBlocker`: targetPlatform, targetDisplay, multiDisplay, acceptanceChecklist, acceptanceMetadata"
    "require valid metadata before final acceptance")

execute_process(
    COMMAND
        "${CMAKE_COMMAND}" -E env
        "QT_QPA_PLATFORM=offscreen"
        "LIZZIE_KATAGO_EXECUTABLE=${APP_EXECUTABLE}"
        "LIZZIE_KATAGO_MODEL=${model_path}"
        "LIZZIE_KATAGO_ANALYSIS_CONFIG=${analysis_config_path}"
        "LIZZIE_KATAGO_GTP_CONFIG=${gtp_config_path}"
        "LIZZIE_DIAGNOSTICS_SETTINGS_FILE=${settings_file}"
        "LIZZIE_TARGET_ACCEPTANCE_RECORD_FILE=${acceptance_record_future_metadata_file}"
        "${APP_EXECUTABLE}" --target-acceptance-report
    WORKING_DIRECTORY "${work_dir}"
    RESULT_VARIABLE future_metadata_report_result
    OUTPUT_VARIABLE future_metadata_report_output
    ERROR_VARIABLE future_metadata_report_error
)
set(future_metadata_report_combined "${future_metadata_report_output}${future_metadata_report_error}")
if(NOT future_metadata_report_result EQUAL 0)
    message(FATAL_ERROR "Target acceptance future-metadata report command failed: ${future_metadata_report_combined}")
endif()

require_output(
    future_metadata_report_combined
    "`plan\\.acceptance\\.record\\.completedUtc`: 2999-01-01T00:00:00Z"
    "report future target acceptance completion timestamp verbatim")
require_output(
    future_metadata_report_combined
    "`plan\\.acceptance\\.record\\.completedUtcStatus`: future"
    "report future target acceptance completion timestamp status")
require_output(
    future_metadata_report_combined
    "`plan\\.acceptance\\.recordMetadata\\.ready`: false"
    "reject future target acceptance completion timestamp")
require_output(
    future_metadata_report_combined
    "`plan\\.acceptance\\.recordMetadata\\.blocker`: completedUtc"
    "report future completion timestamp metadata blocker")
require_output(
    future_metadata_report_combined
    "`plan\\.acceptance\\.finalAcceptanceBlocker`: targetPlatform, targetDisplay, multiDisplay, acceptanceChecklist, acceptanceMetadata"
    "require non-future metadata before final acceptance")

execute_process(
    COMMAND
        "${CMAKE_COMMAND}" -E env
        "QT_QPA_PLATFORM=offscreen"
        "LIZZIE_KATAGO_EXECUTABLE=${APP_EXECUTABLE}"
        "LIZZIE_KATAGO_MODEL=${model_path}"
        "LIZZIE_KATAGO_ANALYSIS_CONFIG=${analysis_config_path}"
        "LIZZIE_KATAGO_GTP_CONFIG=${gtp_config_path}"
        "LIZZIE_DIAGNOSTICS_SETTINGS_FILE=${settings_file}"
        "LIZZIE_TARGET_ACCEPTANCE_RECORD_FILE=${acceptance_record_missing_evidence_file}"
        "${APP_EXECUTABLE}" --target-acceptance-report
    WORKING_DIRECTORY "${work_dir}"
    RESULT_VARIABLE missing_evidence_report_result
    OUTPUT_VARIABLE missing_evidence_report_output
    ERROR_VARIABLE missing_evidence_report_error
)
set(missing_evidence_report_combined "${missing_evidence_report_output}${missing_evidence_report_error}")
if(NOT missing_evidence_report_result EQUAL 0)
    message(FATAL_ERROR "Target acceptance missing-evidence report command failed: ${missing_evidence_report_combined}")
endif()

require_output(
    missing_evidence_report_combined
    "`plan\\.acceptance\\.evidence\\.ready`: false"
    "report incomplete evidence readiness for pass records without evidence")
require_output(
    missing_evidence_report_combined
    "`plan\\.acceptance\\.recordMetadata\\.ready`: true"
    "keep complete metadata readiness when evidence is missing")
require_output(
    missing_evidence_report_combined
    "`plan\\.acceptance\\.evidence\\.blocker`: .*diagnosticsEvidence"
    "report missing diagnostics evidence blocker")
require_output(
    missing_evidence_report_combined
    "`plan\\.acceptance\\.finalAcceptanceBlocker`: targetPlatform, targetDisplay, multiDisplay, acceptanceChecklist, acceptanceEvidence, acceptanceEvidenceSha256"
    "require evidence and pinned evidence SHA-256 before final acceptance")

execute_process(
    COMMAND
        "${CMAKE_COMMAND}" -E env
        "QT_QPA_PLATFORM=offscreen"
        "LIZZIE_KATAGO_EXECUTABLE=${APP_EXECUTABLE}"
        "LIZZIE_KATAGO_MODEL=${model_path}"
        "LIZZIE_KATAGO_ANALYSIS_CONFIG=${analysis_config_path}"
        "LIZZIE_KATAGO_GTP_CONFIG=${gtp_config_path}"
        "LIZZIE_DIAGNOSTICS_SETTINGS_FILE=${settings_file}"
        "LIZZIE_TARGET_ACCEPTANCE_RECORD_FILE=${acceptance_record_empty_evidence_file}"
        "${APP_EXECUTABLE}" --target-acceptance-report
    WORKING_DIRECTORY "${work_dir}"
    RESULT_VARIABLE empty_evidence_report_result
    OUTPUT_VARIABLE empty_evidence_report_output
    ERROR_VARIABLE empty_evidence_report_error
)
set(empty_evidence_report_combined "${empty_evidence_report_output}${empty_evidence_report_error}")
if(NOT empty_evidence_report_result EQUAL 0)
    message(FATAL_ERROR "Target acceptance empty-evidence report command failed: ${empty_evidence_report_combined}")
endif()

require_output(
    empty_evidence_report_combined
    "`plan\\.acceptance\\.evidence\\.diagnostics\\.exists`: true"
    "report empty diagnostics evidence file exists")
require_output(
    empty_evidence_report_combined
    "`plan\\.acceptance\\.evidence\\.diagnostics\\.size`: 0"
    "report empty diagnostics evidence file size")
require_output(
    empty_evidence_report_combined
    "`plan\\.acceptance\\.evidence\\.diagnostics\\.sha256`: \\(unavailable\\)"
    "avoid hashing empty diagnostics evidence file")
require_output(
    empty_evidence_report_combined
    "`plan\\.acceptance\\.evidence\\.ready`: false"
    "reject empty diagnostics evidence file")
require_output(
    empty_evidence_report_combined
    "`plan\\.acceptance\\.evidence\\.blocker`: diagnosticsEvidence"
    "report empty diagnostics evidence blocker")
require_output(
    empty_evidence_report_combined
    "`plan\\.acceptance\\.finalAcceptanceBlocker`: targetPlatform, targetDisplay, multiDisplay, acceptanceChecklist, acceptanceEvidence, acceptanceEvidenceSha256"
    "require non-empty evidence and pinned evidence SHA-256 before final acceptance")

execute_process(
    COMMAND
        "${CMAKE_COMMAND}" -E env
        "QT_QPA_PLATFORM=offscreen"
        "LIZZIE_KATAGO_EXECUTABLE=${APP_EXECUTABLE}"
        "LIZZIE_KATAGO_MODEL=${model_path}"
        "LIZZIE_KATAGO_ANALYSIS_CONFIG=${analysis_config_path}"
        "LIZZIE_KATAGO_GTP_CONFIG=${gtp_config_path}"
        "LIZZIE_DIAGNOSTICS_SETTINGS_FILE=${settings_file}"
        "LIZZIE_TARGET_ACCEPTANCE_RECORD_FILE=${acceptance_record_duplicate_evidence_file}"
        "${APP_EXECUTABLE}" --target-acceptance-report
    WORKING_DIRECTORY "${work_dir}"
    RESULT_VARIABLE duplicate_evidence_report_result
    OUTPUT_VARIABLE duplicate_evidence_report_output
    ERROR_VARIABLE duplicate_evidence_report_error
)
set(duplicate_evidence_report_combined "${duplicate_evidence_report_output}${duplicate_evidence_report_error}")
if(NOT duplicate_evidence_report_result EQUAL 0)
    message(FATAL_ERROR "Target acceptance duplicate-evidence report command failed: ${duplicate_evidence_report_combined}")
endif()

require_output(
    duplicate_evidence_report_combined
    "`plan\\.acceptance\\.evidence\\.rawKataGoComparisonLog\\.value`: ${record_engine_log_path}"
    "report duplicate raw KataGo evidence path")
require_output(
    duplicate_evidence_report_combined
    "`plan\\.acceptance\\.evidence\\.ready`: false"
    "reject duplicate evidence paths")
require_output(
    duplicate_evidence_report_combined
    "`plan\\.acceptance\\.evidence\\.blocker`: duplicateEvidencePath\\.rawKataGoComparisonLogEvidence"
    "report duplicate raw KataGo evidence path blocker")
require_output(
    duplicate_evidence_report_combined
    "`plan\\.acceptance\\.evidenceSha256\\.ready`: true"
    "keep evidence SHA-256 readiness when duplicate evidence hashes match")
require_output(
    duplicate_evidence_report_combined
    "`plan\\.acceptance\\.finalAcceptanceBlocker`: targetPlatform, targetDisplay, multiDisplay, acceptanceChecklist, acceptanceEvidence"
    "require distinct evidence paths before final acceptance")

execute_process(
    COMMAND
        "${CMAKE_COMMAND}" -E env
        "QT_QPA_PLATFORM=offscreen"
        "LIZZIE_KATAGO_EXECUTABLE=${APP_EXECUTABLE}"
        "LIZZIE_KATAGO_MODEL=${model_path}"
        "LIZZIE_KATAGO_ANALYSIS_CONFIG=${analysis_config_path}"
        "LIZZIE_KATAGO_GTP_CONFIG=${gtp_config_path}"
        "LIZZIE_DIAGNOSTICS_SETTINGS_FILE=${settings_file}"
        "LIZZIE_TARGET_ACCEPTANCE_RECORD_FILE=${acceptance_record_uniform_screenshot_file}"
        "${APP_EXECUTABLE}" --target-acceptance-report
    WORKING_DIRECTORY "${work_dir}"
    RESULT_VARIABLE uniform_screenshot_report_result
    OUTPUT_VARIABLE uniform_screenshot_report_output
    ERROR_VARIABLE uniform_screenshot_report_error
)
set(uniform_screenshot_report_combined "${uniform_screenshot_report_output}${uniform_screenshot_report_error}")
if(NOT uniform_screenshot_report_result EQUAL 0)
    message(FATAL_ERROR "Target acceptance uniform-screenshot report command failed: ${uniform_screenshot_report_combined}")
endif()

require_output(
    uniform_screenshot_report_combined
    "`plan\\.acceptance\\.evidence\\.screenshot\\.sha256Status`: match"
    "keep uniform screenshot evidence SHA-256 match")
require_output(
    uniform_screenshot_report_combined
    "`plan\\.acceptance\\.evidence\\.screenshot\\.imageReadable`: true"
    "report uniform screenshot evidence readable")
require_output(
    uniform_screenshot_report_combined
    "`plan\\.acceptance\\.evidence\\.screenshot\\.imageHasPixelVariation`: false"
    "reject uniform screenshot pixel variation")
require_output(
    uniform_screenshot_report_combined
    "`plan\\.acceptance\\.evidence\\.screenshot\\.readyFor4KAcceptance`: false"
    "reject uniform screenshot 4K acceptance readiness")
require_output(
    uniform_screenshot_report_combined
    "`plan\\.acceptance\\.finalAcceptanceBlocker`: targetPlatform, targetDisplay, multiDisplay, acceptanceChecklist, screenshotEvidence4K"
    "keep weak screenshot evidence blocker for uniform screenshots")

execute_process(
    COMMAND
        "${CMAKE_COMMAND}" -E env
        "QT_QPA_PLATFORM=offscreen"
        "LIZZIE_KATAGO_EXECUTABLE=${APP_EXECUTABLE}"
        "LIZZIE_KATAGO_MODEL=${model_path}"
        "LIZZIE_KATAGO_ANALYSIS_CONFIG=${analysis_config_path}"
        "LIZZIE_KATAGO_GTP_CONFIG=${gtp_config_path}"
        "LIZZIE_DIAGNOSTICS_SETTINGS_FILE=${settings_file}"
        "LIZZIE_TARGET_ACCEPTANCE_RECORD_FILE=${acceptance_record_bad_evidence_content_file}"
        "${APP_EXECUTABLE}" --target-acceptance-report
    WORKING_DIRECTORY "${work_dir}"
    RESULT_VARIABLE bad_evidence_content_report_result
    OUTPUT_VARIABLE bad_evidence_content_report_output
    ERROR_VARIABLE bad_evidence_content_report_error
)
set(
    bad_evidence_content_report_combined
    "${bad_evidence_content_report_output}${bad_evidence_content_report_error}")
if(NOT bad_evidence_content_report_result EQUAL 0)
    message(
        FATAL_ERROR
        "Target acceptance bad-evidence-content report command failed: ${bad_evidence_content_report_combined}")
endif()

require_output(
    bad_evidence_content_report_combined
    "`plan\\.acceptance\\.evidence\\.diagnostics\\.sha256Status`: match"
    "keep diagnostics evidence SHA-256 match for bad content")
require_output(
    bad_evidence_content_report_combined
    "`plan\\.acceptance\\.evidence\\.diagnostics\\.contentStatus`: missing-marker"
    "report diagnostics evidence missing marker")
require_output(
    bad_evidence_content_report_combined
    "`plan\\.acceptance\\.evidence\\.ready`: false"
    "reject diagnostics evidence with missing marker")
require_output(
    bad_evidence_content_report_combined
    "`plan\\.acceptance\\.evidence\\.blocker`: diagnosticsEvidenceContent"
    "report diagnostics evidence content blocker")
require_output(
    bad_evidence_content_report_combined
    "`plan\\.acceptance\\.finalAcceptanceBlocker`: targetPlatform, targetDisplay, multiDisplay, acceptanceChecklist, acceptanceEvidence"
    "require diagnostics evidence marker before final acceptance")

execute_process(
    COMMAND
        "${CMAKE_COMMAND}" -E env
        "QT_QPA_PLATFORM=offscreen"
        "LIZZIE_KATAGO_EXECUTABLE=${APP_EXECUTABLE}"
        "LIZZIE_KATAGO_MODEL=${model_path}"
        "LIZZIE_KATAGO_ANALYSIS_CONFIG=${analysis_config_path}"
        "LIZZIE_KATAGO_GTP_CONFIG=${gtp_config_path}"
        "LIZZIE_DIAGNOSTICS_SETTINGS_FILE=${settings_file}"
        "LIZZIE_TARGET_ACCEPTANCE_RECORD_FILE=${acceptance_record_missing_diagnostics_record_sha_file}"
        "${APP_EXECUTABLE}" --target-acceptance-report
    WORKING_DIRECTORY "${work_dir}"
    RESULT_VARIABLE missing_diagnostics_record_sha_report_result
    OUTPUT_VARIABLE missing_diagnostics_record_sha_report_output
    ERROR_VARIABLE missing_diagnostics_record_sha_report_error
)
set(
    missing_diagnostics_record_sha_report_combined
    "${missing_diagnostics_record_sha_report_output}${missing_diagnostics_record_sha_report_error}")
if(NOT missing_diagnostics_record_sha_report_result EQUAL 0)
    message(
        FATAL_ERROR
        "Target acceptance missing-diagnostics-record-sha command failed: ${missing_diagnostics_record_sha_report_combined}")
endif()

require_output(
    missing_diagnostics_record_sha_report_combined
    "`plan\\.acceptance\\.evidence\\.diagnostics\\.sha256Status`: match"
    "keep diagnostics evidence SHA-256 match when record SHA marker is missing")
require_output(
    missing_diagnostics_record_sha_report_combined
    "`plan\\.acceptance\\.evidence\\.diagnostics\\.contentStatus`: missing-marker"
    "reject diagnostics evidence without record SHA marker")
require_output(
    missing_diagnostics_record_sha_report_combined
    "`plan\\.acceptance\\.evidence\\.diagnostics\\.missingContentMarker`: plan.acceptance.recordFile.sha256"
    "report missing diagnostics record SHA marker")
require_output(
    missing_diagnostics_record_sha_report_combined
    "`plan\\.acceptance\\.evidence\\.ready`: false"
    "reject diagnostics evidence with missing record SHA marker")
require_output(
    missing_diagnostics_record_sha_report_combined
    "`plan\\.acceptance\\.evidence\\.blocker`: diagnosticsEvidenceContent"
    "report diagnostics record-SHA content blocker")
require_output(
    missing_diagnostics_record_sha_report_combined
    "`plan\\.acceptance\\.finalAcceptanceBlocker`: targetPlatform, targetDisplay, multiDisplay, acceptanceChecklist, acceptanceEvidence"
    "require diagnostics record SHA marker before final acceptance")

execute_process(
    COMMAND
        "${CMAKE_COMMAND}" -E env
        "QT_QPA_PLATFORM=offscreen"
        "LIZZIE_KATAGO_EXECUTABLE=${APP_EXECUTABLE}"
        "LIZZIE_KATAGO_MODEL=${model_path}"
        "LIZZIE_KATAGO_ANALYSIS_CONFIG=${analysis_config_path}"
        "LIZZIE_KATAGO_GTP_CONFIG=${gtp_config_path}"
        "LIZZIE_DIAGNOSTICS_SETTINGS_FILE=${settings_file}"
        "LIZZIE_TARGET_ACCEPTANCE_RECORD_FILE=${acceptance_record_missing_diagnostics_final_blockers_file}"
        "${APP_EXECUTABLE}" --target-acceptance-report
    WORKING_DIRECTORY "${work_dir}"
    RESULT_VARIABLE missing_diagnostics_final_blockers_report_result
    OUTPUT_VARIABLE missing_diagnostics_final_blockers_report_output
    ERROR_VARIABLE missing_diagnostics_final_blockers_report_error
)
set(
    missing_diagnostics_final_blockers_report_combined
    "${missing_diagnostics_final_blockers_report_output}${missing_diagnostics_final_blockers_report_error}")
if(NOT missing_diagnostics_final_blockers_report_result EQUAL 0)
    message(
        FATAL_ERROR
        "Target acceptance missing-diagnostics-final-blockers command failed: ${missing_diagnostics_final_blockers_report_combined}")
endif()

require_output(
    missing_diagnostics_final_blockers_report_combined
    "`plan\\.acceptance\\.evidence\\.diagnostics\\.sha256Status`: match"
    "keep diagnostics evidence SHA-256 match when final acceptance markers are missing")
require_output(
    missing_diagnostics_final_blockers_report_combined
    "`plan\\.acceptance\\.evidence\\.diagnostics\\.contentStatus`: missing-marker"
    "reject diagnostics evidence without final acceptance markers")
require_output(
    missing_diagnostics_final_blockers_report_combined
    "`plan\\.acceptance\\.evidence\\.diagnostics\\.missingContentMarker`: plan.acceptance.finalAcceptanceStatus, plan.acceptance.finalAcceptanceBlocker, plan.acceptance.finalAcceptanceOutstandingBlocker"
    "report missing diagnostics final acceptance markers")
require_output(
    missing_diagnostics_final_blockers_report_combined
    "`plan\\.acceptance\\.evidence\\.ready`: false"
    "reject diagnostics evidence with missing final acceptance markers")
require_output(
    missing_diagnostics_final_blockers_report_combined
    "`plan\\.acceptance\\.evidence\\.blocker`: diagnosticsEvidenceContent"
    "report diagnostics final-acceptance content blocker")
require_output(
    missing_diagnostics_final_blockers_report_combined
    "`plan\\.acceptance\\.finalAcceptanceBlocker`: targetPlatform, targetDisplay, multiDisplay, acceptanceChecklist, acceptanceEvidence"
    "require diagnostics final acceptance markers before final acceptance")

execute_process(
    COMMAND
        "${CMAKE_COMMAND}" -E env
        "QT_QPA_PLATFORM=offscreen"
        "LIZZIE_KATAGO_EXECUTABLE=${APP_EXECUTABLE}"
        "LIZZIE_KATAGO_MODEL=${model_path}"
        "LIZZIE_KATAGO_ANALYSIS_CONFIG=${analysis_config_path}"
        "LIZZIE_KATAGO_GTP_CONFIG=${gtp_config_path}"
        "LIZZIE_DIAGNOSTICS_SETTINGS_FILE=${settings_file}"
        "LIZZIE_TARGET_ACCEPTANCE_RECORD_FILE=${acceptance_record_mismatched_diagnostics_record_path_file}"
        "${APP_EXECUTABLE}" --target-acceptance-report
    WORKING_DIRECTORY "${work_dir}"
    RESULT_VARIABLE mismatched_diagnostics_record_path_report_result
    OUTPUT_VARIABLE mismatched_diagnostics_record_path_report_output
    ERROR_VARIABLE mismatched_diagnostics_record_path_report_error
)
set(
    mismatched_diagnostics_record_path_report_combined
    "${mismatched_diagnostics_record_path_report_output}${mismatched_diagnostics_record_path_report_error}")
if(NOT mismatched_diagnostics_record_path_report_result EQUAL 0)
    message(
        FATAL_ERROR
        "Target acceptance mismatched-diagnostics-record-path command failed: ${mismatched_diagnostics_record_path_report_combined}")
endif()

require_output(
    mismatched_diagnostics_record_path_report_combined
    "`plan\\.acceptance\\.evidence\\.diagnostics\\.sha256Status`: match"
    "keep diagnostics evidence SHA-256 match for mismatched record path")
require_output(
    mismatched_diagnostics_record_path_report_combined
    "`plan\\.acceptance\\.evidence\\.diagnostics\\.contentStatus`: missing-marker"
    "reject diagnostics evidence for a different record path")
require_output(
    mismatched_diagnostics_record_path_report_combined
    "`plan\\.acceptance\\.evidence\\.ready`: false"
    "reject diagnostics evidence with mismatched record path")
require_output(
    mismatched_diagnostics_record_path_report_combined
    "`plan\\.acceptance\\.evidence\\.blocker`: diagnosticsEvidenceContent"
    "report diagnostics evidence record-path content blocker")
require_output(
    mismatched_diagnostics_record_path_report_combined
    "`plan\\.acceptance\\.finalAcceptanceBlocker`: targetPlatform, targetDisplay, multiDisplay, acceptanceChecklist, acceptanceEvidence"
    "require diagnostics evidence to reference the same record path before final acceptance")

execute_process(
    COMMAND
        "${CMAKE_COMMAND}" -E env
        "QT_QPA_PLATFORM=offscreen"
        "LIZZIE_KATAGO_EXECUTABLE=${APP_EXECUTABLE}"
        "LIZZIE_KATAGO_MODEL=${model_path}"
        "LIZZIE_KATAGO_ANALYSIS_CONFIG=${analysis_config_path}"
        "LIZZIE_KATAGO_GTP_CONFIG=${gtp_config_path}"
        "LIZZIE_DIAGNOSTICS_SETTINGS_FILE=${settings_file}"
        "LIZZIE_TARGET_ACCEPTANCE_RECORD_FILE=${acceptance_record_mismatched_diagnostics_app_file}"
        "${APP_EXECUTABLE}" --target-acceptance-report
    WORKING_DIRECTORY "${work_dir}"
    RESULT_VARIABLE mismatched_diagnostics_app_report_result
    OUTPUT_VARIABLE mismatched_diagnostics_app_report_output
    ERROR_VARIABLE mismatched_diagnostics_app_report_error
)
set(
    mismatched_diagnostics_app_report_combined
    "${mismatched_diagnostics_app_report_output}${mismatched_diagnostics_app_report_error}")
if(NOT mismatched_diagnostics_app_report_result EQUAL 0)
    message(
        FATAL_ERROR
        "Target acceptance mismatched-diagnostics-app command failed: ${mismatched_diagnostics_app_report_combined}")
endif()

require_output(
    mismatched_diagnostics_app_report_combined
    "`plan\\.acceptance\\.evidence\\.diagnostics\\.sha256Status`: match"
    "keep diagnostics evidence SHA-256 match for mismatched app binding")
require_output(
    mismatched_diagnostics_app_report_combined
    "`plan\\.acceptance\\.evidence\\.diagnostics\\.contentStatus`: missing-marker"
    "reject diagnostics evidence without the recorded app version")
require_output(
    mismatched_diagnostics_app_report_combined
    "`plan\\.acceptance\\.evidence\\.ready`: false"
    "reject diagnostics evidence with mismatched app binding")
require_output(
    mismatched_diagnostics_app_report_combined
    "`plan\\.acceptance\\.evidence\\.blocker`: diagnosticsEvidenceContent"
    "report diagnostics evidence app-binding content blocker")
require_output(
    mismatched_diagnostics_app_report_combined
    "`plan\\.acceptance\\.finalAcceptanceBlocker`: targetPlatform, targetDisplay, multiDisplay, acceptanceChecklist, acceptanceEvidence"
    "require diagnostics evidence to reference the recorded app version before final acceptance")

execute_process(
    COMMAND
        "${CMAKE_COMMAND}" -E env
        "QT_QPA_PLATFORM=offscreen"
        "LIZZIE_KATAGO_EXECUTABLE=${APP_EXECUTABLE}"
        "LIZZIE_KATAGO_MODEL=${model_path}"
        "LIZZIE_KATAGO_ANALYSIS_CONFIG=${analysis_config_path}"
        "LIZZIE_KATAGO_GTP_CONFIG=${gtp_config_path}"
        "LIZZIE_DIAGNOSTICS_SETTINGS_FILE=${settings_file}"
        "LIZZIE_TARGET_ACCEPTANCE_RECORD_FILE=${acceptance_record_mismatched_diagnostics_executable_file}"
        "${APP_EXECUTABLE}" --target-acceptance-report
    WORKING_DIRECTORY "${work_dir}"
    RESULT_VARIABLE mismatched_diagnostics_executable_report_result
    OUTPUT_VARIABLE mismatched_diagnostics_executable_report_output
    ERROR_VARIABLE mismatched_diagnostics_executable_report_error
)
set(
    mismatched_diagnostics_executable_report_combined
    "${mismatched_diagnostics_executable_report_output}${mismatched_diagnostics_executable_report_error}")
if(NOT mismatched_diagnostics_executable_report_result EQUAL 0)
    message(
        FATAL_ERROR
        "Target acceptance mismatched-diagnostics-executable command failed: ${mismatched_diagnostics_executable_report_combined}")
endif()

require_output(
    mismatched_diagnostics_executable_report_combined
    "`plan\\.acceptance\\.evidence\\.diagnostics\\.sha256Status`: match"
    "keep diagnostics evidence SHA-256 match for mismatched executable binding")
require_output(
    mismatched_diagnostics_executable_report_combined
    "`plan\\.acceptance\\.evidence\\.diagnostics\\.contentStatus`: missing-marker"
    "reject diagnostics evidence without the current executable path")
require_output(
    mismatched_diagnostics_executable_report_combined
    "`plan\\.acceptance\\.evidence\\.diagnostics\\.missingContentMarker`: ${APP_EXECUTABLE}"
    "report missing diagnostics executable path marker")
require_output(
    mismatched_diagnostics_executable_report_combined
    "`plan\\.acceptance\\.evidence\\.ready`: false"
    "reject diagnostics evidence with mismatched executable binding")
require_output(
    mismatched_diagnostics_executable_report_combined
    "`plan\\.acceptance\\.evidence\\.blocker`: diagnosticsEvidenceContent"
    "report diagnostics evidence executable-binding content blocker")
require_output(
    mismatched_diagnostics_executable_report_combined
    "`plan\\.acceptance\\.finalAcceptanceBlocker`: targetPlatform, targetDisplay, multiDisplay, acceptanceChecklist, acceptanceEvidence"
    "require diagnostics evidence to reference the current executable before final acceptance")

execute_process(
    COMMAND
        "${CMAKE_COMMAND}" -E env
        "QT_QPA_PLATFORM=offscreen"
        "LIZZIE_KATAGO_EXECUTABLE=${APP_EXECUTABLE}"
        "LIZZIE_KATAGO_MODEL=${model_path}"
        "LIZZIE_KATAGO_ANALYSIS_CONFIG=${analysis_config_path}"
        "LIZZIE_KATAGO_GTP_CONFIG=${gtp_config_path}"
        "LIZZIE_DIAGNOSTICS_SETTINGS_FILE=${settings_file}"
        "LIZZIE_TARGET_ACCEPTANCE_RECORD_FILE=${acceptance_record_bad_report_content_file}"
        "${APP_EXECUTABLE}" --target-acceptance-report
    WORKING_DIRECTORY "${work_dir}"
    RESULT_VARIABLE bad_report_content_report_result
    OUTPUT_VARIABLE bad_report_content_report_output
    ERROR_VARIABLE bad_report_content_report_error
)
set(
    bad_report_content_report_combined
    "${bad_report_content_report_output}${bad_report_content_report_error}")
if(NOT bad_report_content_report_result EQUAL 0)
    message(
        FATAL_ERROR
        "Target acceptance bad-report-content report command failed: ${bad_report_content_report_combined}")
endif()

require_output(
    bad_report_content_report_combined
    "`plan\\.acceptance\\.evidence\\.targetAcceptanceReport\\.sha256Status`: match"
    "keep target acceptance report evidence SHA-256 match for bad content")
require_output(
    bad_report_content_report_combined
    "`plan\\.acceptance\\.evidence\\.targetAcceptanceReport\\.contentStatus`: missing-marker"
    "report target acceptance report evidence missing marker")
require_output(
    bad_report_content_report_combined
    "`plan\\.acceptance\\.evidence\\.ready`: false"
    "reject target acceptance report evidence with missing marker")
require_output(
    bad_report_content_report_combined
    "`plan\\.acceptance\\.evidence\\.blocker`: targetAcceptanceReportEvidenceContent"
    "report target acceptance report evidence content blocker")
require_output(
    bad_report_content_report_combined
    "`plan\\.acceptance\\.finalAcceptanceBlocker`: targetPlatform, targetDisplay, multiDisplay, acceptanceChecklist, acceptanceEvidence"
    "require target acceptance report evidence marker before final acceptance")

execute_process(
    COMMAND
        "${CMAKE_COMMAND}" -E env
        "QT_QPA_PLATFORM=offscreen"
        "LIZZIE_KATAGO_EXECUTABLE=${APP_EXECUTABLE}"
        "LIZZIE_KATAGO_MODEL=${model_path}"
        "LIZZIE_KATAGO_ANALYSIS_CONFIG=${analysis_config_path}"
        "LIZZIE_KATAGO_GTP_CONFIG=${gtp_config_path}"
        "LIZZIE_DIAGNOSTICS_SETTINGS_FILE=${settings_file}"
        "LIZZIE_TARGET_ACCEPTANCE_RECORD_FILE=${acceptance_record_missing_report_record_sha_file}"
        "${APP_EXECUTABLE}" --target-acceptance-report
    WORKING_DIRECTORY "${work_dir}"
    RESULT_VARIABLE missing_report_record_sha_report_result
    OUTPUT_VARIABLE missing_report_record_sha_report_output
    ERROR_VARIABLE missing_report_record_sha_report_error
)
set(
    missing_report_record_sha_report_combined
    "${missing_report_record_sha_report_output}${missing_report_record_sha_report_error}")
if(NOT missing_report_record_sha_report_result EQUAL 0)
    message(
        FATAL_ERROR
        "Target acceptance missing-report-record-sha command failed: ${missing_report_record_sha_report_combined}")
endif()

require_output(
    missing_report_record_sha_report_combined
    "`plan\\.acceptance\\.evidence\\.targetAcceptanceReport\\.sha256Status`: match"
    "keep target acceptance report evidence SHA-256 match when record SHA marker is missing")
require_output(
    missing_report_record_sha_report_combined
    "`plan\\.acceptance\\.evidence\\.targetAcceptanceReport\\.contentStatus`: missing-marker"
    "reject target acceptance report evidence without record SHA marker")
require_output(
    missing_report_record_sha_report_combined
    "`plan\\.acceptance\\.evidence\\.targetAcceptanceReport\\.missingContentMarker`: plan.acceptance.recordFile.sha256"
    "report missing target acceptance report record SHA marker")
require_output(
    missing_report_record_sha_report_combined
    "`plan\\.acceptance\\.evidence\\.ready`: false"
    "reject target acceptance report evidence with missing record SHA marker")
require_output(
    missing_report_record_sha_report_combined
    "`plan\\.acceptance\\.evidence\\.blocker`: targetAcceptanceReportEvidenceContent"
    "report target acceptance report record-SHA content blocker")
require_output(
    missing_report_record_sha_report_combined
    "`plan\\.acceptance\\.finalAcceptanceBlocker`: targetPlatform, targetDisplay, multiDisplay, acceptanceChecklist, acceptanceEvidence"
    "require target acceptance report record SHA marker before final acceptance")

execute_process(
    COMMAND
        "${CMAKE_COMMAND}" -E env
        "QT_QPA_PLATFORM=offscreen"
        "LIZZIE_KATAGO_EXECUTABLE=${APP_EXECUTABLE}"
        "LIZZIE_KATAGO_MODEL=${model_path}"
        "LIZZIE_KATAGO_ANALYSIS_CONFIG=${analysis_config_path}"
        "LIZZIE_KATAGO_GTP_CONFIG=${gtp_config_path}"
        "LIZZIE_DIAGNOSTICS_SETTINGS_FILE=${settings_file}"
        "LIZZIE_TARGET_ACCEPTANCE_RECORD_FILE=${acceptance_record_missing_report_final_blockers_file}"
        "${APP_EXECUTABLE}" --target-acceptance-report
    WORKING_DIRECTORY "${work_dir}"
    RESULT_VARIABLE missing_report_final_blockers_report_result
    OUTPUT_VARIABLE missing_report_final_blockers_report_output
    ERROR_VARIABLE missing_report_final_blockers_report_error
)
set(
    missing_report_final_blockers_report_combined
    "${missing_report_final_blockers_report_output}${missing_report_final_blockers_report_error}")
if(NOT missing_report_final_blockers_report_result EQUAL 0)
    message(
        FATAL_ERROR
        "Target acceptance missing-report-final-blockers command failed: ${missing_report_final_blockers_report_combined}")
endif()

require_output(
    missing_report_final_blockers_report_combined
    "`plan\\.acceptance\\.evidence\\.targetAcceptanceReport\\.sha256Status`: match"
    "keep target acceptance report evidence SHA-256 match when final blocker markers are missing")
require_output(
    missing_report_final_blockers_report_combined
    "`plan\\.acceptance\\.evidence\\.targetAcceptanceReport\\.contentStatus`: missing-marker"
    "reject target acceptance report evidence without final blocker markers")
require_output(
    missing_report_final_blockers_report_combined
    "`plan\\.acceptance\\.evidence\\.targetAcceptanceReport\\.missingContentMarker`: plan.acceptance.finalAcceptanceBlocker, plan.acceptance.finalAcceptanceOutstandingBlocker"
    "report missing target acceptance report final blocker markers")
require_output(
    missing_report_final_blockers_report_combined
    "`plan\\.acceptance\\.evidence\\.ready`: false"
    "reject target acceptance report evidence with missing final blocker markers")
require_output(
    missing_report_final_blockers_report_combined
    "`plan\\.acceptance\\.evidence\\.blocker`: targetAcceptanceReportEvidenceContent"
    "report target acceptance report final-blocker content blocker")
require_output(
    missing_report_final_blockers_report_combined
    "`plan\\.acceptance\\.finalAcceptanceBlocker`: targetPlatform, targetDisplay, multiDisplay, acceptanceChecklist, acceptanceEvidence"
    "require target acceptance report final blocker markers before final acceptance")

execute_process(
    COMMAND
        "${CMAKE_COMMAND}" -E env
        "QT_QPA_PLATFORM=offscreen"
        "LIZZIE_KATAGO_EXECUTABLE=${APP_EXECUTABLE}"
        "LIZZIE_KATAGO_MODEL=${model_path}"
        "LIZZIE_KATAGO_ANALYSIS_CONFIG=${analysis_config_path}"
        "LIZZIE_KATAGO_GTP_CONFIG=${gtp_config_path}"
        "LIZZIE_DIAGNOSTICS_SETTINGS_FILE=${settings_file}"
        "LIZZIE_TARGET_ACCEPTANCE_RECORD_FILE=${acceptance_record_mismatched_report_app_file}"
        "${APP_EXECUTABLE}" --target-acceptance-report
    WORKING_DIRECTORY "${work_dir}"
    RESULT_VARIABLE mismatched_report_app_report_result
    OUTPUT_VARIABLE mismatched_report_app_report_output
    ERROR_VARIABLE mismatched_report_app_report_error
)
set(
    mismatched_report_app_report_combined
    "${mismatched_report_app_report_output}${mismatched_report_app_report_error}")
if(NOT mismatched_report_app_report_result EQUAL 0)
    message(
        FATAL_ERROR
        "Target acceptance mismatched-report-app command failed: ${mismatched_report_app_report_combined}")
endif()

require_output(
    mismatched_report_app_report_combined
    "`plan\\.acceptance\\.evidence\\.targetAcceptanceReport\\.sha256Status`: match"
    "keep target acceptance report evidence SHA-256 match for mismatched app binding")
require_output(
    mismatched_report_app_report_combined
    "`plan\\.acceptance\\.evidence\\.targetAcceptanceReport\\.contentStatus`: missing-marker"
    "reject target acceptance report evidence without the recorded app version")
require_output(
    mismatched_report_app_report_combined
    "`plan\\.acceptance\\.evidence\\.ready`: false"
    "reject target acceptance report evidence with mismatched app binding")
require_output(
    mismatched_report_app_report_combined
    "`plan\\.acceptance\\.evidence\\.blocker`: targetAcceptanceReportEvidenceContent"
    "report target acceptance report evidence app-binding content blocker")
require_output(
    mismatched_report_app_report_combined
    "`plan\\.acceptance\\.finalAcceptanceBlocker`: targetPlatform, targetDisplay, multiDisplay, acceptanceChecklist, acceptanceEvidence"
    "require target acceptance report evidence to reference the recorded app version before final acceptance")

execute_process(
    COMMAND
        "${CMAKE_COMMAND}" -E env
        "QT_QPA_PLATFORM=offscreen"
        "LIZZIE_KATAGO_EXECUTABLE=${APP_EXECUTABLE}"
        "LIZZIE_KATAGO_MODEL=${model_path}"
        "LIZZIE_KATAGO_ANALYSIS_CONFIG=${analysis_config_path}"
        "LIZZIE_KATAGO_GTP_CONFIG=${gtp_config_path}"
        "LIZZIE_DIAGNOSTICS_SETTINGS_FILE=${settings_file}"
        "LIZZIE_TARGET_ACCEPTANCE_RECORD_FILE=${acceptance_record_mismatched_report_executable_file}"
        "${APP_EXECUTABLE}" --target-acceptance-report
    WORKING_DIRECTORY "${work_dir}"
    RESULT_VARIABLE mismatched_report_executable_report_result
    OUTPUT_VARIABLE mismatched_report_executable_report_output
    ERROR_VARIABLE mismatched_report_executable_report_error
)
set(
    mismatched_report_executable_report_combined
    "${mismatched_report_executable_report_output}${mismatched_report_executable_report_error}")
if(NOT mismatched_report_executable_report_result EQUAL 0)
    message(
        FATAL_ERROR
        "Target acceptance mismatched-report-executable command failed: ${mismatched_report_executable_report_combined}")
endif()

require_output(
    mismatched_report_executable_report_combined
    "`plan\\.acceptance\\.evidence\\.targetAcceptanceReport\\.sha256Status`: match"
    "keep target acceptance report evidence SHA-256 match for mismatched executable binding")
require_output(
    mismatched_report_executable_report_combined
    "`plan\\.acceptance\\.evidence\\.targetAcceptanceReport\\.contentStatus`: missing-marker"
    "reject target acceptance report evidence without the current executable path")
require_output(
    mismatched_report_executable_report_combined
    "`plan\\.acceptance\\.evidence\\.targetAcceptanceReport\\.missingContentMarker`: ${APP_EXECUTABLE}"
    "report missing target acceptance report executable path marker")
require_output(
    mismatched_report_executable_report_combined
    "`plan\\.acceptance\\.evidence\\.ready`: false"
    "reject target acceptance report evidence with mismatched executable binding")
require_output(
    mismatched_report_executable_report_combined
    "`plan\\.acceptance\\.evidence\\.blocker`: targetAcceptanceReportEvidenceContent"
    "report target acceptance report executable-binding content blocker")
require_output(
    mismatched_report_executable_report_combined
    "`plan\\.acceptance\\.finalAcceptanceBlocker`: targetPlatform, targetDisplay, multiDisplay, acceptanceChecklist, acceptanceEvidence"
    "require target acceptance report evidence to reference the current executable before final acceptance")

execute_process(
    COMMAND
        "${CMAKE_COMMAND}" -E env
        "QT_QPA_PLATFORM=offscreen"
        "LIZZIE_KATAGO_EXECUTABLE=${APP_EXECUTABLE}"
        "LIZZIE_KATAGO_MODEL=${model_path}"
        "LIZZIE_KATAGO_ANALYSIS_CONFIG=${analysis_config_path}"
        "LIZZIE_KATAGO_GTP_CONFIG=${gtp_config_path}"
        "LIZZIE_DIAGNOSTICS_SETTINGS_FILE=${settings_file}"
        "LIZZIE_TARGET_ACCEPTANCE_RECORD_FILE=${acceptance_record_mismatched_report_record_path_file}"
        "${APP_EXECUTABLE}" --target-acceptance-report
    WORKING_DIRECTORY "${work_dir}"
    RESULT_VARIABLE mismatched_report_record_path_report_result
    OUTPUT_VARIABLE mismatched_report_record_path_report_output
    ERROR_VARIABLE mismatched_report_record_path_report_error
)
set(
    mismatched_report_record_path_report_combined
    "${mismatched_report_record_path_report_output}${mismatched_report_record_path_report_error}")
if(NOT mismatched_report_record_path_report_result EQUAL 0)
    message(
        FATAL_ERROR
        "Target acceptance mismatched-report-record-path command failed: ${mismatched_report_record_path_report_combined}")
endif()

require_output(
    mismatched_report_record_path_report_combined
    "`plan\\.acceptance\\.evidence\\.targetAcceptanceReport\\.sha256Status`: match"
    "keep target acceptance report evidence SHA-256 match for mismatched record path")
require_output(
    mismatched_report_record_path_report_combined
    "`plan\\.acceptance\\.evidence\\.targetAcceptanceReport\\.contentStatus`: missing-marker"
    "reject target acceptance report evidence for a different record path")
require_output(
    mismatched_report_record_path_report_combined
    "`plan\\.acceptance\\.evidence\\.ready`: false"
    "reject target acceptance report evidence with mismatched record path")
require_output(
    mismatched_report_record_path_report_combined
    "`plan\\.acceptance\\.evidence\\.blocker`: targetAcceptanceReportEvidenceContent"
    "report target acceptance report record-path content blocker")
require_output(
    mismatched_report_record_path_report_combined
    "`plan\\.acceptance\\.finalAcceptanceBlocker`: targetPlatform, targetDisplay, multiDisplay, acceptanceChecklist, acceptanceEvidence"
    "require target acceptance report evidence to reference the same record path before final acceptance")

execute_process(
    COMMAND
        "${CMAKE_COMMAND}" -E env
        "QT_QPA_PLATFORM=offscreen"
        "LIZZIE_KATAGO_EXECUTABLE=${APP_EXECUTABLE}"
        "LIZZIE_KATAGO_MODEL=${model_path}"
        "LIZZIE_KATAGO_ANALYSIS_CONFIG=${analysis_config_path}"
        "LIZZIE_KATAGO_GTP_CONFIG=${gtp_config_path}"
        "LIZZIE_DIAGNOSTICS_SETTINGS_FILE=${settings_file}"
        "LIZZIE_TARGET_ACCEPTANCE_RECORD_FILE=${acceptance_record_mismatched_engine_log_version_file}"
        "${APP_EXECUTABLE}" --target-acceptance-report
    WORKING_DIRECTORY "${work_dir}"
    RESULT_VARIABLE mismatched_engine_log_version_report_result
    OUTPUT_VARIABLE mismatched_engine_log_version_report_output
    ERROR_VARIABLE mismatched_engine_log_version_report_error
)
set(
    mismatched_engine_log_version_report_combined
    "${mismatched_engine_log_version_report_output}${mismatched_engine_log_version_report_error}")
if(NOT mismatched_engine_log_version_report_result EQUAL 0)
    message(
        FATAL_ERROR
        "Target acceptance mismatched-engine-log-version command failed: ${mismatched_engine_log_version_report_combined}")
endif()

require_output(
    mismatched_engine_log_version_report_combined
    "`plan\\.acceptance\\.evidence\\.engineLog\\.sha256Status`: match"
    "keep engine log evidence SHA-256 match for mismatched recorded KataGo version")
require_output(
    mismatched_engine_log_version_report_combined
    "`plan\\.acceptance\\.evidence\\.engineLog\\.contentStatus`: missing-marker"
    "reject engine log evidence without the recorded KataGo version")
require_output(
    mismatched_engine_log_version_report_combined
    "`plan\\.acceptance\\.evidence\\.engineLog\\.gpuOrStderrContentStatus`: match"
    "keep engine log GPU/stderr evidence match for mismatched recorded KataGo version")
require_output(
    mismatched_engine_log_version_report_combined
    "`plan\\.acceptance\\.evidence\\.ready`: false"
    "reject engine log evidence with mismatched recorded KataGo version")
require_output(
    mismatched_engine_log_version_report_combined
    "`plan\\.acceptance\\.evidence\\.blocker`: engineLogEvidenceContent"
    "report engine log recorded-version content blocker")
require_output(
    mismatched_engine_log_version_report_combined
    "`plan\\.acceptance\\.finalAcceptanceBlocker`: targetPlatform, targetDisplay, multiDisplay, acceptanceChecklist, acceptanceEvidence"
    "require engine log evidence to include the recorded KataGo version before final acceptance")

execute_process(
    COMMAND
        "${CMAKE_COMMAND}" -E env
        "QT_QPA_PLATFORM=offscreen"
        "LIZZIE_KATAGO_EXECUTABLE=${APP_EXECUTABLE}"
        "LIZZIE_KATAGO_MODEL=${model_path}"
        "LIZZIE_KATAGO_ANALYSIS_CONFIG=${analysis_config_path}"
        "LIZZIE_KATAGO_GTP_CONFIG=${gtp_config_path}"
        "LIZZIE_DIAGNOSTICS_SETTINGS_FILE=${settings_file}"
        "LIZZIE_TARGET_ACCEPTANCE_RECORD_FILE=${acceptance_record_missing_engine_gpu_stderr_file}"
        "${APP_EXECUTABLE}" --target-acceptance-report
    WORKING_DIRECTORY "${work_dir}"
    RESULT_VARIABLE missing_engine_gpu_stderr_report_result
    OUTPUT_VARIABLE missing_engine_gpu_stderr_report_output
    ERROR_VARIABLE missing_engine_gpu_stderr_report_error
)
set(
    missing_engine_gpu_stderr_report_combined
    "${missing_engine_gpu_stderr_report_output}${missing_engine_gpu_stderr_report_error}")
if(NOT missing_engine_gpu_stderr_report_result EQUAL 0)
    message(
        FATAL_ERROR
        "Target acceptance missing-engine-gpu-stderr command failed: ${missing_engine_gpu_stderr_report_combined}")
endif()

require_output(
    missing_engine_gpu_stderr_report_combined
    "`plan\\.acceptance\\.evidence\\.engineLog\\.contentStatus`: match"
    "keep engine log version evidence match without GPU/stderr markers")
require_output(
    missing_engine_gpu_stderr_report_combined
    "`plan\\.acceptance\\.evidence\\.engineLog\\.gpuOrStderrContentStatus`: missing-marker"
    "reject engine log evidence without GPU/stderr markers")
require_output(
    missing_engine_gpu_stderr_report_combined
    "`plan\\.acceptance\\.evidence\\.engineLog\\.missingGpuOrStderrContentMarker`: CUDA, OpenCL, TensorRT, GPU, gpu, backend, Backend, stderr:, Stderr:"
    "report engine log GPU/stderr marker candidates")
require_output(
    missing_engine_gpu_stderr_report_combined
    "`plan\\.acceptance\\.evidence\\.blocker`: engineLogGpuOrStderrEvidenceContent"
    "report engine log GPU/stderr content blocker")
require_output(
    missing_engine_gpu_stderr_report_combined
    "`plan\\.acceptance\\.finalAcceptanceBlocker`: targetPlatform, targetDisplay, multiDisplay, acceptanceChecklist, acceptanceEvidence"
    "require engine log evidence to include GPU/stderr markers before final acceptance")

execute_process(
    COMMAND
        "${CMAKE_COMMAND}" -E env
        "QT_QPA_PLATFORM=offscreen"
        "LIZZIE_KATAGO_EXECUTABLE=${APP_EXECUTABLE}"
        "LIZZIE_KATAGO_MODEL=${model_path}"
        "LIZZIE_KATAGO_ANALYSIS_CONFIG=${analysis_config_path}"
        "LIZZIE_KATAGO_GTP_CONFIG=${gtp_config_path}"
        "LIZZIE_DIAGNOSTICS_SETTINGS_FILE=${settings_file}"
        "LIZZIE_TARGET_ACCEPTANCE_RECORD_FILE=${acceptance_record_mismatched_raw_log_version_file}"
        "${APP_EXECUTABLE}" --target-acceptance-report
    WORKING_DIRECTORY "${work_dir}"
    RESULT_VARIABLE mismatched_raw_log_version_report_result
    OUTPUT_VARIABLE mismatched_raw_log_version_report_output
    ERROR_VARIABLE mismatched_raw_log_version_report_error
)
set(
    mismatched_raw_log_version_report_combined
    "${mismatched_raw_log_version_report_output}${mismatched_raw_log_version_report_error}")
if(NOT mismatched_raw_log_version_report_result EQUAL 0)
    message(
        FATAL_ERROR
        "Target acceptance mismatched-raw-log-version command failed: ${mismatched_raw_log_version_report_combined}")
endif()

require_output(
    mismatched_raw_log_version_report_combined
    "`plan\\.acceptance\\.evidence\\.rawKataGoComparisonLog\\.sha256Status`: match"
    "keep raw KataGo comparison log evidence SHA-256 match for mismatched recorded KataGo version")
require_output(
    mismatched_raw_log_version_report_combined
    "`plan\\.acceptance\\.evidence\\.rawKataGoComparisonLog\\.contentStatus`: missing-marker"
    "reject raw KataGo comparison log evidence without the recorded KataGo version")
require_output(
    mismatched_raw_log_version_report_combined
    "`plan\\.acceptance\\.evidence\\.ready`: false"
    "reject raw KataGo comparison log evidence with mismatched recorded KataGo version")
require_output(
    mismatched_raw_log_version_report_combined
    "`plan\\.acceptance\\.evidence\\.blocker`: rawKataGoComparisonLogEvidenceContent"
    "report raw KataGo comparison log recorded-version content blocker")
require_output(
    mismatched_raw_log_version_report_combined
    "`plan\\.acceptance\\.finalAcceptanceBlocker`: targetPlatform, targetDisplay, multiDisplay, acceptanceChecklist, acceptanceEvidence"
    "require raw KataGo comparison log evidence to include the recorded KataGo version before final acceptance")

execute_process(
    COMMAND
        "${CMAKE_COMMAND}" -E env
        "QT_QPA_PLATFORM=offscreen"
        "LIZZIE_KATAGO_EXECUTABLE=${APP_EXECUTABLE}"
        "LIZZIE_KATAGO_MODEL=${model_path}"
        "LIZZIE_KATAGO_ANALYSIS_CONFIG=${analysis_config_path}"
        "LIZZIE_KATAGO_GTP_CONFIG=${gtp_config_path}"
        "LIZZIE_DIAGNOSTICS_SETTINGS_FILE=${settings_file}"
        "LIZZIE_TARGET_ACCEPTANCE_RECORD_FILE=${acceptance_record_bad_raw_log_content_file}"
        "${APP_EXECUTABLE}" --target-acceptance-report
    WORKING_DIRECTORY "${work_dir}"
    RESULT_VARIABLE bad_raw_log_content_report_result
    OUTPUT_VARIABLE bad_raw_log_content_report_output
    ERROR_VARIABLE bad_raw_log_content_report_error
)
set(
    bad_raw_log_content_report_combined
    "${bad_raw_log_content_report_output}${bad_raw_log_content_report_error}")
if(NOT bad_raw_log_content_report_result EQUAL 0)
    message(
        FATAL_ERROR
        "Target acceptance bad-raw-log-content report command failed: ${bad_raw_log_content_report_combined}")
endif()

require_output(
    bad_raw_log_content_report_combined
    "`plan\\.acceptance\\.evidence\\.rawKataGoComparisonLog\\.sha256Status`: match"
    "keep raw KataGo comparison log evidence SHA-256 match for bad content")
require_output(
    bad_raw_log_content_report_combined
    "`plan\\.acceptance\\.evidence\\.rawKataGoComparisonLog\\.contentStatus`: missing-marker"
    "report raw KataGo comparison log evidence missing marker")
require_output(
    bad_raw_log_content_report_combined
    "`plan\\.acceptance\\.evidence\\.ready`: false"
    "reject raw KataGo comparison log evidence with missing marker")
require_output(
    bad_raw_log_content_report_combined
    "`plan\\.acceptance\\.evidence\\.blocker`: rawKataGoComparisonLogEvidenceContent"
    "report raw KataGo comparison log evidence content blocker")
require_output(
    bad_raw_log_content_report_combined
    "`plan\\.acceptance\\.finalAcceptanceBlocker`: targetPlatform, targetDisplay, multiDisplay, acceptanceChecklist, acceptanceEvidence"
    "require raw KataGo comparison log evidence marker before final acceptance")

execute_process(
    COMMAND
        "${CMAKE_COMMAND}" -E env
        "QT_QPA_PLATFORM=offscreen"
        "LIZZIE_KATAGO_EXECUTABLE=${APP_EXECUTABLE}"
        "LIZZIE_KATAGO_MODEL=${model_path}"
        "LIZZIE_KATAGO_ANALYSIS_CONFIG=${analysis_config_path}"
        "LIZZIE_KATAGO_GTP_CONFIG=${gtp_config_path}"
        "LIZZIE_DIAGNOSTICS_SETTINGS_FILE=${settings_file}"
        "LIZZIE_TARGET_ACCEPTANCE_RECORD_FILE=${acceptance_record_bad_manual_ui_log_content_file}"
        "${APP_EXECUTABLE}" --target-acceptance-report
    WORKING_DIRECTORY "${work_dir}"
    RESULT_VARIABLE bad_manual_ui_log_content_report_result
    OUTPUT_VARIABLE bad_manual_ui_log_content_report_output
    ERROR_VARIABLE bad_manual_ui_log_content_report_error
)
set(
    bad_manual_ui_log_content_report_combined
    "${bad_manual_ui_log_content_report_output}${bad_manual_ui_log_content_report_error}")
if(NOT bad_manual_ui_log_content_report_result EQUAL 0)
    message(
        FATAL_ERROR
        "Target acceptance bad-manual-ui-log-content report command failed: ${bad_manual_ui_log_content_report_combined}")
endif()

require_output(
    bad_manual_ui_log_content_report_combined
    "`plan\\.acceptance\\.evidence\\.manualUiInspectionLog\\.sha256Status`: match"
    "keep manual UI inspection log evidence SHA-256 match for bad content")
require_output(
    bad_manual_ui_log_content_report_combined
    "`plan\\.acceptance\\.evidence\\.manualUiInspectionLog\\.contentStatus`: missing-marker"
    "report manual UI inspection log evidence missing marker")
require_output(
    bad_manual_ui_log_content_report_combined
    "`plan\\.acceptance\\.evidence\\.manualUiInspectionLog\\.missingContentMarker`: .*mainWindow4KHighDpiLayout"
    "name the missing manual UI inspection checklist marker")
require_output(
    bad_manual_ui_log_content_report_combined
    "`plan\\.acceptance\\.evidence\\.ready`: false"
    "reject manual UI inspection log evidence with missing marker")
require_output(
    bad_manual_ui_log_content_report_combined
    "`plan\\.acceptance\\.evidence\\.blocker`: manualUiInspectionLogEvidenceContent"
    "report manual UI inspection log evidence content blocker")
require_output(
    bad_manual_ui_log_content_report_combined
    "`plan\\.acceptance\\.finalAcceptanceBlocker`: targetPlatform, targetDisplay, multiDisplay, acceptanceChecklist, acceptanceEvidence"
    "require manual UI inspection log evidence marker before final acceptance")

execute_process(
    COMMAND
        "${CMAKE_COMMAND}" -E env
        "QT_QPA_PLATFORM=offscreen"
        "LIZZIE_KATAGO_EXECUTABLE=${APP_EXECUTABLE}"
        "LIZZIE_KATAGO_MODEL=${model_path}"
        "LIZZIE_KATAGO_ANALYSIS_CONFIG=${analysis_config_path}"
        "LIZZIE_KATAGO_GTP_CONFIG=${gtp_config_path}"
        "LIZZIE_DIAGNOSTICS_SETTINGS_FILE=${settings_file}"
        "LIZZIE_TARGET_ACCEPTANCE_RECORD_FILE=${acceptance_record_bad_external_verification_log_content_file}"
        "${APP_EXECUTABLE}" --target-acceptance-report
    WORKING_DIRECTORY "${work_dir}"
    RESULT_VARIABLE bad_external_verification_log_content_report_result
    OUTPUT_VARIABLE bad_external_verification_log_content_report_output
    ERROR_VARIABLE bad_external_verification_log_content_report_error
)
set(
    bad_external_verification_log_content_report_combined
    "${bad_external_verification_log_content_report_output}${bad_external_verification_log_content_report_error}")
if(NOT bad_external_verification_log_content_report_result EQUAL 0)
    message(
        FATAL_ERROR
        "Target acceptance bad-external-verification-log-content report command failed: ${bad_external_verification_log_content_report_combined}")
endif()

require_output(
    bad_external_verification_log_content_report_combined
    "`plan\\.acceptance\\.evidence\\.externalTargetVerificationLog\\.sha256Status`: match"
    "keep external target verification log evidence SHA-256 match for bad content")
require_output(
    bad_external_verification_log_content_report_combined
    "`plan\\.acceptance\\.evidence\\.externalTargetVerificationLog\\.contentStatus`: missing-marker"
    "report external target verification log evidence missing marker")
require_output(
    bad_external_verification_log_content_report_combined
    "`plan\\.acceptance\\.evidence\\.externalTargetVerificationLog\\.missingContentMarker`: .*linuxKdeWaylandNvidiaRealtimeKataGo"
    "name the missing external target verification checklist marker")
require_output(
    bad_external_verification_log_content_report_combined
    "`plan\\.acceptance\\.evidence\\.ready`: false"
    "reject external target verification log evidence with missing marker")
require_output(
    bad_external_verification_log_content_report_combined
    "`plan\\.acceptance\\.evidence\\.blocker`: externalTargetVerificationLogEvidenceContent"
    "report external target verification log evidence content blocker")
require_output(
    bad_external_verification_log_content_report_combined
    "`plan\\.acceptance\\.finalAcceptanceBlocker`: targetPlatform, targetDisplay, multiDisplay, acceptanceChecklist, acceptanceEvidence"
    "require external target verification log evidence marker before final acceptance")

execute_process(
    COMMAND
        "${CMAKE_COMMAND}" -E env
        "QT_QPA_PLATFORM=offscreen"
        "LIZZIE_KATAGO_EXECUTABLE=${APP_EXECUTABLE}"
        "LIZZIE_KATAGO_MODEL=${model_path}"
        "LIZZIE_KATAGO_ANALYSIS_CONFIG=${analysis_config_path}"
        "LIZZIE_KATAGO_GTP_CONFIG=${gtp_config_path}"
        "LIZZIE_DIAGNOSTICS_SETTINGS_FILE=${settings_file}"
        "LIZZIE_TARGET_ACCEPTANCE_RECORD_FILE=${acceptance_record_incomplete_raw_log_content_file}"
        "${APP_EXECUTABLE}" --target-acceptance-report
    WORKING_DIRECTORY "${work_dir}"
    RESULT_VARIABLE incomplete_raw_log_content_report_result
    OUTPUT_VARIABLE incomplete_raw_log_content_report_output
    ERROR_VARIABLE incomplete_raw_log_content_report_error
)
set(
    incomplete_raw_log_content_report_combined
    "${incomplete_raw_log_content_report_output}${incomplete_raw_log_content_report_error}")
if(NOT incomplete_raw_log_content_report_result EQUAL 0)
    message(
        FATAL_ERROR
        "Target acceptance incomplete-raw-log-content report command failed: ${incomplete_raw_log_content_report_combined}")
endif()

require_output(
    incomplete_raw_log_content_report_combined
    "`plan\\.acceptance\\.evidence\\.rawKataGoComparisonLog\\.sha256Status`: match"
    "keep incomplete raw KataGo comparison log evidence SHA-256 match")
require_output(
    incomplete_raw_log_content_report_combined
    "`plan\\.acceptance\\.evidence\\.rawKataGoComparisonLog\\.contentStatus`: missing-marker"
    "report incomplete raw KataGo comparison log evidence missing raw markers")
require_output(
    incomplete_raw_log_content_report_combined
    "`plan\\.acceptance\\.evidence\\.rawKataGoComparisonLog\\.missingContentMarker`: .*analysisJsonRawResponse"
    "report incomplete raw KataGo comparison log evidence missing raw comparison checklist marker")
require_output(
    incomplete_raw_log_content_report_combined
    "`plan\\.acceptance\\.evidence\\.ready`: false"
    "reject incomplete raw KataGo comparison log evidence")
require_output(
    incomplete_raw_log_content_report_combined
    "`plan\\.acceptance\\.evidence\\.blocker`: rawKataGoComparisonLogEvidenceContent"
    "report incomplete raw KataGo comparison log evidence content blocker")
require_output(
    incomplete_raw_log_content_report_combined
    "`plan\\.acceptance\\.finalAcceptanceBlocker`: targetPlatform, targetDisplay, multiDisplay, acceptanceChecklist, acceptanceEvidence"
    "require raw KataGo comparison raw markers before final acceptance")

execute_process(
    COMMAND
        "${CMAKE_COMMAND}" -E env
        "QT_QPA_PLATFORM=offscreen"
        "LIZZIE_KATAGO_EXECUTABLE=${APP_EXECUTABLE}"
        "LIZZIE_KATAGO_MODEL=${model_path}"
        "LIZZIE_KATAGO_ANALYSIS_CONFIG=${analysis_config_path}"
        "LIZZIE_KATAGO_GTP_CONFIG=${gtp_config_path}"
        "LIZZIE_DIAGNOSTICS_SETTINGS_FILE=${settings_file}"
        "LIZZIE_TARGET_ACCEPTANCE_RECORD_FILE=${acceptance_record_missing_raw_log_winrate_file}"
        "${APP_EXECUTABLE}" --target-acceptance-report
    WORKING_DIRECTORY "${work_dir}"
    RESULT_VARIABLE missing_winrate_raw_log_report_result
    OUTPUT_VARIABLE missing_winrate_raw_log_report_output
    ERROR_VARIABLE missing_winrate_raw_log_report_error
)
set(
    missing_winrate_raw_log_report_combined
    "${missing_winrate_raw_log_report_output}${missing_winrate_raw_log_report_error}")
if(NOT missing_winrate_raw_log_report_result EQUAL 0)
    message(
        FATAL_ERROR
        "Target acceptance missing-winrate-raw-log report command failed: ${missing_winrate_raw_log_report_combined}")
endif()

require_output(
    missing_winrate_raw_log_report_combined
    "`plan\\.acceptance\\.evidence\\.rawKataGoComparisonLog\\.sha256Status`: match"
    "keep missing-winrate raw KataGo comparison log evidence SHA-256 match")
require_output(
    missing_winrate_raw_log_report_combined
    "`plan\\.acceptance\\.evidence\\.rawKataGoComparisonLog\\.contentStatus`: missing-marker"
    "report missing-winrate raw KataGo comparison log evidence missing winrate marker")
require_output(
    missing_winrate_raw_log_report_combined
    "`plan\\.acceptance\\.evidence\\.rawKataGoComparisonLog\\.missingContentMarker`: \"winrate\""
    "name the missing raw KataGo winrate evidence marker")
require_output(
    missing_winrate_raw_log_report_combined
    "`plan\\.acceptance\\.evidence\\.ready`: false"
    "reject missing-winrate raw KataGo comparison log evidence")
require_output(
    missing_winrate_raw_log_report_combined
    "`plan\\.acceptance\\.evidence\\.blocker`: rawKataGoComparisonLogEvidenceContent"
    "report missing-winrate raw KataGo comparison log evidence content blocker")
require_output(
    missing_winrate_raw_log_report_combined
    "`plan\\.acceptance\\.finalAcceptanceBlocker`: targetPlatform, targetDisplay, multiDisplay, acceptanceChecklist, acceptanceEvidence"
    "require raw KataGo comparison winrate marker before final acceptance")

execute_process(
    COMMAND
        "${CMAKE_COMMAND}" -E env
        "QT_QPA_PLATFORM=offscreen"
        "LIZZIE_KATAGO_EXECUTABLE=${APP_EXECUTABLE}"
        "LIZZIE_KATAGO_MODEL=${model_path}"
        "LIZZIE_KATAGO_ANALYSIS_CONFIG=${analysis_config_path}"
        "LIZZIE_KATAGO_GTP_CONFIG=${gtp_config_path}"
        "LIZZIE_DIAGNOSTICS_SETTINGS_FILE=${settings_file}"
        "LIZZIE_TARGET_ACCEPTANCE_RECORD_FILE=${acceptance_record_missing_raw_log_score_mean_file}"
        "${APP_EXECUTABLE}" --target-acceptance-report
    WORKING_DIRECTORY "${work_dir}"
    RESULT_VARIABLE missing_score_mean_raw_log_report_result
    OUTPUT_VARIABLE missing_score_mean_raw_log_report_output
    ERROR_VARIABLE missing_score_mean_raw_log_report_error
)
set(
    missing_score_mean_raw_log_report_combined
    "${missing_score_mean_raw_log_report_output}${missing_score_mean_raw_log_report_error}")
if(NOT missing_score_mean_raw_log_report_result EQUAL 0)
    message(
        FATAL_ERROR
        "Target acceptance missing-score-mean-raw-log report command failed: ${missing_score_mean_raw_log_report_combined}")
endif()

require_output(
    missing_score_mean_raw_log_report_combined
    "`plan\\.acceptance\\.evidence\\.rawKataGoComparisonLog\\.sha256Status`: match"
    "keep missing-scoreMean raw KataGo comparison log evidence SHA-256 match")
require_output(
    missing_score_mean_raw_log_report_combined
    "`plan\\.acceptance\\.evidence\\.rawKataGoComparisonLog\\.contentStatus`: missing-marker"
    "report missing-scoreMean raw KataGo comparison log evidence missing scoreMean marker")
require_output(
    missing_score_mean_raw_log_report_combined
    "`plan\\.acceptance\\.evidence\\.rawKataGoComparisonLog\\.missingContentMarker`: \"scoreMean\""
    "name the missing raw KataGo scoreMean evidence marker")
require_output(
    missing_score_mean_raw_log_report_combined
    "`plan\\.acceptance\\.evidence\\.ready`: false"
    "reject missing-scoreMean raw KataGo comparison log evidence")
require_output(
    missing_score_mean_raw_log_report_combined
    "`plan\\.acceptance\\.evidence\\.blocker`: rawKataGoComparisonLogEvidenceContent"
    "report missing-scoreMean raw KataGo comparison log evidence content blocker")
require_output(
    missing_score_mean_raw_log_report_combined
    "`plan\\.acceptance\\.finalAcceptanceBlocker`: targetPlatform, targetDisplay, multiDisplay, acceptanceChecklist, acceptanceEvidence"
    "require raw KataGo comparison scoreMean marker before final acceptance")

execute_process(
    COMMAND
        "${CMAKE_COMMAND}" -E env
        "QT_QPA_PLATFORM=offscreen"
        "LIZZIE_KATAGO_EXECUTABLE=${APP_EXECUTABLE}"
        "LIZZIE_KATAGO_MODEL=${model_path}"
        "LIZZIE_KATAGO_ANALYSIS_CONFIG=${analysis_config_path}"
        "LIZZIE_KATAGO_GTP_CONFIG=${gtp_config_path}"
        "LIZZIE_DIAGNOSTICS_SETTINGS_FILE=${settings_file}"
        "LIZZIE_TARGET_ACCEPTANCE_RECORD_FILE=${acceptance_record_missing_raw_log_score_stdev_file}"
        "${APP_EXECUTABLE}" --target-acceptance-report
    WORKING_DIRECTORY "${work_dir}"
    RESULT_VARIABLE missing_score_stdev_raw_log_report_result
    OUTPUT_VARIABLE missing_score_stdev_raw_log_report_output
    ERROR_VARIABLE missing_score_stdev_raw_log_report_error
)
set(
    missing_score_stdev_raw_log_report_combined
    "${missing_score_stdev_raw_log_report_output}${missing_score_stdev_raw_log_report_error}")
if(NOT missing_score_stdev_raw_log_report_result EQUAL 0)
    message(
        FATAL_ERROR
        "Target acceptance missing-score-stdev-raw-log report command failed: ${missing_score_stdev_raw_log_report_combined}")
endif()

require_output(
    missing_score_stdev_raw_log_report_combined
    "`plan\\.acceptance\\.evidence\\.rawKataGoComparisonLog\\.sha256Status`: match"
    "keep missing-scoreStdev raw KataGo comparison log evidence SHA-256 match")
require_output(
    missing_score_stdev_raw_log_report_combined
    "`plan\\.acceptance\\.evidence\\.rawKataGoComparisonLog\\.contentStatus`: missing-marker"
    "report missing-scoreStdev raw KataGo comparison log evidence missing scoreStdev marker")
require_output(
    missing_score_stdev_raw_log_report_combined
    "`plan\\.acceptance\\.evidence\\.rawKataGoComparisonLog\\.missingContentMarker`: \"scoreStdev\""
    "name the missing raw KataGo scoreStdev evidence marker")
require_output(
    missing_score_stdev_raw_log_report_combined
    "`plan\\.acceptance\\.evidence\\.ready`: false"
    "reject missing-scoreStdev raw KataGo comparison log evidence")
require_output(
    missing_score_stdev_raw_log_report_combined
    "`plan\\.acceptance\\.evidence\\.blocker`: rawKataGoComparisonLogEvidenceContent"
    "report missing-scoreStdev raw KataGo comparison log evidence content blocker")
require_output(
    missing_score_stdev_raw_log_report_combined
    "`plan\\.acceptance\\.finalAcceptanceBlocker`: targetPlatform, targetDisplay, multiDisplay, acceptanceChecklist, acceptanceEvidence"
    "require raw KataGo comparison scoreStdev marker before final acceptance")

execute_process(
    COMMAND
        "${CMAKE_COMMAND}" -E env
        "QT_QPA_PLATFORM=offscreen"
        "LIZZIE_KATAGO_EXECUTABLE=${APP_EXECUTABLE}"
        "LIZZIE_KATAGO_MODEL=${model_path}"
        "LIZZIE_KATAGO_ANALYSIS_CONFIG=${analysis_config_path}"
        "LIZZIE_KATAGO_GTP_CONFIG=${gtp_config_path}"
        "LIZZIE_DIAGNOSTICS_SETTINGS_FILE=${settings_file}"
        "LIZZIE_TARGET_ACCEPTANCE_RECORD_FILE=${acceptance_record_missing_raw_log_policy_file}"
        "${APP_EXECUTABLE}" --target-acceptance-report
    WORKING_DIRECTORY "${work_dir}"
    RESULT_VARIABLE missing_policy_raw_log_report_result
    OUTPUT_VARIABLE missing_policy_raw_log_report_output
    ERROR_VARIABLE missing_policy_raw_log_report_error
)
set(
    missing_policy_raw_log_report_combined
    "${missing_policy_raw_log_report_output}${missing_policy_raw_log_report_error}")
if(NOT missing_policy_raw_log_report_result EQUAL 0)
    message(
        FATAL_ERROR
        "Target acceptance missing-policy-raw-log report command failed: ${missing_policy_raw_log_report_combined}")
endif()

require_output(
    missing_policy_raw_log_report_combined
    "`plan\\.acceptance\\.evidence\\.rawKataGoComparisonLog\\.sha256Status`: match"
    "keep missing-policy raw KataGo comparison log evidence SHA-256 match")
require_output(
    missing_policy_raw_log_report_combined
    "`plan\\.acceptance\\.evidence\\.rawKataGoComparisonLog\\.contentStatus`: missing-marker"
    "report missing-policy raw KataGo comparison log evidence missing policy marker")
require_output(
    missing_policy_raw_log_report_combined
    "`plan\\.acceptance\\.evidence\\.rawKataGoComparisonLog\\.missingContentMarker`: \"policy\""
    "name the missing raw KataGo policy evidence marker")
require_output(
    missing_policy_raw_log_report_combined
    "`plan\\.acceptance\\.evidence\\.ready`: false"
    "reject missing-policy raw KataGo comparison log evidence")
require_output(
    missing_policy_raw_log_report_combined
    "`plan\\.acceptance\\.evidence\\.blocker`: rawKataGoComparisonLogEvidenceContent"
    "report missing-policy raw KataGo comparison log evidence content blocker")
require_output(
    missing_policy_raw_log_report_combined
    "`plan\\.acceptance\\.finalAcceptanceBlocker`: targetPlatform, targetDisplay, multiDisplay, acceptanceChecklist, acceptanceEvidence"
    "require raw KataGo comparison policy marker before final acceptance")

execute_process(
    COMMAND
        "${CMAKE_COMMAND}" -E env
        "QT_QPA_PLATFORM=offscreen"
        "LIZZIE_KATAGO_EXECUTABLE=${APP_EXECUTABLE}"
        "LIZZIE_KATAGO_MODEL=${model_path}"
        "LIZZIE_KATAGO_ANALYSIS_CONFIG=${analysis_config_path}"
        "LIZZIE_KATAGO_GTP_CONFIG=${gtp_config_path}"
        "LIZZIE_DIAGNOSTICS_SETTINGS_FILE=${settings_file}"
        "LIZZIE_TARGET_ACCEPTANCE_RECORD_FILE=${acceptance_record_missing_raw_log_visits_file}"
        "${APP_EXECUTABLE}" --target-acceptance-report
    WORKING_DIRECTORY "${work_dir}"
    RESULT_VARIABLE missing_visits_raw_log_report_result
    OUTPUT_VARIABLE missing_visits_raw_log_report_output
    ERROR_VARIABLE missing_visits_raw_log_report_error
)
set(
    missing_visits_raw_log_report_combined
    "${missing_visits_raw_log_report_output}${missing_visits_raw_log_report_error}")
if(NOT missing_visits_raw_log_report_result EQUAL 0)
    message(
        FATAL_ERROR
        "Target acceptance missing-visits-raw-log report command failed: ${missing_visits_raw_log_report_combined}")
endif()

require_output(
    missing_visits_raw_log_report_combined
    "`plan\\.acceptance\\.evidence\\.rawKataGoComparisonLog\\.sha256Status`: match"
    "keep missing-visits raw KataGo comparison log evidence SHA-256 match")
require_output(
    missing_visits_raw_log_report_combined
    "`plan\\.acceptance\\.evidence\\.rawKataGoComparisonLog\\.contentStatus`: missing-marker"
    "report missing-visits raw KataGo comparison log evidence missing visits marker")
require_output(
    missing_visits_raw_log_report_combined
    "`plan\\.acceptance\\.evidence\\.rawKataGoComparisonLog\\.missingContentMarker`: \"visits\""
    "name the missing raw KataGo visits evidence marker")
require_output(
    missing_visits_raw_log_report_combined
    "`plan\\.acceptance\\.evidence\\.ready`: false"
    "reject missing-visits raw KataGo comparison log evidence")
require_output(
    missing_visits_raw_log_report_combined
    "`plan\\.acceptance\\.evidence\\.blocker`: rawKataGoComparisonLogEvidenceContent"
    "report missing-visits raw KataGo comparison log evidence content blocker")
require_output(
    missing_visits_raw_log_report_combined
    "`plan\\.acceptance\\.finalAcceptanceBlocker`: targetPlatform, targetDisplay, multiDisplay, acceptanceChecklist, acceptanceEvidence"
    "require raw KataGo comparison visits marker before final acceptance")

execute_process(
    COMMAND
        "${CMAKE_COMMAND}" -E env
        "QT_QPA_PLATFORM=offscreen"
        "LIZZIE_KATAGO_EXECUTABLE=${APP_EXECUTABLE}"
        "LIZZIE_KATAGO_MODEL=${model_path}"
        "LIZZIE_KATAGO_ANALYSIS_CONFIG=${analysis_config_path}"
        "LIZZIE_KATAGO_GTP_CONFIG=${gtp_config_path}"
        "LIZZIE_DIAGNOSTICS_SETTINGS_FILE=${settings_file}"
        "LIZZIE_TARGET_ACCEPTANCE_RECORD_FILE=${acceptance_record_missing_raw_log_pv_file}"
        "${APP_EXECUTABLE}" --target-acceptance-report
    WORKING_DIRECTORY "${work_dir}"
    RESULT_VARIABLE missing_pv_raw_log_report_result
    OUTPUT_VARIABLE missing_pv_raw_log_report_output
    ERROR_VARIABLE missing_pv_raw_log_report_error
)
set(
    missing_pv_raw_log_report_combined
    "${missing_pv_raw_log_report_output}${missing_pv_raw_log_report_error}")
if(NOT missing_pv_raw_log_report_result EQUAL 0)
    message(
        FATAL_ERROR
        "Target acceptance missing-pv-raw-log report command failed: ${missing_pv_raw_log_report_combined}")
endif()

require_output(
    missing_pv_raw_log_report_combined
    "`plan\\.acceptance\\.evidence\\.rawKataGoComparisonLog\\.sha256Status`: match"
    "keep missing-pv raw KataGo comparison log evidence SHA-256 match")
require_output(
    missing_pv_raw_log_report_combined
    "`plan\\.acceptance\\.evidence\\.rawKataGoComparisonLog\\.contentStatus`: missing-marker"
    "report missing-pv raw KataGo comparison log evidence missing pv marker")
require_output(
    missing_pv_raw_log_report_combined
    "`plan\\.acceptance\\.evidence\\.rawKataGoComparisonLog\\.missingContentMarker`: \"pv\""
    "name the missing raw KataGo pv evidence marker")
require_output(
    missing_pv_raw_log_report_combined
    "`plan\\.acceptance\\.evidence\\.ready`: false"
    "reject missing-pv raw KataGo comparison log evidence")
require_output(
    missing_pv_raw_log_report_combined
    "`plan\\.acceptance\\.evidence\\.blocker`: rawKataGoComparisonLogEvidenceContent"
    "report missing-pv raw KataGo comparison log evidence content blocker")
require_output(
    missing_pv_raw_log_report_combined
    "`plan\\.acceptance\\.finalAcceptanceBlocker`: targetPlatform, targetDisplay, multiDisplay, acceptanceChecklist, acceptanceEvidence"
    "require raw KataGo comparison pv marker before final acceptance")

execute_process(
    COMMAND
        "${CMAKE_COMMAND}" -E env
        "QT_QPA_PLATFORM=offscreen"
        "LIZZIE_KATAGO_EXECUTABLE=${APP_EXECUTABLE}"
        "LIZZIE_KATAGO_MODEL=${model_path}"
        "LIZZIE_KATAGO_ANALYSIS_CONFIG=${analysis_config_path}"
        "LIZZIE_KATAGO_GTP_CONFIG=${gtp_config_path}"
        "LIZZIE_DIAGNOSTICS_SETTINGS_FILE=${settings_file}"
        "LIZZIE_TARGET_ACCEPTANCE_RECORD_FILE=${acceptance_record_missing_raw_log_pv_visits_file}"
        "${APP_EXECUTABLE}" --target-acceptance-report
    WORKING_DIRECTORY "${work_dir}"
    RESULT_VARIABLE missing_pv_visits_raw_log_report_result
    OUTPUT_VARIABLE missing_pv_visits_raw_log_report_output
    ERROR_VARIABLE missing_pv_visits_raw_log_report_error
)
set(
    missing_pv_visits_raw_log_report_combined
    "${missing_pv_visits_raw_log_report_output}${missing_pv_visits_raw_log_report_error}")
if(NOT missing_pv_visits_raw_log_report_result EQUAL 0)
    message(
        FATAL_ERROR
        "Target acceptance missing-pv-visits-raw-log report command failed: ${missing_pv_visits_raw_log_report_combined}")
endif()

require_output(
    missing_pv_visits_raw_log_report_combined
    "`plan\\.acceptance\\.evidence\\.rawKataGoComparisonLog\\.sha256Status`: match"
    "keep missing-pvVisits raw KataGo comparison log evidence SHA-256 match")
require_output(
    missing_pv_visits_raw_log_report_combined
    "`plan\\.acceptance\\.evidence\\.rawKataGoComparisonLog\\.contentStatus`: missing-marker"
    "report missing-pvVisits raw KataGo comparison log evidence missing pvVisits marker")
require_output(
    missing_pv_visits_raw_log_report_combined
    "`plan\\.acceptance\\.evidence\\.rawKataGoComparisonLog\\.missingContentMarker`: \"pvVisits\""
    "name the missing raw KataGo pvVisits evidence marker")
require_output(
    missing_pv_visits_raw_log_report_combined
    "`plan\\.acceptance\\.evidence\\.ready`: false"
    "reject missing-pvVisits raw KataGo comparison log evidence")
require_output(
    missing_pv_visits_raw_log_report_combined
    "`plan\\.acceptance\\.evidence\\.blocker`: rawKataGoComparisonLogEvidenceContent"
    "report missing-pvVisits raw KataGo comparison log evidence content blocker")
require_output(
    missing_pv_visits_raw_log_report_combined
    "`plan\\.acceptance\\.finalAcceptanceBlocker`: targetPlatform, targetDisplay, multiDisplay, acceptanceChecklist, acceptanceEvidence"
    "require raw KataGo comparison pvVisits marker before final acceptance")

execute_process(
    COMMAND
        "${CMAKE_COMMAND}" -E env
        "QT_QPA_PLATFORM=offscreen"
        "LIZZIE_KATAGO_EXECUTABLE=${APP_EXECUTABLE}"
        "LIZZIE_KATAGO_MODEL=${model_path}"
        "LIZZIE_KATAGO_ANALYSIS_CONFIG=${analysis_config_path}"
        "LIZZIE_KATAGO_GTP_CONFIG=${gtp_config_path}"
        "LIZZIE_DIAGNOSTICS_SETTINGS_FILE=${settings_file}"
        "LIZZIE_TARGET_ACCEPTANCE_RECORD_FILE=${acceptance_record_missing_raw_log_ownership_file}"
        "${APP_EXECUTABLE}" --target-acceptance-report
    WORKING_DIRECTORY "${work_dir}"
    RESULT_VARIABLE missing_ownership_raw_log_report_result
    OUTPUT_VARIABLE missing_ownership_raw_log_report_output
    ERROR_VARIABLE missing_ownership_raw_log_report_error
)
set(
    missing_ownership_raw_log_report_combined
    "${missing_ownership_raw_log_report_output}${missing_ownership_raw_log_report_error}")
if(NOT missing_ownership_raw_log_report_result EQUAL 0)
    message(
        FATAL_ERROR
        "Target acceptance missing-ownership-raw-log report command failed: ${missing_ownership_raw_log_report_combined}")
endif()

require_output(
    missing_ownership_raw_log_report_combined
    "`plan\\.acceptance\\.evidence\\.rawKataGoComparisonLog\\.sha256Status`: match"
    "keep missing-ownership raw KataGo comparison log evidence SHA-256 match")
require_output(
    missing_ownership_raw_log_report_combined
    "`plan\\.acceptance\\.evidence\\.rawKataGoComparisonLog\\.contentStatus`: missing-marker"
    "report missing-ownership raw KataGo comparison log evidence missing ownership marker")
require_output(
    missing_ownership_raw_log_report_combined
    "`plan\\.acceptance\\.evidence\\.rawKataGoComparisonLog\\.missingContentMarker`: \"ownership\""
    "name the missing raw KataGo ownership evidence marker")
require_output(
    missing_ownership_raw_log_report_combined
    "`plan\\.acceptance\\.evidence\\.ready`: false"
    "reject missing-ownership raw KataGo comparison log evidence")
require_output(
    missing_ownership_raw_log_report_combined
    "`plan\\.acceptance\\.evidence\\.blocker`: rawKataGoComparisonLogEvidenceContent"
    "report missing-ownership raw KataGo comparison log evidence content blocker")
require_output(
    missing_ownership_raw_log_report_combined
    "`plan\\.acceptance\\.finalAcceptanceBlocker`: targetPlatform, targetDisplay, multiDisplay, acceptanceChecklist, acceptanceEvidence"
    "require raw KataGo comparison ownership marker before final acceptance")

execute_process(
    COMMAND
        "${CMAKE_COMMAND}" -E env
        "QT_QPA_PLATFORM=offscreen"
        "LIZZIE_DIAGNOSTICS_SETTINGS_FILE=${mode_blocker_settings_file}"
        "${APP_EXECUTABLE}" --target-acceptance-report
    WORKING_DIRECTORY "${work_dir}"
    RESULT_VARIABLE mode_blocker_report_result
    OUTPUT_VARIABLE mode_blocker_report_output
    ERROR_VARIABLE mode_blocker_report_error
)
set(mode_blocker_report_combined "${mode_blocker_report_output}${mode_blocker_report_error}")
if(NOT mode_blocker_report_result EQUAL 0)
    message(FATAL_ERROR "Target acceptance mode-blocker report command failed: ${mode_blocker_report_combined}")
endif()

require_output(
    mode_blocker_report_combined
    "`plan\\.acceptance\\.finalAcceptanceBlocker`: targetPlatform, mainBatchAnalysisConfig, compareRealtimeGtpConfig, compareBatchAnalysisConfig, targetDisplay, multiDisplay, externalTargetVerification, manualRawEngineComparison, manualUiInspection"
    "report mode-specific final acceptance blockers")
if("${mode_blocker_report_combined}" MATCHES "`plan\\.acceptance\\.finalAcceptanceBlocker`: [^\n]*(mainKataGoConfig|compareKataGoConfig)")
    message(
        FATAL_ERROR
        "Mode-specific final blockers should not fall back to coarse engine config blockers: ${mode_blocker_report_combined}")
endif()

message(STATUS "cli_target_acceptance_report_tests passed")
