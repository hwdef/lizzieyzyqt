if(NOT DEFINED APP_EXECUTABLE)
    message(FATAL_ERROR "APP_EXECUTABLE is required")
endif()

if(NOT DEFINED TEST_BINARY_DIR)
    message(FATAL_ERROR "TEST_BINARY_DIR is required")
endif()

function(extract_record_template_value source_var key output_var)
    string(REGEX MATCH "(^|\n)${key}=([^\n]*)" template_match "${${source_var}}")
    if(NOT template_match)
        message(FATAL_ERROR "Target acceptance record template probe did not include ${key}: ${${source_var}}")
    endif()
    set("${output_var}" "${CMAKE_MATCH_2}" PARENT_SCOPE)
endfunction()

function(record_template_section source_var section_name output_var)
    set(section_marker "\n[${section_name}]\n")
    string(FIND "${${source_var}}" "${section_marker}" section_marker_index)
    if(section_marker_index EQUAL -1)
        message(FATAL_ERROR "Target acceptance record template did not include [${section_name}]: ${${source_var}}")
    endif()
    string(LENGTH "${section_marker}" section_marker_length)
    math(EXPR section_content_start "${section_marker_index} + ${section_marker_length}")
    string(SUBSTRING "${${source_var}}" ${section_content_start} -1 section_tail)
    string(FIND "${section_tail}" "\n[" section_end_index)
    if(section_end_index EQUAL -1)
        set(section_content "${section_tail}")
    else()
        string(SUBSTRING "${section_tail}" 0 ${section_end_index} section_content)
    endif()
    set("${output_var}" "${section_content}" PARENT_SCOPE)
endfunction()

function(pass_checklist_section source_var section_name output_var)
    record_template_section("${source_var}" "${section_name}" section_content)
    set(section_output "\n[${section_name}]\n")
    string(REPLACE "\n" ";" section_lines "${section_content}")
    foreach(section_line IN LISTS section_lines)
        if(section_line MATCHES "^([^=]+)=")
            string(APPEND section_output "${CMAKE_MATCH_1}=pass\n")
        endif()
    endforeach()
    set("${output_var}" "${section_output}" PARENT_SCOPE)
endfunction()

function(all_pass_checklists source_var output_var)
    set(all_checklists "")
    foreach(section_name IN ITEMS
        checklist.linuxKdeWaylandNvidia
        checklist.windowsNvidia
        checklist.physicalDisplay
        checklist.rawKataGoComparison
        checklist.externalTargetVerification
        checklist.manualUiInspection)
        pass_checklist_section("${source_var}" "${section_name}" section_text)
        string(APPEND all_checklists "${section_text}")
    endforeach()
    set("${output_var}" "${all_checklists}" PARENT_SCOPE)
endfunction()

set(work_dir "${TEST_BINARY_DIR}/cli-diagnostics-fields")
file(MAKE_DIRECTORY "${work_dir}")
set(home_dir "${work_dir}/home")
set(settings_home "${work_dir}/settings-home")
set(settings_dir "${settings_home}/LizzieYzy")
set(xdg_data_home "${work_dir}/xdg-data")
set(xdg_cache_home "${work_dir}/xdg-cache")
set(xdg_runtime_dir "${work_dir}/xdg-runtime")
set(user_profile_dir "${work_dir}/user-profile")
set(appdata_dir "${work_dir}/appdata")
set(localappdata_dir "${work_dir}/localappdata")
set(temp_dir "${work_dir}/temp")
set(tmp_dir "${work_dir}/tmp")
file(MAKE_DIRECTORY "${home_dir}")
file(MAKE_DIRECTORY "${settings_dir}")
file(MAKE_DIRECTORY "${xdg_data_home}")
file(MAKE_DIRECTORY "${xdg_cache_home}")
file(MAKE_DIRECTORY "${xdg_runtime_dir}")
file(MAKE_DIRECTORY "${user_profile_dir}")
file(MAKE_DIRECTORY "${appdata_dir}")
file(MAKE_DIRECTORY "${localappdata_dir}")
file(MAKE_DIRECTORY "${temp_dir}")
file(MAKE_DIRECTORY "${tmp_dir}")
file(CHMOD "${xdg_runtime_dir}" PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE)

set(model_path "${work_dir}/diagnostics-model.bin.gz")
set(analysis_config_path "${work_dir}/diagnostics-analysis.cfg")
set(gtp_config_path "${work_dir}/diagnostics-gtp.cfg")
set(compare_model_path "${work_dir}/diagnostics-compare-model.bin.gz")
set(compare_analysis_config_path "${work_dir}/diagnostics-compare-analysis.cfg")
set(compare_gtp_config_path "${work_dir}/diagnostics-compare-gtp.cfg")
set(engine_working_dir "${work_dir}/engine-working-dir")
set(compare_working_dir "${work_dir}/compare-working-dir")
set(session_sgf_path "${work_dir}/diagnostics-session.sgf")
set(black_stone_texture_path "${work_dir}/diagnostics-black-stone.png")
set(white_stone_texture_path "${work_dir}/diagnostics-white-stone.png")
set(kwin_drm_device_0 "${work_dir}/dri-card0")
set(kwin_drm_device_1 "${work_dir}/dri-card1")
set(vulkan_icd_0 "${work_dir}/nvidia_icd_0.json")
set(vulkan_icd_1 "${work_dir}/nvidia_icd_1.json")
set(vulkan_driver_0 "${work_dir}/nvidia_driver_0.json")
set(vulkan_driver_1 "${work_dir}/nvidia_driver_1.json")
set(egl_vendor_0 "${work_dir}/egl_vendor_0.json")
set(egl_vendor_1 "${work_dir}/egl_vendor_1.json")
set(runtime_path_dir_0 "${work_dir}/runtime-bin-0")
set(runtime_path_dir_1 "${work_dir}/runtime-bin-1")
set(runtime_library_dir_0 "${work_dir}/runtime-lib-0")
set(runtime_library_dir_1 "${work_dir}/runtime-lib-1")
set(runtime_dyld_dir_0 "${work_dir}/runtime-dyld-0")
set(runtime_dyld_dir_1 "${work_dir}/runtime-dyld-1")
set(qml_import_dir_0 "${work_dir}/qml-import-0")
set(qml_import_dir_1 "${work_dir}/qml-import-1")
set(qml2_import_dir_0 "${work_dir}/qml2-import-0")
set(qml2_import_dir_1 "${work_dir}/qml2-import-1")
set(quick_controls_style_dir_0 "${work_dir}/quick-controls-style-0")
set(quick_controls_style_dir_1 "${work_dir}/quick-controls-style-1")
set(quick_controls_conf_path "${work_dir}/qtquickcontrols2.conf")
set(acceptance_record_file "${work_dir}/target-acceptance-record.ini")
set(acceptance_record_complete_file "${work_dir}/target-acceptance-record-complete.ini")
set(acceptance_record_failed_manual_file "${work_dir}/target-acceptance-record-failed-manual.ini")
set(record_diagnostics_path "${work_dir}/target-diagnostics.txt")
set(record_report_path "${work_dir}/target-report.md")
set(record_engine_log_path "${work_dir}/target-engine.log")
set(record_raw_log_path "${work_dir}/target-raw-katago.log")
set(record_manual_ui_log_path "${work_dir}/target-manual-ui-inspection.log")
set(record_external_verification_log_path "${work_dir}/target-external-verification.log")
set(record_screenshot_path "${work_dir}/target-screenshot.ppm")
set(record_4k_screenshot_path "${work_dir}/target-screenshot-4k.pgm")
set(expected_app_version "0.1.0")
set(record_diagnostics_content "LizzieYzy Qt Diagnostics\napp.version: ${expected_app_version}\napp.executable: ${APP_EXECUTABLE}\nplan.acceptance.recordFile.canonicalPath: ${acceptance_record_file}\nplan.acceptance.recordFile.sha256: fixture\nplan.acceptance.finalAcceptanceStatus: fixture\nplan.acceptance.finalAcceptanceBlocker.count: 0\nplan.acceptance.finalAcceptanceOutstandingBlocker.count: 0\n${acceptance_record_file}\n${acceptance_record_complete_file}\n${acceptance_record_failed_manual_file}\n")
set(record_report_content "# Target Acceptance Report\napp.version: ${expected_app_version}\napp.executable: ${APP_EXECUTABLE}\n`plan.acceptance.recordFile.canonicalPath`: ${acceptance_record_file}\n`plan.acceptance.recordFile.sha256`: fixture\n`plan.acceptance.finalAcceptanceStatus`: fixture\n`plan.acceptance.finalAcceptanceBlocker`: fixture\n`plan.acceptance.finalAcceptanceOutstandingBlocker`: fixture\n${acceptance_record_file}\n${acceptance_record_complete_file}\n${acceptance_record_failed_manual_file}\n")
set(record_engine_log_content "target KataGo engine log evidence fixture\nKataGo 1.15.3\nstderr: CUDA fake backend ready\n")
set(raw_katago_comparison_checklist_content "analysisJsonRawResponse\nrealtimeGtpRawLine\ncandidateTableRendering\npvPreview\nwinrateScorePerspective\nownershipDisplay\ngenmove\nhumanVsAi\nselfPlay\nengineVsEngine\n")
set(record_raw_log_content "target raw KataGo evidence fixture\nKataGo 1.15.3\nkata-analyze raw line\n{\"rootInfo\":{\"winrate\":0.5,\"scoreMean\":0.0,\"ownership\":[0.1,-0.1]},\"moveInfos\":[{\"move\":\"D4\",\"visits\":10,\"winrate\":0.5,\"scoreMean\":0.0,\"scoreStdev\":4.2,\"policy\":0.12,\"pv\":[\"D4\",\"Q16\"],\"pvVisits\":[10,5],\"ownership\":[0.1,-0.1]}]}\n${raw_katago_comparison_checklist_content}")
set(manual_ui_inspection_log_content "target manual UI inspection evidence fixture\nmainWindow4KHighDpiLayout\nboardLinesCoordinatesAndStarPoints\nstoneRenderingAndCandidateLabels\ncandidateTableColumns\npvPreviewStones\nownershipMainBoardOverlay\nownershipMiniBoardDock\nwinrateScoreGraph\ntoolbarDockAndMenuLayout\nbilingualTextFit\nsavedWindowRestore\nmultiDisplayPlacement\n")
set(external_verification_log_content "target external verification evidence fixture\nlinuxKdeWaylandNvidiaRealtimeKataGo\nwindowsNvidiaRealtimeKataGo\nphysical4KHighDpiMultiDisplayUi\nrawKataGoUiComparison\n")
set(record_screenshot_content "P3\n2 1\n255\n255 255 255 0 0 0\n")
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
string(SHA256 record_diagnostics_sha256 "${record_diagnostics_content}")
string(SHA256 record_report_sha256 "${record_report_content}")
string(SHA256 record_engine_log_sha256 "${record_engine_log_content}")
string(SHA256 record_raw_log_sha256 "${record_raw_log_content}")
string(SHA256 record_manual_ui_log_sha256 "${manual_ui_inspection_log_content}")
string(SHA256 record_external_verification_log_sha256 "${external_verification_log_content}")
string(SHA256 record_screenshot_sha256 "${record_screenshot_content}")
file(WRITE "${record_4k_screenshot_path}" "P2\n3840 2160\n1\n0 ")
string(REPEAT "1 " 3839 record_4k_screenshot_first_row_tail)
file(APPEND "${record_4k_screenshot_path}" "${record_4k_screenshot_first_row_tail}\n")
string(REPEAT "1 " 3840 record_4k_screenshot_row)
foreach(record_4k_screenshot_row_index RANGE 2 2160)
    file(APPEND "${record_4k_screenshot_path}" "${record_4k_screenshot_row}\n")
endforeach()
file(SHA256 "${record_4k_screenshot_path}" record_4k_screenshot_sha256)

file(WRITE "${model_path}" "diagnostics model fixture\n")
file(WRITE "${analysis_config_path}" "diagnostics analysis config fixture\n")
file(WRITE "${gtp_config_path}" "diagnostics gtp config fixture\n")
file(WRITE "${compare_model_path}" "diagnostics compare model fixture\n")
file(WRITE "${compare_analysis_config_path}" "diagnostics compare analysis config fixture\n")
file(WRITE "${compare_gtp_config_path}" "diagnostics compare gtp config fixture\n")
file(WRITE "${session_sgf_path}" "(;GM[1]FF[4]SZ[9];B[dd];W[ee])\n")
file(WRITE "${black_stone_texture_path}" "diagnostics black stone texture fixture\n")
file(WRITE "${white_stone_texture_path}" "diagnostics white stone texture fixture\n")
file(WRITE "${record_diagnostics_path}" "${record_diagnostics_content}")
file(WRITE "${record_report_path}" "${record_report_content}")
file(WRITE "${record_engine_log_path}" "${record_engine_log_content}")
file(WRITE "${record_raw_log_path}" "${record_raw_log_content}")
file(WRITE "${record_manual_ui_log_path}" "${manual_ui_inspection_log_content}")
file(WRITE "${record_external_verification_log_path}" "${external_verification_log_content}")
file(WRITE "${record_screenshot_path}" "${record_screenshot_content}")
string(TIMESTAMP acceptance_completed_utc "%Y-%m-%dT%H:%M:%SZ" UTC)
file(MAKE_DIRECTORY "${engine_working_dir}")
file(MAKE_DIRECTORY "${compare_working_dir}")
file(WRITE "${kwin_drm_device_0}" "diagnostics drm fixture 0\n")
file(WRITE "${kwin_drm_device_1}" "diagnostics drm fixture 1\n")
file(WRITE "${vulkan_icd_0}" "diagnostics vulkan icd fixture 0\n")
file(WRITE "${vulkan_icd_1}" "diagnostics vulkan icd fixture 1\n")
file(WRITE "${vulkan_driver_0}" "diagnostics vulkan driver fixture 0\n")
file(WRITE "${vulkan_driver_1}" "diagnostics vulkan driver fixture 1\n")
file(WRITE "${egl_vendor_0}" "diagnostics egl vendor fixture 0\n")
file(WRITE "${egl_vendor_1}" "diagnostics egl vendor fixture 1\n")
file(MAKE_DIRECTORY "${runtime_path_dir_0}")
file(MAKE_DIRECTORY "${runtime_path_dir_1}")
file(MAKE_DIRECTORY "${runtime_library_dir_0}")
file(MAKE_DIRECTORY "${runtime_library_dir_1}")
file(MAKE_DIRECTORY "${runtime_dyld_dir_0}")
file(MAKE_DIRECTORY "${runtime_dyld_dir_1}")
file(MAKE_DIRECTORY "${qml_import_dir_0}")
file(MAKE_DIRECTORY "${qml_import_dir_1}")
file(MAKE_DIRECTORY "${qml2_import_dir_0}")
file(MAKE_DIRECTORY "${qml2_import_dir_1}")
file(MAKE_DIRECTORY "${quick_controls_style_dir_0}")
file(MAKE_DIRECTORY "${quick_controls_style_dir_1}")
file(WRITE "${quick_controls_conf_path}" "[Controls]\nStyle=Basic\n")
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

[acceptance]
linuxKdeWaylandNvidia=pass
windowsNvidia=accepted
physicalDisplay=yes
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
all_pass_checklists(expected_metadata_template_combined acceptance_record_complete_checklists)
file(WRITE "${acceptance_record_complete_file}" "
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
notes=all target evidence and checklist records attached

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
windowsNvidia=accepted
physicalDisplay=yes
rawKataGoComparison=true
manualUiInspection=passed
${acceptance_record_complete_checklists}")
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
")
file(SHA256 "${acceptance_record_file}" acceptance_record_sha256)
file(WRITE "${settings_dir}/LizzieYzy Qt.conf" "
[startup]
firstRunComplete=true

[engine]
executable=${APP_EXECUTABLE}
model=${model_path}
gtpConfig=${gtp_config_path}
analysisConfig=${analysis_config_path}
workingDirectory=${engine_working_dir}
extraArgs=--main-flag 123

[compare]
executable=${APP_EXECUTABLE}
model=${compare_model_path}
gtpConfig=${compare_gtp_config_path}
analysisConfig=${compare_analysis_config_path}
workingDirectory=${compare_working_dir}
extraArgs=--compare-flag 456

[analysis]
intervalCentiseconds=250
includeOwnership=false
maxVisits=123
maxPlayouts=456
maxTimeSeconds=7.5
playoutDoublingAdvantage=-1.25
analysisWideRootNoise=0.35

[appearance]
theme=dark
language=zh
fontScale=1.25

[board]
showOwnership=false
ownershipDisplayMode=both
ownershipOpacity=0.7
blackStoneTexture=${black_stone_texture_path}
whiteStoneTexture=${white_stone_texture_path}

[files]
importLegacySgfAnalysis=false
loadAnalysisSidecar=false
writeAnalysisSidecarAfterBatch=false

[shortcuts]
new=Ctrl+Alt+N
open=Ctrl+Alt+O
save=Ctrl+Alt+S
saveAs=Ctrl+Shift+S
analyze=Ctrl+Alt+A
estimate=Ctrl+Alt+E
batch=Ctrl+Alt+B
restartEngine=Ctrl+Alt+R
compare=Ctrl+Alt+D
aiMove=Ctrl+Alt+G
humanVsAi=Ctrl+Alt+H
selfPlay=Ctrl+Alt+Shift+G
engineGame=Ctrl+Alt+Shift+E
previous=Alt+Left
next=Alt+Right
undo=Ctrl+Alt+Z
pass=
resign=Ctrl+Alt+P
settings=Ctrl+Alt+Comma

[session]
lastFile=${session_sgf_path}
currentNodeId=2
collectionGameIndex=1
")

set(kwin_drm_devices "${kwin_drm_device_0}" "${kwin_drm_device_1}")
set(vulkan_icd_files "${vulkan_icd_0}" "${vulkan_icd_1}")
set(vulkan_driver_files "${vulkan_driver_0}" "${vulkan_driver_1}")
set(egl_vendor_files "${egl_vendor_0}" "${egl_vendor_1}")
set(blank_path_list_entry "   ")
set(runtime_path_entries "${runtime_path_dir_0}" "${blank_path_list_entry}" "${runtime_path_dir_1}")
if(DEFINED ENV{PATH} AND NOT "$ENV{PATH}" STREQUAL "")
    cmake_path(CONVERT "$ENV{PATH}" TO_CMAKE_PATH_LIST original_path_entries)
    list(APPEND runtime_path_entries ${original_path_entries})
endif()
set(runtime_library_path_entries "${runtime_library_dir_0}" "${runtime_library_dir_1}")
if(DEFINED ENV{LD_LIBRARY_PATH} AND NOT "$ENV{LD_LIBRARY_PATH}" STREQUAL "")
    cmake_path(CONVERT "$ENV{LD_LIBRARY_PATH}" TO_CMAKE_PATH_LIST original_library_path_entries)
    list(APPEND runtime_library_path_entries ${original_library_path_entries})
endif()
set(runtime_dyld_path_entries "${runtime_dyld_dir_0}" "${runtime_dyld_dir_1}")
if(DEFINED ENV{DYLD_LIBRARY_PATH} AND NOT "$ENV{DYLD_LIBRARY_PATH}" STREQUAL "")
    cmake_path(CONVERT "$ENV{DYLD_LIBRARY_PATH}" TO_CMAKE_PATH_LIST original_dyld_path_entries)
    list(APPEND runtime_dyld_path_entries ${original_dyld_path_entries})
endif()
set(qml_import_path_entries "${qml_import_dir_0}" "${qml_import_dir_1}")
set(qml2_import_path_entries "${qml2_import_dir_0}" "${qml2_import_dir_1}")
set(quick_controls_style_path_entries "${quick_controls_style_dir_0}" "${quick_controls_style_dir_1}")
cmake_path(CONVERT "${kwin_drm_devices}" TO_NATIVE_PATH_LIST kwin_drm_devices_env)
cmake_path(CONVERT "${vulkan_icd_files}" TO_NATIVE_PATH_LIST vulkan_icd_files_env)
cmake_path(CONVERT "${vulkan_driver_files}" TO_NATIVE_PATH_LIST vulkan_driver_files_env)
cmake_path(CONVERT "${egl_vendor_files}" TO_NATIVE_PATH_LIST egl_vendor_files_env)
cmake_path(CONVERT "${runtime_path_entries}" TO_NATIVE_PATH_LIST runtime_path_env)
cmake_path(CONVERT "${runtime_library_path_entries}" TO_NATIVE_PATH_LIST runtime_library_path_env)
cmake_path(CONVERT "${runtime_dyld_path_entries}" TO_NATIVE_PATH_LIST runtime_dyld_path_env)
cmake_path(CONVERT "${qml_import_path_entries}" TO_NATIVE_PATH_LIST qml_import_path_env)
cmake_path(CONVERT "${qml2_import_path_entries}" TO_NATIVE_PATH_LIST qml2_import_path_env)
cmake_path(CONVERT "${quick_controls_style_path_entries}" TO_NATIVE_PATH_LIST quick_controls_style_path_env)
string(REPLACE ";" "\\;" kwin_drm_devices_env_arg "${kwin_drm_devices_env}")
string(REPLACE ";" "\\;" vulkan_icd_files_env_arg "${vulkan_icd_files_env}")
string(REPLACE ";" "\\;" vulkan_driver_files_env_arg "${vulkan_driver_files_env}")
string(REPLACE ";" "\\;" egl_vendor_files_env_arg "${egl_vendor_files_env}")
string(REPLACE ";" "\\;" runtime_path_env_arg "${runtime_path_env}")
string(REPLACE ";" "\\;" runtime_library_path_env_arg "${runtime_library_path_env}")
string(REPLACE ";" "\\;" runtime_dyld_path_env_arg "${runtime_dyld_path_env}")
string(REPLACE ";" "\\;" qml_import_path_env_arg "${qml_import_path_env}")
string(REPLACE ";" "\\;" qml2_import_path_env_arg "${qml2_import_path_env}")
string(REPLACE ";" "\\;" quick_controls_style_path_env_arg "${quick_controls_style_path_env}")

execute_process(
    COMMAND
        "${CMAKE_COMMAND}" -E env
        "--unset=LIZZIE_TARGET_ACCEPTANCE_RECORD_FILE"
        "QT_QPA_PLATFORM=offscreen"
        "QT_QPA_PLATFORM_PLUGIN_PATH="
        "PATH=${runtime_path_env_arg}"
        "LD_LIBRARY_PATH=${runtime_library_path_env_arg}"
        "DYLD_LIBRARY_PATH=${runtime_dyld_path_env_arg}"
        "QML_IMPORT_PATH=${qml_import_path_env_arg}"
        "QML2_IMPORT_PATH=${qml2_import_path_env_arg}"
        "QT_QUICK_CONTROLS_CONF=${quick_controls_conf_path}"
        "QT_QUICK_CONTROLS_STYLE_PATH=${quick_controls_style_path_env_arg}"
        "HOME=${home_dir}"
        "USERPROFILE=${user_profile_dir}"
        "APPDATA=${appdata_dir}"
        "LOCALAPPDATA=${localappdata_dir}"
        "XDG_CONFIG_HOME=${settings_home}"
        "XDG_DATA_HOME=${xdg_data_home}"
        "XDG_CACHE_HOME=${xdg_cache_home}"
        "XDG_RUNTIME_DIR=${xdg_runtime_dir}"
        "TEMP=${temp_dir}"
        "TMP=${tmp_dir}"
        "LIZZIE_DIAGNOSTICS_SETTINGS_FILE=${settings_dir}/LizzieYzy Qt.conf"
        "QT_PLUGIN_PATH=${blank_path_list_entry}"
        "KWIN_DRM_DEVICES=${kwin_drm_devices_env_arg}"
        "VK_ICD_FILENAMES=${vulkan_icd_files_env_arg}"
        "VK_DRIVER_FILES=${vulkan_driver_files_env_arg}"
        "__EGL_VENDOR_LIBRARY_FILENAMES=${egl_vendor_files_env_arg}"
        "LIZZIE_KATAGO_EXECUTABLE=${APP_EXECUTABLE}"
        "LIZZIE_KATAGO_MODEL=${model_path}"
        "LIZZIE_KATAGO_ANALYSIS_CONFIG=${analysis_config_path}"
        "LIZZIE_KATAGO_GTP_CONFIG=${gtp_config_path}"
        "${APP_EXECUTABLE}" --diagnostics
    WORKING_DIRECTORY "${work_dir}"
    RESULT_VARIABLE diagnostics_result
    OUTPUT_VARIABLE diagnostics_output
    ERROR_VARIABLE diagnostics_error
)

set(diagnostics_combined "${diagnostics_output}${diagnostics_error}")

if(NOT diagnostics_result EQUAL 0)
    message(FATAL_ERROR "Diagnostics command failed: ${diagnostics_combined}")
endif()

function(require_diagnostics_output pattern description)
    if(NOT "${diagnostics_combined}" MATCHES "${pattern}")
        message(FATAL_ERROR "Diagnostics output did not ${description}: ${diagnostics_combined}")
    endif()
endfunction()

require_diagnostics_output("LizzieYzy Qt Diagnostics" "print its header")
require_diagnostics_output("app\\.executable: " "report the application executable")
require_diagnostics_output("app\\.executable\\.exists: true" "validate that the application executable exists")
require_diagnostics_output("app\\.executable\\.file: true" "validate that the application executable is a file")
require_diagnostics_output("app\\.executable\\.executable: true" "validate that the application executable is runnable")
require_diagnostics_output("app\\.executable\\.size: [0-9]+" "report the application executable size")
require_diagnostics_output("app\\.executable\\.lastModifiedUtc: " "report the application executable modification time")
require_diagnostics_output("app\\.dir: " "report the application directory")
require_diagnostics_output("app\\.dir\\.exists: true" "validate that the application directory exists")
require_diagnostics_output("app\\.dir\\.dir: true" "validate that the application directory is a directory")
require_diagnostics_output("app\\.dir\\.size: -?[0-9]+" "report the application directory size")
require_diagnostics_output("app\\.dir\\.lastModifiedUtc: " "report the application directory modification time")
require_diagnostics_output("qt\\.runtime\\.appLocal\\.dir: " "report the app-local Qt runtime directory")
require_diagnostics_output("qt\\.runtime\\.appLocal\\.dir\\.exists: true" "validate the app-local runtime directory")
require_diagnostics_output("qt\\.runtime\\.appLocal\\.dir\\.dir: true" "classify the app-local runtime directory")
require_diagnostics_output(
    "qt\\.runtime\\.appLocal\\.expectedLibrary\\.count: 8"
    "report expected app-local Qt runtime library count")
require_diagnostics_output(
    "qt\\.runtime\\.appLocal\\.expectedLibrary\\.0\\.module: Core"
    "report the app-local Qt Core runtime candidate")
require_diagnostics_output(
    "qt\\.runtime\\.appLocal\\.expectedLibrary\\.0\\.fileName: "
    "report the app-local Qt Core runtime filename")
require_diagnostics_output(
    "qt\\.runtime\\.appLocal\\.expectedLibrary\\.0\\.path: "
    "report the app-local Qt Core runtime path")
require_diagnostics_output(
    "qt\\.runtime\\.appLocal\\.expectedLibrary\\.0\\.exists: (true|false)"
    "report whether the app-local Qt Core runtime exists")
require_diagnostics_output(
    "qt\\.runtime\\.appLocal\\.expectedLibrary\\.0\\.size: -?[0-9]+"
    "report the app-local Qt Core runtime size")
require_diagnostics_output(
    "qt\\.runtime\\.appLocal\\.expectedLibrary\\.7\\.module: OpenGL"
    "report the app-local Qt OpenGL runtime candidate")
require_diagnostics_output(
    "qt\\.runtime\\.appLocal\\.platformPlugin\\.count: 6"
    "report expected app-local platform plugin count")
require_diagnostics_output(
    "qt\\.runtime\\.appLocal\\.platformPlugin\\.0\\.baseName: qwindows"
    "report the app-local Windows platform plugin candidate")
require_diagnostics_output(
    "qt\\.runtime\\.appLocal\\.platformPlugin\\.0\\.path: "
    "report the app-local Windows platform plugin path")
require_diagnostics_output(
    "qt\\.runtime\\.appLocal\\.platformPlugin\\.0\\.exists: (true|false)"
    "report whether the app-local Windows platform plugin exists")
require_diagnostics_output(
    "qt\\.runtime\\.appLocal\\.platformPlugin\\.5\\.baseName: qoffscreen"
    "report the app-local offscreen platform plugin candidate")
require_diagnostics_output("process\\.currentWorkingDirectory: " "report the process working directory")
require_diagnostics_output(
    "process\\.currentWorkingDirectory\\.exists: true"
    "validate the process working directory")
require_diagnostics_output(
    "process\\.currentWorkingDirectory\\.dir: true"
    "classify the process working directory")
require_diagnostics_output(
    "process\\.currentWorkingDirectory\\.writable: true"
    "validate the process working directory is writable")
require_diagnostics_output("qt\\.version: " "report the Qt runtime version")
require_diagnostics_output("qt\\.buildVersion: " "report the Qt build version")
require_diagnostics_output("qt\\.runtimeVersion: " "report the Qt runtime version explicitly")
require_diagnostics_output(
    "qt\\.version\\.matchesBuild: (true|false)"
    "report whether Qt build and runtime versions match")
foreach(qt_install_path
        prefixPath
        binariesPath
        librariesPath
        libraryExecutablesPath
        pluginsPath
        qmlImportsPath
        archDataPath
        dataPath
        translationsPath)
    require_diagnostics_output("qt\\.install\\.${qt_install_path}: " "report Qt ${qt_install_path}")
    require_diagnostics_output(
        "qt\\.install\\.${qt_install_path}\\.absolutePath: "
        "report Qt ${qt_install_path} absolute path")
    require_diagnostics_output(
        "qt\\.install\\.${qt_install_path}\\.exists: (true|false)"
        "report whether Qt ${qt_install_path} exists")
    require_diagnostics_output(
        "qt\\.install\\.${qt_install_path}\\.size: -?[0-9]+"
        "report Qt ${qt_install_path} size")
    require_diagnostics_output(
        "qt\\.install\\.${qt_install_path}\\.lastModifiedUtc: "
        "report Qt ${qt_install_path} modification time")
endforeach()
require_diagnostics_output("qt\\.platform: " "report the Qt platform")
require_diagnostics_output("qt\\.pluginPath\\.exists: true" "validate the Qt plugin path")
require_diagnostics_output("qt\\.pluginPath\\.dir: true" "validate that the Qt plugin path is a directory")
require_diagnostics_output("qt\\.pluginPath\\.size: -?[0-9]+" "report the Qt plugin path size")
require_diagnostics_output("qt\\.pluginPath\\.lastModifiedUtc: " "report the Qt plugin path modification time")
require_diagnostics_output("qt\\.libraryPath\\.count: [0-9]+" "report Qt library paths")
require_diagnostics_output("qt\\.libraryPath\\.0\\.exists: true" "validate the first Qt library path")
require_diagnostics_output("qt\\.libraryPath\\.0\\.dir: true" "validate that the first Qt library path is a directory")
require_diagnostics_output("qt\\.libraryPath\\.0\\.size: -?[0-9]+" "report the first Qt library path size")
require_diagnostics_output("qt\\.libraryPath\\.0\\.lastModifiedUtc: " "report the first Qt library path modification time")
require_diagnostics_output("qt\\.standardPath\\.config: " "report the writable config location")
require_diagnostics_output(
    "qt\\.standardPath\\.config\\.absolutePath: "
    "report the writable config location absolute path")
require_diagnostics_output(
    "qt\\.standardPath\\.config\\.exists: (true|false)"
    "report whether the writable config location exists")
require_diagnostics_output(
    "qt\\.standardPath\\.config\\.dir: (true|false)"
    "report whether the writable config location is a directory")
require_diagnostics_output("qt\\.standardPath\\.config\\.size: -?[0-9]+" "report the writable config location size")
require_diagnostics_output(
    "qt\\.standardPath\\.config\\.lastModifiedUtc: "
    "report the writable config location modification time")
require_diagnostics_output("qt\\.standardPath\\.appConfig: " "report the writable app config location")
require_diagnostics_output(
    "qt\\.standardPath\\.appConfig\\.absolutePath: "
    "report the writable app config location absolute path")
require_diagnostics_output(
    "qt\\.standardPath\\.appConfig\\.exists: (true|false)"
    "report whether the writable app config location exists")
require_diagnostics_output(
    "qt\\.standardPath\\.appConfig\\.dir: (true|false)"
    "report whether the writable app config location is a directory")
require_diagnostics_output(
    "qt\\.standardPath\\.appConfig\\.size: -?[0-9]+"
    "report the writable app config location size")
require_diagnostics_output(
    "qt\\.standardPath\\.appConfig\\.lastModifiedUtc: "
    "report the writable app config location modification time")
require_diagnostics_output("qt\\.standardPath\\.genericConfig: " "report the writable generic config location")
require_diagnostics_output(
    "qt\\.standardPath\\.genericConfig\\.absolutePath: "
    "report the writable generic config location absolute path")
require_diagnostics_output(
    "qt\\.standardPath\\.genericConfig\\.exists: (true|false)"
    "report whether the writable generic config location exists")
require_diagnostics_output("qt\\.standardPath\\.genericData: " "report the writable generic data location")
require_diagnostics_output("qt\\.standardPath\\.appData: " "report the writable app data location")
require_diagnostics_output(
    "qt\\.standardPath\\.appData\\.absolutePath: "
    "report the writable app data location absolute path")
require_diagnostics_output(
    "qt\\.standardPath\\.appData\\.exists: (true|false)"
    "report whether the writable app data location exists")
require_diagnostics_output(
    "qt\\.standardPath\\.appData\\.dir: (true|false)"
    "report whether the writable app data location is a directory")
require_diagnostics_output("qt\\.standardPath\\.appData\\.size: -?[0-9]+" "report the writable app data location size")
require_diagnostics_output(
    "qt\\.standardPath\\.appData\\.lastModifiedUtc: "
    "report the writable app data location modification time")
require_diagnostics_output("qt\\.standardPath\\.genericCache: " "report the writable generic cache location")
require_diagnostics_output("qt\\.standardPath\\.cache: " "report the writable cache location")
require_diagnostics_output("qt\\.standardPath\\.runtime: " "report the writable runtime location")
require_diagnostics_output("qt\\.standardPath\\.runtime\\.exists: true" "validate the writable runtime location")
require_diagnostics_output("qt\\.standardPath\\.runtime\\.dir: true" "classify the writable runtime location")
require_diagnostics_output("qt\\.standardPath\\.runtime\\.writable: true" "validate the writable runtime location")
require_diagnostics_output("qt\\.standardPath\\.temp: " "report the writable temporary location")
require_diagnostics_output("qt\\.font\\.application\\.family: " "report the application font family")
require_diagnostics_output("qt\\.font\\.application\\.styleName: " "report the application font style name")
require_diagnostics_output("qt\\.font\\.application\\.pointSizeF: -?[0-9]+(\\.[0-9]+)?" "report the application font point size")
require_diagnostics_output("qt\\.font\\.application\\.pixelSize: -?[0-9]+" "report the application font pixel size")
require_diagnostics_output("qt\\.font\\.application\\.weight: [0-9]+" "report the application font weight")
require_diagnostics_output("qt\\.font\\.application\\.bold: (true|false)" "report whether the application font is bold")
require_diagnostics_output("qt\\.font\\.application\\.italic: (true|false)" "report whether the application font is italic")
require_diagnostics_output("qt\\.font\\.application\\.fixedPitch: (true|false)" "report whether the application font is fixed pitch")
require_diagnostics_output("qt\\.font\\.application\\.kerning: (true|false)" "report whether the application font uses kerning")
require_diagnostics_output("qt\\.font\\.application\\.hintingPreference: -?[0-9]+" "report the application font hinting preference")
require_diagnostics_output("qt\\.font\\.metrics\\.height: [0-9]+(\\.[0-9]+)?" "report the application font metrics height")
require_diagnostics_output("qt\\.font\\.metrics\\.lineSpacing: [0-9]+(\\.[0-9]+)?" "report the application font line spacing")
require_diagnostics_output("qt\\.font\\.metrics\\.ascent: [0-9]+(\\.[0-9]+)?" "report the application font ascent")
require_diagnostics_output("qt\\.font\\.metrics\\.descent: [0-9]+(\\.[0-9]+)?" "report the application font descent")
require_diagnostics_output(
    "qt\\.font\\.metrics\\.averageCharWidth: [0-9]+(\\.[0-9]+)?"
    "report the application font average character width")
require_diagnostics_output(
    "qt\\.font\\.metrics\\.sampleAsciiWidth: [0-9]+(\\.[0-9]+)?"
    "report English sample text width")
require_diagnostics_output(
    "qt\\.font\\.metrics\\.sampleCjkWidth: [0-9]+(\\.[0-9]+)?"
    "report CJK sample text width")
require_diagnostics_output("qt\\.appearance\\.style\\.objectName: " "report the application style object name")
require_diagnostics_output("qt\\.appearance\\.style\\.className: " "report the application style class name")
require_diagnostics_output(
    "qt\\.appearance\\.colorScheme: (Unknown|Light|Dark|ColorScheme\\([0-9]+\\))"
    "report the runtime application color scheme")
require_diagnostics_output(
    "qt\\.appearance\\.palette\\.window: #[0-9a-fA-F]+"
    "report the runtime window palette color")
require_diagnostics_output(
    "qt\\.appearance\\.palette\\.windowText: #[0-9a-fA-F]+"
    "report the runtime window-text palette color")
require_diagnostics_output(
    "qt\\.appearance\\.palette\\.base: #[0-9a-fA-F]+"
    "report the runtime base palette color")
require_diagnostics_output(
    "qt\\.appearance\\.palette\\.text: #[0-9a-fA-F]+"
    "report the runtime text palette color")
require_diagnostics_output(
    "qt\\.appearance\\.palette\\.button: #[0-9a-fA-F]+"
    "report the runtime button palette color")
require_diagnostics_output(
    "qt\\.appearance\\.palette\\.buttonText: #[0-9a-fA-F]+"
    "report the runtime button-text palette color")
require_diagnostics_output(
    "qt\\.appearance\\.palette\\.highlight: #[0-9a-fA-F]+"
    "report the runtime highlight palette color")
require_diagnostics_output(
    "qt\\.appearance\\.palette\\.highlightedText: #[0-9a-fA-F]+"
    "report the runtime highlighted-text palette color")
require_diagnostics_output(
    "qt\\.appearance\\.palette\\.windowLooksDark: (true|false)"
    "report whether the runtime window palette looks dark")
require_diagnostics_output(
    "qt\\.appearance\\.styleSheet\\.present: (true|false)"
    "report whether a runtime application stylesheet is installed")
require_diagnostics_output(
    "qt\\.appearance\\.styleSheet\\.length: [0-9]+"
    "report the runtime application stylesheet length")
require_diagnostics_output(
    "qt\\.dialog\\.fileDialog\\.defaultDontUseNativeDialog: (true|false)"
    "report whether default file dialogs disable native dialogs")
require_diagnostics_output(
    "qt\\.dialog\\.fileDialog\\.defaultMayUseNativeDialog: (true|false)"
    "report whether default file dialogs may use native dialogs")
require_diagnostics_output(
    "qt\\.dialog\\.settings\\.pathBrowseButton\\.count: 12"
    "report Settings path browse button count")
require_diagnostics_output(
    "qt\\.dialog\\.settings\\.pathBrowseButton\\.file\\.count: 10"
    "report Settings file-selector browse button count")
require_diagnostics_output(
    "qt\\.dialog\\.settings\\.pathBrowseButton\\.directory\\.count: 2"
    "report Settings directory-selector browse button count")
require_diagnostics_output(
    "qt\\.dialog\\.settings\\.pathBrowseButton\\.[0-9]+\\.rowKey: katago"
    "report Settings KataGo executable browse route")
require_diagnostics_output(
    "qt\\.dialog\\.settings\\.pathBrowseButton\\.[0-9]+\\.rowKey: workDir"
    "report Settings main working-directory browse route")
require_diagnostics_output(
    "qt\\.dialog\\.settings\\.pathBrowseButton\\.[0-9]+\\.rowKey: compareWorkDir"
    "report Settings compare working-directory browse route")
require_diagnostics_output(
    "qt\\.dialog\\.settings\\.pathBrowseButton\\.[0-9]+\\.selectorKind: selectFile"
    "report Settings file selector routing")
require_diagnostics_output(
    "qt\\.dialog\\.settings\\.pathBrowseButton\\.[0-9]+\\.selectorKind: selectDirectory"
    "report Settings directory selector routing")
require_diagnostics_output("qt\\.ui\\.mainWindow\\.windowTitle: LizzieYzy Qt" "report the main window title")
require_diagnostics_output(
    "qt\\.ui\\.mainWindow\\.centralWidget\\.className: QQuickWidget"
    "report the main board Quick central widget")
require_diagnostics_output(
    "qt\\.ui\\.mainWindow\\.centralWidget\\.objectName: lizzieMainBoardQuickWidget"
    "report the main board Quick widget object name")
require_diagnostics_output(
    "qt\\.ui\\.mainWindow\\.quickWidget\\.count: 2"
    "report main and ownership Quick widget count")
require_diagnostics_output(
    "qt\\.ui\\.mainWindow\\.quickWidget\\.[0-9]+\\.objectName: lizzieMainBoardQuickWidget"
    "report the main board Quick widget")
require_diagnostics_output(
    "qt\\.ui\\.mainWindow\\.quickWidget\\.[0-9]+\\.objectName: lizzieOwnershipQuickWidget"
    "report the ownership board Quick widget")
require_diagnostics_output(
    "qt\\.ui\\.mainWindow\\.quickWidget\\.[0-9]+\\.rootObjectPresent: true"
    "report Quick widget root object presence")
require_diagnostics_output(
    "qt\\.ui\\.mainWindow\\.toolBar\\.count: 1"
    "report the main toolbar count")
require_diagnostics_output(
    "qt\\.ui\\.mainWindow\\.toolBar\\.[0-9]+\\.objectName: lizzieMainToolbar"
    "report the named main toolbar")
require_diagnostics_output(
    "qt\\.ui\\.mainWindow\\.toolBar\\.[0-9]+\\.action\\.count: 11"
    "report the main toolbar action count")
require_diagnostics_output(
    "qt\\.ui\\.mainWindow\\.toolBar\\.[0-9]+\\.commandAction\\.count: 8"
    "report the main toolbar command-action count")
require_diagnostics_output(
    "qt\\.ui\\.mainWindow\\.dock\\.count: 8"
    "report main-window dock count")
require_diagnostics_output(
    "qt\\.ui\\.mainWindow\\.dock\\.[0-9]+\\.objectName: lizzieCandidatesDock"
    "report the Candidates dock")
require_diagnostics_output(
    "qt\\.ui\\.mainWindow\\.dock\\.[0-9]+\\.objectName: lizzieOwnershipDock"
    "report the Ownership dock")
require_diagnostics_output(
    "qt\\.ui\\.mainWindow\\.dock\\.[0-9]+\\.objectName: lizzieCompareDock"
    "report the Compare dock")
require_diagnostics_output(
    "qt\\.ui\\.mainWindow\\.dock\\.[0-9]+\\.objectName: lizzieConsoleDock"
    "report the GTP Console dock")
require_diagnostics_output(
    "qt\\.ui\\.mainWindow\\.dock\\.[0-9]+\\.objectName: lizzieEngineLogDock"
    "report the Engine Log dock")
require_diagnostics_output(
    "qt\\.ui\\.mainWindow\\.dock\\.[0-9]+\\.objectName: lizzieGraphDock"
    "report the graph dock")
require_diagnostics_output(
    "qt\\.ui\\.mainWindow\\.dock\\.[0-9]+\\.objectName: lizzieTreeDock"
    "report the game-tree dock")
require_diagnostics_output(
    "qt\\.ui\\.mainWindow\\.table\\.count: 2"
    "report candidate and compare table count")
require_diagnostics_output(
    "qt\\.ui\\.mainWindow\\.table\\.[0-9]+\\.objectName: lizzieCandidatesTable"
    "report the candidate table")
require_diagnostics_output(
    "qt\\.ui\\.mainWindow\\.table\\.[0-9]+\\.objectName: lizzieCompareTable"
    "report the compare table")
require_diagnostics_output(
    "qt\\.ui\\.mainWindow\\.table\\.[0-9]+\\.column\\.count: 8"
    "report the candidate table column count")
require_diagnostics_output(
    "qt\\.ui\\.mainWindow\\.table\\.[0-9]+\\.column\\.count: 9"
    "report the compare table column count")
require_diagnostics_output(
    "qt\\.ui\\.mainWindow\\.tree\\.count: 1"
    "report the game tree count")
require_diagnostics_output(
    "qt\\.ui\\.mainWindow\\.tree\\.[0-9]+\\.objectName: lizzieGameTree"
    "report the named game tree")
require_diagnostics_output(
    "qt\\.ui\\.mainWindow\\.menu\\.count: 5"
    "report the main menu count")
require_diagnostics_output("qt\\.locale\\.default\\.name: " "report the default locale name")
require_diagnostics_output("qt\\.locale\\.default\\.bcp47Name: " "report the default locale BCP-47 name")
require_diagnostics_output("qt\\.locale\\.default\\.nativeLanguageName: " "report the default locale native language")
require_diagnostics_output("qt\\.locale\\.default\\.nativeTerritoryName: " "report the default locale native territory")
require_diagnostics_output(
    "qt\\.locale\\.default\\.textDirection: (LeftToRight|RightToLeft|LayoutDirectionAuto|LayoutDirection\\(-?[0-9]+\\))"
    "report the default locale text direction")
require_diagnostics_output("qt\\.locale\\.default\\.measurementSystem: -?[0-9]+" "report the default locale measurement system")
require_diagnostics_output("qt\\.locale\\.default\\.decimalPoint: " "report the default locale decimal point")
require_diagnostics_output("qt\\.locale\\.default\\.groupSeparator: " "report the default locale group separator")
require_diagnostics_output("qt\\.locale\\.default\\.firstDayOfWeek: -?[0-9]+" "report the default locale first day of week")
require_diagnostics_output("qt\\.locale\\.default\\.uiLanguage\\.count: [0-9]+" "report default locale UI languages")
require_diagnostics_output("qt\\.locale\\.system\\.name: " "report the system locale name")
require_diagnostics_output("qt\\.locale\\.system\\.bcp47Name: " "report the system locale BCP-47 name")
require_diagnostics_output("qt\\.locale\\.system\\.nativeLanguageName: " "report the system locale native language")
require_diagnostics_output("qt\\.locale\\.system\\.nativeTerritoryName: " "report the system locale native territory")
require_diagnostics_output(
    "qt\\.locale\\.system\\.textDirection: (LeftToRight|RightToLeft|LayoutDirectionAuto|LayoutDirection\\(-?[0-9]+\\))"
    "report the system locale text direction")
require_diagnostics_output("qt\\.locale\\.system\\.measurementSystem: -?[0-9]+" "report the system locale measurement system")
require_diagnostics_output("qt\\.locale\\.system\\.decimalPoint: " "report the system locale decimal point")
require_diagnostics_output("qt\\.locale\\.system\\.groupSeparator: " "report the system locale group separator")
require_diagnostics_output("qt\\.locale\\.system\\.firstDayOfWeek: -?[0-9]+" "report the system locale first day of week")
require_diagnostics_output("qt\\.locale\\.system\\.uiLanguage\\.count: [0-9]+" "report system locale UI languages")
require_diagnostics_output("qt\\.settings\\.organizationName: LizzieYzy" "report the QSettings organization name")
require_diagnostics_output("qt\\.settings\\.applicationName: LizzieYzy Qt" "report the QSettings application name")
require_diagnostics_output("qt\\.settings\\.defaultFormat: " "report the QSettings default format")
require_diagnostics_output("qt\\.settings\\.scope: UserScope" "report the QSettings user scope")
require_diagnostics_output("qt\\.settings\\.fileName: " "report the QSettings storage location")
require_diagnostics_output(
    "qt\\.settings\\.fileSystemBacked: (true|false)"
    "report whether the QSettings storage location is filesystem-backed")
require_diagnostics_output(
    "qt\\.settings\\.file\\.absolutePath: "
    "report the QSettings storage absolute path or non-filesystem marker")
require_diagnostics_output(
    "qt\\.settings\\.file\\.exists: (true|false)"
    "report whether the QSettings storage file exists")
require_diagnostics_output(
    "qt\\.settings\\.file\\.file: (true|false)"
    "report whether the QSettings storage location is a file")
require_diagnostics_output(
    "qt\\.settings\\.file\\.readable: (true|false)"
    "report whether the QSettings storage location is readable")
require_diagnostics_output(
    "qt\\.settings\\.file\\.writable: true"
    "validate that the diagnostics QSettings storage location is writable")
require_diagnostics_output("qt\\.settings\\.file\\.size: -?[0-9]+" "report the QSettings storage size")
require_diagnostics_output(
    "qt\\.settings\\.file\\.lastModifiedUtc: "
    "report the QSettings storage modification time or non-filesystem marker")
require_diagnostics_output(
    "qt\\.settings\\.startup\\.firstRunComplete\\.present: true"
    "report whether first-run completion is stored")
require_diagnostics_output(
    "qt\\.settings\\.startup\\.firstRunComplete\\.value: true"
    "report the stored first-run completion value")
require_diagnostics_output("qt\\.settings\\.engine\\.executable\\.present: true" "report stored main engine executable")
require_diagnostics_output("qt\\.settings\\.engine\\.executable\\.value: " "report main engine executable value")
require_diagnostics_output("qt\\.settings\\.engine\\.executable\\.path\\.exists: true" "validate main engine executable path")
require_diagnostics_output("qt\\.settings\\.engine\\.executable\\.path\\.file: true" "classify main engine executable path")
require_diagnostics_output(
    "qt\\.settings\\.engine\\.executable\\.path\\.executable: true"
    "validate main engine executable path executability")
require_diagnostics_output("qt\\.settings\\.engine\\.model\\.present: true" "report stored main engine model")
require_diagnostics_output("qt\\.settings\\.engine\\.model\\.path\\.exists: true" "validate main engine model path")
require_diagnostics_output("qt\\.settings\\.engine\\.gtpConfig\\.present: true" "report stored main engine GTP config")
require_diagnostics_output("qt\\.settings\\.engine\\.gtpConfig\\.path\\.exists: true" "validate main engine GTP config path")
require_diagnostics_output(
    "qt\\.settings\\.engine\\.analysisConfig\\.present: true"
    "report stored main engine analysis config")
require_diagnostics_output(
    "qt\\.settings\\.engine\\.analysisConfig\\.path\\.exists: true"
    "validate main engine analysis config path")
require_diagnostics_output(
    "qt\\.settings\\.engine\\.workingDirectory\\.present: true"
    "report stored main engine working directory")
require_diagnostics_output(
    "qt\\.settings\\.engine\\.workingDirectory\\.path\\.dir: true"
    "validate main engine working directory")
require_diagnostics_output("qt\\.settings\\.engine\\.extraArgs\\.present: true" "report stored main engine extra args")
require_diagnostics_output("qt\\.settings\\.engine\\.extraArgs\\.value: --main-flag 123" "report main engine extra args value")
require_diagnostics_output("qt\\.settings\\.engine\\.extraArgs\\.parsed\\.count: 2" "report parsed main extra arg count")
require_diagnostics_output("qt\\.settings\\.engine\\.extraArgs\\.parsed\\.0: --main-flag" "report parsed main extra arg 0")
require_diagnostics_output("qt\\.settings\\.engine\\.extraArgs\\.parsed\\.1: 123" "report parsed main extra arg 1")
require_diagnostics_output("qt\\.settings\\.engine\\.hasGtpMinimum: true" "report main GTP minimum readiness")
require_diagnostics_output("qt\\.settings\\.engine\\.hasAnalysisMinimum: true" "report main analysis minimum readiness")
require_diagnostics_output("qt\\.settings\\.engine\\.config\\.complete: true" "report complete main engine config")
require_diagnostics_output("qt\\.settings\\.engine\\.config\\.ready: true" "report ready main engine config")
require_diagnostics_output("qt\\.settings\\.engine\\.config\\.gtpReady: true" "report ready main GTP config")
require_diagnostics_output(
    "qt\\.settings\\.engine\\.config\\.analysisReady: true"
    "report ready main analysis config")
require_diagnostics_output("qt\\.settings\\.engine\\.config\\.status: ready" "report ready main config status")
require_diagnostics_output("qt\\.settings\\.engine\\.config\\.missing\\.count: 0" "report no missing main config paths")
require_diagnostics_output("qt\\.settings\\.engine\\.config\\.invalid\\.count: 0" "report no invalid main config paths")
require_diagnostics_output("qt\\.settings\\.compare\\.executable\\.present: true" "report stored compare engine executable")
require_diagnostics_output("qt\\.settings\\.compare\\.executable\\.path\\.exists: true" "validate compare engine executable path")
require_diagnostics_output("qt\\.settings\\.compare\\.model\\.present: true" "report stored compare engine model")
require_diagnostics_output("qt\\.settings\\.compare\\.model\\.path\\.exists: true" "validate compare engine model path")
require_diagnostics_output("qt\\.settings\\.compare\\.gtpConfig\\.present: true" "report stored compare engine GTP config")
require_diagnostics_output(
    "qt\\.settings\\.compare\\.gtpConfig\\.path\\.exists: true"
    "validate compare engine GTP config path")
require_diagnostics_output(
    "qt\\.settings\\.compare\\.analysisConfig\\.present: true"
    "report stored compare engine analysis config")
require_diagnostics_output(
    "qt\\.settings\\.compare\\.analysisConfig\\.path\\.exists: true"
    "validate compare engine analysis config path")
require_diagnostics_output(
    "qt\\.settings\\.compare\\.workingDirectory\\.present: true"
    "report stored compare engine working directory")
require_diagnostics_output(
    "qt\\.settings\\.compare\\.workingDirectory\\.path\\.dir: true"
    "validate compare engine working directory")
require_diagnostics_output("qt\\.settings\\.compare\\.extraArgs\\.present: true" "report stored compare engine extra args")
require_diagnostics_output(
    "qt\\.settings\\.compare\\.extraArgs\\.value: --compare-flag 456"
    "report compare engine extra args value")
require_diagnostics_output("qt\\.settings\\.compare\\.extraArgs\\.parsed\\.count: 2" "report parsed compare extra arg count")
require_diagnostics_output("qt\\.settings\\.compare\\.extraArgs\\.parsed\\.0: --compare-flag" "report parsed compare extra arg 0")
require_diagnostics_output("qt\\.settings\\.compare\\.extraArgs\\.parsed\\.1: 456" "report parsed compare extra arg 1")
require_diagnostics_output("qt\\.settings\\.compare\\.hasGtpMinimum: true" "report compare GTP minimum readiness")
require_diagnostics_output("qt\\.settings\\.compare\\.hasAnalysisMinimum: true" "report compare analysis minimum readiness")
require_diagnostics_output("qt\\.settings\\.compare\\.config\\.complete: true" "report complete compare engine config")
require_diagnostics_output("qt\\.settings\\.compare\\.config\\.ready: true" "report ready compare engine config")
require_diagnostics_output("qt\\.settings\\.compare\\.config\\.gtpReady: true" "report ready compare GTP config")
require_diagnostics_output(
    "qt\\.settings\\.compare\\.config\\.analysisReady: true"
    "report ready compare analysis config")
require_diagnostics_output("qt\\.settings\\.compare\\.config\\.status: ready" "report ready compare config status")
require_diagnostics_output("qt\\.settings\\.compare\\.config\\.missing\\.count: 0" "report no missing compare config paths")
require_diagnostics_output("qt\\.settings\\.compare\\.config\\.invalid\\.count: 0" "report no invalid compare config paths")
require_diagnostics_output(
    "qt\\.settings\\.analysis\\.intervalCentiseconds\\.value: 250"
    "report stored analysis interval")
require_diagnostics_output("qt\\.settings\\.analysis\\.includeOwnership\\.value: false" "report stored ownership setting")
require_diagnostics_output("qt\\.settings\\.analysis\\.maxVisits\\.value: 123" "report stored max visits")
require_diagnostics_output("qt\\.settings\\.analysis\\.maxPlayouts\\.value: 456" "report stored max playouts")
require_diagnostics_output("qt\\.settings\\.analysis\\.maxTimeSeconds\\.value: 7\\.5" "report stored max time")
require_diagnostics_output(
    "qt\\.settings\\.analysis\\.playoutDoublingAdvantage\\.value: -1\\.25"
    "report stored playout doubling advantage")
require_diagnostics_output(
    "qt\\.settings\\.analysis\\.analysisWideRootNoise\\.value: 0\\.35"
    "report stored wide root noise")
require_diagnostics_output("qt\\.settings\\.board\\.showOwnership\\.present: true" "report stored ownership toggle")
require_diagnostics_output("qt\\.settings\\.board\\.showOwnership\\.value: false" "report stored ownership toggle value")
require_diagnostics_output(
    "qt\\.settings\\.board\\.ownershipDisplayMode\\.present: true"
    "report stored ownership display mode")
require_diagnostics_output(
    "qt\\.settings\\.board\\.ownershipDisplayMode\\.value: both"
    "report stored ownership display mode value")
require_diagnostics_output(
    "qt\\.settings\\.board\\.ownershipOpacity\\.present: true"
    "report stored ownership opacity")
require_diagnostics_output(
    "qt\\.settings\\.board\\.ownershipOpacity\\.value: 0\\.7"
    "report stored ownership opacity value")
require_diagnostics_output(
    "qt\\.settings\\.board\\.blackStoneTexture\\.present: true"
    "report stored black stone texture path")
require_diagnostics_output(
    "qt\\.settings\\.board\\.blackStoneTexture\\.value: .*diagnostics-black-stone\\.png"
    "report stored black stone texture value")
require_diagnostics_output(
    "qt\\.settings\\.board\\.blackStoneTexture\\.path\\.exists: true"
    "validate black stone texture path")
require_diagnostics_output(
    "qt\\.settings\\.board\\.blackStoneTexture\\.path\\.file: true"
    "classify black stone texture path")
require_diagnostics_output(
    "qt\\.settings\\.board\\.blackStoneTexture\\.path\\.readable: true"
    "validate black stone texture readability")
require_diagnostics_output(
    "qt\\.settings\\.board\\.blackStoneTexture\\.path\\.size: [0-9]+"
    "report black stone texture size")
require_diagnostics_output(
    "qt\\.settings\\.board\\.blackStoneTexture\\.path\\.lastModifiedUtc: "
    "report black stone texture modification time")
require_diagnostics_output(
    "qt\\.settings\\.board\\.whiteStoneTexture\\.present: true"
    "report stored white stone texture path")
require_diagnostics_output(
    "qt\\.settings\\.board\\.whiteStoneTexture\\.value: .*diagnostics-white-stone\\.png"
    "report stored white stone texture value")
require_diagnostics_output(
    "qt\\.settings\\.board\\.whiteStoneTexture\\.path\\.exists: true"
    "validate white stone texture path")
require_diagnostics_output(
    "qt\\.settings\\.board\\.whiteStoneTexture\\.path\\.file: true"
    "classify white stone texture path")
require_diagnostics_output(
    "qt\\.settings\\.board\\.whiteStoneTexture\\.path\\.readable: true"
    "validate white stone texture readability")
require_diagnostics_output(
    "qt\\.settings\\.board\\.whiteStoneTexture\\.path\\.size: [0-9]+"
    "report white stone texture size")
require_diagnostics_output(
    "qt\\.settings\\.board\\.whiteStoneTexture\\.path\\.lastModifiedUtc: "
    "report white stone texture modification time")
require_diagnostics_output(
    "qt\\.settings\\.files\\.importLegacySgfAnalysis\\.present: true"
    "report legacy SGF import behavior")
require_diagnostics_output(
    "qt\\.settings\\.files\\.importLegacySgfAnalysis\\.value: false"
    "report legacy SGF import behavior value")
require_diagnostics_output(
    "qt\\.settings\\.files\\.loadAnalysisSidecar\\.present: true"
    "report sidecar load behavior")
require_diagnostics_output(
    "qt\\.settings\\.files\\.loadAnalysisSidecar\\.value: false"
    "report sidecar load behavior value")
require_diagnostics_output(
    "qt\\.settings\\.files\\.writeAnalysisSidecarAfterBatch\\.present: true"
    "report sidecar write behavior")
require_diagnostics_output(
    "qt\\.settings\\.files\\.writeAnalysisSidecarAfterBatch\\.value: false"
    "report sidecar write behavior value")
require_diagnostics_output("qt\\.settings\\.shortcuts\\.new\\.present: true" "report stored New shortcut")
require_diagnostics_output("qt\\.settings\\.shortcuts\\.new\\.value: Ctrl\\+Alt\\+N" "report stored New shortcut value")
require_diagnostics_output("qt\\.settings\\.shortcuts\\.open\\.present: true" "report stored Open shortcut")
require_diagnostics_output("qt\\.settings\\.shortcuts\\.open\\.value: Ctrl\\+Alt\\+O" "report stored Open shortcut value")
require_diagnostics_output("qt\\.settings\\.shortcuts\\.save\\.present: true" "report stored Save shortcut")
require_diagnostics_output("qt\\.settings\\.shortcuts\\.save\\.value: Ctrl\\+Alt\\+S" "report stored Save shortcut value")
require_diagnostics_output("qt\\.settings\\.shortcuts\\.saveAs\\.present: true" "report stored Save As shortcut")
require_diagnostics_output(
    "qt\\.settings\\.shortcuts\\.saveAs\\.value: Ctrl\\+Shift\\+S"
    "report stored Save As shortcut value")
require_diagnostics_output("qt\\.settings\\.shortcuts\\.analyze\\.present: true" "report stored Analyze shortcut")
require_diagnostics_output(
    "qt\\.settings\\.shortcuts\\.analyze\\.value: Ctrl\\+Alt\\+A"
    "report stored Analyze shortcut value")
require_diagnostics_output("qt\\.settings\\.shortcuts\\.estimate\\.present: true" "report stored Estimate shortcut")
require_diagnostics_output(
    "qt\\.settings\\.shortcuts\\.estimate\\.value: Ctrl\\+Alt\\+E"
    "report stored Estimate shortcut value")
require_diagnostics_output("qt\\.settings\\.shortcuts\\.batch\\.present: true" "report stored Batch shortcut")
require_diagnostics_output("qt\\.settings\\.shortcuts\\.batch\\.value: Ctrl\\+Alt\\+B" "report stored Batch shortcut value")
require_diagnostics_output(
    "qt\\.settings\\.shortcuts\\.restartEngine\\.present: true"
    "report stored Restart Engine shortcut")
require_diagnostics_output(
    "qt\\.settings\\.shortcuts\\.restartEngine\\.value: Ctrl\\+Alt\\+R"
    "report stored Restart Engine shortcut value")
require_diagnostics_output("qt\\.settings\\.shortcuts\\.compare\\.present: true" "report stored Compare shortcut")
require_diagnostics_output(
    "qt\\.settings\\.shortcuts\\.compare\\.value: Ctrl\\+Alt\\+D"
    "report stored Compare shortcut value")
require_diagnostics_output("qt\\.settings\\.shortcuts\\.aiMove\\.present: true" "report stored AI Move shortcut")
require_diagnostics_output(
    "qt\\.settings\\.shortcuts\\.aiMove\\.value: Ctrl\\+Alt\\+G"
    "report stored AI Move shortcut value")
require_diagnostics_output(
    "qt\\.settings\\.shortcuts\\.humanVsAi\\.present: true"
    "report stored Human vs AI shortcut")
require_diagnostics_output(
    "qt\\.settings\\.shortcuts\\.humanVsAi\\.value: Ctrl\\+Alt\\+H"
    "report stored Human vs AI shortcut value")
require_diagnostics_output(
    "qt\\.settings\\.shortcuts\\.selfPlay\\.present: true"
    "report stored Self Play shortcut")
require_diagnostics_output(
    "qt\\.settings\\.shortcuts\\.selfPlay\\.value: Ctrl\\+Alt\\+Shift\\+G"
    "report stored Self Play shortcut value")
require_diagnostics_output(
    "qt\\.settings\\.shortcuts\\.engineGame\\.present: true"
    "report stored Engine Game shortcut")
require_diagnostics_output(
    "qt\\.settings\\.shortcuts\\.engineGame\\.value: Ctrl\\+Alt\\+Shift\\+E"
    "report stored Engine Game shortcut value")
require_diagnostics_output(
    "qt\\.settings\\.shortcuts\\.previous\\.present: true"
    "report stored Previous shortcut")
require_diagnostics_output(
    "qt\\.settings\\.shortcuts\\.previous\\.value: Alt\\+Left"
    "report stored Previous shortcut value")
require_diagnostics_output("qt\\.settings\\.shortcuts\\.next\\.present: true" "report stored Next shortcut")
require_diagnostics_output("qt\\.settings\\.shortcuts\\.next\\.value: Alt\\+Right" "report stored Next shortcut value")
require_diagnostics_output("qt\\.settings\\.shortcuts\\.undo\\.present: true" "report stored Undo shortcut")
require_diagnostics_output("qt\\.settings\\.shortcuts\\.undo\\.value: Ctrl\\+Alt\\+Z" "report stored Undo shortcut value")
require_diagnostics_output("qt\\.settings\\.shortcuts\\.pass\\.present: true" "report stored disabled Pass shortcut")
require_diagnostics_output("qt\\.settings\\.shortcuts\\.pass\\.value: *\n" "report disabled Pass shortcut value")
require_diagnostics_output("qt\\.settings\\.shortcuts\\.resign\\.present: true" "report stored Resign shortcut")
require_diagnostics_output(
    "qt\\.settings\\.shortcuts\\.resign\\.value: Ctrl\\+Alt\\+P"
    "report stored Resign shortcut value")
require_diagnostics_output("qt\\.settings\\.shortcuts\\.settings\\.present: true" "report stored Settings shortcut")
require_diagnostics_output(
    "qt\\.settings\\.shortcuts\\.settings\\.value: Ctrl\\+Alt\\+Comma"
    "report stored Settings shortcut value")
require_diagnostics_output("qt\\.settings\\.appearance\\.theme\\.present: true" "report stored theme")
require_diagnostics_output("qt\\.settings\\.appearance\\.theme\\.value: dark" "report stored theme value")
require_diagnostics_output("qt\\.settings\\.appearance\\.language\\.present: true" "report stored language")
require_diagnostics_output("qt\\.settings\\.appearance\\.language\\.value: zh" "report stored language value")
require_diagnostics_output("qt\\.settings\\.appearance\\.fontScale\\.present: true" "report stored font scale")
require_diagnostics_output("qt\\.settings\\.appearance\\.fontScale\\.value: 1\\.25" "report stored font scale value")
require_diagnostics_output(
    "qt\\.settings\\.appearance\\.normalized\\.theme: dark"
    "report normalized stored theme")
require_diagnostics_output(
    "qt\\.settings\\.appearance\\.normalized\\.language: zh"
    "report normalized stored language")
require_diagnostics_output(
    "qt\\.settings\\.appearance\\.normalized\\.fontScale: 1\\.25"
    "report normalized stored font scale")
require_diagnostics_output(
    "qt\\.settings\\.session\\.lastFile\\.present: true"
    "report whether a session SGF path is stored")
require_diagnostics_output(
    "qt\\.settings\\.session\\.lastFile\\.value: .*diagnostics-session\\.sgf"
    "report the stored session SGF path")
require_diagnostics_output(
    "qt\\.settings\\.session\\.lastFile\\.path\\.absolutePath: "
    "report the stored session SGF absolute path")
require_diagnostics_output(
    "qt\\.settings\\.session\\.lastFile\\.path\\.canonicalPath: "
    "report the stored session SGF canonical path")
require_diagnostics_output(
    "qt\\.settings\\.session\\.lastFile\\.path\\.exists: true"
    "report whether the stored session SGF path exists")
require_diagnostics_output(
    "qt\\.settings\\.session\\.lastFile\\.path\\.file: true"
    "validate the stored session SGF path is a file")
require_diagnostics_output(
    "qt\\.settings\\.session\\.lastFile\\.path\\.readable: true"
    "validate the stored session SGF path is readable")
require_diagnostics_output(
    "qt\\.settings\\.session\\.lastFile\\.path\\.size: -?[0-9]+"
    "report the stored session SGF path size")
require_diagnostics_output(
    "qt\\.settings\\.session\\.lastFile\\.path\\.lastModifiedUtc: "
    "report the stored session SGF modification time")
require_diagnostics_output(
    "qt\\.settings\\.session\\.currentNodeId\\.present: true"
    "report whether a session current node is stored")
require_diagnostics_output(
    "qt\\.settings\\.session\\.currentNodeId\\.value: 2"
    "report the stored session current node")
require_diagnostics_output(
    "qt\\.settings\\.session\\.collectionGameIndex\\.present: true"
    "report whether a collection game index is stored")
require_diagnostics_output(
    "qt\\.settings\\.session\\.collectionGameIndex\\.value: 1"
    "report the stored collection game index")
require_diagnostics_output(
    "qt\\.settings\\.window\\.geometry\\.present: (true|false)"
    "report whether saved window geometry is stored")
require_diagnostics_output("qt\\.settings\\.window\\.geometry\\.size: [0-9]+" "report saved window geometry size")
require_diagnostics_output(
    "qt\\.settings\\.window\\.geometry\\.restoreSucceeded: (true|false)"
    "report whether saved window geometry can be restored")
require_diagnostics_output(
    "qt\\.settings\\.window\\.geometry\\.restoredGeometry: "
    "report the restored saved window geometry")
require_diagnostics_output(
    "qt\\.settings\\.window\\.geometry\\.restoredFrameGeometry: "
    "report the restored saved window frame geometry")
require_diagnostics_output(
    "qt\\.settings\\.window\\.geometry\\.visibleAreaOnAvailableScreens: [0-9]+"
    "report saved window geometry visible area")
require_diagnostics_output(
    "qt\\.settings\\.window\\.geometry\\.requiredVisibleArea: [0-9]+"
    "report saved window geometry required visible area")
require_diagnostics_output(
    "qt\\.settings\\.window\\.geometry\\.substantiallyVisible: (true|false)"
    "report whether saved window geometry is substantially visible")
require_diagnostics_output(
    "qt\\.settings\\.window\\.geometry\\.wouldRecenter: (true|false)"
    "report whether saved window geometry would be recentered")
require_diagnostics_output(
    "qt\\.settings\\.window\\.geometry\\.adjustedGeometry: "
    "report the adjusted saved window geometry")
require_diagnostics_output(
    "qt\\.settings\\.window\\.state\\.present: (true|false)"
    "report whether saved window state is stored")
require_diagnostics_output("qt\\.settings\\.window\\.state\\.size: [0-9]+" "report saved window state size")
require_diagnostics_output(
    "qt\\.settings\\.window\\.state\\.restoreSucceeded: (true|false)"
    "report whether saved window state can be restored")
require_diagnostics_output("qt\\.platformPlugin\\.current\\.name: " "report the current Qt platform plugin name")
require_diagnostics_output(
    "qt\\.platformPlugin\\.current\\.expectedFile\\.count: [1-9][0-9]*"
    "report expected platform plugin file names")
require_diagnostics_output(
    "qt\\.platformPlugin\\.current\\.candidate\\.count: [1-9][0-9]*"
    "report current platform plugin candidates")
require_diagnostics_output(
    "qt\\.platformPlugin\\.current\\.candidate\\.0\\.path: "
    "report the first current platform plugin candidate path")
require_diagnostics_output(
    "qt\\.platformPlugin\\.current\\.candidate\\.0\\.size: -?[0-9]+"
    "report the first current platform plugin candidate size")
require_diagnostics_output(
    "qt\\.platformPlugin\\.current\\.candidate\\.0\\.lastModifiedUtc: "
    "report the first current platform plugin candidate modification time")
require_diagnostics_output(
    "qt\\.platformPlugin\\.current\\.found: true"
    "find a readable current Qt platform plugin candidate")
require_diagnostics_output(
    "qt\\.platformPlugin\\.commonWayland\\.expectedFile\\.count: 3"
    "report common Wayland platform plugin file names")
require_diagnostics_output(
    "qt\\.platformPlugin\\.commonWayland\\.candidate\\.count: [1-9][0-9]*"
    "report common Wayland platform plugin candidates")
require_diagnostics_output(
    "qt\\.platformPlugin\\.commonWayland\\.candidate\\.0\\.baseName: qwayland"
    "check qwayland before legacy Wayland plugin names")
require_diagnostics_output(
    "qt\\.platformPlugin\\.commonWayland\\.candidate\\.0\\.path: "
    "report the first common Wayland platform plugin candidate path")
require_diagnostics_output(
    "qt\\.platformPlugin\\.commonWayland\\.candidate\\.0\\.size: -?[0-9]+"
    "report the first common Wayland platform plugin candidate size")
require_diagnostics_output(
    "qt\\.platformPlugin\\.commonWayland\\.candidate\\.0\\.lastModifiedUtc: "
    "report the first common Wayland platform plugin candidate modification time")
require_diagnostics_output(
    "qt\\.platformPlugin\\.commonWayland\\.found: (true|false)"
    "report whether a common Wayland platform plugin candidate was found")
require_diagnostics_output(
    "qt\\.platformPlugin\\.commonWindows\\.expectedFile\\.count: 1"
    "report common Windows platform plugin file names")
require_diagnostics_output(
    "qt\\.platformPlugin\\.commonWindows\\.candidate\\.count: [1-9][0-9]*"
    "report common Windows platform plugin candidates")
require_diagnostics_output(
    "qt\\.platformPlugin\\.commonWindows\\.candidate\\.0\\.baseName: qwindows"
    "check qwindows as the common Windows plugin name")
require_diagnostics_output(
    "qt\\.platformPlugin\\.commonWindows\\.candidate\\.0\\.path: "
    "report the first common Windows platform plugin candidate path")
require_diagnostics_output(
    "qt\\.platformPlugin\\.commonWindows\\.candidate\\.0\\.size: -?[0-9]+"
    "report the first common Windows platform plugin candidate size")
require_diagnostics_output(
    "qt\\.platformPlugin\\.commonWindows\\.candidate\\.0\\.lastModifiedUtc: "
    "report the first common Windows platform plugin candidate modification time")
require_diagnostics_output(
    "qt\\.platformPlugin\\.commonWindows\\.found: (true|false)"
    "report whether a common Windows platform plugin candidate was found")
require_diagnostics_output(
    "qt\\.platformPlugin\\.availableRoot\\.count: [1-9][0-9]*"
    "report platform plugin search roots")
require_diagnostics_output(
    "qt\\.platformPlugin\\.availableRoot\\.0\\.platformsDir\\.exists: true"
    "validate the first platform plugin directory")
require_diagnostics_output(
    "qt\\.platformPlugin\\.availableRoot\\.0\\.platformsDir\\.size: -?[0-9]+"
    "report the first platform plugin directory size")
require_diagnostics_output(
    "qt\\.platformPlugin\\.availableRoot\\.0\\.platformsDir\\.lastModifiedUtc: "
    "report the first platform plugin directory modification time")
require_diagnostics_output(
    "qt\\.platformPlugin\\.availableRoot\\.0\\.plugin\\.count: [1-9][0-9]*"
    "report available platform plugins")
require_diagnostics_output(
    "qt\\.platformPlugin\\.availableRoot\\.0\\.plugin\\.0\\.path: "
    "report the first available platform plugin path")
require_diagnostics_output(
    "qt\\.platformPlugin\\.availableRoot\\.0\\.plugin\\.0\\.size: [0-9]+"
    "report the first available platform plugin size")
require_diagnostics_output(
    "qt\\.platformPlugin\\.availableRoot\\.0\\.plugin\\.0\\.lastModifiedUtc: "
    "report the first available platform plugin modification time")
require_diagnostics_output("qt\\.quick\\.graphicsApi: " "report the Qt Quick graphics API")
require_diagnostics_output("qt\\.quick\\.sceneGraphBackend: " "report the Qt Quick scene graph backend")
require_diagnostics_output("qt\\.qmlImportPath\\.count: [1-9][0-9]*" "report Qt QML import paths")
require_diagnostics_output("qt\\.qmlImportPath\\.0: " "report the first Qt QML import path")
require_diagnostics_output("qt\\.qmlImportPath\\.0\\.resource: " "classify the first Qt QML import path")
require_diagnostics_output(
    "qt\\.quick\\.rendererInterface\\.graphicsApi: "
    "report the Qt Quick renderer-interface graphics API")
require_diagnostics_output(
    "qt\\.quick\\.rendererInterface\\.rhiBased: "
    "report whether the renderer-interface API is RHI based")
require_diagnostics_output(
    "qt\\.quick\\.rendererInterface\\.shaderType: "
    "report the Qt Quick renderer-interface shader type")
require_diagnostics_output(
    "qt\\.quick\\.rendererInterface\\.shaderCompilationTypes: "
    "report the Qt Quick shader compilation types")
require_diagnostics_output(
    "qt\\.quick\\.rendererInterface\\.shaderSourceTypes: "
    "report the Qt Quick shader source types")
require_diagnostics_output(
    "qt\\.opengl\\.moduleType: "
    "report the runtime OpenGL module type")
require_diagnostics_output(
    "qt\\.opengl\\.context\\.created: (true|false)"
    "report whether an OpenGL context can be created")
require_diagnostics_output(
    "qt\\.opengl\\.context\\.valid: (true|false)"
    "report whether the OpenGL context is valid")
require_diagnostics_output(
    "qt\\.opengl\\.surface\\.valid: (true|false)"
    "report whether the OpenGL probe surface is valid")
require_diagnostics_output(
    "qt\\.opengl\\.context\\.makeCurrentSucceeded: (true|false)"
    "report whether the OpenGL context can be made current")
require_diagnostics_output(
    "qt\\.opengl\\.context\\.format\\.renderableType: "
    "report the OpenGL context renderable type")
require_diagnostics_output(
    "qt\\.opengl\\.context\\.format\\.majorVersion: -?[0-9]+"
    "report the OpenGL context major version")
require_diagnostics_output(
    "qt\\.opengl\\.context\\.format\\.minorVersion: -?[0-9]+"
    "report the OpenGL context minor version")
require_diagnostics_output(
    "qt\\.opengl\\.context\\.format\\.profile: "
    "report the OpenGL context profile")
require_diagnostics_output(
    "qt\\.opengl\\.context\\.format\\.depthBufferSize: -?[0-9]+"
    "report the OpenGL context depth-buffer size")
require_diagnostics_output(
    "qt\\.opengl\\.context\\.format\\.stencilBufferSize: -?[0-9]+"
    "report the OpenGL context stencil-buffer size")
require_diagnostics_output(
    "qt\\.opengl\\.context\\.format\\.samples: -?[0-9]+"
    "report the OpenGL context sample count")
require_diagnostics_output(
    "qt\\.opengl\\.functions\\.initialized: (true|false)"
    "report whether OpenGL functions initialized")
require_diagnostics_output("qt\\.opengl\\.vendor: " "report the OpenGL vendor string")
require_diagnostics_output("qt\\.opengl\\.renderer: " "report the OpenGL renderer string")
require_diagnostics_output("qt\\.opengl\\.version: " "report the OpenGL version string")
require_diagnostics_output(
    "qt\\.opengl\\.shadingLanguageVersion: "
    "report the OpenGL shading language version string")
require_diagnostics_output("os\\.prettyName: " "report the OS pretty name")
require_diagnostics_output("os\\.productType: " "report the OS product type")
require_diagnostics_output("os\\.productVersion: " "report the OS product version")
require_diagnostics_output("os\\.kernelType: " "report the OS kernel type")
require_diagnostics_output("os\\.kernelVersion: " "report the OS kernel version")
require_diagnostics_output("qt\\.buildAbi: " "report the Qt build ABI")
require_diagnostics_output("cpu\\.buildArchitecture: " "report the build CPU architecture")
require_diagnostics_output("cpu\\.architecture: " "report the current CPU architecture")
require_diagnostics_output("env\\.PATH: " "report PATH")
require_diagnostics_output("envPath\\.PATH\\.count: [0-9]+" "report PATH path-list entries")
require_diagnostics_output("envPath\\.PATH\\.0\\.exists: true" "validate first PATH entry")
require_diagnostics_output("envPath\\.PATH\\.0\\.dir: true" "classify first PATH entry")
require_diagnostics_output("envPath\\.PATH\\.1\\.hasText: false" "classify blank PATH entries")
require_diagnostics_output(
    "envPath\\.PATH\\.1\\.absolutePath: \\(blank\\)"
    "avoid resolving blank PATH entries")
require_diagnostics_output("envPath\\.PATH\\.1\\.exists: false" "reject blank PATH entries")
require_diagnostics_output("envPath\\.PATH\\.2\\.exists: true" "validate second PATH entry")
require_diagnostics_output("envPath\\.PATH\\.2\\.dir: true" "classify second PATH entry")
require_diagnostics_output("env\\.LD_LIBRARY_PATH: " "report LD_LIBRARY_PATH")
require_diagnostics_output("envPath\\.LD_LIBRARY_PATH\\.count: [0-9]+" "report LD_LIBRARY_PATH path-list entries")
require_diagnostics_output("envPath\\.LD_LIBRARY_PATH\\.0\\.exists: true" "validate first LD_LIBRARY_PATH entry")
require_diagnostics_output("envPath\\.LD_LIBRARY_PATH\\.0\\.dir: true" "classify first LD_LIBRARY_PATH entry")
require_diagnostics_output("envPath\\.LD_LIBRARY_PATH\\.1\\.exists: true" "validate second LD_LIBRARY_PATH entry")
require_diagnostics_output("envPath\\.LD_LIBRARY_PATH\\.1\\.dir: true" "classify second LD_LIBRARY_PATH entry")
require_diagnostics_output("env\\.DYLD_LIBRARY_PATH: " "report DYLD_LIBRARY_PATH")
require_diagnostics_output("envPath\\.DYLD_LIBRARY_PATH\\.count: [0-9]+" "report DYLD_LIBRARY_PATH path-list entries")
require_diagnostics_output("envPath\\.DYLD_LIBRARY_PATH\\.0\\.exists: true" "validate first DYLD_LIBRARY_PATH entry")
require_diagnostics_output("envPath\\.DYLD_LIBRARY_PATH\\.0\\.dir: true" "classify first DYLD_LIBRARY_PATH entry")
require_diagnostics_output("envPath\\.DYLD_LIBRARY_PATH\\.1\\.exists: true" "validate second DYLD_LIBRARY_PATH entry")
require_diagnostics_output("envPath\\.DYLD_LIBRARY_PATH\\.1\\.dir: true" "classify second DYLD_LIBRARY_PATH entry")
foreach(single_path_env
        HOME
        USERPROFILE
        APPDATA
        LOCALAPPDATA
        XDG_CONFIG_HOME
        XDG_DATA_HOME
        XDG_CACHE_HOME
        TEMP
        TMP)
    require_diagnostics_output("env\\.${single_path_env}: " "report ${single_path_env}")
    require_diagnostics_output("envPath\\.${single_path_env}\\.exists: true" "validate ${single_path_env}")
    require_diagnostics_output("envPath\\.${single_path_env}\\.dir: true" "classify ${single_path_env}")
    require_diagnostics_output("envPath\\.${single_path_env}\\.readable: true" "validate ${single_path_env} readability")
    require_diagnostics_output("envPath\\.${single_path_env}\\.writable: true" "validate ${single_path_env} writability")
endforeach()
require_diagnostics_output("env\\.XDG_SESSION_TYPE: " "report the desktop session type")
require_diagnostics_output("env\\.XDG_CURRENT_DESKTOP: " "report the current desktop")
require_diagnostics_output("env\\.XDG_SESSION_DESKTOP: " "report the session desktop")
require_diagnostics_output("env\\.XDG_RUNTIME_DIR: " "report XDG_RUNTIME_DIR")
require_diagnostics_output("envPath\\.XDG_RUNTIME_DIR\\.exists: true" "validate XDG_RUNTIME_DIR")
require_diagnostics_output("envPath\\.XDG_RUNTIME_DIR\\.dir: true" "classify XDG_RUNTIME_DIR")
require_diagnostics_output("envPath\\.XDG_RUNTIME_DIR\\.readable: true" "validate XDG_RUNTIME_DIR readability")
require_diagnostics_output("envPath\\.XDG_RUNTIME_DIR\\.writable: true" "validate XDG_RUNTIME_DIR writability")
require_diagnostics_output("env\\.DESKTOP_SESSION: " "report the desktop session name")
require_diagnostics_output("env\\.KDE_FULL_SESSION: " "report KDE session state")
require_diagnostics_output("env\\.KDE_SESSION_VERSION: " "report KDE session version")
require_diagnostics_output("env\\.WAYLAND_DISPLAY: " "report Wayland display")
require_diagnostics_output("env\\.DISPLAY: " "report X11 display")
require_diagnostics_output("env\\.QT_QPA_PLATFORM: " "report QT_QPA_PLATFORM")
require_diagnostics_output("env\\.QT_QPA_PLATFORMTHEME: " "report QT_QPA_PLATFORMTHEME")
require_diagnostics_output("env\\.QT_QPA_PLATFORM_PLUGIN_PATH: " "report QT_QPA_PLATFORM_PLUGIN_PATH")
require_diagnostics_output("env\\.QT_PLUGIN_PATH: " "report QT_PLUGIN_PATH")
require_diagnostics_output(
    "env\\.QT_QPA_PLATFORM_PLUGIN_PATH: \\(empty\\)"
    "distinguish empty QT_QPA_PLATFORM_PLUGIN_PATH from unset")
require_diagnostics_output(
    "envPath\\.QT_QPA_PLATFORM_PLUGIN_PATH\\.value: \\(empty\\)"
    "report empty QT_QPA_PLATFORM_PLUGIN_PATH path-list value")
require_diagnostics_output(
    "envPath\\.QT_QPA_PLATFORM_PLUGIN_PATH\\.count: 1"
    "report empty QT_QPA_PLATFORM_PLUGIN_PATH as one blank path-list entry")
require_diagnostics_output(
    "envPath\\.QT_QPA_PLATFORM_PLUGIN_PATH\\.0\\.hasText: false"
    "classify empty QT_QPA_PLATFORM_PLUGIN_PATH entries")
require_diagnostics_output(
    "envPath\\.QT_QPA_PLATFORM_PLUGIN_PATH\\.0\\.absolutePath: \\(blank\\)"
    "avoid resolving empty QT_QPA_PLATFORM_PLUGIN_PATH entries")
require_diagnostics_output("envPath\\.QT_PLUGIN_PATH\\.count: 1" "report QT_PLUGIN_PATH path-list entry count")
require_diagnostics_output("envPath\\.QT_PLUGIN_PATH\\.0\\.hasText: false" "classify blank QT_PLUGIN_PATH entries")
require_diagnostics_output(
    "envPath\\.QT_PLUGIN_PATH\\.0\\.absolutePath: \\(blank\\)"
    "avoid resolving blank QT_PLUGIN_PATH entries")
require_diagnostics_output("envPath\\.QT_PLUGIN_PATH\\.0\\.exists: false" "reject blank QT_PLUGIN_PATH entries")
require_diagnostics_output("env\\.QT_DEBUG_PLUGINS: " "report QT_DEBUG_PLUGINS")
require_diagnostics_output("env\\.QT_LOGGING_RULES: " "report QT_LOGGING_RULES")
require_diagnostics_output("env\\.QT_STYLE_OVERRIDE: " "report QT_STYLE_OVERRIDE")
require_diagnostics_output("env\\.QT_SCALE_FACTOR: " "report QT_SCALE_FACTOR")
require_diagnostics_output("env\\.QT_SCREEN_SCALE_FACTORS: " "report QT_SCREEN_SCALE_FACTORS")
require_diagnostics_output("env\\.QT_AUTO_SCREEN_SCALE_FACTOR: " "report QT_AUTO_SCREEN_SCALE_FACTOR")
require_diagnostics_output("env\\.QT_ENABLE_HIGHDPI_SCALING: " "report QT_ENABLE_HIGHDPI_SCALING")
require_diagnostics_output("env\\.QT_SCALE_FACTOR_ROUNDING_POLICY: " "report Qt scale-factor rounding policy")
require_diagnostics_output("env\\.QT_USE_PHYSICAL_DPI: " "report Qt physical DPI override")
require_diagnostics_output("env\\.QT_DPI_ADJUSTMENT_POLICY: " "report Qt DPI adjustment policy")
require_diagnostics_output("env\\.QT_FONT_DPI: " "report QT_FONT_DPI")
require_diagnostics_output("env\\.QSG_RHI_BACKEND: " "report QSG_RHI_BACKEND")
require_diagnostics_output("env\\.QT_RHI_BACKEND: " "report QT_RHI_BACKEND")
require_diagnostics_output("env\\.QSG_INFO: " "report QSG_INFO")
require_diagnostics_output("env\\.QT_QUICK_BACKEND: " "report QT_QUICK_BACKEND")
require_diagnostics_output("env\\.QT_QUICK_CONTROLS_STYLE: " "report Qt Quick Controls style")
require_diagnostics_output("env\\.QT_QUICK_CONTROLS_CONF: " "report Qt Quick Controls config")
require_diagnostics_output("envPath\\.QT_QUICK_CONTROLS_CONF\\.exists: true" "validate Qt Quick Controls config")
require_diagnostics_output("envPath\\.QT_QUICK_CONTROLS_CONF\\.file: true" "classify Qt Quick Controls config")
require_diagnostics_output("envPath\\.QT_QUICK_CONTROLS_CONF\\.readable: true" "validate Qt Quick Controls config readability")
require_diagnostics_output("envPath\\.QT_QUICK_CONTROLS_CONF\\.writable: true" "validate Qt Quick Controls config writability")
require_diagnostics_output(
    "envPath\\.QT_QUICK_CONTROLS_STYLE_PATH\\.count: 2"
    "report Qt Quick Controls style path entries")
require_diagnostics_output(
    "envPath\\.QT_QUICK_CONTROLS_STYLE_PATH\\.0\\.exists: true"
    "validate first Qt Quick Controls style path")
require_diagnostics_output(
    "envPath\\.QT_QUICK_CONTROLS_STYLE_PATH\\.0\\.dir: true"
    "classify first Qt Quick Controls style path")
require_diagnostics_output("env\\.QML_IMPORT_PATH: " "report QML_IMPORT_PATH")
require_diagnostics_output("envPath\\.QML_IMPORT_PATH\\.count: 2" "report QML import path entries")
require_diagnostics_output("envPath\\.QML_IMPORT_PATH\\.0\\.exists: true" "validate first QML import path")
require_diagnostics_output("envPath\\.QML_IMPORT_PATH\\.0\\.dir: true" "classify first QML import path")
require_diagnostics_output("envPath\\.QML_IMPORT_PATH\\.1\\.exists: true" "validate second QML import path")
require_diagnostics_output("envPath\\.QML_IMPORT_PATH\\.1\\.dir: true" "classify second QML import path")
require_diagnostics_output("env\\.QML2_IMPORT_PATH: " "report QML2_IMPORT_PATH")
require_diagnostics_output("envPath\\.QML2_IMPORT_PATH\\.count: 2" "report QML2 import path entries")
require_diagnostics_output("envPath\\.QML2_IMPORT_PATH\\.0\\.exists: true" "validate first QML2 import path")
require_diagnostics_output("envPath\\.QML2_IMPORT_PATH\\.0\\.dir: true" "classify first QML2 import path")
require_diagnostics_output("envPath\\.QML2_IMPORT_PATH\\.1\\.exists: true" "validate second QML2 import path")
require_diagnostics_output("envPath\\.QML2_IMPORT_PATH\\.1\\.dir: true" "classify second QML2 import path")
require_diagnostics_output("env\\.QSG_RENDER_LOOP: " "report Qt Quick render loop")
require_diagnostics_output("env\\.QSG_VISUALIZE: " "report Qt Quick visualize mode")
require_diagnostics_output("env\\.QSG_RENDERER_DEBUG: " "report Qt Quick renderer debug")
require_diagnostics_output("env\\.QSG_RHI_DEBUG_LAYER: " "report Qt RHI debug layer")
require_diagnostics_output(
    "env\\.QSG_RHI_PREFER_SOFTWARE_RENDERER: "
    "report Qt RHI software renderer preference")
require_diagnostics_output("env\\.QT_OPENGL: " "report QT_OPENGL")
require_diagnostics_output("env\\.QT_ANGLE_PLATFORM: " "report QT_ANGLE_PLATFORM")
require_diagnostics_output("env\\.QT_D3D_ADAPTER_INDEX: " "report Direct3D adapter selection")
require_diagnostics_output("env\\.QT_D3D_DEBUG: " "report Direct3D debug selection")
require_diagnostics_output("env\\.QT_WAYLAND_DISABLE_WINDOWDECORATION: " "report Wayland decoration selection")
require_diagnostics_output("env\\.QT_WAYLAND_SHELL_INTEGRATION: " "report Wayland shell integration")
require_diagnostics_output("env\\.KWIN_DRM_DEVICES: " "report KWin DRM device selection")
require_diagnostics_output("envPath\\.KWIN_DRM_DEVICES\\.count: 2" "report KWin DRM device path-list entries")
require_diagnostics_output("envPath\\.KWIN_DRM_DEVICES\\.0\\.value: " "report first KWin DRM device path")
require_diagnostics_output("envPath\\.KWIN_DRM_DEVICES\\.0\\.exists: true" "validate first KWin DRM path")
require_diagnostics_output("envPath\\.KWIN_DRM_DEVICES\\.0\\.file: true" "classify first KWin DRM path")
require_diagnostics_output("envPath\\.KWIN_DRM_DEVICES\\.0\\.readable: true" "validate first KWin DRM path readability")
require_diagnostics_output("envPath\\.KWIN_DRM_DEVICES\\.0\\.size: [0-9]+" "report first KWin DRM path size")
require_diagnostics_output(
    "envPath\\.KWIN_DRM_DEVICES\\.0\\.lastModifiedUtc: "
    "report first KWin DRM path modification time")
require_diagnostics_output("envPath\\.KWIN_DRM_DEVICES\\.1\\.exists: true" "validate second KWin DRM path")
require_diagnostics_output("env\\.GBM_BACKEND: " "report GBM_BACKEND")
require_diagnostics_output("env\\.DRI_PRIME: " "report DRI_PRIME")
require_diagnostics_output("env\\.MESA_LOADER_DRIVER_OVERRIDE: " "report Mesa driver override")
require_diagnostics_output("env\\.LIBGL_ALWAYS_SOFTWARE: " "report software OpenGL selection")
require_diagnostics_output("env\\.LIBGL_DEBUG: " "report OpenGL loader debugging")
require_diagnostics_output("env\\.LIBVA_DRIVER_NAME: " "report VA driver selection")
require_diagnostics_output("env\\.EGL_PLATFORM: " "report EGL platform selection")
require_diagnostics_output("env\\.__EGL_VENDOR_LIBRARY_FILENAMES: " "report EGL vendor library files")
require_diagnostics_output(
    "envPath\\.__EGL_VENDOR_LIBRARY_FILENAMES\\.count: 2"
    "report EGL vendor path-list entries")
require_diagnostics_output(
    "envPath\\.__EGL_VENDOR_LIBRARY_FILENAMES\\.0\\.value: "
    "report first EGL vendor file")
require_diagnostics_output(
    "envPath\\.__EGL_VENDOR_LIBRARY_FILENAMES\\.0\\.exists: true"
    "validate first EGL vendor file")
require_diagnostics_output(
    "envPath\\.__EGL_VENDOR_LIBRARY_FILENAMES\\.0\\.file: true"
    "classify first EGL vendor file")
require_diagnostics_output(
    "envPath\\.__EGL_VENDOR_LIBRARY_FILENAMES\\.0\\.readable: true"
    "validate first EGL vendor file readability")
require_diagnostics_output(
    "envPath\\.__EGL_VENDOR_LIBRARY_FILENAMES\\.0\\.size: [0-9]+"
    "report first EGL vendor file size")
require_diagnostics_output(
    "envPath\\.__EGL_VENDOR_LIBRARY_FILENAMES\\.0\\.lastModifiedUtc: "
    "report first EGL vendor file modification time")
require_diagnostics_output(
    "envPath\\.__EGL_VENDOR_LIBRARY_FILENAMES\\.1\\.exists: true"
    "validate second EGL vendor file")
require_diagnostics_output("env\\.__GLX_VENDOR_LIBRARY_NAME: " "report GLX vendor selection")
require_diagnostics_output("env\\.__NV_PRIME_RENDER_OFFLOAD: " "report NVIDIA PRIME offload")
require_diagnostics_output(
    "env\\.__NV_PRIME_RENDER_OFFLOAD_PROVIDER: "
    "report NVIDIA PRIME offload provider")
require_diagnostics_output("env\\.__VK_LAYER_NV_optimus: " "report Vulkan NVIDIA Optimus selection")
require_diagnostics_output("env\\.VK_INSTANCE_LAYERS: " "report Vulkan instance layers")
require_diagnostics_output("env\\.VK_LOADER_DEBUG: " "report Vulkan loader debugging")
require_diagnostics_output("env\\.VK_ICD_FILENAMES: " "report Vulkan ICD selection")
require_diagnostics_output("env\\.VK_DRIVER_FILES: " "report Vulkan driver files")
require_diagnostics_output("envPath\\.VK_ICD_FILENAMES\\.count: 2" "report Vulkan ICD path-list entries")
require_diagnostics_output("envPath\\.VK_ICD_FILENAMES\\.0\\.value: " "report first Vulkan ICD path")
require_diagnostics_output("envPath\\.VK_ICD_FILENAMES\\.0\\.exists: true" "validate first Vulkan ICD path")
require_diagnostics_output("envPath\\.VK_ICD_FILENAMES\\.0\\.file: true" "classify first Vulkan ICD path")
require_diagnostics_output("envPath\\.VK_ICD_FILENAMES\\.0\\.readable: true" "validate first Vulkan ICD path readability")
require_diagnostics_output("envPath\\.VK_ICD_FILENAMES\\.0\\.size: [0-9]+" "report first Vulkan ICD path size")
require_diagnostics_output(
    "envPath\\.VK_ICD_FILENAMES\\.0\\.lastModifiedUtc: "
    "report first Vulkan ICD path modification time")
require_diagnostics_output("envPath\\.VK_ICD_FILENAMES\\.1\\.exists: true" "validate second Vulkan ICD path")
require_diagnostics_output("envPath\\.VK_DRIVER_FILES\\.count: 2" "report Vulkan driver path-list entries")
require_diagnostics_output("envPath\\.VK_DRIVER_FILES\\.0\\.value: " "report first Vulkan driver path")
require_diagnostics_output("envPath\\.VK_DRIVER_FILES\\.0\\.exists: true" "validate first Vulkan driver path")
require_diagnostics_output("envPath\\.VK_DRIVER_FILES\\.0\\.file: true" "classify first Vulkan driver path")
require_diagnostics_output("envPath\\.VK_DRIVER_FILES\\.0\\.readable: true" "validate first Vulkan driver path readability")
require_diagnostics_output("envPath\\.VK_DRIVER_FILES\\.0\\.size: [0-9]+" "report first Vulkan driver path size")
require_diagnostics_output(
    "envPath\\.VK_DRIVER_FILES\\.0\\.lastModifiedUtc: "
    "report first Vulkan driver path modification time")
require_diagnostics_output("envPath\\.VK_DRIVER_FILES\\.1\\.exists: true" "validate second Vulkan driver path")
require_diagnostics_output("env\\.NVIDIA_VISIBLE_DEVICES: " "report visible NVIDIA devices")
require_diagnostics_output("env\\.CUDA_VISIBLE_DEVICES: " "report visible CUDA devices")
require_diagnostics_output("env\\.CUDA_DEVICE_ORDER: " "report CUDA device ordering")
require_diagnostics_output("env\\.NVIDIA_DRIVER_CAPABILITIES: " "report NVIDIA driver capabilities")
require_diagnostics_output("katago\\.env\\.executable\\.value: " "report the KataGo executable env value")
require_diagnostics_output("katago\\.env\\.executable\\.hasText: true" "report the executable env text status")
require_diagnostics_output("katago\\.env\\.executable\\.absolutePath: " "report the KataGo executable absolute path")
require_diagnostics_output("katago\\.env\\.executable\\.canonicalPath: " "report the KataGo executable canonical path")
require_diagnostics_output("katago\\.env\\.executable\\.executable: true" "validate the executable path")
require_diagnostics_output("katago\\.env\\.executable\\.readable: true" "validate that the executable is readable")
require_diagnostics_output("katago\\.env\\.executable\\.size: [0-9]+" "report the executable size")
require_diagnostics_output("katago\\.env\\.executable\\.lastModifiedUtc: " "report the executable modification time")
require_diagnostics_output("katago\\.env\\.model\\.exists: true" "validate the model path")
require_diagnostics_output("katago\\.env\\.model\\.hasText: true" "report the model env text status")
require_diagnostics_output("katago\\.env\\.model\\.canonicalPath: " "report the model canonical path")
require_diagnostics_output("katago\\.env\\.model\\.readable: true" "validate that the model path is readable")
require_diagnostics_output("katago\\.env\\.model\\.writable: true" "validate that the model path is writable")
require_diagnostics_output("katago\\.env\\.model\\.size: [0-9]+" "report the model size")
require_diagnostics_output("katago\\.env\\.model\\.lastModifiedUtc: " "report the model modification time")
require_diagnostics_output("katago\\.env\\.analysisConfig\\.exists: true" "validate the analysis config path")
require_diagnostics_output("katago\\.env\\.analysisConfig\\.hasText: true" "report the analysis config env text status")
require_diagnostics_output("katago\\.env\\.analysisConfig\\.canonicalPath: " "report the analysis config canonical path")
require_diagnostics_output("katago\\.env\\.analysisConfig\\.readable: true" "validate that the analysis config is readable")
require_diagnostics_output("katago\\.env\\.analysisConfig\\.writable: true" "validate that the analysis config is writable")
require_diagnostics_output("katago\\.env\\.analysisConfig\\.size: [0-9]+" "report the analysis config size")
require_diagnostics_output("katago\\.env\\.analysisConfig\\.lastModifiedUtc: " "report the analysis config modification time")
require_diagnostics_output("katago\\.env\\.gtpConfig\\.exists: true" "validate the GTP config path")
require_diagnostics_output("katago\\.env\\.gtpConfig\\.hasText: true" "report the GTP config env text status")
require_diagnostics_output("katago\\.env\\.gtpConfig\\.canonicalPath: " "report the GTP config canonical path")
require_diagnostics_output("katago\\.env\\.gtpConfig\\.readable: true" "validate that the GTP config is readable")
require_diagnostics_output("katago\\.env\\.gtpConfig\\.writable: true" "validate that the GTP config is writable")
require_diagnostics_output("katago\\.env\\.gtpConfig\\.size: [0-9]+" "report the GTP config size")
require_diagnostics_output("katago\\.env\\.gtpConfig\\.lastModifiedUtc: " "report the GTP config modification time")
require_diagnostics_output("katago\\.env\\.complete: true" "summarize complete KataGo env configuration")
require_diagnostics_output("katago\\.env\\.ready: true" "summarize ready KataGo env configuration")
require_diagnostics_output("katago\\.env\\.status: ready" "report ready KataGo env status")
require_diagnostics_output("katago\\.env\\.missing\\.count: 0" "report no missing KataGo env paths")
require_diagnostics_output("katago\\.env\\.invalid\\.count: 0" "report no invalid KataGo env paths")
require_diagnostics_output("plan\\.target\\.osFamily: " "report the target-platform OS family")
require_diagnostics_output(
    "plan\\.target\\.inFirstReleaseScope: (true|false)"
    "summarize whether the OS is in the first-release scope")
require_diagnostics_output(
    "plan\\.target\\.linuxKdeWayland\\.kdeSession: (true|false)"
    "summarize KDE session detection for Linux target acceptance")
require_diagnostics_output(
    "plan\\.target\\.linuxKdeWayland\\.waylandSession: (true|false)"
    "summarize Wayland session detection for Linux target acceptance")
require_diagnostics_output(
    "plan\\.target\\.linuxKdeWayland\\.qtPlatformWayland: (true|false)"
    "summarize Qt Wayland platform detection for Linux target acceptance")
require_diagnostics_output(
    "plan\\.target\\.linuxKdeWayland\\.detected: (true|false)"
    "summarize Linux KDE Wayland target detection")
require_diagnostics_output(
    "plan\\.target\\.windows\\.nativePlatformPlugin: (true|false)"
    "summarize Windows Qt platform plugin detection")
require_diagnostics_output(
    "plan\\.target\\.windows\\.detected: (true|false)"
    "summarize Windows target detection")
require_diagnostics_output(
    "plan\\.target\\.nvidiaEnvironmentHint\\.present: true"
    "summarize NVIDIA environment hints from graphics path-list fixtures")
require_diagnostics_output(
    "plan\\.target\\.nvidiaEnvironmentHint\\.source\\.count: [1-9][0-9]*"
    "report NVIDIA environment hint source count")
require_diagnostics_output(
    "plan\\.target\\.nvidiaEnvironmentHint\\.source\\.[0-9]+: (VK_ICD_FILENAMES|VK_DRIVER_FILES)"
    "report a Vulkan NVIDIA environment hint source")
require_diagnostics_output(
    "plan\\.target\\.display\\.screenCount: [0-9]+"
    "summarize screen count for target display acceptance")
require_diagnostics_output(
    "plan\\.target\\.display\\.multiScreen: (true|false)"
    "summarize multi-display target detection")
require_diagnostics_output(
    "plan\\.target\\.display\\.primaryDevicePixelsAtLeast4K: (true|false)"
    "summarize 4K primary-screen detection")
require_diagnostics_output(
    "plan\\.target\\.display\\.anyDevicePixelsAtLeast4K: (true|false)"
    "summarize 4K detection across all screens")
require_diagnostics_output(
    "plan\\.target\\.display\\.primaryScaleAtLeast1_5: (true|false)"
    "summarize high-DPI primary-screen detection")
require_diagnostics_output(
    "plan\\.target\\.display\\.anyScaleAtLeast1_5: (true|false)"
    "summarize high-DPI detection across all screens")
require_diagnostics_output(
    "plan\\.target\\.display\\.primaryTargetScreenCandidate: (true|false)"
    "summarize same-screen primary target display detection")
require_diagnostics_output(
    "plan\\.target\\.display\\.anyTargetScreenCandidate: (true|false)"
    "summarize same-screen target display detection across all screens")
require_diagnostics_output(
    "plan\\.target\\.acceptance\\.linuxKdeWaylandNvidiaCandidate: (true|false)"
    "summarize Linux KDE Wayland NVIDIA target candidate detection")
require_diagnostics_output(
    "plan\\.target\\.acceptance\\.linuxKdeWaylandNvidiaBlocker\\.count: [1-9][0-9]*"
    "report Linux KDE Wayland NVIDIA target blocker count")
require_diagnostics_output(
    "plan\\.target\\.acceptance\\.linuxKdeWaylandNvidiaBlocker\\.[0-9]+: qtWaylandPlatform"
    "report offscreen Qt platform as a Linux KDE Wayland target blocker")
require_diagnostics_output(
    "plan\\.target\\.acceptance\\.windowsNvidiaCandidate: (true|false)"
    "summarize Windows NVIDIA target candidate detection")
require_diagnostics_output(
    "plan\\.target\\.acceptance\\.windowsNvidiaBlocker\\.count: [1-9][0-9]*"
    "report Windows NVIDIA target blocker count")
require_diagnostics_output(
    "plan\\.target\\.acceptance\\.firstReleaseNvidiaPlatformCandidate: (true|false)"
    "summarize first-release NVIDIA target candidate detection")
require_diagnostics_output(
    "plan\\.target\\.acceptance\\.firstReleaseNvidiaPlatformBlocker\\.count: 2"
    "report first-release target platform blocker count")
require_diagnostics_output(
    "plan\\.target\\.acceptance\\.firstReleaseNvidiaPlatformBlocker\\.0: linuxKdeWaylandNvidia"
    "report Linux target platform blocker")
require_diagnostics_output(
    "plan\\.target\\.acceptance\\.firstReleaseNvidiaPlatformBlocker\\.1: windowsNvidia"
    "report Windows target platform blocker")
require_diagnostics_output(
    "plan\\.target\\.acceptance\\.display4KCandidate: (true|false)"
    "summarize 4K display candidate detection")
require_diagnostics_output(
    "plan\\.target\\.acceptance\\.highDpiCandidate: (true|false)"
    "summarize high-DPI display candidate detection")
require_diagnostics_output(
    "plan\\.target\\.acceptance\\.targetDisplayCandidate: (true|false)"
    "summarize combined target display candidate detection")
require_diagnostics_output(
    "plan\\.target\\.acceptance\\.targetDisplayBlocker\\.count: [1-9][0-9]*"
    "report target display blocker count")
require_diagnostics_output(
    "plan\\.target\\.acceptance\\.targetDisplayBlocker\\.[0-9]+: display4K"
    "report missing 4K display target blocker")
require_diagnostics_output(
    "plan\\.target\\.acceptance\\.targetDisplayBlocker\\.[0-9]+: highDpiScale"
    "report missing high-DPI display target blocker")
require_diagnostics_output(
    "plan\\.target\\.acceptance\\.multiDisplayBlocker\\.0: multiDisplay"
    "report missing multi-display target blocker")
require_diagnostics_output(
    "plan\\.acceptance\\.linuxKdeWaylandNvidiaCandidate: (true|false)"
    "summarize Linux KDE Wayland NVIDIA PLAN acceptance candidate status")
require_diagnostics_output(
    "plan\\.acceptance\\.windowsNvidiaCandidate: (true|false)"
    "summarize Windows NVIDIA PLAN acceptance candidate status")
require_diagnostics_output(
    "plan\\.acceptance\\.targetPlatformCandidate: false"
    "summarize that offscreen diagnostics still need a target machine")
require_diagnostics_output(
    "plan\\.acceptance\\.targetPlatformBlocker\\.count: 2"
    "report PLAN acceptance target-platform blocker count")
require_diagnostics_output(
    "plan\\.acceptance\\.targetPlatformBlocker\\.0: linuxKdeWaylandNvidia"
    "report PLAN acceptance Linux target-platform blocker")
require_diagnostics_output(
    "plan\\.acceptance\\.targetPlatformBlocker\\.1: windowsNvidia"
    "report PLAN acceptance Windows target-platform blocker")
require_diagnostics_output(
    "plan\\.acceptance\\.realKataGoEnvReady: true"
    "summarize ready real-KataGo environment fixtures")
require_diagnostics_output(
    "plan\\.acceptance\\.realKataGoEnvStatus: ready"
    "report real-KataGo environment readiness status")
require_diagnostics_output(
    "plan\\.acceptance\\.realKataGoTargetRunCandidate: false"
    "avoid treating offscreen fixtures as a target-machine real-KataGo run")
require_diagnostics_output(
    "plan\\.acceptance\\.realKataGoManualVerificationCandidate: false"
    "avoid treating offscreen fixtures as fully ready for real-KataGo manual verification")
require_diagnostics_output(
    "plan\\.acceptance\\.realKataGoManualVerificationBlocker\\.0: targetPlatform"
    "report target-platform blocker for offscreen real-KataGo manual verification")
require_diagnostics_output(
    "plan\\.acceptance\\.savedMainConfigReady: true"
    "summarize ready saved main engine config for PLAN acceptance")
require_diagnostics_output(
    "plan\\.acceptance\\.savedMainGtpReady: true"
    "summarize ready saved main GTP config for PLAN acceptance")
require_diagnostics_output(
    "plan\\.acceptance\\.savedMainAnalysisReady: true"
    "summarize ready saved main analysis config for PLAN acceptance")
require_diagnostics_output(
    "plan\\.acceptance\\.savedMainConfigStatus: ready"
    "report saved main engine config readiness status")
require_diagnostics_output(
    "plan\\.acceptance\\.savedCompareConfigReady: true"
    "summarize ready saved compare engine config for PLAN acceptance")
require_diagnostics_output(
    "plan\\.acceptance\\.savedCompareGtpReady: true"
    "summarize ready saved compare GTP config for PLAN acceptance")
require_diagnostics_output(
    "plan\\.acceptance\\.savedCompareAnalysisReady: true"
    "summarize ready saved compare analysis config for PLAN acceptance")
require_diagnostics_output(
    "plan\\.acceptance\\.savedCompareConfigStatus: ready"
    "report saved compare engine config readiness status")
require_diagnostics_output(
    "plan\\.acceptance\\.envOrSavedMainConfigReady: true"
    "summarize ready env-or-saved main engine config")
require_diagnostics_output(
    "plan\\.acceptance\\.envOrSavedMainGtpReady: true"
    "summarize ready env-or-saved main GTP config")
require_diagnostics_output(
    "plan\\.acceptance\\.envOrSavedMainAnalysisReady: true"
    "summarize ready env-or-saved main analysis config")
require_diagnostics_output(
    "plan\\.acceptance\\.configuredMainConfigSource: env-and-saved"
    "report env-and-saved main config source")
require_diagnostics_output(
    "plan\\.acceptance\\.configuredMainGtpSource: env-and-saved"
    "report env-and-saved main GTP source")
require_diagnostics_output(
    "plan\\.acceptance\\.configuredMainAnalysisSource: env-and-saved"
    "report env-and-saved main analysis source")
require_diagnostics_output(
    "plan\\.acceptance\\.configuredCompareConfigSource: saved"
    "report saved compare config source")
require_diagnostics_output(
    "plan\\.acceptance\\.configuredCompareGtpSource: saved"
    "report saved compare GTP source")
require_diagnostics_output(
    "plan\\.acceptance\\.configuredCompareAnalysisSource: saved"
    "report saved compare analysis source")
require_diagnostics_output(
    "plan\\.acceptance\\.configuredDualEngineConfigReady: true"
    "summarize ready configured dual-engine config")
require_diagnostics_output(
    "plan\\.acceptance\\.configuredDualEngineGtpReady: true"
    "summarize ready configured dual-engine GTP config")
require_diagnostics_output(
    "plan\\.acceptance\\.configuredDualEngineAnalysisReady: true"
    "summarize ready configured dual-engine analysis config")
require_diagnostics_output(
    "plan\\.acceptance\\.configuredKataGoTargetRunCandidate: false"
    "avoid treating offscreen saved config as a target-machine run")
require_diagnostics_output(
    "plan\\.acceptance\\.configuredManualVerificationCandidate: false"
    "avoid treating offscreen saved config as fully ready for manual verification")
require_diagnostics_output(
    "plan\\.acceptance\\.configuredManualVerificationBlocker\\.0: targetPlatform"
    "report target-platform blocker for offscreen configured manual verification")
require_diagnostics_output(
    "plan\\.acceptance\\.configuredRealtimeGtpTargetRunCandidate: false"
    "avoid treating offscreen realtime GTP config as a target-machine run")
require_diagnostics_output(
    "plan\\.acceptance\\.configuredRealtimeGtpManualVerificationCandidate: false"
    "avoid treating offscreen realtime GTP config as fully ready for manual verification")
require_diagnostics_output(
    "plan\\.acceptance\\.configuredBatchAnalysisTargetRunCandidate: false"
    "avoid treating offscreen batch analysis config as a target-machine run")
require_diagnostics_output(
    "plan\\.acceptance\\.configuredBatchAnalysisManualVerificationCandidate: false"
    "avoid treating offscreen batch analysis config as fully ready for manual verification")
require_diagnostics_output(
    "plan\\.acceptance\\.configuredDualEngineTargetRunCandidate: false"
    "avoid treating offscreen dual-engine config as a target-machine run")
require_diagnostics_output(
    "plan\\.acceptance\\.configuredDualEngineManualVerificationCandidate: false"
    "avoid treating offscreen dual-engine config as fully ready for manual verification")
require_diagnostics_output(
    "plan\\.acceptance\\.configuredDualEngineManualVerificationBlocker\\.0: targetPlatform"
    "report target-platform blocker for offscreen dual-engine manual verification")
require_diagnostics_output(
    "plan\\.acceptance\\.configuredDualRealtimeGtpTargetRunCandidate: false"
    "avoid treating offscreen dual-engine realtime GTP config as a target-machine run")
require_diagnostics_output(
    "plan\\.acceptance\\.configuredDualRealtimeGtpManualVerificationCandidate: false"
    "avoid treating offscreen dual-engine realtime GTP config as fully ready for manual verification")
require_diagnostics_output(
    "plan\\.acceptance\\.configuredDualBatchAnalysisTargetRunCandidate: false"
    "avoid treating offscreen dual-engine batch analysis config as a target-machine run")
require_diagnostics_output(
    "plan\\.acceptance\\.configuredDualBatchAnalysisManualVerificationCandidate: false"
    "avoid treating offscreen dual-engine batch analysis config as fully ready for manual verification")
require_diagnostics_output(
    "plan\\.acceptance\\.configuredStatus: katago-config-ready-needs-target-machine"
    "summarize offscreen configured-KataGo diagnostics acceptance status")
require_diagnostics_output(
    "plan\\.acceptance\\.recordFile: \\(unset\\)"
    "report unset target acceptance record file")
require_diagnostics_output(
    "plan\\.acceptance\\.recordFile\\.set: false"
    "report unset target acceptance record file set flag")
require_diagnostics_output(
    "plan\\.acceptance\\.recordFile\\.exists: false"
    "report unset target acceptance record file exists flag")
require_diagnostics_output(
    "plan\\.acceptance\\.recordFile\\.readable: false"
    "report unset target acceptance record file readable flag")
require_diagnostics_output(
    "plan\\.acceptance\\.record\\.completedUtc: \\(unset\\)"
    "report missing target acceptance completion time")
require_diagnostics_output(
    "plan\\.acceptance\\.record\\.completedUtcStatus: missing"
    "report missing target acceptance completion time status")
require_diagnostics_output(
    "plan\\.acceptance\\.record\\.appVersion: \\(unset\\)"
    "report missing target acceptance app version")
require_diagnostics_output(
    "plan\\.acceptance\\.record\\.appVersionStatus: missing"
    "report missing target acceptance app version status")
require_diagnostics_output(
    "plan\\.acceptance\\.record\\.appExecutableSha256: \\(unset\\)"
    "report missing target acceptance app executable SHA-256")
require_diagnostics_output(
    "plan\\.acceptance\\.record\\.appExecutableSha256Status: missing"
    "report missing target acceptance app executable SHA-256 status")
require_diagnostics_output(
    "plan\\.acceptance\\.record\\.osPrettyName: \\(unset\\)"
    "report missing target acceptance OS pretty name")
require_diagnostics_output(
    "plan\\.acceptance\\.record\\.osPrettyNameStatus: missing"
    "report missing target acceptance OS pretty name status")
require_diagnostics_output(
    "plan\\.acceptance\\.record\\.osKernelType: \\(unset\\)"
    "report missing target acceptance OS kernel type")
require_diagnostics_output(
    "plan\\.acceptance\\.record\\.osKernelTypeStatus: missing"
    "report missing target acceptance OS kernel type status")
require_diagnostics_output(
    "plan\\.acceptance\\.record\\.osKernelVersion: \\(unset\\)"
    "report missing target acceptance OS kernel version")
require_diagnostics_output(
    "plan\\.acceptance\\.record\\.osKernelVersionStatus: missing"
    "report missing target acceptance OS kernel version status")
require_diagnostics_output(
    "plan\\.acceptance\\.record\\.qtRuntimeVersion: \\(unset\\)"
    "report missing target acceptance Qt runtime version")
require_diagnostics_output(
    "plan\\.acceptance\\.record\\.qtRuntimeVersionStatus: missing"
    "report missing target acceptance Qt runtime version status")
require_diagnostics_output(
    "plan\\.acceptance\\.record\\.qtBuildAbi: \\(unset\\)"
    "report missing target acceptance Qt build ABI")
require_diagnostics_output(
    "plan\\.acceptance\\.record\\.qtBuildAbiStatus: missing"
    "report missing target acceptance Qt build ABI status")
require_diagnostics_output(
    "plan\\.acceptance\\.record\\.cpuArchitecture: \\(unset\\)"
    "report missing target acceptance CPU architecture")
require_diagnostics_output(
    "plan\\.acceptance\\.record\\.cpuArchitectureStatus: missing"
    "report missing target acceptance CPU architecture status")
require_diagnostics_output(
    "plan\\.acceptance\\.record\\.reviewer: \\(unset\\)"
    "report missing target acceptance reviewer")
require_diagnostics_output(
    "plan\\.acceptance\\.record\\.targetMachine: \\(unset\\)"
    "report missing target acceptance machine")
require_diagnostics_output(
    "plan\\.acceptance\\.record\\.targetMachineStatus: missing"
    "report missing target acceptance machine status")
require_diagnostics_output(
    "plan\\.acceptance\\.record\\.gpuDriver: \\(unset\\)"
    "report missing target acceptance GPU driver")
require_diagnostics_output(
    "plan\\.acceptance\\.record\\.gpuDriverStatus: missing"
    "report missing target acceptance GPU driver status")
require_diagnostics_output(
    "plan\\.acceptance\\.record\\.kataGoVersion: \\(unset\\)"
    "report missing target acceptance KataGo version")
require_diagnostics_output(
    "plan\\.acceptance\\.record\\.kataGoVersionStatus: missing"
    "report missing target acceptance KataGo version status")
require_diagnostics_output(
    "plan\\.acceptance\\.recordMetadata\\.ready: false"
    "report incomplete target acceptance metadata by default")
require_diagnostics_output(
    "plan\\.acceptance\\.recordMetadata\\.blocker\\.count: 13"
    "report default target acceptance metadata blocker count")
require_diagnostics_output(
    "plan\\.acceptance\\.recordMetadata\\.blocker\\.0: completedUtc"
    "report missing target acceptance completion metadata blocker")
require_diagnostics_output(
    "plan\\.acceptance\\.evidence\\.diagnostics\\.value: \\(unset\\)"
    "report missing diagnostics evidence path")
require_diagnostics_output(
    "plan\\.acceptance\\.evidence\\.diagnostics\\.hasText: false"
    "report missing diagnostics evidence path text flag")
require_diagnostics_output(
    "plan\\.acceptance\\.evidence\\.ready: false"
    "report incomplete acceptance evidence readiness by default")
require_diagnostics_output(
    "plan\\.acceptance\\.evidence\\.blocker\\.count: 14"
    "report default acceptance evidence blocker count")
require_diagnostics_output(
    "plan\\.acceptance\\.evidence\\.blocker\\.0: diagnosticsEvidence"
    "report missing diagnostics evidence blocker")
require_diagnostics_output(
    "plan\\.acceptance\\.evidence\\.blocker\\.10: engineLogGpuOrStderrEvidenceContent"
    "report missing engine log GPU/stderr evidence content blocker")
require_diagnostics_output(
    "plan\\.acceptance\\.linuxKdeWaylandNvidiaManualResult: manual-record-required"
    "report missing Linux target manual result")
require_diagnostics_output(
    "plan\\.acceptance\\.windowsNvidiaManualResult: manual-record-required"
    "report missing Windows target manual result")
require_diagnostics_output(
    "plan\\.acceptance\\.physicalDisplayManualResult: manual-record-required"
    "report missing physical display manual result")
require_diagnostics_output(
    "plan\\.acceptance\\.externalTargetVerificationManualResult: manual-record-required"
    "report missing external target manual result")
require_diagnostics_output(
    "plan\\.acceptance\\.checklistPassedRecord\\.count: 0"
    "report no passed target checklist records by default")
require_diagnostics_output(
    "plan\\.acceptance\\.checklistFailedRecord\\.count: 0"
    "report no failed target checklist records by default")
require_diagnostics_output(
    "plan\\.acceptance\\.checklistInvalidRecord\\.count: 0"
    "report no invalid target checklist records by default")
require_diagnostics_output(
    "plan\\.acceptance\\.manualFailedRecord\\.count: 0"
    "report no failed aggregate manual records by default")
require_diagnostics_output(
    "plan\\.acceptance\\.manualInvalidRecord\\.count: 0"
    "report no invalid aggregate manual records by default")
require_diagnostics_output(
    "plan\\.acceptance\\.display4KCandidate: (true|false)"
    "summarize PLAN 4K display acceptance candidate status")
require_diagnostics_output(
    "plan\\.acceptance\\.highDpiCandidate: (true|false)"
    "summarize PLAN high-DPI acceptance candidate status")
require_diagnostics_output(
    "plan\\.acceptance\\.targetDisplayCandidate: (true|false)"
    "summarize PLAN combined target display acceptance candidate status")
require_diagnostics_output(
    "plan\\.acceptance\\.sameScreenTargetDisplayCandidate: (true|false)"
    "summarize same-screen 4K/high-DPI PLAN acceptance candidate status")
require_diagnostics_output(
    "plan\\.acceptance\\.multiDisplayCandidate: (true|false)"
    "summarize PLAN multi-display acceptance candidate status")
require_diagnostics_output(
    "plan\\.acceptance\\.targetDisplayBlocker\\.[0-9]+: display4K"
    "report PLAN acceptance missing 4K display blocker")
require_diagnostics_output(
    "plan\\.acceptance\\.targetDisplayBlocker\\.[0-9]+: highDpiScale"
    "report PLAN acceptance missing high-DPI display blocker")
require_diagnostics_output(
    "plan\\.acceptance\\.multiDisplayBlocker\\.0: multiDisplay"
    "report PLAN acceptance missing multi-display blocker")
require_diagnostics_output(
    "plan\\.acceptance\\.manualUiInspectionRequired: true"
    "make manual target UI inspection explicit")
require_diagnostics_output(
    "plan\\.acceptance\\.manualUiInspectionStatus: manual-record-required"
    "report manual UI inspection status")
require_diagnostics_output(
    "plan\\.acceptance\\.manualUiInspectionChecklist\\.count: 12"
    "report manual UI inspection checklist size")
require_diagnostics_output(
    "plan\\.acceptance\\.manualUiInspectionChecklist\\.0: mainWindow4KHighDpiLayout"
    "report main-window high-DPI layout inspection item")
require_diagnostics_output(
    "plan\\.acceptance\\.manualUiInspectionChecklist\\.1: boardLinesCoordinatesAndStarPoints"
    "report board line and coordinate inspection item")
require_diagnostics_output(
    "plan\\.acceptance\\.manualUiInspectionChecklist\\.2: stoneRenderingAndCandidateLabels"
    "report stone and candidate-label inspection item")
require_diagnostics_output(
    "plan\\.acceptance\\.manualUiInspectionChecklist\\.3: candidateTableColumns"
    "report candidate-table inspection item")
require_diagnostics_output(
    "plan\\.acceptance\\.manualUiInspectionChecklist\\.4: pvPreviewStones"
    "report PV preview inspection item")
require_diagnostics_output(
    "plan\\.acceptance\\.manualUiInspectionChecklist\\.5: ownershipMainBoardOverlay"
    "report main-board ownership inspection item")
require_diagnostics_output(
    "plan\\.acceptance\\.manualUiInspectionChecklist\\.6: ownershipMiniBoardDock"
    "report mini-board ownership inspection item")
require_diagnostics_output(
    "plan\\.acceptance\\.manualUiInspectionChecklist\\.7: winrateScoreGraph"
    "report graph inspection item")
require_diagnostics_output(
    "plan\\.acceptance\\.manualUiInspectionChecklist\\.8: toolbarDockAndMenuLayout"
    "report toolbar, dock, and menu layout inspection item")
require_diagnostics_output(
    "plan\\.acceptance\\.manualUiInspectionChecklist\\.9: bilingualTextFit"
    "report bilingual text-fit inspection item")
require_diagnostics_output(
    "plan\\.acceptance\\.manualUiInspectionChecklist\\.10: savedWindowRestore"
    "report saved-window restore inspection item")
require_diagnostics_output(
    "plan\\.acceptance\\.manualUiInspectionChecklist\\.11: multiDisplayPlacement"
    "report multi-display placement inspection item")
require_diagnostics_output(
    "plan\\.acceptance\\.manualRawEngineComparisonRequired: true"
    "make manual raw-engine comparison explicit")
require_diagnostics_output(
    "plan\\.acceptance\\.rawKataGoComparisonStatus: manual-record-required"
    "report raw KataGo comparison status")
require_diagnostics_output(
    "plan\\.acceptance\\.rawKataGoComparisonChecklist\\.count: 10"
    "report raw KataGo comparison checklist size")
require_diagnostics_output(
    "plan\\.acceptance\\.rawKataGoComparisonChecklist\\.0: analysisJsonRawResponse"
    "report Analysis JSON raw-response comparison item")
require_diagnostics_output(
    "plan\\.acceptance\\.rawKataGoComparisonChecklist\\.1: realtimeGtpRawLine"
    "report realtime GTP raw-line comparison item")
require_diagnostics_output(
    "plan\\.acceptance\\.rawKataGoComparisonChecklist\\.2: candidateTableRendering"
    "report candidate-table raw comparison item")
require_diagnostics_output(
    "plan\\.acceptance\\.rawKataGoComparisonChecklist\\.3: pvPreview"
    "report PV preview raw comparison item")
require_diagnostics_output(
    "plan\\.acceptance\\.rawKataGoComparisonChecklist\\.4: winrateScorePerspective"
    "report winrate and score perspective raw comparison item")
require_diagnostics_output(
    "plan\\.acceptance\\.rawKataGoComparisonChecklist\\.5: ownershipDisplay"
    "report ownership raw comparison item")
require_diagnostics_output(
    "plan\\.acceptance\\.rawKataGoComparisonChecklist\\.6: genmove"
    "report genmove raw comparison item")
require_diagnostics_output(
    "plan\\.acceptance\\.rawKataGoComparisonChecklist\\.7: humanVsAi"
    "report Human-vs-AI raw comparison item")
require_diagnostics_output(
    "plan\\.acceptance\\.rawKataGoComparisonChecklist\\.8: selfPlay"
    "report self-play raw comparison item")
require_diagnostics_output(
    "plan\\.acceptance\\.rawKataGoComparisonChecklist\\.9: engineVsEngine"
    "report engine-vs-engine raw comparison item")
require_diagnostics_output(
    "plan\\.acceptance\\.externalTargetVerificationRequired: true"
    "make external target verification explicit")
require_diagnostics_output(
    "plan\\.acceptance\\.externalTargetVerificationStatus: manual-record-required"
    "report external target verification status")
require_diagnostics_output(
    "plan\\.acceptance\\.externalTargetVerificationChecklist\\.count: 4"
    "report external target verification checklist size")
require_diagnostics_output(
    "plan\\.acceptance\\.externalTargetVerificationChecklist\\.0: linuxKdeWaylandNvidiaRealtimeKataGo"
    "report Linux KDE Wayland NVIDIA external verification item")
require_diagnostics_output(
    "plan\\.acceptance\\.externalTargetVerificationChecklist\\.1: windowsNvidiaRealtimeKataGo"
    "report Windows NVIDIA external verification item")
require_diagnostics_output(
    "plan\\.acceptance\\.externalTargetVerificationChecklist\\.2: physical4KHighDpiMultiDisplayUi"
    "report physical 4K/high-DPI/multi-display external verification item")
require_diagnostics_output(
    "plan\\.acceptance\\.externalTargetVerificationChecklist\\.3: rawKataGoUiComparison"
    "report raw KataGo UI comparison external verification item")
require_diagnostics_output(
    "plan\\.acceptance\\.finalAcceptanceStatus: needs-readiness-and-final-manual-acceptance"
    "report final acceptance status")
require_diagnostics_output(
    "plan\\.acceptance\\.finalAcceptanceBlocker\\.count: 6"
    "report final acceptance blocker count")
require_diagnostics_output(
    "plan\\.acceptance\\.finalAcceptanceBlocker\\.0: targetPlatform"
    "report final target-platform blocker")
require_diagnostics_output(
    "plan\\.acceptance\\.finalAcceptanceBlocker\\.1: targetDisplay"
    "report final target-display blocker")
require_diagnostics_output(
    "plan\\.acceptance\\.finalAcceptanceBlocker\\.2: multiDisplay"
    "report final multi-display blocker")
require_diagnostics_output(
    "plan\\.acceptance\\.finalAcceptanceBlocker\\.3: externalTargetVerification"
    "report final external target verification blocker")
require_diagnostics_output(
    "plan\\.acceptance\\.finalAcceptanceBlocker\\.4: manualRawEngineComparison"
    "report final raw-engine comparison blocker")
require_diagnostics_output(
    "plan\\.acceptance\\.finalAcceptanceBlocker\\.5: manualUiInspection"
    "report final manual UI inspection blocker")
require_diagnostics_output(
    "plan\\.acceptance\\.finalAcceptanceOutstandingBlocker\\.count: 6"
    "report final outstanding blocker count")
require_diagnostics_output(
    "plan\\.acceptance\\.finalAcceptanceOutstandingBlocker\\.0: targetPlatform"
    "report final outstanding target-platform blocker")
require_diagnostics_output(
    "plan\\.acceptance\\.finalAcceptanceOutstandingBlocker\\.1: targetDisplay"
    "report final outstanding target-display blocker")
require_diagnostics_output(
    "plan\\.acceptance\\.finalAcceptanceOutstandingBlocker\\.2: multiDisplay"
    "report final outstanding multi-display blocker")
require_diagnostics_output(
    "plan\\.acceptance\\.finalAcceptanceOutstandingBlocker\\.3: externalTargetVerification"
    "report final outstanding external target verification blocker")
require_diagnostics_output(
    "plan\\.acceptance\\.finalAcceptanceOutstandingBlocker\\.4: manualRawEngineComparison"
    "report final outstanding raw-engine comparison blocker")
require_diagnostics_output(
    "plan\\.acceptance\\.finalAcceptanceOutstandingBlocker\\.5: manualUiInspection"
    "report final outstanding manual UI inspection blocker")
require_diagnostics_output(
    "plan\\.acceptance\\.linuxKdeWaylandNvidiaVerificationChecklist\\.count: 10"
    "report Linux KDE Wayland NVIDIA verification checklist size")
require_diagnostics_output(
    "plan\\.acceptance\\.linuxKdeWaylandNvidiaVerificationChecklist\\.0: linuxOs"
    "report Linux OS verification item")
require_diagnostics_output(
    "plan\\.acceptance\\.linuxKdeWaylandNvidiaVerificationChecklist\\.1: kdeSession"
    "report KDE session verification item")
require_diagnostics_output(
    "plan\\.acceptance\\.linuxKdeWaylandNvidiaVerificationChecklist\\.2: waylandSession"
    "report Wayland session verification item")
require_diagnostics_output(
    "plan\\.acceptance\\.linuxKdeWaylandNvidiaVerificationChecklist\\.3: qwaylandPlatformPlugin"
    "report qwayland platform plugin verification item")
require_diagnostics_output(
    "plan\\.acceptance\\.linuxKdeWaylandNvidiaVerificationChecklist\\.4: nvidiaEnvironmentHint"
    "report Linux NVIDIA environment verification item")
require_diagnostics_output(
    "plan\\.acceptance\\.linuxKdeWaylandNvidiaVerificationChecklist\\.5: packageStarts"
    "report Linux package startup verification item")
require_diagnostics_output(
    "plan\\.acceptance\\.linuxKdeWaylandNvidiaVerificationChecklist\\.6: configureKataGoPaths"
    "report Linux KataGo path configuration verification item")
require_diagnostics_output(
    "plan\\.acceptance\\.linuxKdeWaylandNvidiaVerificationChecklist\\.7: realtimeAnalyzeCurrentPosition"
    "report Linux realtime analysis verification item")
require_diagnostics_output(
    "plan\\.acceptance\\.linuxKdeWaylandNvidiaVerificationChecklist\\.8: branchResync"
    "report Linux branch resync verification item")
require_diagnostics_output(
    "plan\\.acceptance\\.linuxKdeWaylandNvidiaVerificationChecklist\\.9: gpuStderrCaptured"
    "report Linux GPU stderr verification item")
require_diagnostics_output(
    "plan\\.acceptance\\.linuxKdeWaylandNvidiaVerificationStatus: katago-gtp-config-ready-needs-linux-kde-wayland-nvidia"
    "report Linux target verification readiness status")
require_diagnostics_output(
    "plan\\.acceptance\\.linuxKdeWaylandNvidiaVerificationReadinessBlocker\\.count: 1"
    "report Linux target verification readiness blocker count")
require_diagnostics_output(
    "plan\\.acceptance\\.linuxKdeWaylandNvidiaVerificationReadinessBlocker\\.0: qtWaylandPlatform"
    "report Linux target qwayland readiness blocker")
require_diagnostics_output(
    "plan\\.acceptance\\.windowsNvidiaVerificationChecklist\\.count: 9"
    "report Windows NVIDIA verification checklist size")
require_diagnostics_output(
    "plan\\.acceptance\\.windowsNvidiaVerificationChecklist\\.0: windowsOs"
    "report Windows OS verification item")
require_diagnostics_output(
    "plan\\.acceptance\\.windowsNvidiaVerificationChecklist\\.1: qwindowsPlatformPlugin"
    "report qwindows platform plugin verification item")
require_diagnostics_output(
    "plan\\.acceptance\\.windowsNvidiaVerificationChecklist\\.2: packageExtracts"
    "report Windows package extraction verification item")
require_diagnostics_output(
    "plan\\.acceptance\\.windowsNvidiaVerificationChecklist\\.3: appLocalQtRuntime"
    "report Windows app-local Qt runtime verification item")
require_diagnostics_output(
    "plan\\.acceptance\\.windowsNvidiaVerificationChecklist\\.4: nvidiaEnvironmentHint"
    "report Windows NVIDIA environment verification item")
require_diagnostics_output(
    "plan\\.acceptance\\.windowsNvidiaVerificationChecklist\\.5: configureKataGoPaths"
    "report Windows KataGo path configuration verification item")
require_diagnostics_output(
    "plan\\.acceptance\\.windowsNvidiaVerificationChecklist\\.6: nativeSettingsPathDialog"
    "report Windows native settings path-dialog verification item")
require_diagnostics_output(
    "plan\\.acceptance\\.windowsNvidiaVerificationChecklist\\.7: realtimeAnalyzeCurrentPosition"
    "report Windows realtime analysis verification item")
require_diagnostics_output(
    "plan\\.acceptance\\.windowsNvidiaVerificationChecklist\\.8: engineDiagnosticsCaptured"
    "report Windows engine diagnostics verification item")
require_diagnostics_output(
    "plan\\.acceptance\\.windowsNvidiaVerificationStatus: katago-gtp-config-ready-needs-windows-nvidia"
    "report Windows target verification readiness status")
require_diagnostics_output(
    "plan\\.acceptance\\.windowsNvidiaVerificationReadinessBlocker\\.count: 2"
    "report Windows target verification readiness blocker count")
require_diagnostics_output(
    "plan\\.acceptance\\.windowsNvidiaVerificationReadinessBlocker\\.0: windowsOs"
    "report Windows target OS readiness blocker")
require_diagnostics_output(
    "plan\\.acceptance\\.windowsNvidiaVerificationReadinessBlocker\\.1: windowsPlatformPlugin"
    "report Windows target platform plugin readiness blocker")
require_diagnostics_output(
    "plan\\.acceptance\\.physicalDisplayVerificationChecklist\\.count: 9"
    "report physical display verification checklist size")
require_diagnostics_output(
    "plan\\.acceptance\\.physicalDisplayVerificationChecklist\\.0: physical4KPanel"
    "report physical 4K panel verification item")
require_diagnostics_output(
    "plan\\.acceptance\\.physicalDisplayVerificationChecklist\\.1: highDpiScale150Or200"
    "report high-DPI scale verification item")
require_diagnostics_output(
    "plan\\.acceptance\\.physicalDisplayVerificationChecklist\\.2: multiDisplayLayout"
    "report multi-display layout verification item")
require_diagnostics_output(
    "plan\\.acceptance\\.physicalDisplayVerificationChecklist\\.3: boardTextSharpness"
    "report board text sharpness verification item")
require_diagnostics_output(
    "plan\\.acceptance\\.physicalDisplayVerificationChecklist\\.4: candidateLabelsNoOverlap"
    "report candidate label overlap verification item")
require_diagnostics_output(
    "plan\\.acceptance\\.physicalDisplayVerificationChecklist\\.5: pvPreviewNoOverlap"
    "report PV preview overlap verification item")
require_diagnostics_output(
    "plan\\.acceptance\\.physicalDisplayVerificationChecklist\\.6: ownershipOverlayReadable"
    "report ownership overlay readability verification item")
require_diagnostics_output(
    "plan\\.acceptance\\.physicalDisplayVerificationChecklist\\.7: winrateScoreGraphReadable"
    "report winrate and score graph readability verification item")
require_diagnostics_output(
    "plan\\.acceptance\\.physicalDisplayVerificationChecklist\\.8: restoredWindowVisible"
    "report restored window visibility verification item")
require_diagnostics_output(
    "plan\\.acceptance\\.physicalDisplayVerificationStatus: needs-target-display-and-multi-display"
    "report physical display verification readiness status")
require_diagnostics_output(
    "plan\\.acceptance\\.physicalDisplayVerificationReadinessBlocker\\.count: 3"
    "report physical display verification readiness blocker count")
require_diagnostics_output(
    "plan\\.acceptance\\.physicalDisplayVerificationReadinessBlocker\\.0: display4K"
    "report physical display 4K readiness blocker")
require_diagnostics_output(
    "plan\\.acceptance\\.physicalDisplayVerificationReadinessBlocker\\.1: highDpiScale"
    "report physical display high-DPI readiness blocker")
require_diagnostics_output(
    "plan\\.acceptance\\.physicalDisplayVerificationReadinessBlocker\\.2: multiDisplay"
    "report physical display multi-display readiness blocker")
require_diagnostics_output(
    "plan\\.acceptance\\.status: katago-env-ready-needs-target-machine"
    "summarize offscreen diagnostics acceptance status")
require_diagnostics_output("screen\\.primary\\.index: -?[0-9]+" "report the primary screen index")
require_diagnostics_output("screen\\.primary\\.name: " "report the primary screen name")
require_diagnostics_output("screen\\.primary\\.geometry: " "report primary screen geometry")
require_diagnostics_output("screen\\.primary\\.availableGeometry: " "report primary screen available geometry")
require_diagnostics_output("screen\\.primary\\.virtualGeometry: " "report primary screen virtual geometry")
require_diagnostics_output(
    "screen\\.primary\\.availableVirtualGeometry: " "report primary screen available virtual geometry")
require_diagnostics_output("screen\\.primary\\.geometryDevicePixels: " "report primary screen geometry in device pixels")
require_diagnostics_output(
    "screen\\.primary\\.availableGeometryDevicePixels: "
    "report primary screen available geometry in device pixels")
require_diagnostics_output(
    "screen\\.primary\\.virtualGeometryDevicePixels: "
    "report primary screen virtual geometry in device pixels")
require_diagnostics_output(
    "screen\\.primary\\.availableVirtualGeometryDevicePixels: "
    "report primary screen available virtual geometry in device pixels")
require_diagnostics_output("screen\\.primary\\.orientation: " "report primary screen orientation")
require_diagnostics_output("screen\\.primary\\.primaryOrientation: " "report primary screen primary orientation")
require_diagnostics_output("screen\\.primary\\.nativeOrientation: " "report primary screen native orientation")
require_diagnostics_output("screen\\.primary\\.devicePixelRatio: " "report primary screen device-pixel ratio")
require_diagnostics_output("screen\\.primary\\.logicalDpi: " "report primary screen logical DPI")
require_diagnostics_output("screen\\.primary\\.logicalDpiX: " "report primary screen horizontal logical DPI")
require_diagnostics_output("screen\\.primary\\.logicalDpiY: " "report primary screen vertical logical DPI")
require_diagnostics_output("screen\\.primary\\.physicalDpi: " "report primary screen physical DPI")
require_diagnostics_output("screen\\.primary\\.physicalDpiX: " "report primary screen horizontal physical DPI")
require_diagnostics_output("screen\\.primary\\.physicalDpiY: " "report primary screen vertical physical DPI")
require_diagnostics_output("screen\\.primary\\.physicalSizeMm: " "report primary screen physical size")
require_diagnostics_output("screen\\.primary\\.refreshRate: " "report primary screen refresh rate")
require_diagnostics_output("screen\\.primary\\.depth: -?[0-9]+" "report primary screen color depth")
require_diagnostics_output("screen\\.primary\\.manufacturer: " "report primary screen manufacturer")
require_diagnostics_output("screen\\.primary\\.model: " "report primary screen model")
require_diagnostics_output("screen\\.primary\\.serialNumber: " "report primary screen serial number")
require_diagnostics_output("screen\\.count: [0-9]+" "report screen count")
require_diagnostics_output("screen\\.0\\.geometry: " "report screen geometry")
require_diagnostics_output("screen\\.0\\.availableGeometry: " "report available screen geometry")
require_diagnostics_output("screen\\.0\\.virtualGeometry: " "report screen virtual geometry")
require_diagnostics_output("screen\\.0\\.availableVirtualGeometry: " "report available virtual geometry")
require_diagnostics_output("screen\\.0\\.geometryDevicePixels: " "report screen geometry in device pixels")
require_diagnostics_output(
    "screen\\.0\\.availableGeometryDevicePixels: " "report available screen geometry in device pixels")
require_diagnostics_output("screen\\.0\\.virtualGeometryDevicePixels: " "report virtual geometry in device pixels")
require_diagnostics_output(
    "screen\\.0\\.availableVirtualGeometryDevicePixels: " "report available virtual geometry in device pixels")
require_diagnostics_output("screen\\.0\\.orientation: " "report screen orientation")
require_diagnostics_output("screen\\.0\\.primaryOrientation: " "report primary screen orientation")
require_diagnostics_output("screen\\.0\\.nativeOrientation: " "report native screen orientation")
require_diagnostics_output("screen\\.0\\.devicePixelRatio: " "report screen device-pixel ratio")
require_diagnostics_output("screen\\.0\\.logicalDpi: " "report logical screen DPI")
require_diagnostics_output("screen\\.0\\.logicalDpiX: " "report horizontal logical screen DPI")
require_diagnostics_output("screen\\.0\\.logicalDpiY: " "report vertical logical screen DPI")
require_diagnostics_output("screen\\.0\\.physicalDpi: " "report physical screen DPI")
require_diagnostics_output("screen\\.0\\.physicalDpiX: " "report horizontal physical screen DPI")
require_diagnostics_output("screen\\.0\\.physicalDpiY: " "report vertical physical screen DPI")
require_diagnostics_output("screen\\.0\\.physicalSizeMm: " "report physical screen size")
require_diagnostics_output("screen\\.0\\.refreshRate: " "report screen refresh rate")
require_diagnostics_output("screen\\.0\\.depth: [0-9]+" "report screen color depth")
require_diagnostics_output("screen\\.0\\.manufacturer: " "report screen manufacturer")
require_diagnostics_output("screen\\.0\\.model: " "report screen model")
require_diagnostics_output("screen\\.0\\.serialNumber: " "report screen serial number")
require_diagnostics_output("screen\\.0\\.virtualSibling\\.count: [0-9]+" "report virtual sibling count")
require_diagnostics_output("screen\\.0\\.virtualSibling\\.0\\.index: -?[0-9]+" "report virtual sibling screen index")
require_diagnostics_output("screen\\.0\\.virtualSibling\\.0\\.name: " "report virtual sibling screen name")

execute_process(
    COMMAND
        "${CMAKE_COMMAND}" -E env
        "QT_QPA_PLATFORM=offscreen"
        "QT_QPA_PLATFORM_PLUGIN_PATH="
        "PATH=${runtime_path_env_arg}"
        "LD_LIBRARY_PATH=${runtime_library_path_env_arg}"
        "DYLD_LIBRARY_PATH=${runtime_dyld_path_env_arg}"
        "QML_IMPORT_PATH=${qml_import_path_env_arg}"
        "QML2_IMPORT_PATH=${qml2_import_path_env_arg}"
        "QT_QUICK_CONTROLS_CONF=${quick_controls_conf_path}"
        "QT_QUICK_CONTROLS_STYLE_PATH=${quick_controls_style_path_env_arg}"
        "HOME=${home_dir}"
        "USERPROFILE=${user_profile_dir}"
        "APPDATA=${appdata_dir}"
        "LOCALAPPDATA=${localappdata_dir}"
        "XDG_CONFIG_HOME=${settings_home}"
        "XDG_DATA_HOME=${xdg_data_home}"
        "XDG_CACHE_HOME=${xdg_cache_home}"
        "XDG_RUNTIME_DIR=${xdg_runtime_dir}"
        "TEMP=${temp_dir}"
        "TMP=${tmp_dir}"
        "LIZZIE_DIAGNOSTICS_SETTINGS_FILE=${settings_dir}/LizzieYzy Qt.conf"
        "QT_PLUGIN_PATH=${blank_path_list_entry}"
        "KWIN_DRM_DEVICES=${kwin_drm_devices_env_arg}"
        "VK_ICD_FILENAMES=${vulkan_icd_files_env_arg}"
        "VK_DRIVER_FILES=${vulkan_driver_files_env_arg}"
        "__EGL_VENDOR_LIBRARY_FILENAMES=${egl_vendor_files_env_arg}"
        "LIZZIE_KATAGO_EXECUTABLE=${APP_EXECUTABLE}"
        "LIZZIE_KATAGO_MODEL=${model_path}"
        "LIZZIE_KATAGO_ANALYSIS_CONFIG=${analysis_config_path}"
        "LIZZIE_KATAGO_GTP_CONFIG=${gtp_config_path}"
        "${APP_EXECUTABLE}" --diagnostics "--target-acceptance-record=${acceptance_record_file}"
    WORKING_DIRECTORY "${work_dir}"
    RESULT_VARIABLE recorded_diagnostics_result
    OUTPUT_VARIABLE recorded_diagnostics_output
    ERROR_VARIABLE recorded_diagnostics_error
)
set(recorded_diagnostics_combined "${recorded_diagnostics_output}${recorded_diagnostics_error}")
if(NOT recorded_diagnostics_result EQUAL 0)
    message(FATAL_ERROR "Recorded-acceptance diagnostics command failed: ${recorded_diagnostics_combined}")
endif()

function(require_recorded_diagnostics_output pattern description)
    if(NOT "${recorded_diagnostics_combined}" MATCHES "${pattern}")
        message(FATAL_ERROR "Recorded-acceptance diagnostics output did not ${description}: ${recorded_diagnostics_combined}")
    endif()
endfunction()

function(require_no_recorded_diagnostics_output pattern description)
    if("${recorded_diagnostics_combined}" MATCHES "${pattern}")
        message(FATAL_ERROR "Recorded-acceptance diagnostics output unexpectedly ${description}: ${recorded_diagnostics_combined}")
    endif()
endfunction()

require_recorded_diagnostics_output(
    "plan\\.acceptance\\.recordFile: .*target-acceptance-record\\.ini"
    "report target acceptance record file")
require_recorded_diagnostics_output(
    "plan\\.acceptance\\.recordFile\\.set: true"
    "report target acceptance record file set flag")
require_recorded_diagnostics_output(
    "plan\\.acceptance\\.recordFile\\.exists: true"
    "report target acceptance record file exists")
require_recorded_diagnostics_output(
    "plan\\.acceptance\\.recordFile\\.readable: true"
    "report target acceptance record file readable")
require_recorded_diagnostics_output(
    "plan\\.acceptance\\.recordFile\\.canonicalPath: .*target-acceptance-record\\.ini"
    "report target acceptance record canonical path")
require_recorded_diagnostics_output(
    "plan\\.acceptance\\.recordFile\\.size: [1-9][0-9]*"
    "report target acceptance record file size")
require_recorded_diagnostics_output(
    "plan\\.acceptance\\.recordFile\\.sha256: ${acceptance_record_sha256}"
    "report target acceptance record SHA-256")
require_recorded_diagnostics_output(
    "plan\\.acceptance\\.recordFile\\.lastModifiedUtc: [0-9]"
    "report target acceptance record modification time")
require_recorded_diagnostics_output(
    "plan\\.acceptance\\.recordFile\\.timestampStatus: not-after-completed-utc"
    "report target acceptance record timestamp status")
require_recorded_diagnostics_output(
    "plan\\.acceptance\\.record\\.completedUtc: ${acceptance_completed_utc}"
    "report target acceptance completion time")
require_recorded_diagnostics_output(
    "plan\\.acceptance\\.record\\.completedUtcStatus: ready"
    "report target acceptance completion time status")
require_recorded_diagnostics_output(
    "plan\\.acceptance\\.record\\.appVersion: ${expected_app_version}"
    "report target acceptance app version")
require_recorded_diagnostics_output(
    "plan\\.acceptance\\.record\\.appVersionStatus: match"
    "report matching target acceptance app version")
require_recorded_diagnostics_output(
    "plan\\.acceptance\\.record\\.appExecutableSha256: ${expected_app_executable_sha256}"
    "report target acceptance app executable SHA-256")
require_recorded_diagnostics_output(
    "plan\\.acceptance\\.record\\.appExecutableSha256Status: match"
    "report matching target acceptance app executable SHA-256")
require_recorded_diagnostics_output(
    "plan\\.acceptance\\.record\\.osPrettyName: .+"
    "report target acceptance OS pretty name")
require_recorded_diagnostics_output(
    "plan\\.acceptance\\.record\\.osPrettyNameStatus: match"
    "report matching target acceptance OS pretty name")
require_recorded_diagnostics_output(
    "plan\\.acceptance\\.record\\.osKernelType: .+"
    "report target acceptance OS kernel type")
require_recorded_diagnostics_output(
    "plan\\.acceptance\\.record\\.osKernelTypeStatus: match"
    "report matching target acceptance OS kernel type")
require_recorded_diagnostics_output(
    "plan\\.acceptance\\.record\\.osKernelVersion: .+"
    "report target acceptance OS kernel version")
require_recorded_diagnostics_output(
    "plan\\.acceptance\\.record\\.osKernelVersionStatus: match"
    "report matching target acceptance OS kernel version")
require_recorded_diagnostics_output(
    "plan\\.acceptance\\.record\\.qtRuntimeVersion: .+"
    "report target acceptance Qt runtime version")
require_recorded_diagnostics_output(
    "plan\\.acceptance\\.record\\.qtRuntimeVersionStatus: match"
    "report matching target acceptance Qt runtime version")
require_recorded_diagnostics_output(
    "plan\\.acceptance\\.record\\.qtBuildAbi: .+"
    "report target acceptance Qt build ABI")
require_recorded_diagnostics_output(
    "plan\\.acceptance\\.record\\.qtBuildAbiStatus: match"
    "report matching target acceptance Qt build ABI")
require_recorded_diagnostics_output(
    "plan\\.acceptance\\.record\\.cpuArchitecture: .+"
    "report target acceptance CPU architecture")
require_recorded_diagnostics_output(
    "plan\\.acceptance\\.record\\.cpuArchitectureStatus: match"
    "report matching target acceptance CPU architecture")
require_recorded_diagnostics_output(
    "plan\\.acceptance\\.record\\.reviewer: target tester"
    "report target acceptance reviewer")
require_recorded_diagnostics_output(
    "plan\\.acceptance\\.record\\.targetMachine: ${expected_target_machine}"
    "report target acceptance machine")
require_recorded_diagnostics_output(
    "plan\\.acceptance\\.record\\.targetMachineStatus: match"
    "report matching target acceptance machine")
require_recorded_diagnostics_output(
    "plan\\.acceptance\\.record\\.gpuDriver: NVIDIA 555\\.55"
    "report target acceptance GPU driver")
require_recorded_diagnostics_output(
    "plan\\.acceptance\\.record\\.gpuDriverStatus: match"
    "report matching target acceptance GPU driver status")
require_recorded_diagnostics_output(
    "plan\\.acceptance\\.record\\.kataGoVersion: KataGo 1\\.15\\.3"
    "report target acceptance KataGo version")
require_recorded_diagnostics_output(
    "plan\\.acceptance\\.record\\.kataGoVersionStatus: match"
    "report matching target acceptance KataGo version status")
require_recorded_diagnostics_output(
    "plan\\.acceptance\\.record\\.notes: all target evidence attached"
    "report target acceptance notes")
require_recorded_diagnostics_output(
    "plan\\.acceptance\\.recordMetadata\\.ready: true"
    "report complete target acceptance metadata readiness")
require_recorded_diagnostics_output(
    "plan\\.acceptance\\.recordMetadata\\.blocker\\.count: 0"
    "report no target acceptance metadata blockers")
require_recorded_diagnostics_output(
    "plan\\.acceptance\\.evidence\\.diagnostics\\.value: .*target-diagnostics\\.txt"
    "report diagnostics evidence path")
require_recorded_diagnostics_output(
    "plan\\.acceptance\\.evidence\\.diagnostics\\.exists: true"
    "report diagnostics evidence exists")
require_recorded_diagnostics_output(
    "plan\\.acceptance\\.evidence\\.diagnostics\\.readable: true"
    "report diagnostics evidence readable")
require_recorded_diagnostics_output(
    "plan\\.acceptance\\.evidence\\.diagnostics\\.sha256: ${record_diagnostics_sha256}"
    "report diagnostics evidence SHA-256")
require_recorded_diagnostics_output(
    "plan\\.acceptance\\.evidence\\.diagnostics\\.expectedSha256: ${record_diagnostics_sha256}"
    "report diagnostics evidence expected SHA-256")
require_recorded_diagnostics_output(
    "plan\\.acceptance\\.evidence\\.diagnostics\\.sha256Status: match"
    "report diagnostics evidence SHA-256 match status")
require_recorded_diagnostics_output(
    "plan\\.acceptance\\.evidence\\.diagnostics\\.timestampStatus: not-after-completed-utc"
    "report diagnostics evidence timestamp status")
require_recorded_diagnostics_output(
    "plan\\.acceptance\\.evidence\\.diagnostics\\.contentStatus: match"
    "report diagnostics evidence content status")
require_recorded_diagnostics_output(
    "plan\\.acceptance\\.evidence\\.diagnostics\\.missingContentMarker\\.count: 0"
    "report no missing diagnostics evidence content markers")
require_recorded_diagnostics_output(
    "plan\\.acceptance\\.evidence\\.targetAcceptanceReport\\.exists: true"
    "report target report evidence exists")
require_recorded_diagnostics_output(
    "plan\\.acceptance\\.evidence\\.targetAcceptanceReport\\.contentStatus: match"
    "report target acceptance report evidence content status")
require_recorded_diagnostics_output(
    "plan\\.acceptance\\.evidence\\.targetAcceptanceReport\\.missingContentMarker\\.count: 0"
    "report no missing target acceptance report evidence content markers")
require_recorded_diagnostics_output(
    "plan\\.acceptance\\.evidence\\.engineLog\\.exists: true"
    "report engine log evidence exists")
require_recorded_diagnostics_output(
    "plan\\.acceptance\\.evidence\\.engineLog\\.contentStatus: match"
    "report engine log evidence content status")
require_recorded_diagnostics_output(
    "plan\\.acceptance\\.evidence\\.engineLog\\.missingContentMarker\\.count: 0"
    "report no missing engine log evidence content markers")
require_recorded_diagnostics_output(
    "plan\\.acceptance\\.evidence\\.engineLog\\.gpuOrStderrContentStatus: match"
    "report engine log GPU/stderr evidence content status")
require_recorded_diagnostics_output(
    "plan\\.acceptance\\.evidence\\.engineLog\\.missingGpuOrStderrContentMarker\\.count: 0"
    "report no missing engine log GPU/stderr evidence content markers")
require_recorded_diagnostics_output(
    "plan\\.acceptance\\.evidence\\.rawKataGoComparisonLog\\.exists: true"
    "report raw KataGo comparison evidence exists")
require_recorded_diagnostics_output(
    "plan\\.acceptance\\.evidence\\.rawKataGoComparisonLog\\.contentStatus: match"
    "report raw KataGo comparison evidence content status")
require_recorded_diagnostics_output(
    "plan\\.acceptance\\.evidence\\.rawKataGoComparisonLog\\.missingContentMarker\\.count: 0"
    "report no missing raw KataGo comparison evidence content markers")
require_recorded_diagnostics_output(
    "plan\\.acceptance\\.evidence\\.manualUiInspectionLog\\.exists: true"
    "report manual UI inspection evidence exists")
require_recorded_diagnostics_output(
    "plan\\.acceptance\\.evidence\\.manualUiInspectionLog\\.contentStatus: match"
    "report manual UI inspection evidence content status")
require_recorded_diagnostics_output(
    "plan\\.acceptance\\.evidence\\.manualUiInspectionLog\\.missingContentMarker\\.count: 0"
    "report no missing manual UI inspection evidence content markers")
require_recorded_diagnostics_output(
    "plan\\.acceptance\\.evidence\\.externalTargetVerificationLog\\.exists: true"
    "report external target verification evidence exists")
require_recorded_diagnostics_output(
    "plan\\.acceptance\\.evidence\\.externalTargetVerificationLog\\.contentStatus: match"
    "report external target verification evidence content status")
require_recorded_diagnostics_output(
    "plan\\.acceptance\\.evidence\\.externalTargetVerificationLog\\.missingContentMarker\\.count: 0"
    "report no missing external target verification evidence content markers")
require_recorded_diagnostics_output(
    "plan\\.acceptance\\.evidence\\.screenshot\\.exists: true"
    "report screenshot evidence exists")
require_recorded_diagnostics_output(
    "plan\\.acceptance\\.evidence\\.screenshot\\.imageReadable: true"
    "report readable screenshot image evidence")
require_recorded_diagnostics_output(
    "plan\\.acceptance\\.evidence\\.screenshot\\.imageFormat: ppm"
    "report screenshot image format")
require_recorded_diagnostics_output(
    "plan\\.acceptance\\.evidence\\.screenshot\\.imageWidth: 2"
    "report screenshot image width")
require_recorded_diagnostics_output(
    "plan\\.acceptance\\.evidence\\.screenshot\\.imageHeight: 1"
    "report screenshot image height")
require_recorded_diagnostics_output(
    "plan\\.acceptance\\.evidence\\.screenshot\\.imagePixelsAtLeast4K: false"
    "report screenshot 4K pixel candidate")
require_recorded_diagnostics_output(
    "plan\\.acceptance\\.evidence\\.screenshot\\.imageHasPixelVariation: true"
    "report screenshot pixel variation")
require_recorded_diagnostics_output(
    "plan\\.acceptance\\.evidence\\.screenshot\\.readyFor4KAcceptance: false"
    "report screenshot 4K acceptance readiness")
require_recorded_diagnostics_output(
    "plan\\.acceptance\\.evidence\\.ready: true"
    "report complete acceptance evidence readiness")
require_recorded_diagnostics_output(
    "plan\\.acceptance\\.evidence\\.blocker\\.count: 0"
    "report no acceptance evidence blockers")
require_recorded_diagnostics_output(
    "plan\\.acceptance\\.evidenceSha256\\.ready: true"
    "report acceptance evidence SHA-256 readiness")
require_recorded_diagnostics_output(
    "plan\\.acceptance\\.evidenceSha256\\.blocker\\.count: 0"
    "report no acceptance evidence SHA-256 blockers")
require_recorded_diagnostics_output(
    "plan\\.acceptance\\.recordTimestamp\\.ready: true"
    "report target acceptance record timestamp readiness")
require_recorded_diagnostics_output(
    "plan\\.acceptance\\.recordTimestamp\\.blocker\\.count: 0"
    "report no target acceptance record timestamp blockers")
require_recorded_diagnostics_output(
    "plan\\.acceptance\\.evidenceTimestamp\\.ready: true"
    "report acceptance evidence timestamp readiness")
require_recorded_diagnostics_output(
    "plan\\.acceptance\\.evidenceTimestamp\\.blocker\\.count: 0"
    "report no acceptance evidence timestamp blockers")
require_recorded_diagnostics_output(
    "plan\\.acceptance\\.linuxKdeWaylandNvidiaManualResult: pass"
    "normalize Linux target manual result")
require_recorded_diagnostics_output(
    "plan\\.acceptance\\.windowsNvidiaManualResult: pass"
    "normalize Windows target manual result")
require_recorded_diagnostics_output(
    "plan\\.acceptance\\.physicalDisplayManualResult: pass"
    "normalize physical display manual result")
require_recorded_diagnostics_output(
    "plan\\.acceptance\\.externalTargetVerificationManualResult: pass"
    "derive external target verification checklist result")
require_recorded_diagnostics_output(
    "plan\\.acceptance\\.checklistPassedRecord\\.count: 6"
    "report passed target checklist record count")
require_recorded_diagnostics_output(
    "plan\\.acceptance\\.checklistPassedRecord\\.[0-9]+: linuxKdeWaylandNvidia\\.linuxOs"
    "report passed Linux target checklist item")
require_recorded_diagnostics_output(
    "plan\\.acceptance\\.checklistPassedRecord\\.[0-9]+: externalTargetVerification\\.rawKataGoUiComparison"
    "report passed external target checklist item")
require_recorded_diagnostics_output(
    "plan\\.acceptance\\.checklistFailedRecord\\.count: 0"
    "report no failed target checklist records")
require_recorded_diagnostics_output(
    "plan\\.acceptance\\.checklistInvalidRecord\\.count: 0"
    "report no invalid target checklist records")
require_recorded_diagnostics_output(
    "plan\\.acceptance\\.manualFailedRecord\\.count: 0"
    "report no failed aggregate manual records")
require_recorded_diagnostics_output(
    "plan\\.acceptance\\.manualInvalidRecord\\.count: 0"
    "report no invalid aggregate manual records")
require_recorded_diagnostics_output(
    "plan\\.acceptance\\.checklistMissingRecord\\.count: 48"
    "report missing target checklist record count")
require_recorded_diagnostics_output(
    "plan\\.acceptance\\.checklistMissingRecord\\.[0-9]+: rawKataGoComparison\\.analysisJsonRawResponse"
    "report missing raw KataGo checklist item record")
require_recorded_diagnostics_output(
    "plan\\.acceptance\\.checklistMissingRecord\\.[0-9]+: manualUiInspection\\.mainWindow4KHighDpiLayout"
    "report missing manual UI checklist item record")
require_recorded_diagnostics_output(
    "plan\\.acceptance\\.checklist\\.ready: false"
    "report incomplete target acceptance checklist readiness")
require_recorded_diagnostics_output(
    "plan\\.acceptance\\.checklist\\.blocker\\.[0-9]+: rawKataGoComparison\\.analysisJsonRawResponse"
    "report incomplete raw KataGo checklist blocker")
require_recorded_diagnostics_output(
    "plan\\.acceptance\\.checklist\\.blocker\\.[0-9]+: manualUiInspection\\.mainWindow4KHighDpiLayout"
    "report incomplete manual UI checklist blocker")
require_recorded_diagnostics_output(
    "plan\\.acceptance\\.externalTargetVerificationStatus: pass"
    "derive external target verification pass status")
require_recorded_diagnostics_output(
    "plan\\.acceptance\\.rawKataGoComparisonStatus: pass"
    "normalize raw KataGo comparison pass status")
require_recorded_diagnostics_output(
    "plan\\.acceptance\\.manualUiInspectionStatus: pass"
    "normalize manual UI inspection pass status")
require_recorded_diagnostics_output(
    "plan\\.acceptance\\.finalAcceptanceBlocker\\.count: 5"
    "remove satisfied manual final acceptance blockers while keeping incomplete checklist and weak screenshot evidence blockers")
require_recorded_diagnostics_output(
    "plan\\.acceptance\\.finalAcceptanceBlocker\\.0: targetPlatform"
    "keep target-platform blocker in offscreen recorded diagnostics")
require_recorded_diagnostics_output(
    "plan\\.acceptance\\.finalAcceptanceBlocker\\.1: targetDisplay"
    "keep target-display blocker in offscreen recorded diagnostics")
require_recorded_diagnostics_output(
    "plan\\.acceptance\\.finalAcceptanceBlocker\\.2: multiDisplay"
    "keep multi-display blocker in offscreen recorded diagnostics")
require_recorded_diagnostics_output(
    "plan\\.acceptance\\.finalAcceptanceBlocker\\.3: acceptanceChecklist"
    "keep incomplete checklist blocker in recorded diagnostics")
require_recorded_diagnostics_output(
    "plan\\.acceptance\\.finalAcceptanceBlocker\\.4: screenshotEvidence4K"
    "keep weak screenshot evidence blocker in recorded diagnostics")
require_no_recorded_diagnostics_output(
    "plan\\.acceptance\\.finalAcceptanceBlocker\\.[0-9]+: (externalTargetVerification|manualRawEngineComparison|manualUiInspection)"
    "kept satisfied manual final acceptance blockers")

execute_process(
    COMMAND
        "${CMAKE_COMMAND}" -E env
        "HOME=${home_dir}"
        "XDG_CONFIG_HOME=${settings_home}"
        "XDG_DATA_HOME=${xdg_data_home}"
        "XDG_CACHE_HOME=${xdg_cache_home}"
        "XDG_RUNTIME_DIR=${xdg_runtime_dir}"
        "QT_QPA_PLATFORM=offscreen"
        "LIZZIE_KATAGO_EXECUTABLE=${APP_EXECUTABLE}"
        "LIZZIE_KATAGO_MODEL=${model_path}"
        "LIZZIE_KATAGO_ANALYSIS_CONFIG=${analysis_config_path}"
        "LIZZIE_KATAGO_GTP_CONFIG=${gtp_config_path}"
        "LIZZIE_DIAGNOSTICS_SETTINGS_FILE=${settings_dir}/LizzieYzy Qt.conf"
        "LIZZIE_TARGET_ACCEPTANCE_RECORD_FILE=${acceptance_record_complete_file}"
        "${APP_EXECUTABLE}" --diagnostics
    WORKING_DIRECTORY "${work_dir}"
    RESULT_VARIABLE complete_record_diagnostics_result
    OUTPUT_VARIABLE complete_record_diagnostics_output
    ERROR_VARIABLE complete_record_diagnostics_error
)
set(complete_record_diagnostics_combined "${complete_record_diagnostics_output}${complete_record_diagnostics_error}")
if(NOT complete_record_diagnostics_result EQUAL 0)
    message(FATAL_ERROR "Complete-record diagnostics command failed: ${complete_record_diagnostics_combined}")
endif()

function(require_complete_record_diagnostics_output pattern description)
    if(NOT "${complete_record_diagnostics_combined}" MATCHES "${pattern}")
        message(FATAL_ERROR "Complete-record diagnostics output did not ${description}: ${complete_record_diagnostics_combined}")
    endif()
endfunction()

function(require_no_complete_record_diagnostics_output pattern description)
    if("${complete_record_diagnostics_combined}" MATCHES "${pattern}")
        message(FATAL_ERROR "Complete-record diagnostics output unexpectedly ${description}: ${complete_record_diagnostics_combined}")
    endif()
endfunction()

require_complete_record_diagnostics_output(
    "plan\\.acceptance\\.evidence\\.screenshot\\.imageWidth: 3840"
    "report complete 4K screenshot width")
require_complete_record_diagnostics_output(
    "plan\\.acceptance\\.evidence\\.screenshot\\.imageHeight: 2160"
    "report complete 4K screenshot height")
require_complete_record_diagnostics_output(
    "plan\\.acceptance\\.evidence\\.screenshot\\.imagePixelsAtLeast4K: true"
    "report complete 4K screenshot pixel envelope")
require_complete_record_diagnostics_output(
    "plan\\.acceptance\\.evidence\\.screenshot\\.imageHasPixelVariation: true"
    "report complete 4K screenshot pixel variation")
require_complete_record_diagnostics_output(
    "plan\\.acceptance\\.evidence\\.screenshot\\.readyFor4KAcceptance: true"
    "report complete 4K screenshot acceptance readiness")
require_complete_record_diagnostics_output(
    "plan\\.acceptance\\.checklistMissingRecord\\.count: 0"
    "report no missing checklist records for complete diagnostics")
require_complete_record_diagnostics_output(
    "plan\\.acceptance\\.checklist\\.ready: true"
    "report complete checklist readiness in diagnostics")
require_complete_record_diagnostics_output(
    "plan\\.acceptance\\.checklist\\.blocker\\.count: 0"
    "report no checklist blockers in diagnostics")
require_complete_record_diagnostics_output(
    "plan\\.acceptance\\.checklistPassedRecord\\.[0-9]+: manualUiInspection\\.multiDisplayPlacement"
    "report all-pass manual UI checklist record")
require_complete_record_diagnostics_output(
    "plan\\.acceptance\\.finalAcceptanceBlocker\\.count: 3"
    "remove non-environment final acceptance blockers in diagnostics")
require_complete_record_diagnostics_output(
    "plan\\.acceptance\\.finalAcceptanceBlocker\\.0: targetPlatform"
    "keep target-platform blocker for complete offscreen diagnostics")
require_complete_record_diagnostics_output(
    "plan\\.acceptance\\.finalAcceptanceBlocker\\.1: targetDisplay"
    "keep target-display blocker for complete offscreen diagnostics")
require_complete_record_diagnostics_output(
    "plan\\.acceptance\\.finalAcceptanceBlocker\\.2: multiDisplay"
    "keep multi-display blocker for complete offscreen diagnostics")
require_complete_record_diagnostics_output(
    "plan\\.acceptance\\.finalAcceptanceOutstandingBlocker\\.count: 3"
    "report complete final outstanding blocker count")
require_complete_record_diagnostics_output(
    "plan\\.acceptance\\.finalAcceptanceOutstandingBlocker\\.0: targetPlatform"
    "keep target-platform outstanding blocker for complete offscreen diagnostics")
require_complete_record_diagnostics_output(
    "plan\\.acceptance\\.finalAcceptanceOutstandingBlocker\\.1: targetDisplay"
    "keep target-display outstanding blocker for complete offscreen diagnostics")
require_complete_record_diagnostics_output(
    "plan\\.acceptance\\.finalAcceptanceOutstandingBlocker\\.2: multiDisplay"
    "keep multi-display outstanding blocker for complete offscreen diagnostics")
require_no_complete_record_diagnostics_output(
    "plan\\.acceptance\\.finalAcceptanceBlocker\\.[0-9]+: (acceptanceChecklist|screenshotEvidence4K|externalTargetVerification|manualRawEngineComparison|manualUiInspection|acceptanceMetadata|acceptanceEvidence|acceptanceRecordTimestamp)"
    "kept non-environment final acceptance blockers for complete diagnostics")

execute_process(
    COMMAND
        "${CMAKE_COMMAND}" -E env
        "HOME=${home_dir}"
        "XDG_CONFIG_HOME=${settings_home}"
        "XDG_DATA_HOME=${xdg_data_home}"
        "XDG_CACHE_HOME=${xdg_cache_home}"
        "XDG_RUNTIME_DIR=${xdg_runtime_dir}"
        "QT_QPA_PLATFORM=offscreen"
        "LIZZIE_KATAGO_EXECUTABLE=${APP_EXECUTABLE}"
        "LIZZIE_KATAGO_MODEL=${model_path}"
        "LIZZIE_KATAGO_ANALYSIS_CONFIG=${analysis_config_path}"
        "LIZZIE_KATAGO_GTP_CONFIG=${gtp_config_path}"
        "LIZZIE_DIAGNOSTICS_SETTINGS_FILE=${settings_dir}/LizzieYzy Qt.conf"
        "LIZZIE_TARGET_ACCEPTANCE_RECORD_FILE=${acceptance_record_failed_manual_file}"
        "${APP_EXECUTABLE}" --diagnostics
    WORKING_DIRECTORY "${work_dir}"
    RESULT_VARIABLE failed_manual_diagnostics_result
    OUTPUT_VARIABLE failed_manual_diagnostics_output
    ERROR_VARIABLE failed_manual_diagnostics_error
)
set(failed_manual_diagnostics_combined "${failed_manual_diagnostics_output}${failed_manual_diagnostics_error}")
if(NOT failed_manual_diagnostics_result EQUAL 0)
    message(FATAL_ERROR "Failed-manual diagnostics command failed: ${failed_manual_diagnostics_combined}")
endif()

function(require_failed_manual_diagnostics_output pattern description)
    if(NOT "${failed_manual_diagnostics_combined}" MATCHES "${pattern}")
        message(FATAL_ERROR "Failed-manual diagnostics output did not ${description}: ${failed_manual_diagnostics_combined}")
    endif()
endfunction()

require_failed_manual_diagnostics_output(
    "plan\\.acceptance\\.rawKataGoComparisonStatus: fail"
    "report failed raw KataGo manual result")
require_failed_manual_diagnostics_output(
    "plan\\.acceptance\\.manualFailedRecord\\.[0-9]+: rawKataGoComparison"
    "list failed raw KataGo aggregate manual result")
require_failed_manual_diagnostics_output(
    "plan\\.acceptance\\.finalAcceptanceBlocker\\.[0-9]+: manualRawEngineComparison"
    "keep failed raw KataGo manual blocker")
require_failed_manual_diagnostics_output(
    "plan\\.acceptance\\.finalAcceptanceStatus: manual-acceptance-failed"
    "prioritize failed manual acceptance records over readiness blockers")
