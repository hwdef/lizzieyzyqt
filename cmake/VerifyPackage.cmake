if(NOT DEFINED PACKAGE_GLOB)
    message(FATAL_ERROR "PACKAGE_GLOB is required")
endif()

if(NOT DEFINED PACKAGE_EXECUTABLE)
    message(FATAL_ERROR "PACKAGE_EXECUTABLE is required")
endif()

file(GLOB packages "${PACKAGE_GLOB}")
list(LENGTH packages package_count)
if(NOT package_count EQUAL 1)
    message(FATAL_ERROR "Expected one package matching '${PACKAGE_GLOB}', found ${package_count}")
endif()

list(GET packages 0 package_path)
get_filename_component(package_name "${package_path}" NAME)
if(DEFINED PACKAGE_EXPECT_VERSION)
    set(package_expected_app_version "${PACKAGE_EXPECT_VERSION}")
else()
    string(REGEX MATCH "^LizzieYzyQt-([0-9]+(\\.[0-9]+)*)-" package_version_match "${package_name}")
    if(package_version_match)
        set(package_expected_app_version "${CMAKE_MATCH_1}")
    else()
        set(package_expected_app_version "0.1.0")
    endif()
endif()

if(DEFINED PACKAGE_EXPECT_PLATFORM)
    if(PACKAGE_EXPECT_PLATFORM STREQUAL "Linux")
        set(expected_package_name_pattern "^LizzieYzyQt-[0-9]+(\\.[0-9]+)*-Linux\\.tar\\.gz$")
        if(PACKAGE_EXECUTABLE MATCHES "\\.exe$")
            message(FATAL_ERROR "Linux package verification should use a non-.exe PACKAGE_EXECUTABLE")
        endif()
    elseif(PACKAGE_EXPECT_PLATFORM STREQUAL "Windows")
        set(expected_package_name_pattern "^LizzieYzyQt-[0-9]+(\\.[0-9]+)*-Windows\\.zip$")
        if(NOT PACKAGE_EXECUTABLE MATCHES "\\.exe$")
            message(FATAL_ERROR "Windows package verification should use an .exe PACKAGE_EXECUTABLE")
        endif()
    else()
        message(FATAL_ERROR "PACKAGE_EXPECT_PLATFORM must be Linux or Windows, got '${PACKAGE_EXPECT_PLATFORM}'")
    endif()

    if(NOT package_name MATCHES "${expected_package_name_pattern}")
        message(
            FATAL_ERROR
            "Package '${package_name}' does not match the ${PACKAGE_EXPECT_PLATFORM} platform-specific package filename pattern '${expected_package_name_pattern}'")
    endif()
endif()

execute_process(
    COMMAND "${CMAKE_COMMAND}" -E tar tf "${package_path}"
    RESULT_VARIABLE list_result
    OUTPUT_VARIABLE package_contents
    ERROR_VARIABLE list_error
)
if(NOT list_result EQUAL 0)
    message(FATAL_ERROR "Could not list package '${package_path}': ${list_error}")
endif()

string(REPLACE "\\" "/" package_contents "${package_contents}")
string(TOLOWER "${package_contents}" package_contents_lower)

function(require_package_entry suffix)
    string(REGEX MATCH "(^|\n)([^\n]*/${suffix})(\n|$)" entry_match "${package_contents}")
    if(NOT entry_match)
        message(FATAL_ERROR "Package '${package_path}' is missing '${suffix}'")
    endif()
endfunction()

function(find_package_entry suffix output_variable)
    string(REGEX MATCH "(^|\n)([^\n]*/${suffix})(\n|$)" entry_match "${package_contents}")
    if(NOT entry_match)
        message(FATAL_ERROR "Package '${package_path}' is missing '${suffix}'")
    endif()
    set("${output_variable}" "${CMAKE_MATCH_2}" PARENT_SCOPE)
endfunction()

function(forbid_package_entry regex description)
    string(REGEX MATCH "${regex}" entry_match "${package_contents_lower}")
    if(entry_match)
        message(FATAL_ERROR "Package '${package_path}' must not bundle ${description}: ${entry_match}")
    endif()
endfunction()

find_package_entry("bin/${PACKAGE_EXECUTABLE}" package_executable_entry)
find_package_entry("share/doc/LizzieYzyQt/README.md" package_readme_entry)
find_package_entry("share/doc/LizzieYzyQt/PLAN.md" package_plan_entry)
find_package_entry("share/doc/LizzieYzyQt/docs/PlanCoverage.md" package_plan_coverage_entry)
find_package_entry("share/doc/LizzieYzyQt/docs/PlanRequirementAudit.md" package_plan_requirement_audit_entry)
find_package_entry("share/doc/LizzieYzyQt/docs/Migration.md" package_migration_entry)
find_package_entry("share/doc/LizzieYzyQt/docs/Verification.md" package_verification_entry)
find_package_entry("share/doc/LizzieYzyQt/docs/TargetAcceptanceReport.md" package_target_acceptance_entry)

set(package_doc_smoke_dir_owned FALSE)
if(NOT DEFINED PACKAGE_DOC_SMOKE_DIR)
    get_filename_component(package_directory "${package_path}" DIRECTORY)
    set(PACKAGE_DOC_SMOKE_DIR "${package_directory}/verify-package-docs")
    set(package_doc_smoke_dir_owned TRUE)
endif()
file(REMOVE_RECURSE "${PACKAGE_DOC_SMOKE_DIR}")
file(MAKE_DIRECTORY "${PACKAGE_DOC_SMOKE_DIR}")
execute_process(
    COMMAND "${CMAKE_COMMAND}" -E tar xf "${package_path}"
    WORKING_DIRECTORY "${PACKAGE_DOC_SMOKE_DIR}"
    RESULT_VARIABLE doc_extract_result
    ERROR_VARIABLE doc_extract_error
)
if(NOT doc_extract_result EQUAL 0)
    message(FATAL_ERROR "Could not extract package '${package_path}' for documentation smoke: ${doc_extract_error}")
endif()

function(require_document_text document_name document_text pattern description)
    if(NOT "${document_text}" MATCHES "${pattern}")
        message(FATAL_ERROR "${document_name} should ${description}")
    endif()
endfunction()

function(forbid_document_text document_name document_text pattern description)
    if("${document_text}" MATCHES "${pattern}")
        message(FATAL_ERROR "${document_name} should not ${description}")
    endif()
endfunction()

function(require_package_target_acceptance_record_template_output source_var expected_app_version expected_executable_sha256 failure_prefix)
    set(record_template_output "${${source_var}}")

    function(_require_package_record_template pattern description)
        if(NOT "${record_template_output}" MATCHES "${pattern}")
            message(FATAL_ERROR "${failure_prefix} did not ${description}: ${record_template_output}")
        endif()
    endfunction()

    function(_require_package_record_template_match_count pattern expected_count description)
        string(REGEX MATCHALL "${pattern}" record_template_matches "${record_template_output}")
        list(LENGTH record_template_matches record_template_match_count)
        if(NOT record_template_match_count EQUAL expected_count)
            message(
                FATAL_ERROR
                "${failure_prefix} did not ${description}; expected ${expected_count}, got ${record_template_match_count}: ${record_template_output}")
        endif()
    endfunction()

    _require_package_record_template("^\\[metadata\\]" "print metadata first")
    _require_package_record_template(
        "completedUtc=[0-9][0-9][0-9][0-9]-[0-9][0-9]-[0-9][0-9]T[0-9][0-9]:[0-9][0-9]:[0-9][0-9]Z"
        "include a generated UTC timestamp")
    _require_package_record_template("appVersion=${expected_app_version}\n" "include current app version metadata")
    _require_package_record_template(
        "appExecutableSha256=${expected_executable_sha256}\n"
        "include current app executable SHA-256 metadata")
    foreach(template_metadata_key IN ITEMS
        osPrettyName
        osKernelType
        osKernelVersion
        qtRuntimeVersion
        qtBuildAbi
        cpuArchitecture
        targetMachine)
        _require_package_record_template(
            "${template_metadata_key}=[^\n]+\n"
            "include current ${template_metadata_key} metadata")
    endforeach()
    foreach(template_metadata_key IN ITEMS reviewer gpuDriver kataGoVersion notes)
        _require_package_record_template("${template_metadata_key}=\n" "include ${template_metadata_key} metadata")
    endforeach()

    _require_package_record_template("\\[evidence\\]" "include evidence section")
    _require_package_record_template("\\[evidenceSha256\\]" "include evidence SHA-256 pinning section")
    foreach(template_evidence_key IN ITEMS
        diagnostics
        targetAcceptanceReport
        engineLog
        rawKataGoComparisonLog
        manualUiInspectionLog
        externalTargetVerificationLog
        screenshot)
        _require_package_record_template_match_count(
            "(^|\n)${template_evidence_key}=\n"
            2
            "include ${template_evidence_key} evidence and SHA-256 keys")
    endforeach()

    _require_package_record_template("\\[evidenceContentMarkers\\]" "include evidence content marker guidance")
    _require_package_record_template(
        "diagnostics=LizzieYzy Qt Diagnostics.*app\\.version.*<record appVersion>.*app\\.executable.*<current app\\.executable>.*plan\\.acceptance\\.recordFile\\.sha256.*plan\\.acceptance\\.finalAcceptanceOutstandingBlocker"
        "include diagnostics evidence content markers")
    _require_package_record_template(
        "targetAcceptanceReport=# Target Acceptance Report.*app\\.version.*<record appVersion>.*app\\.executable.*<current app\\.executable>.*plan\\.acceptance\\.recordFile\\.sha256.*plan\\.acceptance\\.finalAcceptanceOutstandingBlocker"
        "include target acceptance report evidence content markers")
    _require_package_record_template(
        "engineLog=KataGo; <record kataGoVersion>"
        "include engine log evidence content markers")
    _require_package_record_template(
        "engineLog\\.gpuOrStderrAny=.*CUDA.*OpenCL.*TensorRT.*stderr:"
        "include engine log GPU/stderr marker choices")
    _require_package_record_template(
        "rawKataGoComparisonLog=.*kata-analyze.*\"moveInfos\".*\"rootInfo\".*analysisJsonRawResponse.*engineVsEngine"
        "include raw KataGo comparison evidence content markers")
    _require_package_record_template(
        "manualUiInspectionLog=.*mainWindow4KHighDpiLayout.*multiDisplayPlacement"
        "include manual UI inspection evidence content markers")
    _require_package_record_template(
        "externalTargetVerificationLog=.*linuxKdeWaylandNvidiaRealtimeKataGo.*rawKataGoUiComparison"
        "include external target verification evidence content markers")

    _require_package_record_template("\\[recordHints\\]" "include record hint guidance")
    _require_package_record_template(
        "passValues=pass; passed; accepted; true; yes"
        "include accepted pass value hints")
    _require_package_record_template(
        "failValues=fail; failed; rejected; false; no"
        "include accepted fail value hints")
    _require_package_record_template(
        "metadataKeysRequired=completedUtc; appVersion; appExecutableSha256; osPrettyName; osKernelType; osKernelVersion; qtRuntimeVersion; qtBuildAbi; cpuArchitecture; reviewer; targetMachine; gpuDriver; kataGoVersion"
        "include required metadata key hints")
    _require_package_record_template(
        "metadataExactMatchKeys=appVersion; appExecutableSha256; osPrettyName; osKernelType; osKernelVersion; qtRuntimeVersion; qtBuildAbi; cpuArchitecture; targetMachine"
        "include exact-match metadata key hints")
    _require_package_record_template(
        "metadataValueCheckedKeys=completedUtc; reviewer; gpuDriver; kataGoVersion"
        "include value-checked metadata key hints")
    _require_package_record_template(
        "completedUtcRequired=ISO-UTC-not-future"
        "include completedUtc requirement hint")
    _require_package_record_template(
        "aggregateAcceptanceKeysRequired=pass"
        "include aggregate acceptance pass requirement hint")
    _require_package_record_template(
        "checklistItemsRequired=pass"
        "include checklist pass requirement hint")
    _require_package_record_template(
        "evidencePathsRequired=readable-non-empty-distinct"
        "include evidence path requirement hint")
    _require_package_record_template(
        "evidenceSha256Required=true"
        "include evidence SHA-256 requirement hint")
    _require_package_record_template(
        "evidenceContentMarkersRequired=true"
        "include evidence content marker requirement hint")
    _require_package_record_template(
        "recordAndEvidenceTimestampsMustNotAfterCompletedUtc=true"
        "include record/evidence timestamp requirement hint")

    _require_package_record_template("\\[acceptance\\]" "include acceptance section")
    foreach(template_acceptance_key IN ITEMS
        linuxKdeWaylandNvidia
        windowsNvidia
        physicalDisplay
        externalTargetVerification
        rawKataGoComparison
        manualUiInspection)
        _require_package_record_template("${template_acceptance_key}=\n" "include ${template_acceptance_key} result key")
    endforeach()

    _require_package_record_template("\\[checklist\\.linuxKdeWaylandNvidia\\]" "include Linux target checklist")
    foreach(template_checklist_key IN ITEMS
        linuxOs
        kdeSession
        waylandSession
        qwaylandPlatformPlugin
        nvidiaEnvironmentHint
        packageStarts
        configureKataGoPaths
        realtimeAnalyzeCurrentPosition
        branchResync
        gpuStderrCaptured)
        _require_package_record_template("${template_checklist_key}=\n" "include ${template_checklist_key} checklist key")
    endforeach()

    _require_package_record_template("\\[checklist\\.windowsNvidia\\]" "include Windows checklist")
    foreach(template_checklist_key IN ITEMS
        windowsOs
        qwindowsPlatformPlugin
        packageExtracts
        appLocalQtRuntime
        nativeSettingsPathDialog
        engineDiagnosticsCaptured)
        _require_package_record_template("${template_checklist_key}=\n" "include ${template_checklist_key} checklist key")
    endforeach()

    _require_package_record_template("\\[checklist\\.physicalDisplay\\]" "include physical display checklist")
    foreach(template_checklist_key IN ITEMS
        physical4KPanel
        highDpiScale150Or200
        multiDisplayLayout
        boardTextSharpness
        candidateLabelsNoOverlap
        pvPreviewNoOverlap
        ownershipOverlayReadable
        winrateScoreGraphReadable
        restoredWindowVisible)
        _require_package_record_template("${template_checklist_key}=\n" "include ${template_checklist_key} checklist key")
    endforeach()

    _require_package_record_template("\\[checklist\\.rawKataGoComparison\\]" "include raw KataGo comparison checklist")
    foreach(template_checklist_key IN ITEMS
        analysisJsonRawResponse
        realtimeGtpRawLine
        candidateTableRendering
        pvPreview
        winrateScorePerspective
        ownershipDisplay
        genmove
        humanVsAi
        selfPlay
        engineVsEngine)
        _require_package_record_template("${template_checklist_key}=\n" "include ${template_checklist_key} checklist key")
    endforeach()

    _require_package_record_template(
        "\\[checklist\\.externalTargetVerification\\]"
        "include external target verification checklist")
    foreach(template_checklist_key IN ITEMS
        linuxKdeWaylandNvidiaRealtimeKataGo
        windowsNvidiaRealtimeKataGo
        physical4KHighDpiMultiDisplayUi
        rawKataGoUiComparison)
        _require_package_record_template("${template_checklist_key}=\n" "include ${template_checklist_key} checklist key")
    endforeach()

    _require_package_record_template("\\[checklist\\.manualUiInspection\\]" "include manual UI checklist")
    foreach(template_checklist_key IN ITEMS
        mainWindow4KHighDpiLayout
        boardLinesCoordinatesAndStarPoints
        stoneRenderingAndCandidateLabels
        candidateTableColumns
        pvPreviewStones
        ownershipMainBoardOverlay
        ownershipMiniBoardDock
        winrateScoreGraph
        toolbarDockAndMenuLayout
        bilingualTextFit
        savedWindowRestore
        multiDisplayPlacement)
        _require_package_record_template("${template_checklist_key}=\n" "include ${template_checklist_key} checklist key")
    endforeach()

    foreach(template_shared_checklist_key IN ITEMS
        nvidiaEnvironmentHint
        configureKataGoPaths
        realtimeAnalyzeCurrentPosition)
        _require_package_record_template_match_count(
            "${template_shared_checklist_key}=\n"
            2
            "include ${template_shared_checklist_key} checklist key in both Linux and Windows sections")
    endforeach()
endfunction()

file(READ "${PACKAGE_DOC_SMOKE_DIR}/${package_readme_entry}" package_readme)
file(READ "${PACKAGE_DOC_SMOKE_DIR}/${package_plan_entry}" package_plan)
file(READ "${PACKAGE_DOC_SMOKE_DIR}/${package_plan_coverage_entry}" package_plan_coverage)
file(READ "${PACKAGE_DOC_SMOKE_DIR}/${package_plan_requirement_audit_entry}" package_plan_requirement_audit)
file(READ "${PACKAGE_DOC_SMOKE_DIR}/${package_migration_entry}" package_migration)
file(READ "${PACKAGE_DOC_SMOKE_DIR}/${package_verification_entry}" package_verification)
file(READ "${PACKAGE_DOC_SMOKE_DIR}/${package_target_acceptance_entry}" package_target_acceptance)
require_document_text("README.md" "${package_readme}" "Configure Engine" "document first-run engine configuration")
require_document_text(
    "README.md" "${package_readme}" "GTP config, and analysis config"
    "document first-run analysis config")
require_document_text(
    "README.md" "${package_readme}" "platform-specific package filenames"
    "document platform-specific package filenames")
require_document_text(
    "README.md" "${package_readme}" "0.*2.*9.*handicap"
    "document New Game standard handicap option gating")
require_document_text("README.md" "${package_readme}" "No Engine Mode" "document no-engine startup")
require_document_text("README.md" "${package_readme}" "repository-root .*PLAN\\.md" "describe the PLAN.md scope without a source-only path")
require_document_text("README.md" "${package_readme}" "docs/Migration\\.md" "link Migration.md")
require_document_text("README.md" "${package_readme}" "docs/PlanRequirementAudit\\.md" "link PlanRequirementAudit.md")
require_document_text("README.md" "${package_readme}" "docs/Verification\\.md" "link Verification.md")
require_document_text("README.md" "${package_readme}" "docs/TargetAcceptanceReport\\.md" "link TargetAcceptanceReport.md")
require_document_text("README.md" "${package_readme}" "LIZZIE_KATAGO_EXECUTABLE" "document real KataGo integration")
require_document_text(
    "README.md" "${package_readme}" "KataGo env readiness summary"
    "document KataGo env readiness diagnostics")
require_document_text("README.md" "${package_readme}" "--diagnostics" "document the diagnostics CLI")
require_document_text(
    "README.md" "${package_readme}" "--target-acceptance-report"
    "document the target acceptance report CLI")
require_document_text(
    "README.md" "${package_readme}" "--target-acceptance-record-template"
    "document the target acceptance record template CLI")
require_document_text(
    "README.md" "${package_readme}" "template[ \n]+command runs before Qt platform initialization"
    "document target acceptance record template QPA independence")
require_document_text(
    "README.md" "${package_readme}" "\\[recordHints\\]"
    "document target acceptance record hint section")
require_document_text(
    "README.md" "${package_readme}" "plan\\.acceptance\\.recordHint\\.\\*"
    "document generated target acceptance record hint fields")
require_document_text(
    "README.md" "${package_readme}" "recordHint\\.metadataKeysRequired"
    "document generated target acceptance metadata record hint fields")
require_document_text(
    "README.md" "${package_readme}" "--target-acceptance-record"
    "document the target acceptance record CLI argument")
require_document_text(
    "README.md" "${package_readme}" "--target-acceptance-record -- <ini>"
    "document target acceptance record paths that start with dash")
require_document_text(
    "README.md" "${package_readme}" "pixel envelope"
    "document screenshot evidence image diagnostics")
require_document_text(
    "README.md" "${package_readme}" "screenshotEvidence4K"
    "document weak screenshot evidence final blocker")
require_document_text(
    "README.md" "${package_readme}" "acceptanceChecklist"
    "document checklist final blocker")
require_document_text(
    "README.md" "${package_readme}" "checklistMissingRecord"
    "document missing checklist record diagnostics")
require_document_text(
    "README.md" "${package_readme}" "record-file[ \n]+canonical path, size, SHA-256, and modification time"
    "document target acceptance record file digest diagnostics")
require_document_text(
    "README.md" "${package_readme}" "acceptanceEvidenceSha256"
    "document evidence SHA-256 final blocker")
require_document_text(
    "README.md" "${package_readme}" "appVersion"
    "document target acceptance app version metadata")
require_document_text(
    "README.md" "${package_readme}" "appExecutableSha256"
    "document target acceptance app executable SHA-256 metadata")
require_document_text(
    "README.md" "${package_readme}" "osPrettyName"
    "document target acceptance OS metadata")
require_document_text(
    "README.md" "${package_readme}" "qtRuntimeVersion"
    "document target acceptance Qt runtime metadata")
require_document_text(
    "README.md" "${package_readme}" "acceptanceEvidenceTimestamp"
    "document evidence timestamp final blocker")
require_document_text(
    "README.md" "${package_readme}" "common config file extensions"
    "document common KataGo config file rejection")
require_document_text("README.md" "${package_readme}" "app directory" "document app directory diagnostics")
require_document_text("README.md" "${package_readme}" "Qt library paths" "document runtime Qt library path diagnostics")
require_document_text("README.md" "${package_readme}" "Qt plugin path status" "document Qt plugin path status diagnostics")
require_document_text(
    "README.md" "${package_readme}" "Path-list environment diagnostics"
    "document path-list environment diagnostics")
require_document_text(
    "README.md" "${package_readme}" "QStandardPaths writable locations"
    "document Qt standard writable path diagnostics")
require_document_text(
    "README.md" "${package_readme}" "application font/text metrics"
    "document application font and text metrics diagnostics")
require_document_text(
    "README.md" "${package_readme}" "locale/UI-language diagnostics"
    "document locale and UI-language diagnostics")
require_document_text(
    "README.md" "${package_readme}" "app-local Qt runtime artifact diagnostics"
    "document app-local Qt runtime diagnostics")
require_document_text(
    "README.md" "${package_readme}" "runtime appearance style/palette diagnostics"
    "document runtime appearance diagnostics")
require_document_text(
    "README.md" "${package_readme}" "native file dialog/settings path selector diagnostics"
    "document native file dialog and settings selector diagnostics")
require_document_text(
    "README.md" "${package_readme}" "main-window UI structure diagnostics"
    "document main-window UI structure diagnostics")
require_document_text(
    "README.md" "${package_readme}" "stored and normalized appearance settings"
    "document stored and normalized appearance diagnostics")
require_document_text(
    "README.md" "${package_readme}" "first-run completion diagnostics"
    "document first-run completion diagnostics")
require_document_text(
    "README.md" "${package_readme}" "saved engine setting path diagnostics"
    "document saved engine setting path diagnostics")
require_document_text(
    "README.md" "${package_readme}" "stored engine path-readiness diagnostics"
    "document stored engine path-readiness diagnostics")
require_document_text(
    "README.md" "${package_readme}" "stored analysis option diagnostics"
    "document stored analysis option diagnostics")
require_document_text(
    "README.md" "${package_readme}" "stored board display diagnostics"
    "document stored board display diagnostics")
require_document_text(
    "README.md" "${package_readme}" "stored file behavior diagnostics"
    "document stored file behavior diagnostics")
require_document_text(
    "README.md" "${package_readme}" "stored shortcut diagnostics"
    "document stored shortcut diagnostics")
require_document_text(
    "README.md" "${package_readme}" "QSettings storage location"
    "document QSettings storage diagnostics")
require_document_text(
    "README.md" "${package_readme}" "QSettings session/window restore keys"
    "document QSettings session and window restore diagnostics")
require_document_text(
    "README.md" "${package_readme}" "saved session SGF path-status"
    "document saved session SGF path diagnostics")
require_document_text(
    "README.md" "${package_readme}" "saved window-geometry visibility checks"
    "document saved window geometry visibility diagnostics")
require_document_text(
    "README.md" "${package_readme}" "current platform plugin candidates"
    "document current platform plugin diagnostics")
require_document_text(
    "README.md" "${package_readme}" "common Wayland platform plugin candidates"
    "document common Wayland platform plugin diagnostics")
require_document_text(
    "README.md" "${package_readme}" "common Windows platform plugin candidates"
    "document common Windows platform plugin diagnostics")
require_document_text(
    "README.md" "${package_readme}" "available platform[ \n]+plugin"
    "document available platform plugin diagnostics")
require_document_text(
    "README.md" "${package_readme}" "target-platform summary diagnostics"
    "document target-platform summary diagnostics")
require_document_text(
    "README.md" "${package_readme}" "target-platform blocker diagnostics"
    "document target-platform blocker diagnostics")
require_document_text(
    "README.md" "${package_readme}" "same-screen target display summaries"
    "document same-screen target-display diagnostics")
require_document_text(
    "README.md" "${package_readme}" "display blocker diagnostics"
    "document display blocker diagnostics")
require_document_text(
    "README.md" "${package_readme}" "PLAN acceptance candidate flags"
    "document PLAN acceptance candidate diagnostics")
require_document_text(
    "README.md" "${package_readme}" "PLAN acceptance summary flags"
    "document PLAN acceptance summary diagnostics")
require_document_text(
    "README.md" "${package_readme}" "env-or-saved main config"
    "document env-or-saved main config acceptance diagnostics")
require_document_text(
    "README.md" "${package_readme}" "configured acceptance status"
    "document configured acceptance status diagnostics")
require_document_text(
    "README.md" "${package_readme}" "configured source diagnostics"
    "document configured source diagnostics")
require_document_text(
    "README.md" "${package_readme}" "manual verification[ \n]+candidate flags"
    "document manual verification candidate diagnostics")
require_document_text(
    "README.md" "${package_readme}" "manual verification blocker diagnostics"
    "document manual verification blocker diagnostics")
require_document_text(
    "README.md" "${package_readme}" "mode-specific final acceptance blocker diagnostics"
    "document final acceptance blocker diagnostics")
require_document_text(
    "README.md" "${package_readme}" "manual UI inspection checklist diagnostics"
    "document manual UI inspection checklist diagnostics")
require_document_text(
    "README.md" "${package_readme}" "raw KataGo comparison checklist diagnostics"
    "document raw KataGo comparison checklist diagnostics")
require_document_text(
    "README.md" "${package_readme}" "external target verification checklist"
    "document external target verification checklist diagnostics")
require_document_text(
    "README.md" "${package_readme}" "target-specific verification checklist diagnostics"
    "document target-specific verification checklist diagnostics")
require_document_text(
    "README.md" "${package_readme}" "target-specific verification status diagnostics"
    "document target-specific verification status diagnostics")
require_document_text(
    "README.md" "${package_readme}" "dual-engine acceptance candidate flags"
    "document dual-engine acceptance candidate diagnostics")
require_document_text(
    "README.md" "${package_readme}" "realtime GTP and batch-analysis acceptance candidate flags"
    "document realtime and batch-analysis acceptance candidate diagnostics")
require_document_text("README.md" "${package_readme}" "scene graph" "document Qt Quick scene graph diagnostics")
require_document_text("README.md" "${package_readme}" "QML import paths" "document QML import path diagnostics")
require_document_text("README.md" "${package_readme}" "renderer-interface" "document renderer-interface diagnostics")
require_document_text(
    "README.md" "${package_readme}" "OpenGL runtime context diagnostics"
    "document OpenGL runtime context diagnostics")
require_document_text("README.md" "${package_readme}" "Vulkan" "document GPU/Vulkan diagnostics")
require_document_text(
    "README.md" "${package_readme}" "native[ \n]+`windows` QPA plugin"
    "document Windows native QPA diagnostics")
require_document_text(
    "README.md" "${package_readme}" "Qt build ABI"
    "document OS/CPU/ABI diagnostics")
require_document_text(
    "README.md" "${package_readme}" "Qt build/runtime version match"
    "document Qt build/runtime version diagnostics")
require_document_text(
    "README.md" "${package_readme}" "Qt install paths"
    "document Qt install-path diagnostics")
require_document_text(
    "README.md" "${package_readme}" "extended graphics environment diagnostics"
    "document extended graphics environment diagnostics")
require_document_text(
    "README.md" "${package_readme}" "__EGL_VENDOR_LIBRARY_FILENAMES"
    "document EGL vendor diagnostics")
require_document_text("README.md" "${package_readme}" "screen[ \n]+orientation" "document screen orientation diagnostics")
require_document_text(
    "README.md" "${package_readme}" "device-pixel geometry"
    "document screen device-pixel geometry diagnostics")
require_document_text("README.md" "${package_readme}" "per-axis DPI" "document per-axis DPI diagnostics")
require_document_text(
    "README.md" "${package_readme}" "size, and modification time"
    "document filesystem path size and modification-time diagnostics")
require_document_text(
    "README.md" "${package_readme}" "out-of-scope artifacts"
    "document out-of-scope package artifact rejection")
require_document_text(
    "README.md" "${package_readme}" "non-KataGo engine artifacts"
    "document non-KataGo engine artifact rejection")
require_document_text("README.md" "${package_readme}" "macOS artifacts" "document macOS artifact rejection")
require_document_text(
    "README.md" "${package_readme}" "Windows runtime artifacts in Linux packages"
    "document Windows runtime artifact rejection for Linux packages")
require_document_text(
    "README.md" "${package_readme}" "Linux runtime[ \n]+artifacts in Windows packages"
    "document Linux runtime artifact rejection for Windows packages")
require_document_text("PLAN.md" "${package_plan}" "Acceptance Criteria" "ship the PLAN.md acceptance criteria")
require_document_text("PLAN.md" "${package_plan}" "KDE Wayland" "ship the target-platform acceptance scope")
require_document_text(
    "PLAN.md" "${package_plan}" "GTP config 和 analysis config"
    "ship the complete first-run engine configuration scope")
require_document_text("PlanCoverage.md" "${package_plan_coverage}" "Automated Evidence" "summarize automated coverage")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "repository-root .*PLAN\\.md"
    "describe the PLAN.md scope without a source-only path")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "External Acceptance Still Required"
    "name remaining external checks")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "PlanRequirementAudit\\.md"
    "cross-link the requirement audit")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "legacy `Lizzie[.][*]` global-state"
    "document legacy Lizzie global-state marker scanning")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "LayerBoundaryCheck\\.cmake"
    "document core/engine/app layer boundary source scanning")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "lizzie_layer_boundary_check"
    "document core/engine/app layer boundary CTest coverage")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "core/engine/app layer boundary violations"
    "document core/engine/app layer boundary violation scanning")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "executable-entry separation from the app library"
    "document executable-entry separation from the app library")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "non-engine QProcess/QThread dependency leaks"
    "document non-engine QProcess/QThread dependency leak scanning")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "app/UI engine-private process header leaks"
    "document app/UI engine-private process header leak scanning")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "engine public QThread API leaks"
    "document engine public QThread API leak scanning")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "FirstReleaseScopeCheck\\.cmake"
    "document first-release scope source scanning")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "lizzie_first_release_scope_check"
    "document first-release scope CTest coverage")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "first-release out-of-scope implementation markers"
    "document first-release out-of-scope implementation marker scanning")
require_document_text(
    "PlanRequirementAudit.md" "${package_plan_requirement_audit}" "PLAN\\.md Requirement Audit"
    "name the PLAN.md requirement audit")
require_document_text(
    "PlanRequirementAudit.md" "${package_plan_requirement_audit}" "Evidence Legend"
    "define audit evidence categories")
require_document_text(
    "PlanRequirementAudit.md" "${package_plan_requirement_audit}" "External acceptance"
    "separate target-machine acceptance evidence")
require_document_text(
    "PlanRequirementAudit.md" "${package_plan_requirement_audit}" "Optional real KataGo"
    "separate optional real KataGo evidence")
require_document_text(
    "PlanRequirementAudit.md" "${package_plan_requirement_audit}" "External acceptance record"
    "tie target-machine criteria to acceptance records")
require_document_text(
    "PlanRequirementAudit.md" "${package_plan_requirement_audit}" "KDE Wayland NVIDIA"
    "audit the Linux target acceptance criterion")
require_document_text(
    "PlanRequirementAudit.md" "${package_plan_requirement_audit}" "Windows NVIDIA"
    "audit the Windows target acceptance criterion")
require_document_text(
    "PlanRequirementAudit.md" "${package_plan_requirement_audit}" "4K pixel envelope"
    "audit physical 4K screenshot evidence")
require_document_text(
    "PlanRequirementAudit.md" "${package_plan_requirement_audit}" "No Java Swing/AWT dependency"
    "audit the no-Java-UI acceptance criterion")
require_document_text(
    "PlanRequirementAudit.md" "${package_plan_requirement_audit}" "LayerBoundaryCheck\\.cmake"
    "audit core/engine/app layer boundary enforcement")
require_document_text(
    "PlanRequirementAudit.md" "${package_plan_requirement_audit}" "executable-entry separation from the app library"
    "audit executable-entry separation from the app library")
require_document_text(
    "PlanRequirementAudit.md" "${package_plan_requirement_audit}" "non-engine `QProcess`/`QThread` API leaks"
    "audit non-engine QProcess/QThread leak enforcement")
require_document_text(
    "PlanRequirementAudit.md" "${package_plan_requirement_audit}" "app/UI engine-private process header leaks"
    "audit app/UI engine-private process header leak enforcement")
require_document_text(
    "PlanRequirementAudit.md" "${package_plan_requirement_audit}" "engine public `QThread` API leaks are rejected"
    "audit engine public QThread leak enforcement")
require_document_text(
    "PlanRequirementAudit.md" "${package_plan_requirement_audit}" "FirstReleaseScopeCheck\\.cmake"
    "audit first-release source-scope enforcement")
require_document_text(
    "PlanRequirementAudit.md" "${package_plan_requirement_audit}" "first-release out-of-scope implementation markers"
    "audit deferred-feature source-scope enforcement")
require_document_text(
    "PlanRequirementAudit.md" "${package_plan_requirement_audit}" "legacy `Lizzie[.][*]` global-state"
    "audit the legacy Lizzie global-state ban")
require_document_text(
    "PlanRequirementAudit.md" "${package_plan_requirement_audit}" "Core Model"
    "audit the core model requirements")
require_document_text(
    "PlanRequirementAudit.md" "${package_plan_requirement_audit}" "KataGo Engine"
    "audit the KataGo engine requirements")
require_document_text(
    "PlanRequirementAudit.md" "${package_plan_requirement_audit}" "UI/UX"
    "audit the UI requirements")
require_document_text(
    "PlanRequirementAudit.md" "${package_plan_requirement_audit}"
    "Human-vs-AI, self-play, dual-engine comparison, and engine-vs-engine play"
    "audit automated play requirements")
require_document_text(
    "PlanRequirementAudit.md" "${package_plan_requirement_audit}" "No macOS first-release support"
    "audit the macOS exclusion")
require_document_text(
    "PlanRequirementAudit.md" "${package_plan_requirement_audit}" "Completion Gate"
    "document the final completion gate")
require_document_text(
    "PlanRequirementAudit.md" "${package_plan_requirement_audit}" "finalAcceptanceStatus"
    "tie completion to target acceptance status")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "PLAN\\.md-triggered CI"
    "document PLAN.md-triggered CI coverage")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "Windows native `windows` QPA"
    "document Windows native QPA diagnostics coverage")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "Qt build ABI"
    "document OS/CPU/ABI diagnostics coverage")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "Qt build/runtime version match"
    "document Qt build/runtime version diagnostics coverage")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "Qt install-path diagnostics"
    "document Qt install-path diagnostics coverage")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "installed diagnostics smoke"
    "document installed diagnostics smoke coverage")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "installed target acceptance report smoke"
    "document installed target acceptance report smoke coverage")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "installed target acceptance record template smoke"
    "document installed target acceptance record template smoke coverage")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "target acceptance report CLI smoke"
    "document target acceptance report CLI smoke coverage")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "packaged target acceptance record template smoke"
    "document packaged target acceptance record template smoke coverage")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "PACKAGE_RUN_TARGET_ACCEPTANCE_RECORD_TEMPLATE_SMOKE"
    "document the independent packaged target acceptance record template smoke option")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "--target-acceptance-record-template"
    "document target acceptance record template CLI coverage")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "--target-acceptance-record"
    "document explicit target acceptance record argument coverage")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "target acceptance report configured-path and argument summaries"
    "document target acceptance report configured-path and argument coverage")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "screenshot evidence image format/dimension/4K-pixel/variation diagnostics"
    "document screenshot evidence image diagnostics coverage")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "complete 4K screenshot evidence blocker removal"
    "document complete screenshot evidence final blocker coverage")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "screenshotEvidence4K"
    "document weak screenshot evidence final blocker coverage")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "acceptanceChecklist"
    "document checklist final blocker coverage")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "ready/missing/invalid KataGo env"
    "document installed package KataGo env readiness smoke coverage")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "KataGo env readiness summary"
    "document KataGo env readiness diagnostics")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "missing, blank, and invalid path coverage"
    "document negative KataGo env readiness coverage")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "all-unset KataGo integration skip path-status diagnostics"
    "document all-unset KataGo integration skip path-status diagnostics")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "analysis-config and[ \n]+GTP-config diagnostics"
    "document mixed config diagnostic coverage")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "strict realtime GTP visit-count parsing"
    "document strict realtime GTP visit-count parsing coverage")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "strict realtime GTP PV/PV-visit validation and length consistency"
    "document strict realtime GTP PV/PV-visit validation and length consistency coverage")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "finite realtime GTP numeric parsing"
    "document finite realtime GTP numeric parsing coverage")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "per-field realtime rootInfo fallback"
    "document per-field realtime rootInfo fallback coverage")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "invalid-board realtime success-update rejection"
    "document invalid-board realtime success-update rejection coverage")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "realtime candidate ownership size filtering"
    "document realtime candidate ownership size filtering coverage")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "finite realtime direct-update ownership filtering"
    "document finite realtime direct-update ownership filtering coverage")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "finite SGF/Analysis/GTP komi handling"
    "document finite SGF/Analysis/GTP komi handling coverage")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "trimmed SGF numeric metadata parsing with positive `HA` and nonnegative `MN` filtering"
    "document SGF integer metadata range filtering coverage")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "invalid setup-position diagnostics across core, GTP sync, and batch analysis"
    "document invalid setup-position diagnostics coverage")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "invalid in-memory SGF property-name filtering"
    "document invalid SGF property-name filtering coverage")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}"
    "core/editor duplicate/conflicting setup, comment, and markup canonicalization"
    "document core/editor duplicate/conflicting setup, comment, and markup canonicalization coverage")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "finite legacy Java analysis option import"
    "document finite legacy Java analysis option import coverage")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "bounded legacy Java analysis option import"
    "document bounded legacy Java analysis option import coverage")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "strict legacy Java boolean option import"
    "document strict legacy Java boolean option import coverage")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "rootInfo ownership fallback"
    "document Analysis JSON rootInfo ownership fallback coverage")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "top-level realtime ownership precedence"
    "document top-level realtime ownership precedence coverage")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "movesOwnership realtime candidate precedence"
    "document movesOwnership realtime candidate precedence coverage")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "visits-sorted sidecar candidate reload/export"
    "document sidecar candidate ordering coverage")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "sidecar scoreLead/prior alias import"
    "document sidecar scoreLead/prior alias import coverage")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "sidecar invalid root field fallback"
    "document sidecar invalid root field fallback coverage")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "sidecar success/error exclusivity"
    "document sidecar success/error exclusivity coverage")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "sidecar top-level nodes diagnostics"
    "document sidecar top-level nodes diagnostics coverage")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "sidecar malformed node-entry diagnostics"
    "document sidecar malformed node-entry diagnostics coverage")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "strict sidecar nodeId diagnostics"
    "document strict sidecar nodeId diagnostics coverage")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "strict sidecar visit-count parsing"
    "document strict sidecar visit-count parsing coverage")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "finite sidecar numeric parsing"
    "document finite sidecar numeric parsing coverage")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "strict sidecar ownership array validation"
    "document strict sidecar ownership array validation coverage")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "trimmed numeric-string sidecar scalar/ownership parsing"
    "document numeric-string sidecar scalar and ownership parsing coverage")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "finite sidecar/SGF/readable analysis export"
    "document finite sidecar/SGF/readable analysis export coverage")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "nonnegative sidecar/SGF visit export"
    "document nonnegative sidecar/SGF visit export coverage")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "visits-sorted legacy analysis import"
    "document legacy analysis candidate ordering coverage")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "finite legacy/generated analysis numeric parsing"
    "document finite legacy/generated analysis numeric parsing coverage")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "normalized readable analysis rate export"
    "document normalized readable analysis rate export coverage")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "legacy/generated visit-count overflow rejection"
    "document legacy/generated visit-count overflow rejection coverage")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "strict legacy/generated PV validation"
    "document strict legacy/generated PV validation coverage")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "strict legacy/generated PV-visit validation"
    "document strict legacy/generated PV-visit validation coverage")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "strict sidecar/Analysis JSON PV validation"
    "document strict sidecar/Analysis JSON PV validation coverage")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "strict sidecar/Analysis JSON PV-visit validation"
    "document strict sidecar/Analysis JSON PV-visit validation coverage")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "PV/PV-visit length consistency"
    "document PV/PV-visit length consistency coverage")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "legacy candidate ownership alias import"
    "document legacy candidate ownership alias import coverage")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "strict legacy candidate ownership validation"
    "document strict legacy candidate ownership validation coverage")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "generated-comment root/candidate ownership import/write-back"
    "document generated-comment root/candidate ownership import/write-back coverage")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "strict generated-comment ownership validation"
    "document strict generated-comment ownership validation coverage")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "persisted analysis import ownership filtering"
    "document persisted analysis ownership filtering coverage")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "sidecar/SGF ownership write-back size filtering"
    "document sidecar/SGF ownership write-back filtering coverage")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "visits-sorted Analysis JSON candidates"
    "document Analysis JSON candidate ordering coverage")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "numeric-string Analysis JSON parsing"
    "document numeric-string Analysis JSON parsing coverage")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "newline-safe GTP command serialization"
    "document newline-safe GTP command serialization coverage")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "positive GTP command id serialization/reservation"
    "document positive GTP command id serialization coverage")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "threaded GTP command-id alignment"
    "document threaded GTP command id alignment coverage")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "engine-level outbound GTP command-name validation"
    "document engine-level outbound GTP command-name validation coverage")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "strict GTP response id parsing"
    "document strict GTP response id parsing coverage")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "numeric GTP response payload preservation"
    "document numeric GTP response payload coverage")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "invalid GTP board-size coordinate rejection"
    "document invalid GTP board-size coordinate rejection coverage")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "Analysis JSON request point filtering"
    "document Analysis JSON request point filtering coverage")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "Analysis JSON request[ \n]+board-size fallback"
    "document Analysis JSON request board-size fallback coverage")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "per-field Analysis JSON root fallback"
    "document per-field Analysis JSON root fallback coverage")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "zero Analysis JSON root visit fallback/rejection"
    "document zero Analysis JSON root visit fallback coverage")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "complete Analysis JSON ownership filtering"
    "document complete Analysis JSON ownership filtering coverage")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "strict Analysis JSON ownership array validation"
    "document strict Analysis JSON ownership array validation coverage")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "strict Analysis JSON response id parsing"
    "document strict Analysis JSON response id parsing coverage")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "strict Analysis JSON visit-count parsing"
    "document strict Analysis JSON visit-count parsing coverage")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "isolated Analysis JSON error payload parsing"
    "document isolated Analysis JSON error payload parsing coverage")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "bounded realtime `kata-analyze` interval serialization"
    "document bounded realtime kata-analyze interval coverage")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "finite Analysis JSON request option serialization"
    "document finite Analysis JSON request option serialization coverage")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "finite realtime search-parameter serialization"
    "document finite realtime search-parameter serialization coverage")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "positive Analysis JSON/realtime search-limit serialization"
    "document positive Analysis JSON and realtime search-limit serialization coverage")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "empty Analysis JSON success response rejection"
    "document empty Analysis JSON success response rejection coverage")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "unknown/duplicate Analysis JSON response ignoring"
    "document stale and duplicate Analysis JSON response coverage")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "invalid-board Analysis JSON success-response rejection"
    "document invalid-board Analysis JSON success-response rejection coverage")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "UI-level unknown/duplicate Analysis JSON response diagnostics"
    "document UI Analysis JSON response diagnostics coverage")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "incomplete ownership filtering"
    "document incomplete ownership filtering coverage")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "finite applied-response ownership filtering"
    "document finite applied-response ownership filtering coverage")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "finite applied-response scalar/ownership filtering"
    "document finite applied-response scalar filtering coverage")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "visits-sorted applied-response candidates[ \n]+with root fallback"
    "document visits-sorted applied-response candidate fallback coverage")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "platform-specific package filename verification"
    "document platform-specific package filename verification")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "common config file extensions"
    "document common KataGo config file rejection")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "localized main/compare clean-exit diagnostics"
    "document localized clean-exit coverage")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "zero-value unlimited/off semantics"
    "document Settings zero-value search limit semantics")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "available platform[ \n]+plugin"
    "document available platform plugin diagnostics")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "path-list environment diagnostics"
    "document path-list environment diagnostics")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "extended graphics environment diagnostics"
    "document extended graphics environment diagnostics")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "OpenGL runtime context diagnostics"
    "document OpenGL runtime context diagnostics")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "__EGL_VENDOR_LIBRARY_FILENAMES"
    "document EGL vendor diagnostics")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "QML import paths"
    "document QML import path diagnostics")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "device-pixel geometry"
    "document screen device-pixel geometry diagnostics")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "primary screen geometry"
    "document primary screen geometry diagnostics")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "per-axis DPI"
    "document per-axis DPI diagnostics")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "out-of-scope artifacts"
    "document out-of-scope package artifact rejection")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "non-KataGo engine artifacts"
    "document non-KataGo engine artifact rejection")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "macOS artifacts"
    "document macOS artifact rejection")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "Windows runtime artifacts in Linux packages"
    "document Windows runtime artifact rejection for Linux packages")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "Linux runtime artifacts in Windows packages"
    "document Linux runtime artifact rejection for Windows packages")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "app/Qt filesystem path size/mtime"
    "document app and Qt filesystem path size/mtime diagnostics")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "app-local Qt runtime artifact diagnostics"
    "document app-local Qt runtime diagnostics")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "QStandardPaths writable locations"
    "document Qt standard writable path diagnostics")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "application font/text metrics"
    "document application font and text metrics diagnostics")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "locale/UI-language diagnostics"
    "document locale and UI-language diagnostics")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "runtime appearance style/palette diagnostics"
    "document runtime appearance diagnostics")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "native file dialog/settings path selector diagnostics"
    "document native file dialog and settings selector diagnostics")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "main-window UI structure diagnostics"
    "document main-window UI structure diagnostics")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "main-window 4K/high-DPI layout geometry smoke"
    "document main-window 4K/high-DPI layout geometry smoke")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "single-token command-name validation"
    "document raw GTP console command-name validation coverage")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "balanced-quote validation"
    "document raw GTP console balanced-quote validation coverage")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "invalid raw GTP console diagnostics"
    "document invalid raw GTP console diagnostics coverage")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "stored and normalized appearance settings"
    "document stored and normalized appearance diagnostics")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "finite stored/saved UI settings normalization"
    "document finite stored and saved UI settings normalization coverage")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "strict stored boolean settings parsing"
    "document strict stored boolean settings parsing coverage")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "first-run completion diagnostics"
    "document first-run completion diagnostics")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "saved engine setting path diagnostics"
    "document saved engine setting path diagnostics")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "stored extra-args parsed diagnostics"
    "document stored extra-args parsed diagnostics")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "stored engine minimum-readiness diagnostics"
    "document stored engine minimum-readiness diagnostics")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "stored engine path-readiness diagnostics"
    "document stored engine path-readiness diagnostics")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "missing/invalid saved settings coverage"
    "document missing and invalid saved settings diagnostics coverage")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "stored analysis option diagnostics"
    "document stored analysis option diagnostics")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "stored board display diagnostics"
    "document stored board display diagnostics")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "stored file behavior diagnostics"
    "document stored file behavior diagnostics")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "stored shortcut diagnostics"
    "document stored shortcut diagnostics")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "QSettings storage location"
    "document QSettings storage diagnostics")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "QSettings session/window restore keys"
    "document QSettings session and window restore diagnostics")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "saved session SGF path-status"
    "document saved session SGF path diagnostics")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "saved window-geometry visibility checks"
    "document saved window geometry visibility diagnostics")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "saved window-state restore checks"
    "document saved window state restore diagnostics")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "invalid SGF collection-index restore diagnostics"
    "document invalid SGF collection-index restore diagnostics")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "shared window placement helper"
    "document shared window placement helper coverage")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "standard handicap option gating"
    "document New Game standard handicap option gating")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "always-reported common Wayland"
    "document common Wayland platform plugin diagnostics")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "always-reported common Windows"
    "document common Windows platform plugin diagnostics")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "target-platform summary diagnostics"
    "document target-platform summary diagnostics")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "target-platform blocker diagnostics"
    "document target-platform blocker diagnostics")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "same-screen target-display candidate diagnostics"
    "document same-screen target-display candidate diagnostics")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "display blocker diagnostics"
    "document display blocker diagnostics")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "acceptance candidate flags"
    "document acceptance candidate diagnostics")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "PLAN acceptance summary diagnostics"
    "document PLAN acceptance summary diagnostics")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "env-or-saved main config"
    "document env-or-saved main config acceptance diagnostics coverage")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "configured acceptance status"
    "document configured acceptance status diagnostics coverage")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "configured source diagnostics"
    "document configured source diagnostics coverage")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "manual[ \n]+verification candidate flags"
    "document manual verification candidate diagnostics coverage")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "manual verification blocker diagnostics"
    "document manual verification blocker diagnostics coverage")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "mode-specific final acceptance blocker diagnostics"
    "document final acceptance blocker diagnostics coverage")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "manual UI inspection checklist diagnostics"
    "document manual UI inspection checklist diagnostics coverage")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "raw KataGo comparison checklist diagnostics"
    "document raw KataGo comparison checklist diagnostics coverage")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "external target verification checklist"
    "document external target verification checklist coverage")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "target-specific verification checklist diagnostics"
    "document target-specific verification checklist coverage")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "target-specific verification status diagnostics"
    "document target-specific verification status coverage")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "final-section outstanding blocker diagnostics"
    "document final-section outstanding blocker diagnostics")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "target-machine acceptance report full evidence/SHA-256/checklist template coverage"
    "document target-machine acceptance report template coverage")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "evidence-content-marker guidance"
    "document target-machine acceptance report evidence-content-marker template guidance")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "record-hint metadata requirement manifest"
    "document target-machine acceptance report metadata record-hint coverage")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "record-hint guidance"
    "document target-machine acceptance report record-hint template guidance")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "installed strict CLI and target acceptance record argument validation smoke"
    "document installed strict CLI and target acceptance record argument validation smoke coverage")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "target acceptance report manual-record, record-file path/size/mtime/SHA-256 diagnostics, checklist-item, checklist missing-record diagnostics, checklist readiness/final `acceptanceChecklist` blocker enforcement, UTC metadata-readiness with `completedUtcStatus` future-timestamp rejection, record-file timestamp diagnostics/final `acceptanceRecordTimestamp` blocker enforcement, app-version/app-executable SHA-256/OS/Qt-runtime/CPU/GPU-driver/KataGo-version metadata match diagnostics/final `acceptanceMetadata` blocker enforcement, non-empty evidence-path ingestion with distinct-path rejection, diagnostics/report/engine-log/raw-log/manual-UI-log/external-target-log content-marker checks and missing-marker diagnostics including diagnostics app-version/executable-path/record-path/record-sha/final-blocker binding, target-report app-version/executable-path/record-path/record-sha/final-blocker binding, engine-log and raw-log recorded KataGo-version binding, engine-log GPU/stderr marker evidence gates, raw comparison checklist-item evidence markers, manual UI inspection log evidence markers, external target verification log evidence markers, and raw `kata-analyze`/`rootInfo`/`moveInfos`/`winrate`/`scoreMean`/`scoreStdev`/`visits`/`policy`/`pv`/`pvVisits`/`ownership` evidence markers with targeted winrate/scoreMean/scoreStdev/policy/visits/pv/pvVisits/ownership missing-marker diagnostics, and SHA-256 evidence digests, required evidence SHA-256 pin missing/mismatch diagnostics/final `acceptanceEvidenceSha256` blocker enforcement, evidence timestamp diagnostics/final `acceptanceEvidenceTimestamp` blocker enforcement, screenshot evidence image format/dimension/4K-pixel/variation diagnostics"
    "document target acceptance report manual-record ingestion coverage")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "complete checklist final blocker removal"
    "document complete checklist final blocker coverage")
    require_document_text(
        "PlanCoverage.md" "${package_plan_coverage}" "diagnostics complete checklist final blocker removal"
        "document diagnostics complete checklist final blocker coverage")
    require_document_text(
        "PlanCoverage.md" "${package_plan_coverage}" "installed diagnostics complete checklist final blocker smoke"
        "document installed diagnostics complete checklist final blocker package smoke")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "target-component/checklist failed/invalid manual final status priority"
    "document failed and invalid manual final status priority")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "aggregate manual failed/invalid record diagnostics"
    "document aggregate failed and invalid manual diagnostics")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "target acceptance report mode-specific final blocker smoke output"
    "document target acceptance report mode-specific final blocker package smoke coverage")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "dual-engine acceptance candidate flags"
    "document dual-engine acceptance candidate diagnostics coverage")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "realtime GTP and batch-analysis acceptance[ \n]+candidate flags"
    "document realtime and batch-analysis acceptance candidate diagnostics coverage")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "human-vs-AI, self-play, and engine-vs-engine paths"
    "document automated play workflow coverage")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "cold and already-running"
    "document automated play cold/running engine coverage")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "missing-genmove capability shutdown"
    "document automated play missing-genmove shutdown coverage")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "no stale[ \n]+follow-up or auto-restart"
    "document automated play stale reply cleanup coverage")
require_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "pending automated replies"
    "document pending automated reply cleanup coverage")
forbid_document_text("README.md" "${package_readme}" "\\.\\./PLAN\\.md" "use source-only PLAN.md paths")
forbid_document_text(
    "PlanCoverage.md" "${package_plan_coverage}" "\\.\\./PLAN\\.md|docs/Verification\\.md"
    "use source-only documentation paths")
require_document_text("Migration.md" "${package_migration}" "Java Config Import" "document Java config migration")
require_document_text(
    "Migration.md" "${package_migration}" "GTP config, and[ \n]+analysis config"
    "document first-run analysis config migration")
require_document_text("Migration.md" "${package_migration}" "quoted arguments" "document quoted command arguments")
require_document_text("Migration.md" "${package_migration}" "empty quoted arguments" "document empty command arguments")
require_document_text("Migration.md" "${package_migration}" "Windows-style paths" "document Windows-style command paths")
require_document_text("Migration.md" "${package_migration}" "sidecar" "document sidecar analysis behavior")
require_document_text("Migration.md" "${package_migration}" "Game Info" "document Game Info migration semantics")
require_document_text(
    "Migration.md" "${package_migration}" "0.*2.*9.*handicap"
    "document New Game standard handicap option gating")
require_document_text("Verification.md" "${package_verification}" "Real KataGo" "document real KataGo verification")
require_document_text(
    "Verification.md" "${package_verification}" "GTP config, and analysis config"
    "document first-run analysis config verification")
require_document_text("Verification.md" "${package_verification}" "KDE Wayland" "document KDE Wayland verification")
require_document_text("Verification.md" "${package_verification}" "Windows" "document Windows verification")
require_document_text(
    "Verification.md" "${package_verification}" "High DPI And Multi-Display"
    "document high-DPI and multi-display verification")
require_document_text("Verification.md" "${package_verification}" "--diagnostics" "document the diagnostics CLI")
require_document_text(
    "Verification.md" "${package_verification}" "--target-acceptance-report"
    "document the target acceptance report CLI")
require_document_text(
    "Verification.md" "${package_verification}" "--target-acceptance-record-template"
    "document the target acceptance record template CLI")
require_document_text(
    "Verification.md" "${package_verification}" "--target-acceptance-record"
    "document the target acceptance record CLI argument")
require_document_text(
    "Verification.md" "${package_verification}" "imagePixelsAtLeast4K"
    "document screenshot evidence image diagnostics")
require_document_text(
    "Verification.md" "${package_verification}" "imageHasPixelVariation"
    "document screenshot evidence pixel variation diagnostics")
require_document_text(
    "Verification.md" "${package_verification}" "screenshotEvidence4K"
    "document weak screenshot evidence final blocker")
require_document_text(
    "Verification.md" "${package_verification}" "installed diagnostics smoke"
    "document installed diagnostics smoke")
require_document_text(
    "Verification.md" "${package_verification}" "installed target acceptance[ \n]+report smoke"
    "document installed target acceptance report smoke")
require_document_text(
    "Verification.md" "${package_verification}" "installed target acceptance record template smoke"
    "document installed target acceptance record template smoke")
require_document_text(
    "Verification.md" "${package_verification}" "missing-qpa-for-installed-record-template-smoke"
    "document installed target acceptance record template QPA-independence")
require_document_text(
    "Verification.md" "${package_verification}" "PACKAGE_EXPECT_PLATFORM"
    "document platform-specific package filename verification")
require_document_text(
    "Verification.md" "${package_verification}" "PACKAGE_DIAGNOSTICS_QPA_PLATFORM=windows"
    "document Windows native QPA package diagnostics")
require_document_text(
    "Verification.md" "${package_verification}" "PACKAGE_RUN_TARGET_ACCEPTANCE_REPORT_SMOKE"
    "document target acceptance report package smoke")
require_document_text(
    "Verification.md" "${package_verification}" "PACKAGE_RUN_TARGET_ACCEPTANCE_RECORD_TEMPLATE_SMOKE"
    "document independent target acceptance record template package smoke")
require_document_text(
    "Verification.md" "${package_verification}" "PACKAGE_TARGET_ACCEPTANCE_REPORT_QPA_PLATFORM"
    "document target acceptance report QPA platform")
require_document_text(
    "Verification.md" "${package_verification}" "packaged[ \n]+target acceptance report CLI smoke"
    "document packaged target acceptance report CLI smoke")
require_document_text(
    "Verification.md" "${package_verification}" "invalid QPA platform"
    "document packaged target acceptance record template QPA-independent smoke")
require_document_text(
    "Verification.md" "${package_verification}" "ready KataGo env status"
    "document target acceptance report ready-env package smoke")
require_document_text(
    "Verification.md" "${package_verification}" "ready saved config status"
    "document target acceptance report saved-config package smoke")
require_document_text(
    "Verification.md" "${package_verification}" "dual-engine config[ \n]+readiness"
    "document target acceptance report dual-engine package smoke")
require_document_text(
    "Verification.md" "${package_verification}" "mode-specific final blocker package smoke"
    "document target acceptance report mode-specific final blocker package smoke")
require_document_text(
    "Verification.md" "${package_verification}" "complete 4K[ \n]+screenshot evidence blocker removal"
    "document packaged complete 4K screenshot evidence blocker removal smoke")
require_document_text(
    "Verification.md" "${package_verification}" "complete checklist final blocker[ \n]+removal"
    "document packaged complete checklist final blocker removal smoke")
require_document_text(
    "Verification.md" "${package_verification}" "raw diagnostics attached"
    "document raw diagnostics attachment for target-machine reports")
require_document_text(
    "Verification.md" "${package_verification}" "Qt build ABI"
    "document OS/CPU/ABI diagnostics")
require_document_text(
    "Verification.md" "${package_verification}" "Qt build/runtime version match"
    "document Qt build/runtime version diagnostics")
require_document_text(
    "Verification.md" "${package_verification}" "Qt install paths"
    "document Qt install-path diagnostics")
require_document_text("Verification.md" "${package_verification}" "Qt Quick graphics API" "document graphics diagnostics")
require_document_text("Verification.md" "${package_verification}" "scene graph" "document scene graph diagnostics")
require_document_text("Verification.md" "${package_verification}" "QML import paths" "document QML import path diagnostics")
require_document_text(
    "Verification.md" "${package_verification}" "renderer-interface" "document renderer-interface diagnostics")
require_document_text(
    "Verification.md" "${package_verification}" "OpenGL runtime context diagnostics"
    "document OpenGL runtime context diagnostics")
require_document_text("Verification.md" "${package_verification}" "Qt library paths" "document runtime Qt library path diagnostics")
require_document_text("Verification.md" "${package_verification}" "path-status fields" "document path-status diagnostics")
require_document_text("Verification.md" "${package_verification}" "writability" "document path writability diagnostics")
require_document_text(
    "Verification.md" "${package_verification}" "Path-list environment[ \n]+diagnostics"
    "document path-list environment diagnostics")
require_document_text(
    "Verification.md" "${package_verification}" "QStandardPaths writable locations"
    "document Qt standard writable path diagnostics")
require_document_text(
    "Verification.md" "${package_verification}" "application font/text metrics"
    "document application font and text metrics diagnostics")
require_document_text(
    "Verification.md" "${package_verification}" "locale/UI-language diagnostics"
    "document locale and UI-language diagnostics")
require_document_text(
    "Verification.md" "${package_verification}" "app-local Qt runtime artifact diagnostics"
    "document app-local Qt runtime diagnostics")
require_document_text(
    "Verification.md" "${package_verification}" "runtime appearance style/palette diagnostics"
    "document runtime appearance diagnostics")
require_document_text(
    "Verification.md" "${package_verification}" "native file dialog/settings path selector diagnostics"
    "document native file dialog and settings selector diagnostics")
require_document_text(
    "Verification.md" "${package_verification}" "main-window UI structure diagnostics"
    "document main-window UI structure diagnostics")
require_document_text(
    "Verification.md" "${package_verification}" "stored and normalized appearance settings"
    "document stored and normalized appearance diagnostics")
require_document_text(
    "Verification.md" "${package_verification}" "first-run completion diagnostics"
    "document first-run completion diagnostics")
require_document_text(
    "Verification.md" "${package_verification}" "saved engine setting path diagnostics"
    "document saved engine setting path diagnostics")
require_document_text(
    "Verification.md" "${package_verification}" "stored engine path-readiness diagnostics"
    "document stored engine path-readiness diagnostics")
require_document_text(
    "Verification.md" "${package_verification}" "missing/invalid saved settings coverage"
    "document missing and invalid saved settings diagnostics")
require_document_text(
    "Verification.md" "${package_verification}" "stored analysis option diagnostics"
    "document stored analysis option diagnostics")
require_document_text(
    "Verification.md" "${package_verification}" "stored board display diagnostics"
    "document stored board display diagnostics")
require_document_text(
    "Verification.md" "${package_verification}" "stored file behavior diagnostics"
    "document stored file behavior diagnostics")
require_document_text(
    "Verification.md" "${package_verification}" "stored shortcut diagnostics"
    "document stored shortcut diagnostics")
require_document_text(
    "Verification.md" "${package_verification}" "QSettings storage location"
    "document QSettings storage diagnostics")
require_document_text(
    "Verification.md" "${package_verification}" "QSettings session/window restore keys"
    "document QSettings session and window restore diagnostics")
require_document_text(
    "Verification.md" "${package_verification}" "saved session SGF path-status"
    "document saved session SGF path diagnostics")
require_document_text(
    "Verification.md" "${package_verification}" "saved window-geometry visibility checks"
    "document saved window geometry visibility diagnostics")
require_document_text(
    "Verification.md" "${package_verification}" "current platform plugin candidates"
    "document current platform plugin diagnostics")
require_document_text(
    "Verification.md" "${package_verification}" "common Wayland platform plugin candidates"
    "document common Wayland platform plugin diagnostics")
require_document_text(
    "Verification.md" "${package_verification}" "common Windows platform plugin candidates"
    "document common Windows platform plugin diagnostics")
require_document_text(
    "Verification.md" "${package_verification}" "available platform[ \n]+plugin"
    "document available platform plugin diagnostics")
require_document_text(
    "Verification.md" "${package_verification}" "target-platform summary diagnostics"
    "document target-platform summary diagnostics")
require_document_text(
    "Verification.md" "${package_verification}" "target-platform blocker diagnostics"
    "document target-platform blocker diagnostics")
require_document_text(
    "Verification.md" "${package_verification}" "plan\\.target\\.acceptance"
    "document target-platform acceptance candidate diagnostics")
require_document_text(
    "Verification.md" "${package_verification}" "plan\\.acceptance\\.\\*"
    "document PLAN acceptance summary diagnostics")
require_document_text(
    "Verification.md" "${package_verification}" "env-or-saved main config"
    "document env-or-saved main config acceptance diagnostics")
require_document_text(
    "Verification.md" "${package_verification}" "configuredStatus"
    "document configured acceptance status diagnostics")
require_document_text(
    "Verification.md" "${package_verification}" "recordFile"
    "document target acceptance record file diagnostics")
require_document_text(
    "Verification.md" "${package_verification}" "record\\.completedUtc"
    "document target acceptance record completion metadata")
require_document_text(
    "Verification.md" "${package_verification}" "recordFile\\.sha256"
    "document target acceptance record file SHA-256")
require_document_text(
    "Verification.md" "${package_verification}" "recordFile\\.lastModifiedUtc"
    "document target acceptance record file modification time")
require_document_text(
    "Verification.md" "${package_verification}" "recordFile\\.timestampStatus"
    "document target acceptance record file timestamp status")
require_document_text(
    "Verification.md" "${package_verification}" "completedUtcStatus"
    "document target acceptance record completion timestamp status")
require_document_text(
    "Verification.md" "${package_verification}" "record\\.appVersion"
    "document target acceptance app version")
require_document_text(
    "Verification.md" "${package_verification}" "record\\.appVersionStatus"
    "document target acceptance app version status")
require_document_text(
    "Verification.md" "${package_verification}" "record\\.appExecutableSha256"
    "document target acceptance app executable SHA-256")
require_document_text(
    "Verification.md" "${package_verification}" "record\\.appExecutableSha256Status"
    "document target acceptance app executable SHA-256 status")
require_document_text(
    "Verification.md" "${package_verification}" "record\\.osPrettyName"
    "document target acceptance OS pretty name")
require_document_text(
    "Verification.md" "${package_verification}" "record\\.osPrettyNameStatus"
    "document target acceptance OS pretty-name status")
require_document_text(
    "Verification.md" "${package_verification}" "record\\.osKernelType"
    "document target acceptance OS kernel type")
require_document_text(
    "Verification.md" "${package_verification}" "record\\.osKernelTypeStatus"
    "document target acceptance OS kernel-type status")
require_document_text(
    "Verification.md" "${package_verification}" "record\\.osKernelVersion"
    "document target acceptance OS kernel version")
require_document_text(
    "Verification.md" "${package_verification}" "record\\.osKernelVersionStatus"
    "document target acceptance OS kernel-version status")
require_document_text(
    "Verification.md" "${package_verification}" "record\\.qtRuntimeVersion"
    "document target acceptance Qt runtime version")
require_document_text(
    "Verification.md" "${package_verification}" "record\\.qtRuntimeVersionStatus"
    "document target acceptance Qt runtime version status")
require_document_text(
    "Verification.md" "${package_verification}" "record\\.qtBuildAbi"
    "document target acceptance Qt build ABI")
require_document_text(
    "Verification.md" "${package_verification}" "record\\.qtBuildAbiStatus"
    "document target acceptance Qt build ABI status")
require_document_text(
    "Verification.md" "${package_verification}" "record\\.cpuArchitecture"
    "document target acceptance CPU architecture")
require_document_text(
    "Verification.md" "${package_verification}" "record\\.cpuArchitectureStatus"
    "document target acceptance CPU architecture status")
require_document_text(
    "Verification.md" "${package_verification}" "future"
    "document target acceptance record future timestamp rejection")
require_document_text(
    "Verification.md" "${package_verification}" "record\\.reviewer"
    "document target acceptance record reviewer metadata")
require_document_text(
    "Verification.md" "${package_verification}" "record\\.targetMachineStatus"
    "document target acceptance target-machine status")
require_document_text(
    "Verification.md" "${package_verification}" "record\\.gpuDriverStatus"
    "document target acceptance GPU driver status")
require_document_text(
    "Verification.md" "${package_verification}" "record\\.kataGoVersionStatus"
    "document target acceptance KataGo version status")
require_document_text(
    "Verification.md" "${package_verification}" "recordMetadata\\.ready"
    "document target acceptance record metadata readiness")
require_document_text(
    "Verification.md" "${package_verification}" "recordMetadata\\.blocker"
    "document target acceptance record metadata blockers")
require_document_text(
    "Verification.md" "${package_verification}" "--target-acceptance-record -- <ini>"
    "document target acceptance record paths that start with dash")
require_document_text(
    "Verification.md" "${package_verification}" "recordHint\\.passValues"
    "document target acceptance record pass-value hints")
require_document_text(
    "Verification.md" "${package_verification}" "recordHint\\.metadataKeysRequired"
    "document target acceptance required metadata key hints")
require_document_text(
    "Verification.md" "${package_verification}" "recordHint\\.completedUtcRequired"
    "document target acceptance completedUtc requirement hints")
require_document_text(
    "Verification.md" "${package_verification}" "recordHint\\.checklistItemsRequired"
    "document target acceptance record checklist requirement hints")
require_document_text(
    "Verification.md" "${package_verification}" "recordHint\\.recordAndEvidenceTimestampsMustNotAfterCompletedUtc"
    "document target acceptance record timestamp requirement hints")
require_document_text(
    "Verification.md" "${package_verification}" "ISO UTC"
    "document target acceptance completedUtc timestamp format")
require_document_text(
    "Verification.md" "${package_verification}" "non-empty file"
    "document target acceptance non-empty evidence requirement")
require_document_text(
    "Verification.md" "${package_verification}" "SHA-256"
    "document target acceptance evidence digest output")
require_document_text(
    "Verification.md" "${package_verification}" "evidence\\.diagnostics"
    "document target acceptance diagnostics evidence path")
require_document_text(
    "Verification.md" "${package_verification}" "evidence\\.engineLog"
    "document target acceptance engine-log evidence path")
require_document_text(
    "Verification.md" "${package_verification}" "evidence\\.rawKataGoComparisonLog"
    "document target acceptance raw KataGo evidence path")
require_document_text(
    "Verification.md" "${package_verification}" "evidence\\.manualUiInspectionLog"
    "document target acceptance manual UI inspection evidence path")
require_document_text(
    "Verification.md" "${package_verification}" "evidence\\.externalTargetVerificationLog"
    "document target acceptance external target verification evidence path")
require_document_text(
    "Verification.md" "${package_verification}" "evidence\\.\\*\\.lastModifiedUtc"
    "document target acceptance evidence modification time")
require_document_text(
    "Verification.md" "${package_verification}" "evidence\\.\\*\\.timestampStatus"
    "document target acceptance evidence timestamp status")
require_document_text(
    "Verification.md" "${package_verification}" "contentStatus"
    "document target acceptance evidence content status")
require_document_text(
    "Verification.md" "${package_verification}" "missingContentMarker"
    "document target acceptance missing evidence content markers")
require_document_text(
    "Verification.md" "${package_verification}" "\\[evidenceContentMarkers\\]"
    "document target acceptance record template evidence content marker guidance")
require_document_text(
    "Verification.md" "${package_verification}" "\\[recordHints\\]"
    "document target acceptance record template hint guidance")
require_document_text(
    "Verification.md" "${package_verification}" "diagnostics=LizzieYzy Qt Diagnostics"
    "document diagnostics evidence content marker guidance")
require_document_text(
    "Verification.md" "${package_verification}" "targetAcceptanceReport=# Target Acceptance Report"
    "document target acceptance report evidence content marker guidance")
require_document_text(
    "Verification.md" "${package_verification}" "passValues=pass; passed; accepted; true; yes"
    "document accepted pass value record hints")
require_document_text(
    "Verification.md" "${package_verification}" "checklistItemsRequired=pass"
    "document checklist pass requirement record hint")
require_document_text(
    "Verification.md" "${package_verification}" "final blocker fields"
    "document target report evidence final blocker marker requirements")
require_document_text(
    "Verification.md" "${package_verification}" "evidence\\.engineLog\\.contentStatus"
    "document target acceptance engine-log content status")
require_document_text(
    "Verification.md" "${package_verification}" "engineLog\\.gpuOrStderrContentStatus"
    "document target acceptance engine-log GPU/stderr content status")
require_document_text(
    "Verification.md" "${package_verification}" "engineLog\\.missingGpuOrStderrContentMarker"
    "document target acceptance engine-log GPU/stderr missing marker diagnostics")
require_document_text(
    "Verification.md" "${package_verification}" "evidence\\.rawKataGoComparisonLog\\.contentStatus"
    "document target acceptance raw-log content status")
require_document_text(
    "Verification.md" "${package_verification}" "evidence\\.manualUiInspectionLog\\.contentStatus"
    "document target acceptance manual UI log content status")
require_document_text(
    "Verification.md" "${package_verification}" "evidence\\.externalTargetVerificationLog\\.contentStatus"
    "document target acceptance external target verification log content status")
require_document_text(
    "Verification.md" "${package_verification}" "raw comparison checklist item names"
    "document target acceptance raw-log checklist item evidence markers")
require_document_text(
    "Verification.md" "${package_verification}" "manual UI inspection checklist item names"
    "document target acceptance manual UI checklist item evidence markers")
require_document_text(
    "Verification.md" "${package_verification}" "external target verification checklist item names"
    "document target acceptance external target checklist item evidence markers")
require_document_text(
    "Verification.md" "${package_verification}" "kata-analyze"
    "document raw KataGo comparison realtime GTP evidence marker")
require_document_text(
    "Verification.md" "${package_verification}" "moveInfos"
    "document raw KataGo comparison Analysis JSON evidence marker")
require_document_text(
    "Verification.md" "${package_verification}" "rootInfo"
    "document raw KataGo comparison rootInfo evidence marker")
require_document_text(
    "Verification.md" "${package_verification}" "winrate"
    "document raw KataGo comparison winrate evidence marker")
require_document_text(
    "Verification.md" "${package_verification}" "scoreMean"
    "document raw KataGo comparison scoreMean evidence marker")
require_document_text(
    "Verification.md" "${package_verification}" "scoreStdev"
    "document raw KataGo comparison scoreStdev evidence marker")
require_document_text(
    "Verification.md" "${package_verification}" "visits"
    "document raw KataGo comparison visits evidence marker")
require_document_text(
    "Verification.md" "${package_verification}" "policy"
    "document raw KataGo comparison policy evidence marker")
require_document_text(
    "Verification.md" "${package_verification}" "`pv`"
    "document raw KataGo comparison PV evidence marker")
require_document_text(
    "Verification.md" "${package_verification}" "pvVisits"
    "document raw KataGo comparison PV visits evidence marker")
require_document_text(
    "Verification.md" "${package_verification}" "ownership"
    "document raw KataGo comparison ownership evidence marker")
require_document_text(
    "Verification.md" "${package_verification}" "LizzieYzy Qt Diagnostics"
    "document diagnostics evidence marker")
require_document_text(
    "Verification.md" "${package_verification}" "evidence\\.ready"
    "document target acceptance evidence readiness")
require_document_text(
    "Verification.md" "${package_verification}" "evidence\\.blocker"
    "document target acceptance evidence blockers")
require_document_text(
    "Verification.md" "${package_verification}" "distinct canonical files"
    "document target acceptance distinct evidence paths")
require_document_text(
    "Verification.md" "${package_verification}" "expectedSha256"
    "document target acceptance expected evidence SHA-256")
require_document_text(
    "Verification.md" "${package_verification}" "sha256Status"
    "document target acceptance evidence SHA-256 status")
require_document_text(
    "Verification.md" "${package_verification}" "evidenceSha256\\.ready"
    "document target acceptance evidence SHA-256 readiness")
require_document_text(
    "Verification.md" "${package_verification}" "evidenceSha256\\.blocker"
    "document target acceptance evidence SHA-256 blockers")
require_document_text(
    "Verification.md" "${package_verification}" "recordTimestamp\\.ready"
    "document target acceptance record timestamp readiness")
require_document_text(
    "Verification.md" "${package_verification}" "recordTimestamp\\.blocker"
    "document target acceptance record timestamp blockers")
require_document_text(
    "Verification.md" "${package_verification}" "evidenceTimestamp\\.ready"
    "document target acceptance evidence timestamp readiness")
require_document_text(
    "Verification.md" "${package_verification}" "evidenceTimestamp\\.blocker"
    "document target acceptance evidence timestamp blockers")
require_document_text(
    "Verification.md" "${package_verification}" "acceptanceEvidenceTimestamp"
    "document evidence timestamp final blocker")
require_document_text(
    "Verification.md" "${package_verification}" "acceptanceRecordTimestamp"
    "document record timestamp final blocker")
require_document_text(
    "Verification.md" "${package_verification}" "linuxKdeWaylandNvidiaManualResult"
    "document Linux manual acceptance result diagnostics")
require_document_text(
    "Verification.md" "${package_verification}" "windowsNvidiaManualResult"
    "document Windows manual acceptance result diagnostics")
require_document_text(
    "Verification.md" "${package_verification}" "physicalDisplayManualResult"
    "document physical display manual acceptance result diagnostics")
require_document_text(
    "Verification.md" "${package_verification}" "externalTargetVerificationManualResult"
    "document external target manual acceptance result diagnostics")
require_document_text(
    "Verification.md" "${package_verification}" "checklistPassedRecord"
    "document passed checklist item record diagnostics")
require_document_text(
    "Verification.md" "${package_verification}" "checklistFailedRecord"
    "document failed checklist item record diagnostics")
require_document_text(
    "Verification.md" "${package_verification}" "checklistInvalidRecord"
    "document invalid checklist item record diagnostics")
require_document_text(
    "Verification.md" "${package_verification}" "checklistMissingRecord"
    "document missing checklist item record diagnostics")
require_document_text(
    "Verification.md" "${package_verification}" "manualFailedRecord"
    "document failed aggregate manual record diagnostics")
require_document_text(
    "Verification.md" "${package_verification}" "manualInvalidRecord"
    "document invalid aggregate manual record diagnostics")
require_document_text(
    "Verification.md" "${package_verification}" "plan\\.acceptance\\.checklist\\.ready"
    "document checklist readiness diagnostics")
require_document_text(
    "Verification.md" "${package_verification}" "acceptanceChecklist"
    "document checklist final blocker")
require_document_text(
    "Verification.md" "${package_verification}" "acceptanceEvidenceSha256"
    "document evidence SHA-256 final blocker")
require_document_text(
    "Verification.md" "${package_verification}" "configuredMainConfigSource"
    "document configured main source diagnostics")
require_document_text(
    "Verification.md" "${package_verification}" "configuredCompareConfigSource"
    "document configured compare source diagnostics")
require_document_text(
    "Verification.md" "${package_verification}" "ManualVerificationCandidate"
    "document manual verification candidate diagnostics")
require_document_text(
    "Verification.md" "${package_verification}" "ManualVerificationBlocker"
    "document manual verification blocker diagnostics")
require_document_text(
    "Verification.md" "${package_verification}" "mode-specific final acceptance[ \n]+blocker diagnostics"
    "document final acceptance blocker diagnostics")
require_document_text(
    "Verification.md" "${package_verification}" "manualUiInspectionChecklist"
    "document manual UI inspection checklist diagnostics")
require_document_text(
    "Verification.md" "${package_verification}" "externalTargetVerificationChecklist"
    "document external target verification checklist diagnostics")
require_document_text(
    "Verification.md" "${package_verification}" "externalTargetVerificationStatus"
    "document external target verification status diagnostics")
require_document_text(
    "Verification.md" "${package_verification}" "rawKataGoComparisonStatus"
    "document raw KataGo comparison status diagnostics")
require_document_text(
    "Verification.md" "${package_verification}" "manualUiInspectionStatus"
    "document manual UI inspection status diagnostics")
require_document_text(
    "Verification.md" "${package_verification}" "finalAcceptanceStatus"
    "document final acceptance status diagnostics")
require_document_text(
    "Verification.md" "${package_verification}" "finalAcceptanceOutstandingBlocker"
    "document final acceptance outstanding blocker diagnostics")
require_document_text(
    "Verification.md" "${package_verification}" "linuxKdeWaylandNvidiaVerificationChecklist"
    "document Linux target verification checklist diagnostics")
require_document_text(
    "Verification.md" "${package_verification}" "windowsNvidiaVerificationChecklist"
    "document Windows target verification checklist diagnostics")
require_document_text(
    "Verification.md" "${package_verification}" "physicalDisplayVerificationChecklist"
    "document physical display verification checklist diagnostics")
require_document_text(
    "Verification.md" "${package_verification}" "VerificationStatus"
    "document target verification status diagnostics")
require_document_text(
    "Verification.md" "${package_verification}" "VerificationReadinessBlocker"
    "document target verification readiness blocker diagnostics")
require_document_text(
    "Verification.md" "${package_verification}" "rawKataGoComparisonChecklist"
    "document raw KataGo comparison checklist diagnostics")
require_document_text(
    "Verification.md" "${package_verification}" "configuredDualEngine"
    "document dual-engine acceptance candidate diagnostics")
require_document_text(
    "Verification.md" "${package_verification}" "configuredRealtimeGtp"
    "document realtime GTP acceptance candidate diagnostics")
require_document_text(
    "Verification.md" "${package_verification}" "configuredBatchAnalysis"
    "document batch analysis acceptance candidate diagnostics")
require_document_text(
    "Verification.md" "${package_verification}" "same-screen target-display candidates"
    "document same-screen target-display verification")
require_document_text(
    "Verification.md" "${package_verification}" "display blocker diagnostics"
    "document display blocker diagnostics")
require_document_text("Verification.md" "${package_verification}" "QT_PLUGIN_PATH" "document Qt plugin env diagnostics")
require_document_text("Verification.md" "${package_verification}" "QSG_RHI_BACKEND" "document Qt RHI env diagnostics")
require_document_text("Verification.md" "${package_verification}" "VK_INSTANCE_LAYERS" "document Vulkan layer env diagnostics")
require_document_text("Verification.md" "${package_verification}" "NVIDIA_VISIBLE_DEVICES" "document NVIDIA env diagnostics")
require_document_text(
    "Verification.md" "${package_verification}" "extended graphics environment diagnostics"
    "document extended graphics environment diagnostics")
require_document_text(
    "Verification.md" "${package_verification}" "__EGL_VENDOR_LIBRARY_FILENAMES"
    "document EGL vendor diagnostics")
require_document_text("Verification.md" "${package_verification}" "katago.env.executable" "document KataGo env diagnostics")
require_document_text(
    "Verification.md" "${package_verification}" "KataGo env readiness summary"
    "document KataGo env readiness diagnostics")
require_document_text(
    "Verification.md" "${package_verification}" "common config file extensions"
    "document common KataGo config file rejection")
require_document_text(
    "Verification.md" "${package_verification}" "screen\\.0\\.virtualSibling\\.count"
    "document virtual-sibling screen diagnostics")
require_document_text(
    "Verification.md" "${package_verification}" "device-pixel geometr"
    "document screen device-pixel geometry diagnostics")
require_document_text(
    "Verification.md" "${package_verification}" "primary screen geometry"
    "document primary screen geometry diagnostics")
require_document_text(
    "Verification.md" "${package_verification}" "per-axis DPI"
    "document per-axis DPI diagnostics")
require_document_text(
    "Verification.md" "${package_verification}" "main-window 4K/high-DPI layout geometry smoke"
    "document main-window 4K/high-DPI layout geometry smoke")
require_document_text(
    "Verification.md" "${package_verification}" "out-of-scope artifacts"
    "document out-of-scope package artifact rejection")
require_document_text(
    "Verification.md" "${package_verification}" "non-KataGo engine artifacts"
    "document non-KataGo engine artifact rejection")
require_document_text(
    "Verification.md" "${package_verification}" "macOS artifacts"
    "document macOS artifact rejection")
require_document_text(
    "Verification.md" "${package_verification}" "Windows runtime[ \n]+artifacts"
    "document Windows runtime artifact rejection for Linux packages")
require_document_text(
    "Verification.md" "${package_verification}" "Linux runtime[ \n]+artifacts"
    "document Linux runtime artifact rejection for Windows packages")
require_document_text(
    "Verification.md" "${package_verification}" "size/modification time"
    "document filesystem path size and modification-time diagnostics")
require_document_text(
    "Verification.md" "${package_verification}" "\\[evidenceSha256\\]"
    "document target acceptance evidence SHA-256 record section")
require_document_text(
    "Verification.md" "${package_verification}" "\\[checklist\\.linuxKdeWaylandNvidia\\]"
    "document Linux target checklist record section")
require_document_text(
    "Verification.md" "${package_verification}" "\\[checklist\\.windowsNvidia\\]"
    "document Windows target checklist record section")
require_document_text(
    "Verification.md" "${package_verification}" "\\[checklist\\.physicalDisplay\\]"
    "document physical display checklist record section")
require_document_text(
    "Verification.md" "${package_verification}" "\\[checklist\\.rawKataGoComparison\\]"
    "document raw KataGo comparison checklist record section")
require_document_text(
    "Verification.md" "${package_verification}" "\\[checklist\\.externalTargetVerification\\]"
    "document external target verification checklist record section")
require_document_text(
    "Verification.md" "${package_verification}" "\\[checklist\\.manualUiInspection\\]"
    "document manual UI inspection checklist record section")
require_document_text(
    "Verification.md" "${package_verification}" "windowsOs=pass"
    "document Windows checklist item pass record")
require_document_text(
    "Verification.md" "${package_verification}" "physical4KPanel=pass"
    "document physical-display checklist item pass record")
require_document_text(
    "Verification.md" "${package_verification}" "linuxKdeWaylandNvidiaRealtimeKataGo=pass"
    "document external target checklist item pass record")
require_document_text(
    "Verification.md" "${package_verification}" "mainWindow4KHighDpiLayout=pass"
    "document manual UI checklist item pass record")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "Target Acceptance Report"
    "provide a target acceptance report title")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "--target-acceptance-report"
    "document the generated target acceptance report command")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "--target-acceptance-record-template"
    "document the target acceptance record template command")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "record-template command runs before Qt platform initialization"
    "document target acceptance record template QPA independence")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "--target-acceptance-record"
    "document the target acceptance record CLI argument")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "imagePixelsAtLeast4K"
    "document screenshot evidence image diagnostics")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "imageHasPixelVariation"
    "document screenshot evidence pixel variation diagnostics")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "readyFor4KAcceptance"
    "document screenshot evidence 4K readiness")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "screenshotEvidence4K"
    "document weak screenshot evidence final blocker")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "app\\.version"
    "capture app version in the target acceptance report")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "generatedUtc"
    "capture target acceptance report generation timestamp")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "app\\.executable"
    "capture target acceptance app executable path")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "app\\.dir"
    "capture target acceptance app directory")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "process\\.currentWorkingDirectory"
    "capture target acceptance working directory")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "qt\\.version"
    "capture target acceptance Qt runtime version")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "qt\\.platform"
    "capture target acceptance Qt platform plugin")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "os\\.prettyName"
    "capture target acceptance OS pretty name")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "os\\.kernelType"
    "capture target acceptance OS kernel type")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "os\\.kernelVersion"
    "capture target acceptance OS kernel version")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "plan\\.target\\.osFamily"
    "capture target acceptance OS family")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "Configured Paths"
    "include the target acceptance configured path section")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "katago\\.env\\.executable"
    "capture target acceptance KataGo env executable path")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "katago\\.env\\.model"
    "capture target acceptance KataGo env model path")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "katago\\.env\\.gtpConfig"
    "capture target acceptance KataGo env GTP config path")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "katago\\.env\\.analysisConfig"
    "capture target acceptance KataGo env analysis config path")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "qt\\.settings\\.diagnosticsOverrideFile"
    "capture target acceptance diagnostics settings file")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "qt\\.settings\\.fileName"
    "capture target acceptance QSettings file")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "qt\\.settings\\.engine\\.executable\\.value"
    "capture target acceptance saved main executable path")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "qt\\.settings\\.engine\\.model\\.value"
    "capture target acceptance saved main model path")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "qt\\.settings\\.engine\\.gtpConfig\\.value"
    "capture target acceptance saved main GTP config path")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "qt\\.settings\\.engine\\.analysisConfig\\.value"
    "capture target acceptance saved main analysis config path")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "qt\\.settings\\.engine\\.workingDirectory\\.value"
    "capture target acceptance saved main working directory")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "qt\\.settings\\.engine\\.extraArgs\\.value"
    "capture target acceptance saved main extra args")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "qt\\.settings\\.engine\\.extraArgs\\.parsed\\.count"
    "capture target acceptance parsed main extra args")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "qt\\.settings\\.compare\\.executable\\.value"
    "capture target acceptance saved compare executable path")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "qt\\.settings\\.compare\\.model\\.value"
    "capture target acceptance saved compare model path")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "qt\\.settings\\.compare\\.gtpConfig\\.value"
    "capture target acceptance saved compare GTP config path")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "qt\\.settings\\.compare\\.analysisConfig\\.value"
    "capture target acceptance saved compare analysis config path")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "qt\\.settings\\.compare\\.workingDirectory\\.value"
    "capture target acceptance saved compare working directory")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "qt\\.settings\\.compare\\.extraArgs\\.value"
    "capture target acceptance saved compare extra args")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "qt\\.settings\\.compare\\.extraArgs\\.parsed\\.count"
    "capture target acceptance parsed compare extra args")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "plan\\.acceptance\\.status"
    "capture PLAN acceptance status diagnostics")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "configuredStatus"
    "capture configured acceptance status diagnostics")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "realKataGoEnvReady"
    "capture real KataGo env ready diagnostics")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "plan\\.target\\.inFirstReleaseScope"
    "capture first-release OS scope diagnostics")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "targetPlatformCandidate"
    "capture target platform acceptance candidate diagnostics")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "realKataGoTargetRunCandidate"
    "capture real KataGo target-run candidate diagnostics")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "realKataGoManualVerificationCandidate"
    "capture real KataGo manual verification candidate diagnostics")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "savedMainConfigReady"
    "capture saved main config readiness diagnostics")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "savedMainGtpReady"
    "capture saved main realtime GTP readiness diagnostics")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "savedMainAnalysisReady"
    "capture saved main batch-analysis readiness diagnostics")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "savedCompareConfigReady"
    "capture saved compare config readiness diagnostics")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "savedCompareGtpReady"
    "capture saved compare realtime GTP readiness diagnostics")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "savedCompareAnalysisReady"
    "capture saved compare batch-analysis readiness diagnostics")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "envOrSavedMainConfigReady"
    "capture env-or-saved main config readiness diagnostics")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "envOrSavedMainGtpReady"
    "capture env-or-saved main realtime GTP readiness diagnostics")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "envOrSavedMainAnalysisReady"
    "capture env-or-saved main batch-analysis readiness diagnostics")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "configuredMainConfigSource"
    "capture configured main config source diagnostics")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "configuredMainGtpSource"
    "capture configured main realtime GTP source diagnostics")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "configuredMainAnalysisSource"
    "capture configured main batch-analysis source diagnostics")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "configuredCompareConfigSource"
    "capture configured compare config source diagnostics")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "configuredCompareGtpSource"
    "capture configured compare realtime GTP source diagnostics")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "configuredCompareAnalysisSource"
    "capture configured compare batch-analysis source diagnostics")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "configuredKataGoTargetRunCandidate"
    "capture configured KataGo target-run candidate diagnostics")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "configuredManualVerificationCandidate"
    "capture configured manual verification candidate diagnostics")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "configuredRealtimeGtpTargetRunCandidate"
    "capture realtime GTP target-run candidate diagnostics")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "configuredBatchAnalysisTargetRunCandidate"
    "capture batch-analysis target-run candidate diagnostics")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "configuredRealtimeGtpManualVerificationCandidate"
    "capture realtime GTP manual verification candidate diagnostics")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "configuredBatchAnalysisManualVerificationCandidate"
    "capture batch-analysis manual verification candidate diagnostics")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "configuredDualEngineConfigReady"
    "capture dual-engine config readiness diagnostics")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "configuredDualEngineTargetRunCandidate"
    "capture dual-engine target-run candidate diagnostics")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "configuredDualEngineManualVerificationCandidate"
    "capture dual-engine manual verification candidate diagnostics")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "configuredDualEngineGtpReady"
    "capture dual-engine realtime GTP readiness diagnostics")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "configuredDualEngineAnalysisReady"
    "capture dual-engine batch-analysis readiness diagnostics")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "configuredDualRealtimeGtpTargetRunCandidate"
    "capture dual realtime GTP target-run candidate diagnostics")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "configuredDualBatchAnalysisTargetRunCandidate"
    "capture dual batch-analysis target-run candidate diagnostics")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "configuredDualRealtimeGtpManualVerificationCandidate"
    "capture dual realtime GTP manual verification candidate diagnostics")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "configuredDualBatchAnalysisManualVerificationCandidate"
    "capture dual batch-analysis manual verification candidate diagnostics")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "firstReleaseNvidiaPlatformBlocker"
    "capture first-release NVIDIA platform blocker diagnostics")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "linuxKdeWayland\\.detected"
    "capture Linux KDE Wayland target detection diagnostics")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "linuxKdeWaylandNvidiaBlocker"
    "capture Linux KDE Wayland NVIDIA blocker diagnostics")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "windows\\.detected"
    "capture Windows target detection diagnostics")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "windowsNvidiaBlocker"
    "capture Windows NVIDIA blocker diagnostics")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "targetDisplayBlocker"
    "capture target display blocker diagnostics")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "multiDisplayBlocker"
    "capture multi-display blocker diagnostics")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "primaryTargetScreenCandidate"
    "capture primary target display diagnostics")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "display4KCandidate"
    "capture PLAN acceptance 4K display candidate diagnostics")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "highDpiCandidate"
    "capture PLAN acceptance high-DPI candidate diagnostics")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "sameScreenTargetDisplayCandidate"
    "capture same-screen target-display candidate diagnostics")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "recordFile"
    "capture target acceptance record file diagnostics")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "record\\.completedUtc"
    "capture target acceptance record completion metadata")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "recordFile\\.sha256"
    "capture target acceptance record file SHA-256")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "recordFile\\.lastModifiedUtc"
    "capture target acceptance record file modification time")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "recordFile\\.timestampStatus"
    "capture target acceptance record file timestamp status")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "completedUtcStatus"
    "capture target acceptance record completion timestamp status")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "record\\.appVersion"
    "capture target acceptance app version")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "record\\.appVersionStatus"
    "capture target acceptance app version status")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "record\\.appExecutableSha256"
    "capture target acceptance app executable SHA-256")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "record\\.appExecutableSha256Status"
    "capture target acceptance app executable SHA-256 status")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "record\\.osPrettyName"
    "capture target acceptance OS pretty name")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "record\\.osPrettyNameStatus"
    "capture target acceptance OS pretty-name status")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "record\\.osKernelType"
    "capture target acceptance OS kernel type")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "record\\.osKernelTypeStatus"
    "capture target acceptance OS kernel-type status")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "record\\.osKernelVersion"
    "capture target acceptance OS kernel version")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "record\\.osKernelVersionStatus"
    "capture target acceptance OS kernel-version status")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "record\\.qtRuntimeVersion"
    "capture target acceptance Qt runtime version")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "record\\.qtRuntimeVersionStatus"
    "capture target acceptance Qt runtime version status")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "record\\.qtBuildAbi"
    "capture target acceptance Qt build ABI")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "record\\.qtBuildAbiStatus"
    "capture target acceptance Qt build ABI status")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "record\\.cpuArchitecture"
    "capture target acceptance CPU architecture")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "record\\.cpuArchitectureStatus"
    "capture target acceptance CPU architecture status")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "future"
    "capture target acceptance record future timestamp rejection")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "record\\.reviewer"
    "capture target acceptance record reviewer metadata")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "record\\.targetMachineStatus"
    "capture target acceptance target-machine status")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "record\\.gpuDriverStatus"
    "capture target acceptance GPU driver status")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "record\\.kataGoVersionStatus"
    "capture target acceptance KataGo version status")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "recordMetadata\\.ready"
    "capture target acceptance record metadata readiness")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "recordMetadata\\.blocker"
    "capture target acceptance record metadata blockers")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "--target-acceptance-record -- <ini>"
    "capture target acceptance record paths that start with dash")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "recordHint\\.passValues"
    "capture target acceptance record pass-value hints")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "recordHint\\.metadataKeysRequired"
    "capture target acceptance required metadata key hints")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "recordHint\\.completedUtcRequired"
    "capture target acceptance completedUtc requirement hints")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "recordHint\\.checklistItemsRequired"
    "capture target acceptance record checklist requirement hints")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "recordHint\\.recordAndEvidenceTimestampsMustNotAfterCompletedUtc"
    "capture target acceptance record timestamp requirement hints")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "ISO UTC"
    "capture target acceptance completedUtc timestamp format")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "non-empty file"
    "capture target acceptance non-empty evidence requirement")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "SHA-256"
    "capture target acceptance evidence digest output")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "evidence\\.diagnostics"
    "capture target acceptance diagnostics evidence path")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "evidence\\.engineLog"
    "capture target acceptance engine-log evidence path")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "evidence\\.rawKataGoComparisonLog"
    "capture target acceptance raw KataGo evidence path")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "evidence\\.manualUiInspectionLog"
    "capture target acceptance manual UI inspection evidence path")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "evidence\\.externalTargetVerificationLog"
    "capture target acceptance external target verification evidence path")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "evidence\\.\\*\\.lastModifiedUtc"
    "capture target acceptance evidence modification time")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "evidence\\.\\*\\.timestampStatus"
    "capture target acceptance evidence timestamp status")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "contentStatus"
    "capture target acceptance evidence content status")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "missingContentMarker"
    "capture target acceptance missing evidence content markers")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "\\[evidenceContentMarkers\\]"
    "capture target acceptance record template evidence content marker guidance")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "\\[recordHints\\]"
    "capture target acceptance record template hint guidance")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "diagnostics=LizzieYzy Qt Diagnostics"
    "capture diagnostics evidence content marker guidance")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "targetAcceptanceReport=# Target Acceptance Report"
    "capture target acceptance report evidence content marker guidance")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "passValues=pass; passed; accepted; true; yes"
    "capture accepted pass value record hints")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "checklistItemsRequired=pass"
    "capture checklist pass requirement record hint")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "seven evidence paths"
    "capture target acceptance distinct evidence path count")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "final blocker fields"
    "capture target report evidence final blocker marker requirements")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "evidence\\.engineLog\\.contentStatus"
    "capture target acceptance engine-log content status")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "engineLog\\.gpuOrStderrContentStatus"
    "capture target acceptance engine-log GPU/stderr content status")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "engineLog\\.missingGpuOrStderrContentMarker"
    "capture target acceptance engine-log GPU/stderr missing marker diagnostics")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "evidence\\.rawKataGoComparisonLog\\.contentStatus"
    "capture target acceptance raw-log content status")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "evidence\\.manualUiInspectionLog\\.contentStatus"
    "capture target acceptance manual UI log content status")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "evidence\\.externalTargetVerificationLog\\.contentStatus"
    "capture target acceptance external target verification log content status")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "raw comparison[ \n]+checklist item names"
    "capture target acceptance raw-log checklist item evidence markers")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "manual UI inspection[ \n]+checklist item names"
    "capture target acceptance manual UI checklist item evidence markers")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "external target verification[ \n]+checklist item names"
    "capture target acceptance external target checklist item evidence markers")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "kata-analyze"
    "capture raw KataGo comparison realtime GTP evidence marker")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "moveInfos"
    "capture raw KataGo comparison Analysis JSON evidence marker")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "rootInfo"
    "capture raw KataGo comparison rootInfo evidence marker")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "winrate"
    "capture raw KataGo comparison winrate evidence marker")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "scoreMean"
    "capture raw KataGo comparison scoreMean evidence marker")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "scoreStdev"
    "capture raw KataGo comparison scoreStdev evidence marker")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "visits"
    "capture raw KataGo comparison visits evidence marker")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "policy"
    "capture raw KataGo comparison policy evidence marker")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "`pv`"
    "capture raw KataGo comparison PV evidence marker")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "pvVisits"
    "capture raw KataGo comparison PV visits evidence marker")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "ownership"
    "capture raw KataGo comparison ownership evidence marker")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "LizzieYzy Qt Diagnostics"
    "capture diagnostics evidence marker")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "evidence\\.ready"
    "capture target acceptance evidence readiness")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "evidence\\.blocker"
    "capture target acceptance evidence blockers")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "distinct canonical files"
    "capture target acceptance distinct evidence paths")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "expectedSha256"
    "capture target acceptance expected evidence SHA-256")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "sha256Status"
    "capture target acceptance evidence SHA-256 status")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "evidenceSha256\\.ready"
    "capture target acceptance evidence SHA-256 readiness")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "evidenceSha256\\.blocker"
    "capture target acceptance evidence SHA-256 blockers")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "recordTimestamp\\.ready"
    "capture target acceptance record timestamp readiness")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "recordTimestamp\\.blocker"
    "capture target acceptance record timestamp blockers")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "evidenceTimestamp\\.ready"
    "capture target acceptance evidence timestamp readiness")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "evidenceTimestamp\\.blocker"
    "capture target acceptance evidence timestamp blockers")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "acceptanceEvidenceTimestamp"
    "document evidence timestamp final blocker")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "acceptanceRecordTimestamp"
    "document record timestamp final blocker")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "linuxKdeWaylandNvidiaManualResult"
    "capture Linux manual acceptance result")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "windowsNvidiaManualResult"
    "capture Windows manual acceptance result")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "physicalDisplayManualResult"
    "capture physical display manual acceptance result")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "externalTargetVerificationManualResult"
    "capture external target manual acceptance result")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "checklistPassedRecord"
    "capture passed checklist item records")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "checklistFailedRecord"
    "capture failed checklist item records")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "checklistInvalidRecord"
    "capture invalid checklist item records")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "checklistMissingRecord"
    "capture missing checklist item records")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "manualFailedRecord"
    "capture failed aggregate manual records")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "manualInvalidRecord"
    "capture invalid aggregate manual records")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "plan\\.acceptance\\.checklist\\.ready"
    "capture checklist readiness diagnostics")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "acceptanceChecklist"
    "document checklist final blocker")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "\\[checklist\\.linuxKdeWaylandNvidia\\]"
    "include Linux target checklist record skeleton")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "\\[checklist\\.windowsNvidia\\]"
    "include Windows target checklist record skeleton")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "\\[checklist\\.physicalDisplay\\]"
    "include physical display checklist record skeleton")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "\\[checklist\\.rawKataGoComparison\\]"
    "include raw KataGo comparison checklist record skeleton")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "\\[checklist\\.externalTargetVerification\\]"
    "include external target verification checklist record skeleton")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "\\[checklist\\.manualUiInspection\\]"
    "include manual UI inspection checklist record skeleton")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "windowsOs=pass"
    "include Windows checklist item pass placeholder")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "physical4KPanel=pass"
    "include physical display checklist item pass placeholder")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "linuxKdeWaylandNvidiaRealtimeKataGo=pass"
    "include external target checklist item pass placeholder")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "mainWindow4KHighDpiLayout=pass"
    "include manual UI checklist item pass placeholder")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "acceptanceEvidenceSha256"
    "document evidence SHA-256 final blocker")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "externalTargetVerificationRequired"
    "capture external target verification requirement")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "externalTargetVerificationStatus"
    "capture external target verification status")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "manualRawEngineComparisonRequired"
    "capture manual raw engine comparison requirement")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "rawKataGoComparisonStatus"
    "capture raw KataGo comparison status")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "manualUiInspectionRequired"
    "capture manual UI inspection requirement")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "manualUiInspectionStatus"
    "capture manual UI inspection status")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "finalAcceptanceStatus"
    "capture final acceptance status")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "finalAcceptanceBlocker"
    "capture final acceptance blocker diagnostics")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "finalAcceptanceOutstandingBlocker"
    "capture final acceptance outstanding blocker diagnostics")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "mainBatchAnalysisConfig"
    "capture mode-specific final acceptance blocker names")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "configuredDualEngineManualVerificationBlocker"
    "capture dual-engine manual verification blockers")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "linuxKdeWaylandNvidiaVerificationStatus"
    "capture Linux target verification status")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "windowsNvidiaVerificationStatus"
    "capture Windows target verification status")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "physicalDisplayVerificationStatus"
    "capture physical display verification status")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "rawKataGoComparisonChecklist"
    "capture raw KataGo comparison checklist")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "externalTargetVerificationChecklist"
    "capture external target verification checklist")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "manualUiInspectionChecklist"
    "capture manual UI inspection checklist")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "KDE Wayland \\+ NVIDIA"
    "include the Linux KDE Wayland NVIDIA section")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "Windows \\+ NVIDIA"
    "include the Windows NVIDIA section")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "High DPI And Multi-Display"
    "include the high-DPI and multi-display section")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "Raw KataGo Comparison"
    "include the raw KataGo comparison section")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "External Target Verification"
    "include the external target verification section")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "Manual UI Inspection"
    "include the manual UI inspection section")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "Result: pass / fail"
    "include manual result placeholders")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "Notes:"
    "include manual notes placeholders")
require_document_text(
    "TargetAcceptanceReport.md" "${package_target_acceptance}" "Final Acceptance"
    "include the final acceptance section")
forbid_package_entry(
    "(^|\n)[^\n]*/(bin|tools|libexec)/[^/\n]*katago[^/\n]*(\\.exe)?(\n|$)"
    "KataGo executables")
forbid_package_entry("(^|\n)[^\n]*/[^/\n]*katago[^/\n]*\\.exe(\n|$)" "KataGo executables")
forbid_package_entry(
    "(^|\n)[^\n]*/[^/\n]*(katago|gtp|analysis)[^/\n]*\\.(cfg|conf|config|yaml|yml|toml)(\n|$)"
    "KataGo config files")
forbid_package_entry(
    "(^|\n)[^\n]*/[^/\n]*(katago|gtp|analysis)[^/\n]*config[^/\n]*\\.json(\n|$)"
    "KataGo config files")
forbid_package_entry("(^|\n)[^\n]*/[^/\n]*\\.(bin|txt)\\.gz(\n|$)" "KataGo neural-network model files")
forbid_package_entry("(^|\n)[^\n]*/(models?|nets?|networks?)/[^\n]*\\.(bin|txt)(\\.gz)?(\n|$)" "KataGo neural-network model files")
forbid_package_entry(
    "(^|\n)[^\n]*/[^/\n]*\\.(onnx|pb|pbtxt|pt|pth|safetensors|ckpt)(\\.gz)?(\n|$)"
    "KataGo neural-network model files")
forbid_package_entry(
    "(^|\n)[^\n]*/[^/\n]*(leelaz|leela[-_]?zero|gnugo|gnu[-_]?go|pachi|fuego)[^/\n]*(\\.exe)?(\n|$)"
    "non-KataGo engine artifacts")
forbid_package_entry("(^|\n)[^\n]*\\.(jar|class)(\n|$)" "Java archives or bytecode")
forbid_package_entry("(^|\n)[^\n]*/jre/[^\n]*(\n|$)" "Java runtime directories")
forbid_package_entry("(^|\n)[^\n]*/jdk/[^\n]*(\n|$)" "Java runtime directories")
forbid_package_entry("(^|\n)[^\n]*/lib/jvm/[^\n]*(\n|$)" "Java runtime directories")
forbid_package_entry("(^|\n)[^\n]*/bin/javaw?(\\.exe)?(\n|$)" "Java runtime executables")
forbid_package_entry(
    "(^|\n)[^\n]*/(jvm\\.dll|java\\.dll|jawt\\.dll|libjvm\\.so|libjli\\.so|libjava\\.so|libjawt\\.so|libawt\\.so)(\n|$)"
    "Java runtime libraries")
forbid_package_entry(
    "(^|\n)[^\n]*/[^/\n]*(remote[-_]?ssh|ssh[-_]?engine|remote[-_]?engine)[^/\n]*(\n|$)"
    "out-of-scope remote SSH engine artifacts")
forbid_package_entry(
    "(^|\n)[^\n]*/[^/\n]*(fox|online[-_]?board|online[-_]?sgf)[^/\n]*(\n|$)"
    "out-of-scope online board artifacts")
forbid_package_entry(
    "(^|\n)[^\n]*/[^/\n]*(screen[-_]?sync|screen[-_]?board[-_]?sync|board[-_]?sync)[^/\n]*(\n|$)"
    "out-of-scope screen board synchronization artifacts")
forbid_package_entry(
    "(^|\n)[^\n]*/[^/\n]*(distributed[-_]?training|katago[-_]?training|training[-_]?worker)[^/\n]*(\n|$)"
    "out-of-scope distributed training artifacts")
forbid_package_entry("(^|\n)[^\n]*\\.app(/[^\n]*)?(\n|$)" "macOS app bundles")
forbid_package_entry("(^|\n)[^\n]*/[^/\n]*\\.dylib(\n|$)" "macOS dynamic libraries")
forbid_package_entry("(^|\n)[^\n]*\\.framework(/[^\n]*)?(\n|$)" "macOS framework bundles")

if(NOT PACKAGE_EXECUTABLE MATCHES "\\.exe$")
    forbid_package_entry("(^|\n)[^\n]*/[^/\n]*\\.(dll|pdb|lib)(\n|$)" "Windows runtime artifacts in Linux packages")
    forbid_package_entry("(^|\n)[^\n]*/bin/[^/\n]*\\.exe(\n|$)" "Windows runtime artifacts in Linux packages")
else()
    forbid_package_entry("(^|\n)[^\n]*/[^/\n]*\\.so(\\.[^/\n]*)?(\n|$)" "Linux runtime artifacts in Windows packages")
    forbid_package_entry("(^|\n)[^\n]*/[^/\n]*\\.appimage(\n|$)" "Linux runtime artifacts in Windows packages")
    forbid_package_entry("(^|\n)[^\n]*/bin/lizzieyzy_qt(\n|$)" "Linux runtime artifacts in Windows packages")
endif()

if(PACKAGE_REQUIRE_QT_DEPLOY)
    require_package_entry("bin/Qt6Core.dll")
    require_package_entry("bin/Qt6Gui.dll")
    require_package_entry("bin/Qt6Widgets.dll")
    require_package_entry("bin/Qt6Quick.dll")
    require_package_entry("bin/Qt6QuickWidgets.dll")
    require_package_entry("bin/Qt6Qml.dll")
    require_package_entry("bin/Qt6Network.dll")
    require_package_entry("bin/Qt6OpenGL.dll")
    require_package_entry("bin/platforms/qwindows.dll")
endif()

if(PACKAGE_RUN_VERSION_SMOKE)
    set(package_smoke_dir_owned FALSE)
    if(NOT DEFINED PACKAGE_SMOKE_DIR)
        get_filename_component(package_directory "${package_path}" DIRECTORY)
        set(PACKAGE_SMOKE_DIR "${package_directory}/verify-package-smoke")
        set(package_smoke_dir_owned TRUE)
    endif()

    if(NOT DEFINED PACKAGE_EXPECT_VERSION_OUTPUT)
        string(REGEX MATCH "LizzieYzyQt-([0-9][^-]*)-" package_version_match "${package_name}")
        if(package_version_match)
            set(PACKAGE_EXPECT_VERSION_OUTPUT "LizzieYzy Qt ${CMAKE_MATCH_1}")
        else()
            set(PACKAGE_EXPECT_VERSION_OUTPUT "LizzieYzy Qt")
        endif()
    endif()

    file(REMOVE_RECURSE "${PACKAGE_SMOKE_DIR}")
    file(MAKE_DIRECTORY "${PACKAGE_SMOKE_DIR}")
    execute_process(
        COMMAND "${CMAKE_COMMAND}" -E tar xf "${package_path}"
        WORKING_DIRECTORY "${PACKAGE_SMOKE_DIR}"
        RESULT_VARIABLE extract_result
        ERROR_VARIABLE extract_error
    )
    if(NOT extract_result EQUAL 0)
        message(FATAL_ERROR "Could not extract package '${package_path}' for version smoke: ${extract_error}")
    endif()

    set(package_executable_path "${PACKAGE_SMOKE_DIR}/${package_executable_entry}")
    if(NOT EXISTS "${package_executable_path}")
        message(FATAL_ERROR "Extracted package is missing executable '${package_executable_entry}'")
    endif()

    execute_process(
        COMMAND "${package_executable_path}" --version
        WORKING_DIRECTORY "${PACKAGE_SMOKE_DIR}"
        RESULT_VARIABLE version_result
        OUTPUT_VARIABLE version_output
        ERROR_VARIABLE version_error
    )
    if(NOT version_result STREQUAL "0")
        message(FATAL_ERROR "Package version smoke failed for '${package_executable_entry}': ${version_output}${version_error}")
    endif()
    if(NOT "${version_output}${version_error}" MATCHES "${PACKAGE_EXPECT_VERSION_OUTPUT}")
        message(
            FATAL_ERROR
            "Package version smoke output did not match '${PACKAGE_EXPECT_VERSION_OUTPUT}': ${version_output}${version_error}")
    endif()

    execute_process(
        COMMAND "${package_executable_path}" --unknown-target-acceptance-option
        WORKING_DIRECTORY "${PACKAGE_SMOKE_DIR}"
        RESULT_VARIABLE strict_cli_result
        OUTPUT_VARIABLE strict_cli_output
        ERROR_VARIABLE strict_cli_error
    )
    set(strict_cli_combined "${strict_cli_output}${strict_cli_error}")
    if(strict_cli_result STREQUAL "0")
        message(
            FATAL_ERROR
            "Package strict CLI argument validation smoke should reject unknown long options: ${strict_cli_combined}")
    endif()
    if(NOT "${strict_cli_combined}" MATCHES "Unknown option: --unknown-target-acceptance-option")
        message(
            FATAL_ERROR
            "Package strict CLI argument validation smoke did not report the unknown option: ${strict_cli_combined}")
    endif()

    execute_process(
        COMMAND "${package_executable_path}" --target-acceptance-record -platform offscreen
        WORKING_DIRECTORY "${PACKAGE_SMOKE_DIR}"
        RESULT_VARIABLE target_acceptance_record_cli_result
        OUTPUT_VARIABLE target_acceptance_record_cli_output
        ERROR_VARIABLE target_acceptance_record_cli_error
    )
    set(target_acceptance_record_cli_combined "${target_acceptance_record_cli_output}${target_acceptance_record_cli_error}")
    if(target_acceptance_record_cli_result STREQUAL "0")
        message(
            FATAL_ERROR
            "Package target acceptance record argument validation smoke should reject option-like missing values: ${target_acceptance_record_cli_combined}")
    endif()
    if(NOT "${target_acceptance_record_cli_combined}" MATCHES "--target-acceptance-record requires an INI file path")
        message(
            FATAL_ERROR
            "Package target acceptance record argument validation smoke did not report the missing INI file path: ${target_acceptance_record_cli_combined}")
    endif()

    execute_process(
        COMMAND
            "${package_executable_path}"
            --target-acceptance-record
            --
            -target-acceptance-record.ini
            --version
        WORKING_DIRECTORY "${PACKAGE_SMOKE_DIR}"
        RESULT_VARIABLE target_acceptance_record_separator_cli_result
        OUTPUT_VARIABLE target_acceptance_record_separator_cli_output
        ERROR_VARIABLE target_acceptance_record_separator_cli_error
    )
    set(
        target_acceptance_record_separator_cli_combined
        "${target_acceptance_record_separator_cli_output}${target_acceptance_record_separator_cli_error}")
    if(NOT target_acceptance_record_separator_cli_result STREQUAL "0")
        message(
            FATAL_ERROR
            "Package target acceptance record argument separator smoke should accept dash-prefixed record paths: ${target_acceptance_record_separator_cli_combined}")
    endif()
    if(NOT "${target_acceptance_record_separator_cli_combined}" MATCHES "${PACKAGE_EXPECT_VERSION_OUTPUT}")
        message(
            FATAL_ERROR
            "Package target acceptance record argument separator smoke did not continue parsing later options: ${target_acceptance_record_separator_cli_combined}")
    endif()
endif()

if(PACKAGE_RUN_TARGET_ACCEPTANCE_RECORD_TEMPLATE_SMOKE)
    set(package_target_acceptance_record_template_smoke_dir_owned FALSE)
    if(NOT DEFINED PACKAGE_TARGET_ACCEPTANCE_RECORD_TEMPLATE_SMOKE_DIR)
        get_filename_component(package_directory "${package_path}" DIRECTORY)
        set(PACKAGE_TARGET_ACCEPTANCE_RECORD_TEMPLATE_SMOKE_DIR "${package_directory}/verify-package-target-acceptance-record-template")
        set(package_target_acceptance_record_template_smoke_dir_owned TRUE)
    endif()

    file(REMOVE_RECURSE "${PACKAGE_TARGET_ACCEPTANCE_RECORD_TEMPLATE_SMOKE_DIR}")
    file(MAKE_DIRECTORY "${PACKAGE_TARGET_ACCEPTANCE_RECORD_TEMPLATE_SMOKE_DIR}")
    execute_process(
        COMMAND "${CMAKE_COMMAND}" -E tar xf "${package_path}"
        WORKING_DIRECTORY "${PACKAGE_TARGET_ACCEPTANCE_RECORD_TEMPLATE_SMOKE_DIR}"
        RESULT_VARIABLE target_acceptance_record_template_extract_result
        ERROR_VARIABLE target_acceptance_record_template_extract_error
    )
    if(NOT target_acceptance_record_template_extract_result EQUAL 0)
        message(
            FATAL_ERROR
            "Could not extract package '${package_path}' for target acceptance record template smoke: ${target_acceptance_record_template_extract_error}")
    endif()

    set(
        package_target_acceptance_record_template_executable_path
        "${PACKAGE_TARGET_ACCEPTANCE_RECORD_TEMPLATE_SMOKE_DIR}/${package_executable_entry}")
    if(NOT EXISTS "${package_target_acceptance_record_template_executable_path}")
        message(FATAL_ERROR "Extracted package is missing executable '${package_executable_entry}'")
    endif()
    file(
        SHA256
        "${package_target_acceptance_record_template_executable_path}"
        package_target_acceptance_record_template_executable_sha256)

    execute_process(
        COMMAND
            "${CMAKE_COMMAND}" -E env
            "QT_QPA_PLATFORM=missing-qpa-for-record-template-smoke"
            "${package_target_acceptance_record_template_executable_path}" --target-acceptance-record-template
        WORKING_DIRECTORY "${PACKAGE_TARGET_ACCEPTANCE_RECORD_TEMPLATE_SMOKE_DIR}"
        RESULT_VARIABLE target_acceptance_record_template_smoke_result
        OUTPUT_VARIABLE target_acceptance_record_template_smoke_output
        ERROR_VARIABLE target_acceptance_record_template_smoke_error
    )
    set(
        package_target_acceptance_record_template_smoke_combined
        "${target_acceptance_record_template_smoke_output}${target_acceptance_record_template_smoke_error}")
    if(NOT target_acceptance_record_template_smoke_result STREQUAL "0")
        message(
            FATAL_ERROR
            "Package target acceptance record template smoke failed for '${package_executable_entry}': ${package_target_acceptance_record_template_smoke_combined}")
    endif()
    require_package_target_acceptance_record_template_output(
        package_target_acceptance_record_template_smoke_combined
        "${package_expected_app_version}"
        "${package_target_acceptance_record_template_executable_sha256}"
        "Package target acceptance record template smoke")
endif()

if(PACKAGE_RUN_TARGET_ACCEPTANCE_REPORT_SMOKE)
    set(package_target_acceptance_report_smoke_dir_owned FALSE)
    if(NOT DEFINED PACKAGE_TARGET_ACCEPTANCE_REPORT_SMOKE_DIR)
        get_filename_component(package_directory "${package_path}" DIRECTORY)
        set(PACKAGE_TARGET_ACCEPTANCE_REPORT_SMOKE_DIR "${package_directory}/verify-package-target-acceptance-report")
        set(package_target_acceptance_report_smoke_dir_owned TRUE)
    endif()

    file(REMOVE_RECURSE "${PACKAGE_TARGET_ACCEPTANCE_REPORT_SMOKE_DIR}")
    file(MAKE_DIRECTORY "${PACKAGE_TARGET_ACCEPTANCE_REPORT_SMOKE_DIR}")
    execute_process(
        COMMAND "${CMAKE_COMMAND}" -E tar xf "${package_path}"
        WORKING_DIRECTORY "${PACKAGE_TARGET_ACCEPTANCE_REPORT_SMOKE_DIR}"
        RESULT_VARIABLE target_acceptance_report_extract_result
        ERROR_VARIABLE target_acceptance_report_extract_error
    )
    if(NOT target_acceptance_report_extract_result EQUAL 0)
        message(
            FATAL_ERROR
            "Could not extract package '${package_path}' for target acceptance report smoke: ${target_acceptance_report_extract_error}")
    endif()

    set(
        package_target_acceptance_report_executable_path
        "${PACKAGE_TARGET_ACCEPTANCE_REPORT_SMOKE_DIR}/${package_executable_entry}")
    if(NOT EXISTS "${package_target_acceptance_report_executable_path}")
        message(FATAL_ERROR "Extracted package is missing executable '${package_executable_entry}'")
    endif()
    file(SHA256 "${package_target_acceptance_report_executable_path}" package_target_acceptance_report_executable_sha256)

    set(
        package_target_acceptance_report_model_path
        "${PACKAGE_TARGET_ACCEPTANCE_REPORT_SMOKE_DIR}/target-acceptance-report-model.bin.gz")
    set(
        package_target_acceptance_report_analysis_config_path
        "${PACKAGE_TARGET_ACCEPTANCE_REPORT_SMOKE_DIR}/target-acceptance-report-analysis.cfg")
    set(
        package_target_acceptance_report_gtp_config_path
        "${PACKAGE_TARGET_ACCEPTANCE_REPORT_SMOKE_DIR}/target-acceptance-report-gtp.cfg")
    set(
        package_target_acceptance_report_compare_model_path
        "${PACKAGE_TARGET_ACCEPTANCE_REPORT_SMOKE_DIR}/target-acceptance-report-compare-model.bin.gz")
    set(
        package_target_acceptance_report_compare_analysis_config_path
        "${PACKAGE_TARGET_ACCEPTANCE_REPORT_SMOKE_DIR}/target-acceptance-report-compare-analysis.cfg")
    set(
        package_target_acceptance_report_compare_gtp_config_path
        "${PACKAGE_TARGET_ACCEPTANCE_REPORT_SMOKE_DIR}/target-acceptance-report-compare-gtp.cfg")
    set(
        package_target_acceptance_report_engine_working_dir
        "${PACKAGE_TARGET_ACCEPTANCE_REPORT_SMOKE_DIR}/target-acceptance-report-engine-working-dir")
    set(
        package_target_acceptance_report_compare_working_dir
        "${PACKAGE_TARGET_ACCEPTANCE_REPORT_SMOKE_DIR}/target-acceptance-report-compare-working-dir")
    set(
        package_target_acceptance_report_settings_file
        "${PACKAGE_TARGET_ACCEPTANCE_REPORT_SMOKE_DIR}/target-acceptance-report-settings.ini")
    set(
        package_target_acceptance_report_mode_blocker_settings_file
        "${PACKAGE_TARGET_ACCEPTANCE_REPORT_SMOKE_DIR}/target-acceptance-report-mode-blocker-settings.ini")
    set(
        package_target_acceptance_report_record_file
        "${PACKAGE_TARGET_ACCEPTANCE_REPORT_SMOKE_DIR}/target-acceptance-record.ini")
    set(
        package_target_acceptance_report_diagnostics_evidence
        "${PACKAGE_TARGET_ACCEPTANCE_REPORT_SMOKE_DIR}/target-diagnostics.txt")
    set(
        package_target_acceptance_report_report_evidence
        "${PACKAGE_TARGET_ACCEPTANCE_REPORT_SMOKE_DIR}/target-report.md")
    set(
        package_target_acceptance_report_engine_log_evidence
        "${PACKAGE_TARGET_ACCEPTANCE_REPORT_SMOKE_DIR}/target-engine.log")
    set(
        package_target_acceptance_report_raw_log_evidence
        "${PACKAGE_TARGET_ACCEPTANCE_REPORT_SMOKE_DIR}/target-raw-katago.log")
    set(
        package_target_acceptance_report_manual_ui_log_evidence
        "${PACKAGE_TARGET_ACCEPTANCE_REPORT_SMOKE_DIR}/target-manual-ui-inspection.log")
    set(
        package_target_acceptance_report_external_verification_log_evidence
        "${PACKAGE_TARGET_ACCEPTANCE_REPORT_SMOKE_DIR}/target-external-verification.log")
    set(
        package_target_acceptance_report_screenshot_evidence
        "${PACKAGE_TARGET_ACCEPTANCE_REPORT_SMOKE_DIR}/target-screenshot.ppm")
    set(
        package_target_acceptance_report_4k_screenshot_evidence
        "${PACKAGE_TARGET_ACCEPTANCE_REPORT_SMOKE_DIR}/target-screenshot-4k.pgm")
    file(WRITE "${package_target_acceptance_report_model_path}" "target acceptance report model fixture\n")
    file(WRITE "${package_target_acceptance_report_analysis_config_path}" "target acceptance report analysis config fixture\n")
    file(WRITE "${package_target_acceptance_report_gtp_config_path}" "target acceptance report gtp config fixture\n")
    file(WRITE "${package_target_acceptance_report_compare_model_path}" "target acceptance report compare model fixture\n")
    file(
        WRITE
        "${package_target_acceptance_report_compare_analysis_config_path}"
        "target acceptance report compare analysis config fixture\n")
    file(WRITE "${package_target_acceptance_report_compare_gtp_config_path}" "target acceptance report compare gtp config fixture\n")
    set(
        package_target_acceptance_report_diagnostics_evidence_content
        "LizzieYzy Qt Diagnostics\napp.version: ${package_expected_app_version}\napp.executable: ${package_target_acceptance_report_executable_path}\nplan.acceptance.recordFile.canonicalPath: ${package_target_acceptance_report_record_file}\nplan.acceptance.recordFile.sha256: fixture\nplan.acceptance.finalAcceptanceStatus: fixture\nplan.acceptance.finalAcceptanceBlocker.count: 0\nplan.acceptance.finalAcceptanceOutstandingBlocker.count: 0\n${package_target_acceptance_report_record_file}\n")
    set(
        package_target_acceptance_report_report_evidence_content
        "# Target Acceptance Report\napp.version: ${package_expected_app_version}\napp.executable: ${package_target_acceptance_report_executable_path}\n`plan.acceptance.recordFile.canonicalPath`: ${package_target_acceptance_report_record_file}\n`plan.acceptance.recordFile.sha256`: fixture\n`plan.acceptance.finalAcceptanceStatus`: fixture\n`plan.acceptance.finalAcceptanceBlocker`: fixture\n`plan.acceptance.finalAcceptanceOutstandingBlocker`: fixture\n")
    set(
        package_target_acceptance_report_engine_log_evidence_content
        "target KataGo engine log evidence fixture\nKataGo 1.15.3\nstderr: CUDA fake backend ready\n")
    set(
        package_raw_katago_comparison_checklist_content
        "analysisJsonRawResponse\nrealtimeGtpRawLine\ncandidateTableRendering\npvPreview\nwinrateScorePerspective\nownershipDisplay\ngenmove\nhumanVsAi\nselfPlay\nengineVsEngine\n")
    set(
        package_target_acceptance_report_raw_log_evidence_content
        "target raw KataGo evidence fixture\nKataGo 1.15.3\nkata-analyze raw line\n{\"rootInfo\":{\"winrate\":0.5,\"scoreMean\":0.0,\"ownership\":[0.1,-0.1]},\"moveInfos\":[{\"move\":\"D4\",\"visits\":10,\"winrate\":0.5,\"scoreMean\":0.0,\"scoreStdev\":4.2,\"policy\":0.12,\"pv\":[\"D4\",\"Q16\"],\"pvVisits\":[10,5],\"ownership\":[0.1,-0.1]}]}\n${package_raw_katago_comparison_checklist_content}")
    set(
        package_target_acceptance_report_manual_ui_log_evidence_content
        "target manual UI inspection evidence fixture\nmainWindow4KHighDpiLayout\nboardLinesCoordinatesAndStarPoints\nstoneRenderingAndCandidateLabels\ncandidateTableColumns\npvPreviewStones\nownershipMainBoardOverlay\nownershipMiniBoardDock\nwinrateScoreGraph\ntoolbarDockAndMenuLayout\nbilingualTextFit\nsavedWindowRestore\nmultiDisplayPlacement\n")
    set(
        package_target_acceptance_report_external_verification_log_evidence_content
        "target external verification evidence fixture\nlinuxKdeWaylandNvidiaRealtimeKataGo\nwindowsNvidiaRealtimeKataGo\nphysical4KHighDpiMultiDisplayUi\nrawKataGoUiComparison\n")
    set(package_target_acceptance_report_screenshot_evidence_content "P3\n2 1\n255\n255 255 255 0 0 0\n")
    file(WRITE "${package_target_acceptance_report_4k_screenshot_evidence}" "P2\n3840 2160\n1\n0 ")
    string(REPEAT "1 " 3839 package_target_acceptance_report_4k_first_row_tail)
    file(APPEND "${package_target_acceptance_report_4k_screenshot_evidence}" "${package_target_acceptance_report_4k_first_row_tail}\n")
    string(REPEAT "1 " 3840 package_target_acceptance_report_4k_row)
    foreach(_package_target_acceptance_report_4k_row_index RANGE 2 2160)
        file(APPEND "${package_target_acceptance_report_4k_screenshot_evidence}" "${package_target_acceptance_report_4k_row}\n")
    endforeach()
    string(SHA256 package_target_acceptance_report_diagnostics_evidence_sha256 "${package_target_acceptance_report_diagnostics_evidence_content}")
    string(SHA256 package_target_acceptance_report_report_evidence_sha256 "${package_target_acceptance_report_report_evidence_content}")
    string(SHA256 package_target_acceptance_report_engine_log_evidence_sha256 "${package_target_acceptance_report_engine_log_evidence_content}")
    string(SHA256 package_target_acceptance_report_raw_log_evidence_sha256 "${package_target_acceptance_report_raw_log_evidence_content}")
    string(SHA256 package_target_acceptance_report_manual_ui_log_evidence_sha256 "${package_target_acceptance_report_manual_ui_log_evidence_content}")
    string(SHA256 package_target_acceptance_report_external_verification_log_evidence_sha256 "${package_target_acceptance_report_external_verification_log_evidence_content}")
    string(SHA256 package_target_acceptance_report_screenshot_evidence_sha256 "${package_target_acceptance_report_screenshot_evidence_content}")
    file(SHA256 "${package_target_acceptance_report_4k_screenshot_evidence}" package_target_acceptance_report_4k_screenshot_evidence_sha256)
    file(WRITE "${package_target_acceptance_report_diagnostics_evidence}" "${package_target_acceptance_report_diagnostics_evidence_content}")
    file(WRITE "${package_target_acceptance_report_report_evidence}" "${package_target_acceptance_report_report_evidence_content}")
    file(WRITE "${package_target_acceptance_report_engine_log_evidence}" "${package_target_acceptance_report_engine_log_evidence_content}")
    file(WRITE "${package_target_acceptance_report_raw_log_evidence}" "${package_target_acceptance_report_raw_log_evidence_content}")
    file(WRITE "${package_target_acceptance_report_manual_ui_log_evidence}" "${package_target_acceptance_report_manual_ui_log_evidence_content}")
    file(WRITE "${package_target_acceptance_report_external_verification_log_evidence}" "${package_target_acceptance_report_external_verification_log_evidence_content}")
    file(WRITE "${package_target_acceptance_report_screenshot_evidence}" "${package_target_acceptance_report_screenshot_evidence_content}")
    string(TIMESTAMP package_target_acceptance_report_completed_utc "%Y-%m-%dT%H:%M:%SZ" UTC)
    file(MAKE_DIRECTORY "${package_target_acceptance_report_engine_working_dir}")
    file(MAKE_DIRECTORY "${package_target_acceptance_report_compare_working_dir}")
    file(WRITE "${package_target_acceptance_report_settings_file}" "
[engine]
executable=${package_target_acceptance_report_executable_path}
model=${package_target_acceptance_report_model_path}
gtpConfig=${package_target_acceptance_report_gtp_config_path}
analysisConfig=${package_target_acceptance_report_analysis_config_path}
workingDirectory=${package_target_acceptance_report_engine_working_dir}
extraArgs=--target-report-main \\\"spaced value\\\" \\\"\\\"

[compare]
executable=${package_target_acceptance_report_executable_path}
model=${package_target_acceptance_report_compare_model_path}
gtpConfig=${package_target_acceptance_report_compare_gtp_config_path}
analysisConfig=${package_target_acceptance_report_compare_analysis_config_path}
workingDirectory=${package_target_acceptance_report_compare_working_dir}
extraArgs=--target-report-compare \\\"compare spaced\\\" \\\"\\\"
")
    file(WRITE "${package_target_acceptance_report_mode_blocker_settings_file}" "
[engine]
executable=${package_target_acceptance_report_executable_path}
model=${package_target_acceptance_report_model_path}
gtpConfig=${package_target_acceptance_report_gtp_config_path}
workingDirectory=${package_target_acceptance_report_engine_working_dir}
")
    function(extract_package_target_acceptance_record_template_value source_var key output_var)
        string(REGEX MATCH "(^|\n)${key}=([^\n]*)" template_match "${${source_var}}")
        if(NOT template_match)
            message(FATAL_ERROR "Package target acceptance record template probe did not include ${key}: ${${source_var}}")
        endif()
        set("${output_var}" "${CMAKE_MATCH_2}" PARENT_SCOPE)
    endfunction()

    execute_process(
        COMMAND
            "${CMAKE_COMMAND}" -E env
            "QT_QPA_PLATFORM=missing-qpa-for-template-smoke"
            "${package_target_acceptance_report_executable_path}" --target-acceptance-record-template
        WORKING_DIRECTORY "${PACKAGE_TARGET_ACCEPTANCE_REPORT_SMOKE_DIR}"
        RESULT_VARIABLE package_target_acceptance_record_metadata_template_result
        OUTPUT_VARIABLE package_target_acceptance_record_metadata_template_output
        ERROR_VARIABLE package_target_acceptance_record_metadata_template_error
    )
    set(
        package_target_acceptance_record_metadata_template_combined
        "${package_target_acceptance_record_metadata_template_output}${package_target_acceptance_record_metadata_template_error}")
    if(NOT package_target_acceptance_record_metadata_template_result STREQUAL "0")
        message(
            FATAL_ERROR
            "Package target acceptance record metadata template probe failed for '${package_executable_entry}': ${package_target_acceptance_record_metadata_template_combined}")
    endif()
    extract_package_target_acceptance_record_template_value(
        package_target_acceptance_record_metadata_template_combined
        "osPrettyName"
        package_target_acceptance_report_os_pretty_name)
    extract_package_target_acceptance_record_template_value(
        package_target_acceptance_record_metadata_template_combined
        "osKernelType"
        package_target_acceptance_report_os_kernel_type)
    extract_package_target_acceptance_record_template_value(
        package_target_acceptance_record_metadata_template_combined
        "osKernelVersion"
        package_target_acceptance_report_os_kernel_version)
    extract_package_target_acceptance_record_template_value(
        package_target_acceptance_record_metadata_template_combined
        "qtRuntimeVersion"
        package_target_acceptance_report_qt_runtime_version)
    extract_package_target_acceptance_record_template_value(
        package_target_acceptance_record_metadata_template_combined
        "qtBuildAbi"
        package_target_acceptance_report_qt_build_abi)
    extract_package_target_acceptance_record_template_value(
        package_target_acceptance_record_metadata_template_combined
        "cpuArchitecture"
        package_target_acceptance_report_cpu_architecture)
    extract_package_target_acceptance_record_template_value(
        package_target_acceptance_record_metadata_template_combined
        "targetMachine"
        package_target_acceptance_report_target_machine)

    file(WRITE "${package_target_acceptance_report_record_file}" "
[metadata]
completedUtc=${package_target_acceptance_report_completed_utc}
appVersion=${package_expected_app_version}
appExecutableSha256=${package_target_acceptance_report_executable_sha256}
osPrettyName=${package_target_acceptance_report_os_pretty_name}
osKernelType=${package_target_acceptance_report_os_kernel_type}
osKernelVersion=${package_target_acceptance_report_os_kernel_version}
qtRuntimeVersion=${package_target_acceptance_report_qt_runtime_version}
qtBuildAbi=${package_target_acceptance_report_qt_build_abi}
cpuArchitecture=${package_target_acceptance_report_cpu_architecture}
reviewer=target tester
targetMachine=${package_target_acceptance_report_target_machine}
gpuDriver=NVIDIA 555.55
kataGoVersion=KataGo 1.15.3
notes=all target evidence attached

[evidence]
diagnostics=${package_target_acceptance_report_diagnostics_evidence}
targetAcceptanceReport=${package_target_acceptance_report_report_evidence}
engineLog=${package_target_acceptance_report_engine_log_evidence}
rawKataGoComparisonLog=${package_target_acceptance_report_raw_log_evidence}
manualUiInspectionLog=${package_target_acceptance_report_manual_ui_log_evidence}
externalTargetVerificationLog=${package_target_acceptance_report_external_verification_log_evidence}
screenshot=${package_target_acceptance_report_screenshot_evidence}

[evidenceSha256]
diagnostics=${package_target_acceptance_report_diagnostics_evidence_sha256}
targetAcceptanceReport=${package_target_acceptance_report_report_evidence_sha256}
engineLog=${package_target_acceptance_report_engine_log_evidence_sha256}
rawKataGoComparisonLog=${package_target_acceptance_report_raw_log_evidence_sha256}
manualUiInspectionLog=${package_target_acceptance_report_manual_ui_log_evidence_sha256}
externalTargetVerificationLog=${package_target_acceptance_report_external_verification_log_evidence_sha256}
screenshot=${package_target_acceptance_report_screenshot_evidence_sha256}

[evidenceContentMarkers]
diagnostics=LizzieYzy Qt Diagnostics; app.version; <record appVersion>; app.executable; <current app.executable>; plan.acceptance.recordFile.canonicalPath; <record canonical path>; plan.acceptance.recordFile.sha256; plan.acceptance.finalAcceptanceStatus; plan.acceptance.finalAcceptanceBlocker; plan.acceptance.finalAcceptanceOutstandingBlocker
targetAcceptanceReport=# Target Acceptance Report; app.version; <record appVersion>; app.executable; <current app.executable>; plan.acceptance.recordFile.canonicalPath; <record canonical path>; plan.acceptance.recordFile.sha256; plan.acceptance.finalAcceptanceStatus; plan.acceptance.finalAcceptanceBlocker; plan.acceptance.finalAcceptanceOutstandingBlocker
engineLog=KataGo; <record kataGoVersion>
engineLog.gpuOrStderrAny=CUDA; OpenCL; TensorRT; GPU; gpu; backend; Backend; stderr:; Stderr:
rawKataGoComparisonLog=KataGo; <record kataGoVersion>; kata-analyze; \"moveInfos\"; \"rootInfo\"; \"winrate\"; \"scoreMean\"; \"scoreStdev\"; \"visits\"; \"policy\"; \"pv\"; \"pvVisits\"; \"ownership\"; analysisJsonRawResponse; realtimeGtpRawLine; candidateTableRendering; pvPreview; winrateScorePerspective; ownershipDisplay; genmove; humanVsAi; selfPlay; engineVsEngine
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
    file(SHA256 "${package_target_acceptance_report_record_file}" package_target_acceptance_report_record_sha256)

    set(package_target_acceptance_report_environment)
    if(DEFINED PACKAGE_TARGET_ACCEPTANCE_REPORT_QPA_PLATFORM
       AND NOT "${PACKAGE_TARGET_ACCEPTANCE_REPORT_QPA_PLATFORM}" STREQUAL "")
        list(
            APPEND
            package_target_acceptance_report_environment
            "QT_QPA_PLATFORM=${PACKAGE_TARGET_ACCEPTANCE_REPORT_QPA_PLATFORM}")
    endif()
    list(
        APPEND
        package_target_acceptance_report_environment
        "LIZZIE_KATAGO_EXECUTABLE=${package_target_acceptance_report_executable_path}"
        "LIZZIE_KATAGO_MODEL=${package_target_acceptance_report_model_path}"
        "LIZZIE_KATAGO_ANALYSIS_CONFIG=${package_target_acceptance_report_analysis_config_path}"
        "LIZZIE_KATAGO_GTP_CONFIG=${package_target_acceptance_report_gtp_config_path}"
        "LIZZIE_DIAGNOSTICS_SETTINGS_FILE=${package_target_acceptance_report_settings_file}")
    execute_process(
        COMMAND
            "${CMAKE_COMMAND}" -E env
            ${package_target_acceptance_report_environment}
            "${package_target_acceptance_report_executable_path}" --target-acceptance-report
        WORKING_DIRECTORY "${PACKAGE_TARGET_ACCEPTANCE_REPORT_SMOKE_DIR}"
        RESULT_VARIABLE target_acceptance_report_result
        OUTPUT_VARIABLE target_acceptance_report_output
        ERROR_VARIABLE target_acceptance_report_error
    )
    if(NOT target_acceptance_report_result STREQUAL "0")
        message(
            FATAL_ERROR
            "Package target acceptance report smoke failed for '${package_executable_entry}': ${target_acceptance_report_output}${target_acceptance_report_error}")
    endif()
    set(package_target_acceptance_report_combined "${target_acceptance_report_output}${target_acceptance_report_error}")
    function(require_package_target_acceptance_report pattern description)
        if(NOT "${package_target_acceptance_report_combined}" MATCHES "${pattern}")
            message(
                FATAL_ERROR
                "Package target acceptance report smoke did not ${description}: ${package_target_acceptance_report_combined}")
        endif()
    endfunction()
    function(require_no_package_target_acceptance_report pattern description)
        if("${package_target_acceptance_report_combined}" MATCHES "${pattern}")
            message(
                FATAL_ERROR
                "Package target acceptance report smoke unexpectedly ${description}: ${package_target_acceptance_report_combined}")
        endif()
    endfunction()
    function(require_package_target_acceptance_report_match_count pattern expected_count description)
        string(REGEX MATCHALL "${pattern}" matches "${package_target_acceptance_report_combined}")
        list(LENGTH matches match_count)
        if(NOT match_count EQUAL expected_count)
            message(
                FATAL_ERROR
                "Package target acceptance report smoke did not ${description}; expected ${expected_count}, got ${match_count}: ${package_target_acceptance_report_combined}")
        endif()
    endfunction()

    execute_process(
        COMMAND
            "${CMAKE_COMMAND}" -E env
            "QT_QPA_PLATFORM=missing-qpa-for-template-smoke"
            "${package_target_acceptance_report_executable_path}" --target-acceptance-record-template
        WORKING_DIRECTORY "${PACKAGE_TARGET_ACCEPTANCE_REPORT_SMOKE_DIR}"
        RESULT_VARIABLE target_acceptance_record_template_result
        OUTPUT_VARIABLE target_acceptance_record_template_output
        ERROR_VARIABLE target_acceptance_record_template_error
    )
    if(NOT target_acceptance_record_template_result STREQUAL "0")
        message(
            FATAL_ERROR
            "Package target acceptance record template smoke failed for '${package_executable_entry}': ${target_acceptance_record_template_output}${target_acceptance_record_template_error}")
    endif()
    set(
        package_target_acceptance_record_template_combined
        "${target_acceptance_record_template_output}${target_acceptance_record_template_error}")
    require_package_target_acceptance_record_template_output(
        package_target_acceptance_record_template_combined
        "${package_expected_app_version}"
        "${package_target_acceptance_report_executable_sha256}"
        "Package target acceptance record template smoke")
    function(require_package_target_acceptance_record_template pattern description)
        if(NOT "${package_target_acceptance_record_template_combined}" MATCHES "${pattern}")
            message(
                FATAL_ERROR
                "Package target acceptance record template smoke did not ${description}: ${package_target_acceptance_record_template_combined}")
        endif()
    endfunction()
    function(require_package_target_acceptance_record_template_match_count pattern expected_count description)
        string(REGEX MATCHALL "${pattern}" matches "${package_target_acceptance_record_template_combined}")
        list(LENGTH matches match_count)
        if(NOT match_count EQUAL expected_count)
            message(
                FATAL_ERROR
                "Package target acceptance record template smoke did not ${description}; expected ${expected_count}, got ${match_count}: ${package_target_acceptance_record_template_combined}")
        endif()
    endfunction()
    function(package_target_acceptance_record_template_section source_var section_name output_var)
        set(section_marker "\n[${section_name}]\n")
        string(FIND "${${source_var}}" "${section_marker}" section_marker_index)
        if(section_marker_index EQUAL -1)
            message(FATAL_ERROR "Package target acceptance record template did not include [${section_name}]: ${${source_var}}")
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
    function(package_target_acceptance_pass_checklist_section source_var section_name output_var)
        package_target_acceptance_record_template_section("${source_var}" "${section_name}" section_content)
        set(section_output "\n[${section_name}]\n")
        string(REPLACE "\n" ";" section_lines "${section_content}")
        foreach(section_line IN LISTS section_lines)
            if(section_line MATCHES "^([^=]+)=")
                string(APPEND section_output "${CMAKE_MATCH_1}=pass\n")
            endif()
        endforeach()
        set("${output_var}" "${section_output}" PARENT_SCOPE)
    endfunction()
    function(package_target_acceptance_all_pass_checklists source_var output_var)
        set(all_checklists "")
        foreach(section_name IN ITEMS
            checklist.linuxKdeWaylandNvidia
            checklist.windowsNvidia
            checklist.physicalDisplay
            checklist.rawKataGoComparison
            checklist.externalTargetVerification
            checklist.manualUiInspection)
            package_target_acceptance_pass_checklist_section("${source_var}" "${section_name}" section_text)
            string(APPEND all_checklists "${section_text}")
        endforeach()
        set("${output_var}" "${all_checklists}" PARENT_SCOPE)
    endfunction()
    require_package_target_acceptance_record_template("^\\[metadata\\]" "print metadata first")
    require_package_target_acceptance_record_template(
        "completedUtc=[0-9][0-9][0-9][0-9]-[0-9][0-9]-[0-9][0-9]T[0-9][0-9]:[0-9][0-9]:[0-9][0-9]Z"
        "include a generated UTC timestamp")
    require_package_target_acceptance_record_template(
        "appVersion=${package_expected_app_version}\n"
        "include current app version metadata")
    require_package_target_acceptance_record_template(
        "appExecutableSha256=${package_target_acceptance_report_executable_sha256}\n"
        "include current app executable SHA-256 metadata")
    require_package_target_acceptance_record_template("osPrettyName=[^\n]+\n" "include current OS pretty-name metadata")
    require_package_target_acceptance_record_template("osKernelType=[^\n]+\n" "include current OS kernel-type metadata")
    require_package_target_acceptance_record_template("osKernelVersion=[^\n]+\n" "include current OS kernel-version metadata")
    require_package_target_acceptance_record_template("qtRuntimeVersion=[^\n]+\n" "include current Qt runtime metadata")
    require_package_target_acceptance_record_template("qtBuildAbi=[^\n]+\n" "include current Qt build ABI metadata")
    require_package_target_acceptance_record_template("cpuArchitecture=[^\n]+\n" "include current CPU architecture metadata")
    require_package_target_acceptance_record_template("reviewer=\n" "include reviewer metadata")
    require_package_target_acceptance_record_template("targetMachine=[^\n]+\n" "include current target machine metadata")
    require_package_target_acceptance_record_template("gpuDriver=\n" "include GPU driver metadata")
    require_package_target_acceptance_record_template("kataGoVersion=\n" "include KataGo version metadata")
    require_package_target_acceptance_record_template("notes=\n" "include notes metadata")
    require_package_target_acceptance_record_template("\\[evidence\\]" "include evidence section")
    require_package_target_acceptance_record_template("\\[evidenceSha256\\]" "include evidence SHA-256 pinning section")
    require_package_target_acceptance_record_template_match_count(
        "(^|\n)diagnostics=\n"
        2
        "include diagnostics evidence and SHA-256 keys")
    require_package_target_acceptance_record_template_match_count(
        "(^|\n)targetAcceptanceReport=\n"
        2
        "include target report evidence and SHA-256 keys")
    require_package_target_acceptance_record_template_match_count(
        "(^|\n)engineLog=\n"
        2
        "include engine log evidence and SHA-256 keys")
    require_package_target_acceptance_record_template_match_count(
        "(^|\n)rawKataGoComparisonLog=\n"
        2
        "include raw KataGo evidence and SHA-256 keys")
    require_package_target_acceptance_record_template_match_count(
        "(^|\n)manualUiInspectionLog=\n"
        2
        "include manual UI inspection evidence and SHA-256 keys")
    require_package_target_acceptance_record_template_match_count(
        "(^|\n)externalTargetVerificationLog=\n"
        2
        "include external target verification evidence and SHA-256 keys")
    require_package_target_acceptance_record_template_match_count(
        "(^|\n)screenshot=\n"
        2
        "include screenshot evidence and SHA-256 keys")
    require_package_target_acceptance_record_template("\\[acceptance\\]" "include acceptance section")
    require_package_target_acceptance_record_template(
        "linuxKdeWaylandNvidia=\n"
        "include Linux target result key")
    require_package_target_acceptance_record_template(
        "windowsNvidia=\n"
        "include Windows target result key")
    require_package_target_acceptance_record_template(
        "physicalDisplay=\n"
        "include physical display result key")
    require_package_target_acceptance_record_template(
        "externalTargetVerification=\n"
        "include external target verification result key")
    require_package_target_acceptance_record_template(
        "rawKataGoComparison=\n"
        "include raw KataGo comparison result key")
    require_package_target_acceptance_record_template(
        "manualUiInspection=\n"
        "include manual UI inspection result key")
    require_package_target_acceptance_record_template(
        "\\[checklist\\.linuxKdeWaylandNvidia\\]"
        "include Linux target checklist")
    require_package_target_acceptance_record_template("linuxOs=\n" "include Linux OS checklist key")
    require_package_target_acceptance_record_template("kdeSession=\n" "include KDE session checklist key")
    require_package_target_acceptance_record_template("waylandSession=\n" "include Wayland session checklist key")
    require_package_target_acceptance_record_template(
        "qwaylandPlatformPlugin=\n"
        "include Linux Wayland checklist key")
    require_package_target_acceptance_record_template(
        "nvidiaEnvironmentHint=\n"
        "include NVIDIA environment checklist key")
    require_package_target_acceptance_record_template("packageStarts=\n" "include package startup checklist key")
    require_package_target_acceptance_record_template(
        "configureKataGoPaths=\n"
        "include KataGo path configuration checklist key")
    require_package_target_acceptance_record_template(
        "realtimeAnalyzeCurrentPosition=\n"
        "include realtime analysis checklist key")
    require_package_target_acceptance_record_template("branchResync=\n" "include branch resync checklist key")
    require_package_target_acceptance_record_template("gpuStderrCaptured=\n" "include GPU stderr checklist key")
    require_package_target_acceptance_record_template("\\[checklist\\.windowsNvidia\\]" "include Windows checklist")
    require_package_target_acceptance_record_template("windowsOs=\n" "include Windows OS checklist key")
    require_package_target_acceptance_record_template("qwindowsPlatformPlugin=\n" "include Windows checklist key")
    require_package_target_acceptance_record_template(
        "packageExtracts=\n"
        "include Windows package extraction checklist key")
    require_package_target_acceptance_record_template(
        "appLocalQtRuntime=\n"
        "include Windows app-local Qt runtime checklist key")
    require_package_target_acceptance_record_template(
        "nativeSettingsPathDialog=\n"
        "include Windows native settings path-dialog checklist key")
    require_package_target_acceptance_record_template(
        "engineDiagnosticsCaptured=\n"
        "include Windows engine diagnostics checklist key")
    require_package_target_acceptance_record_template(
        "\\[checklist\\.physicalDisplay\\]"
        "include physical display checklist")
    require_package_target_acceptance_record_template("physical4KPanel=\n" "include physical 4K panel checklist key")
    require_package_target_acceptance_record_template("highDpiScale150Or200=\n" "include high-DPI scale checklist key")
    require_package_target_acceptance_record_template(
        "multiDisplayLayout=\n"
        "include multi-display checklist key")
    require_package_target_acceptance_record_template(
        "boardTextSharpness=\n"
        "include board text sharpness checklist key")
    require_package_target_acceptance_record_template(
        "candidateLabelsNoOverlap=\n"
        "include candidate label overlap checklist key")
    require_package_target_acceptance_record_template(
        "pvPreviewNoOverlap=\n"
        "include PV preview overlap checklist key")
    require_package_target_acceptance_record_template(
        "ownershipOverlayReadable=\n"
        "include ownership overlay readability checklist key")
    require_package_target_acceptance_record_template(
        "winrateScoreGraphReadable=\n"
        "include graph readability checklist key")
    require_package_target_acceptance_record_template(
        "restoredWindowVisible=\n"
        "include restored window visibility checklist key")
    require_package_target_acceptance_record_template(
        "\\[checklist\\.rawKataGoComparison\\]"
        "include raw KataGo comparison checklist")
    require_package_target_acceptance_record_template(
        "analysisJsonRawResponse=\n"
        "include Analysis JSON raw comparison checklist key")
    require_package_target_acceptance_record_template(
        "realtimeGtpRawLine=\n"
        "include realtime GTP raw comparison checklist key")
    require_package_target_acceptance_record_template(
        "candidateTableRendering=\n"
        "include candidate table raw comparison checklist key")
    require_package_target_acceptance_record_template(
        "pvPreview=\n"
        "include PV preview raw comparison checklist key")
    require_package_target_acceptance_record_template(
        "winrateScorePerspective=\n"
        "include winrate and score raw comparison checklist key")
    require_package_target_acceptance_record_template(
        "ownershipDisplay=\n"
        "include ownership display raw comparison checklist key")
    require_package_target_acceptance_record_template("genmove=\n" "include genmove raw comparison checklist key")
    require_package_target_acceptance_record_template(
        "humanVsAi=\n"
        "include Human-vs-AI raw comparison checklist key")
    require_package_target_acceptance_record_template("selfPlay=\n" "include self-play raw comparison checklist key")
    require_package_target_acceptance_record_template(
        "engineVsEngine=\n"
        "include engine-vs-engine raw comparison checklist key")
    require_package_target_acceptance_record_template(
        "\\[checklist\\.externalTargetVerification\\]"
        "include external target verification checklist")
    require_package_target_acceptance_record_template(
        "linuxKdeWaylandNvidiaRealtimeKataGo=\n"
        "include Linux target realtime KataGo external verification checklist key")
    require_package_target_acceptance_record_template(
        "windowsNvidiaRealtimeKataGo=\n"
        "include Windows target realtime KataGo external verification checklist key")
    require_package_target_acceptance_record_template(
        "physical4KHighDpiMultiDisplayUi=\n"
        "include physical 4K high-DPI multi-display external verification checklist key")
    require_package_target_acceptance_record_template(
        "rawKataGoUiComparison=\n"
        "include raw KataGo UI comparison checklist key")
    require_package_target_acceptance_record_template(
        "\\[checklist\\.manualUiInspection\\]"
        "include manual UI checklist")
    require_package_target_acceptance_record_template(
        "mainWindow4KHighDpiLayout=\n"
        "include high-DPI UI inspection checklist key")
    require_package_target_acceptance_record_template(
        "boardLinesCoordinatesAndStarPoints=\n"
        "include board line, coordinate, and star-point UI inspection checklist key")
    require_package_target_acceptance_record_template(
        "stoneRenderingAndCandidateLabels=\n"
        "include stone rendering and candidate-label UI inspection checklist key")
    require_package_target_acceptance_record_template(
        "candidateTableColumns=\n"
        "include candidate-table UI inspection checklist key")
    require_package_target_acceptance_record_template(
        "pvPreviewStones=\n"
        "include PV preview stones UI inspection checklist key")
    require_package_target_acceptance_record_template(
        "ownershipMainBoardOverlay=\n"
        "include main-board ownership UI inspection checklist key")
    require_package_target_acceptance_record_template(
        "ownershipMiniBoardDock=\n"
        "include mini-board ownership UI inspection checklist key")
    require_package_target_acceptance_record_template(
        "winrateScoreGraph=\n"
        "include winrate and score graph UI inspection checklist key")
    require_package_target_acceptance_record_template(
        "toolbarDockAndMenuLayout=\n"
        "include toolbar, dock, and menu UI inspection checklist key")
    require_package_target_acceptance_record_template(
        "bilingualTextFit=\n"
        "include bilingual text-fit UI inspection checklist key")
    require_package_target_acceptance_record_template(
        "savedWindowRestore=\n"
        "include saved-window restore UI inspection checklist key")
    require_package_target_acceptance_record_template(
        "multiDisplayPlacement=\n"
        "include multi-display placement UI inspection checklist key")
    require_package_target_acceptance_record_template_match_count(
        "nvidiaEnvironmentHint=\n"
        2
        "include NVIDIA environment checklist key in both Linux and Windows sections")
    require_package_target_acceptance_record_template_match_count(
        "configureKataGoPaths=\n"
        2
        "include KataGo path configuration checklist key in both Linux and Windows sections")
    require_package_target_acceptance_record_template_match_count(
        "realtimeAnalyzeCurrentPosition=\n"
        2
        "include realtime analysis checklist key in both Linux and Windows sections")

    require_package_target_acceptance_report("^# Target Acceptance Report" "print the report header")
    require_package_target_acceptance_report(
        "lizzieyzy_qt --target-acceptance-report"
        "name the report CLI")
    require_package_target_acceptance_report("`app\\.version`: " "report the app version")
    require_package_target_acceptance_report("`generatedUtc`: " "report the generation timestamp")
    require_package_target_acceptance_report("`app\\.executable`: " "report the app executable path")
    require_package_target_acceptance_report("`app\\.dir`: " "report the app directory")
    require_package_target_acceptance_report(
        "`process\\.currentWorkingDirectory`: "
        "report the process working directory")
    require_package_target_acceptance_report("`qt\\.version`: " "report the Qt runtime version")
    require_package_target_acceptance_report(
        "`qt\\.platform`: "
        "report the Qt platform plugin")
    require_package_target_acceptance_report("`os\\.prettyName`: " "report the OS pretty name")
    require_package_target_acceptance_report("`os\\.kernelType`: " "report the OS kernel type")
    require_package_target_acceptance_report("`os\\.kernelVersion`: " "report the OS kernel version")
    require_package_target_acceptance_report("`plan\\.target\\.osFamily`: " "report the target OS family")
    require_package_target_acceptance_report("## Configured Paths" "include configured path summary section")
    require_package_target_acceptance_report(
        "`katago\\.env\\.executable`: .*lizzieyzy_qt"
        "report target acceptance KataGo env executable path")
    require_package_target_acceptance_report(
        "`katago\\.env\\.model`: .*target-acceptance-report-model\\.bin\\.gz"
        "report target acceptance KataGo env model path")
    require_package_target_acceptance_report(
        "`katago\\.env\\.gtpConfig`: .*target-acceptance-report-gtp\\.cfg"
        "report target acceptance KataGo env GTP config path")
    require_package_target_acceptance_report(
        "`katago\\.env\\.analysisConfig`: .*target-acceptance-report-analysis\\.cfg"
        "report target acceptance KataGo env analysis config path")
    require_package_target_acceptance_report(
        "`qt\\.settings\\.diagnosticsOverrideFile`: .*target-acceptance-report-settings\\.ini"
        "report target acceptance diagnostics settings file")
    require_package_target_acceptance_report(
        "`qt\\.settings\\.fileName`: .*target-acceptance-report-settings\\.ini"
        "report target acceptance QSettings file")
    require_package_target_acceptance_report(
        "`qt\\.settings\\.engine\\.executable\\.value`: .*lizzieyzy_qt"
        "report target acceptance saved main executable path")
    require_package_target_acceptance_report(
        "`qt\\.settings\\.engine\\.model\\.value`: .*target-acceptance-report-model\\.bin\\.gz"
        "report target acceptance saved main model path")
    require_package_target_acceptance_report(
        "`qt\\.settings\\.engine\\.gtpConfig\\.value`: .*target-acceptance-report-gtp\\.cfg"
        "report target acceptance saved main GTP config path")
    require_package_target_acceptance_report(
        "`qt\\.settings\\.engine\\.analysisConfig\\.value`: .*target-acceptance-report-analysis\\.cfg"
        "report target acceptance saved main analysis config path")
    require_package_target_acceptance_report(
        "`qt\\.settings\\.engine\\.workingDirectory\\.value`: .*target-acceptance-report-engine-working-dir"
        "report target acceptance saved main working directory")
    require_package_target_acceptance_report(
        "`qt\\.settings\\.engine\\.extraArgs\\.value`: --target-report-main \\\"spaced value\\\" \\\"\\\""
        "report target acceptance saved main extra args")
    require_package_target_acceptance_report(
        "`qt\\.settings\\.engine\\.extraArgs\\.parsed\\.count`: 3"
        "report target acceptance parsed main extra arg count")
    require_package_target_acceptance_report(
        "`qt\\.settings\\.engine\\.extraArgs\\.parsed\\.0`: --target-report-main"
        "report target acceptance first parsed main extra arg")
    require_package_target_acceptance_report(
        "`qt\\.settings\\.engine\\.extraArgs\\.parsed\\.1`: spaced value"
        "report target acceptance spaced parsed main extra arg")
    require_package_target_acceptance_report(
        "`qt\\.settings\\.engine\\.extraArgs\\.parsed\\.2`: \\(empty\\)"
        "report target acceptance empty parsed main extra arg")
    require_package_target_acceptance_report(
        "`qt\\.settings\\.compare\\.model\\.value`: .*target-acceptance-report-compare-model\\.bin\\.gz"
        "report target acceptance saved compare model path")
    require_package_target_acceptance_report(
        "`qt\\.settings\\.compare\\.gtpConfig\\.value`: .*target-acceptance-report-compare-gtp\\.cfg"
        "report target acceptance saved compare GTP config path")
    require_package_target_acceptance_report(
        "`qt\\.settings\\.compare\\.analysisConfig\\.value`: .*target-acceptance-report-compare-analysis\\.cfg"
        "report target acceptance saved compare analysis config path")
    require_package_target_acceptance_report(
        "`qt\\.settings\\.compare\\.workingDirectory\\.value`: .*target-acceptance-report-compare-working-dir"
        "report target acceptance saved compare working directory")
    require_package_target_acceptance_report(
        "`qt\\.settings\\.compare\\.extraArgs\\.value`: --target-report-compare \\\"compare spaced\\\" \\\"\\\""
        "report target acceptance saved compare extra args")
    require_package_target_acceptance_report(
        "`qt\\.settings\\.compare\\.extraArgs\\.parsed\\.count`: 3"
        "report target acceptance parsed compare extra arg count")
    require_package_target_acceptance_report(
        "`qt\\.settings\\.compare\\.extraArgs\\.parsed\\.0`: --target-report-compare"
        "report target acceptance first parsed compare extra arg")
    require_package_target_acceptance_report(
        "`qt\\.settings\\.compare\\.extraArgs\\.parsed\\.1`: compare spaced"
        "report target acceptance spaced parsed compare extra arg")
    require_package_target_acceptance_report(
        "`qt\\.settings\\.compare\\.extraArgs\\.parsed\\.2`: \\(empty\\)"
        "report target acceptance empty parsed compare extra arg")
    require_package_target_acceptance_report("`katago\\.env\\.status`: ready" "report ready KataGo env fixture status")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.recordFile`: \\(unset\\)"
        "report unset target acceptance record file")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.recordFile\\.set`: false"
        "report unset target acceptance record file set flag")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.recordMetadata\\.ready`: false"
        "report incomplete target acceptance metadata without a record file")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.recordMetadata\\.blocker`: .*completedUtc"
        "report missing target acceptance completion metadata")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.recordHint\\.passValues`: pass; passed; accepted; true; yes"
        "report target acceptance accepted pass value hints")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.recordHint\\.failValues`: fail; failed; rejected; false; no"
        "report target acceptance accepted fail value hints")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.recordHint\\.metadataKeysRequired`: completedUtc; appVersion; appExecutableSha256; osPrettyName; osKernelType; osKernelVersion; qtRuntimeVersion; qtBuildAbi; cpuArchitecture; reviewer; targetMachine; gpuDriver; kataGoVersion"
        "report target acceptance required metadata key hints")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.recordHint\\.metadataExactMatchKeys`: appVersion; appExecutableSha256; osPrettyName; osKernelType; osKernelVersion; qtRuntimeVersion; qtBuildAbi; cpuArchitecture; targetMachine"
        "report target acceptance exact-match metadata key hints")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.recordHint\\.metadataValueCheckedKeys`: completedUtc; reviewer; gpuDriver; kataGoVersion"
        "report target acceptance value-checked metadata key hints")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.recordHint\\.completedUtcRequired`: ISO-UTC-not-future"
        "report target acceptance completedUtc requirement hint")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.recordHint\\.aggregateAcceptanceKeysRequired`: pass"
        "report target acceptance aggregate pass requirement hint")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.recordHint\\.checklistItemsRequired`: pass"
        "report target acceptance checklist pass requirement hint")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.recordHint\\.evidencePathsRequired`: readable-non-empty-distinct"
        "report target acceptance evidence path requirement hint")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.recordHint\\.evidenceSha256Required`: true"
        "report target acceptance evidence SHA-256 requirement hint")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.recordHint\\.evidenceContentMarkersRequired`: true"
        "report target acceptance evidence content marker requirement hint")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.recordHint\\.recordAndEvidenceTimestampsMustNotAfterCompletedUtc`: true"
        "report target acceptance timestamp requirement hint")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.linuxKdeWaylandNvidiaManualResult`: manual-record-required"
        "report missing Linux target manual result")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.windowsNvidiaManualResult`: manual-record-required"
        "report missing Windows target manual result")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.physicalDisplayManualResult`: manual-record-required"
        "report missing physical display manual result")
    require_package_target_acceptance_report("`katago\\.env\\.ready`: true" "report ready KataGo env fixture flag")
    require_package_target_acceptance_report(
        "`plan\\.target\\.inFirstReleaseScope`: true"
        "report first-release OS scope")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.configuredStatus`: katago-config-ready-needs-target-machine"
        "summarize configured target-machine acceptance status")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.targetPlatformCandidate`: false"
        "report offscreen target platform candidate")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.realKataGoEnvReady`: true"
        "report ready KataGo env fixture flag in PLAN acceptance fields")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.realKataGoEnvStatus`: ready"
        "summarize real KataGo env readiness")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.savedMainConfigReady`: true"
        "report ready saved main engine config flag")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.savedMainGtpReady`: true"
        "report ready saved main engine GTP flag")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.savedMainAnalysisReady`: true"
        "report ready saved main engine analysis flag")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.savedMainConfigStatus`: ready"
        "report ready saved main engine config")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.savedCompareConfigReady`: true"
        "report ready saved compare engine config flag")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.savedCompareGtpReady`: true"
        "report ready saved compare engine GTP flag")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.savedCompareAnalysisReady`: true"
        "report ready saved compare engine analysis flag")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.savedCompareConfigStatus`: ready"
        "report ready saved compare engine config")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.envOrSavedMainConfigReady`: true"
        "report ready env-or-saved main config flag")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.envOrSavedMainGtpReady`: true"
        "report ready env-or-saved main GTP flag")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.envOrSavedMainAnalysisReady`: true"
        "report ready env-or-saved main analysis flag")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.configuredMainConfigSource`: env-and-saved"
        "report env and saved main config source")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.configuredMainGtpSource`: env-and-saved"
        "report env and saved main GTP source")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.configuredMainAnalysisSource`: env-and-saved"
        "report env and saved main analysis source")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.configuredCompareConfigSource`: saved"
        "report saved compare config source")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.configuredCompareGtpSource`: saved"
        "report saved compare GTP source")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.configuredCompareAnalysisSource`: saved"
        "report saved compare analysis source")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.configuredDualEngineConfigReady`: true"
        "report ready dual-engine config")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.configuredDualEngineGtpReady`: true"
        "report ready dual-engine GTP config")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.configuredDualEngineAnalysisReady`: true"
        "report ready dual-engine analysis config")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.realKataGoTargetRunCandidate`: false"
        "report offscreen real KataGo target-run candidate")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.realKataGoManualVerificationCandidate`: false"
        "report offscreen real KataGo manual verification candidate")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.configuredKataGoTargetRunCandidate`: false"
        "report offscreen configured KataGo target-run candidate")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.configuredManualVerificationCandidate`: false"
        "report offscreen configured manual verification candidate")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.configuredRealtimeGtpTargetRunCandidate`: false"
        "report offscreen realtime GTP target-run candidate")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.configuredBatchAnalysisTargetRunCandidate`: false"
        "report offscreen batch-analysis target-run candidate")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.configuredRealtimeGtpManualVerificationCandidate`: false"
        "report offscreen realtime GTP manual verification candidate")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.configuredBatchAnalysisManualVerificationCandidate`: false"
        "report offscreen batch-analysis manual verification candidate")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.configuredDualEngineTargetRunCandidate`: false"
        "report offscreen dual-engine target-run candidate")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.configuredDualEngineManualVerificationCandidate`: false"
        "report offscreen dual-engine manual verification candidate")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.configuredDualRealtimeGtpTargetRunCandidate`: false"
        "report offscreen dual realtime GTP target-run candidate")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.configuredDualBatchAnalysisTargetRunCandidate`: false"
        "report offscreen dual batch-analysis target-run candidate")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.configuredDualRealtimeGtpManualVerificationCandidate`: false"
        "report offscreen dual realtime GTP manual verification candidate")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.configuredDualBatchAnalysisManualVerificationCandidate`: false"
        "report offscreen dual batch-analysis manual verification candidate")
    require_package_target_acceptance_report(
        "`plan\\.target\\.acceptance\\.linuxKdeWaylandNvidiaCandidate`: false"
        "report offscreen Linux KDE Wayland NVIDIA target candidate")
    require_package_target_acceptance_report(
        "`plan\\.target\\.linuxKdeWayland\\.detected`: false"
        "report offscreen Linux KDE Wayland target detection")
    require_package_target_acceptance_report(
        "`plan\\.target\\.acceptance\\.windowsNvidiaCandidate`: false"
        "report offscreen Windows NVIDIA target candidate")
    require_package_target_acceptance_report(
        "`plan\\.target\\.windows\\.detected`: false"
        "report offscreen Windows target detection")
    require_package_target_acceptance_report(
        "`plan\\.target\\.acceptance\\.firstReleaseNvidiaPlatformCandidate`: false"
        "report offscreen first-release NVIDIA platform candidate")
    require_package_target_acceptance_report(
        "`plan\\.target\\.acceptance\\.firstReleaseNvidiaPlatformBlocker`: linuxKdeWaylandNvidia, windowsNvidia"
        "report offscreen first-release NVIDIA platform blockers")
    require_package_target_acceptance_report(
        "`plan\\.target\\.acceptance\\.targetDisplayBlocker`: display4K, highDpiScale"
        "report offscreen target display blockers")
    require_package_target_acceptance_report(
        "`plan\\.target\\.acceptance\\.multiDisplayBlocker`: multiDisplay"
        "report offscreen target multi-display blocker")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.targetDisplayBlocker`: display4K, highDpiScale"
        "report offscreen acceptance target display blockers")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.multiDisplayBlocker`: multiDisplay"
        "report offscreen acceptance multi-display blocker")
    require_package_target_acceptance_report(
        "`plan\\.target\\.display\\.primaryDevicePixelsAtLeast4K`: false"
        "report offscreen primary 4K display candidate")
    require_package_target_acceptance_report(
        "`plan\\.target\\.display\\.primaryScaleAtLeast1_5`: false"
        "report offscreen primary high-DPI scale candidate")
    require_package_target_acceptance_report(
        "`plan\\.target\\.display\\.primaryTargetScreenCandidate`: false"
        "report offscreen primary target display candidate")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.display4KCandidate`: false"
        "report offscreen PLAN acceptance 4K display candidate")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.highDpiCandidate`: false"
        "report offscreen PLAN acceptance high-DPI candidate")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.sameScreenTargetDisplayCandidate`: false"
        "report offscreen same-screen target display candidate")
    require_package_target_acceptance_report("`plan\\.acceptance\\.status`: " "report the PLAN acceptance status")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.status`: katago-env-ready-needs-target-machine"
        "summarize package report acceptance status from the temporary KataGo env")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.externalTargetVerificationRequired`: true"
        "report external target verification requirement")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.externalTargetVerificationStatus`: manual-record-required"
        "report external target verification status")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.manualRawEngineComparisonRequired`: true"
        "report manual raw engine comparison requirement")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.rawKataGoComparisonStatus`: manual-record-required"
        "report raw KataGo comparison status")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.manualUiInspectionRequired`: true"
        "report manual UI inspection requirement")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.manualUiInspectionStatus`: manual-record-required"
        "report manual UI inspection status")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.finalAcceptanceStatus`: needs-readiness-and-final-manual-acceptance"
        "report package final acceptance status")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.finalAcceptanceBlocker`: targetPlatform, targetDisplay, multiDisplay, externalTargetVerification, manualRawEngineComparison, manualUiInspection"
        "report package final acceptance blockers")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.finalAcceptanceOutstandingBlocker`: targetPlatform, targetDisplay, multiDisplay, externalTargetVerification, manualRawEngineComparison, manualUiInspection"
        "report package final-section outstanding blockers")
    require_no_package_target_acceptance_report(
        "- Outstanding blockers:"
        "kept empty package final-section outstanding blocker prompt")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.linuxKdeWaylandNvidiaVerificationStatus`: "
        "report Linux KDE Wayland NVIDIA verification status")
    require_package_target_acceptance_report(
        "`plan\\.target\\.acceptance\\.linuxKdeWaylandNvidiaBlocker`: .*qtWaylandPlatform.*nvidiaEnvironmentHint"
        "report Linux KDE Wayland NVIDIA platform blockers")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.windowsNvidiaVerificationStatus`: "
        "report Windows NVIDIA verification status")
    require_package_target_acceptance_report(
        "`plan\\.target\\.acceptance\\.windowsNvidiaBlocker`: .*windowsPlatformPlugin.*nvidiaEnvironmentHint"
        "report Windows NVIDIA platform blockers")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.physicalDisplayVerificationStatus`: "
        "report physical display verification status")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.rawKataGoComparisonChecklist`: .*analysisJsonRawResponse"
        "include the raw KataGo comparison checklist field")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.externalTargetVerificationChecklist`: .*linuxKdeWaylandNvidiaRealtimeKataGo"
        "include the external target verification checklist field")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.manualUiInspectionChecklist`: .*mainWindow4KHighDpiLayout"
        "include the manual UI inspection checklist field")
    require_package_target_acceptance_report("## Raw KataGo Comparison" "include the raw KataGo comparison section")
    require_package_target_acceptance_report(
        "## External Target Verification"
        "include the external target verification section")
    require_package_target_acceptance_report("## Manual UI Inspection" "include the manual UI inspection section")
    require_package_target_acceptance_report("## Final Acceptance" "include the final acceptance section")
    require_package_target_acceptance_report_match_count(
        "- Result: pass / fail"
        6
        "include manual result placeholders for each manual acceptance section")
    require_package_target_acceptance_report_match_count(
        "- Notes:"
        6
        "include manual notes placeholders for each manual acceptance section")

    set(package_target_acceptance_report_recorded_environment ${package_target_acceptance_report_environment})
    list(
        APPEND
        package_target_acceptance_report_recorded_environment
        "--unset=LIZZIE_TARGET_ACCEPTANCE_RECORD_FILE")
    execute_process(
        COMMAND
            "${CMAKE_COMMAND}" -E env
            ${package_target_acceptance_report_recorded_environment}
            "${package_target_acceptance_report_executable_path}" --target-acceptance-report
            --target-acceptance-record "${package_target_acceptance_report_record_file}"
        WORKING_DIRECTORY "${PACKAGE_TARGET_ACCEPTANCE_REPORT_SMOKE_DIR}"
        RESULT_VARIABLE target_acceptance_recorded_report_result
        OUTPUT_VARIABLE target_acceptance_recorded_report_output
        ERROR_VARIABLE target_acceptance_recorded_report_error
    )
    if(NOT target_acceptance_recorded_report_result STREQUAL "0")
        message(
            FATAL_ERROR
            "Package target acceptance recorded-result report smoke failed for '${package_executable_entry}': ${target_acceptance_recorded_report_output}${target_acceptance_recorded_report_error}")
    endif()
    set(
        package_target_acceptance_report_combined
        "${target_acceptance_recorded_report_output}${target_acceptance_recorded_report_error}")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.recordFile`: .*target-acceptance-record\\.ini"
        "report target acceptance record file path")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.recordFile\\.set`: true"
        "report target acceptance record file set flag")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.recordFile\\.exists`: true"
        "report target acceptance record file exists")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.recordFile\\.readable`: true"
        "report target acceptance record file readable")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.recordFile\\.canonicalPath`: .*target-acceptance-record\\.ini"
        "report target acceptance record canonical path")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.recordFile\\.size`: [1-9][0-9]*"
        "report target acceptance record file size")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.recordFile\\.sha256`: ${package_target_acceptance_report_record_sha256}"
        "report target acceptance record SHA-256")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.recordFile\\.lastModifiedUtc`: [0-9]"
        "report target acceptance record modification time")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.recordFile\\.timestampStatus`: not-after-completed-utc"
        "report target acceptance record timestamp status")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.record\\.completedUtc`: ${package_target_acceptance_report_completed_utc}"
        "report target acceptance completion time")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.record\\.completedUtcStatus`: ready"
        "report target acceptance completion time status")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.record\\.appVersion`: ${package_expected_app_version}"
        "report target acceptance app version")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.record\\.appVersionStatus`: match"
        "report matching target acceptance app version")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.record\\.appExecutableSha256`: ${package_target_acceptance_report_executable_sha256}"
        "report target acceptance app executable SHA-256")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.record\\.appExecutableSha256Status`: match"
        "report matching target acceptance app executable SHA-256")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.record\\.osPrettyName`: [^\n]+"
        "report target acceptance OS pretty name")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.record\\.osPrettyNameStatus`: match"
        "report matching target acceptance OS pretty name")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.record\\.osKernelType`: [^\n]+"
        "report target acceptance OS kernel type")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.record\\.osKernelTypeStatus`: match"
        "report matching target acceptance OS kernel type")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.record\\.osKernelVersion`: [^\n]+"
        "report target acceptance OS kernel version")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.record\\.osKernelVersionStatus`: match"
        "report matching target acceptance OS kernel version")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.record\\.qtRuntimeVersion`: [^\n]+"
        "report target acceptance Qt runtime version")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.record\\.qtRuntimeVersionStatus`: match"
        "report matching target acceptance Qt runtime version")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.record\\.qtBuildAbi`: [^\n]+"
        "report target acceptance Qt build ABI")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.record\\.qtBuildAbiStatus`: match"
        "report matching target acceptance Qt build ABI")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.record\\.cpuArchitecture`: [^\n]+"
        "report target acceptance CPU architecture")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.record\\.cpuArchitectureStatus`: match"
        "report matching target acceptance CPU architecture")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.record\\.reviewer`: target tester"
        "report target acceptance reviewer")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.record\\.targetMachine`: [^\n]+"
        "report target acceptance machine")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.record\\.targetMachineStatus`: match"
        "report matching target acceptance machine")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.record\\.gpuDriver`: NVIDIA 555\\.55"
        "report target acceptance GPU driver")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.record\\.gpuDriverStatus`: match"
        "report matching target acceptance GPU driver status")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.record\\.kataGoVersion`: KataGo 1\\.15\\.3"
        "report target acceptance KataGo version")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.record\\.kataGoVersionStatus`: match"
        "report matching target acceptance KataGo version status")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.record\\.notes`: all target evidence attached"
        "report target acceptance notes")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.recordMetadata\\.ready`: true"
        "report complete target acceptance metadata readiness")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.recordMetadata\\.blocker`: \\(none\\)"
        "report no target acceptance metadata blockers")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.evidence\\.diagnostics\\.value`: .*target-diagnostics\\.txt"
        "report diagnostics evidence path")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.evidence\\.diagnostics\\.exists`: true"
        "report diagnostics evidence exists")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.evidence\\.diagnostics\\.sha256`: ${package_target_acceptance_report_diagnostics_evidence_sha256}"
        "report diagnostics evidence SHA-256")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.evidence\\.diagnostics\\.lastModifiedUtc`: [0-9]"
        "report diagnostics evidence modification time")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.evidence\\.diagnostics\\.expectedSha256`: ${package_target_acceptance_report_diagnostics_evidence_sha256}"
        "report diagnostics evidence expected SHA-256")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.evidence\\.diagnostics\\.sha256Status`: match"
        "report diagnostics evidence SHA-256 match status")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.evidence\\.diagnostics\\.timestampStatus`: not-after-completed-utc"
        "report diagnostics evidence timestamp status")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.evidence\\.diagnostics\\.contentStatus`: match"
        "report diagnostics evidence content status")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.evidence\\.diagnostics\\.missingContentMarker`: \\(none\\)"
        "report no missing diagnostics evidence content markers")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.evidence\\.targetAcceptanceReport\\.readable`: true"
        "report target report evidence readable")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.evidence\\.targetAcceptanceReport\\.contentStatus`: match"
        "report target acceptance report evidence content status")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.evidence\\.targetAcceptanceReport\\.missingContentMarker`: \\(none\\)"
        "report no missing target acceptance report evidence content markers")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.evidence\\.engineLog\\.exists`: true"
        "report engine log evidence exists")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.evidence\\.engineLog\\.contentStatus`: match"
        "report engine log evidence content status")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.evidence\\.engineLog\\.missingContentMarker`: \\(none\\)"
        "report no missing engine log evidence content markers")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.evidence\\.engineLog\\.gpuOrStderrContentStatus`: match"
        "report engine log GPU/stderr evidence content status")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.evidence\\.engineLog\\.missingGpuOrStderrContentMarker`: \\(none\\)"
        "report no missing engine log GPU/stderr evidence content markers")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.evidence\\.rawKataGoComparisonLog\\.exists`: true"
        "report raw KataGo evidence exists")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.evidence\\.rawKataGoComparisonLog\\.contentStatus`: match"
        "report raw KataGo comparison log evidence content status")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.evidence\\.rawKataGoComparisonLog\\.missingContentMarker`: \\(none\\)"
        "report no missing raw KataGo comparison log evidence content markers")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.evidence\\.manualUiInspectionLog\\.exists`: true"
        "report manual UI inspection evidence exists")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.evidence\\.manualUiInspectionLog\\.contentStatus`: match"
        "report manual UI inspection evidence content status")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.evidence\\.manualUiInspectionLog\\.missingContentMarker`: \\(none\\)"
        "report no missing manual UI inspection evidence content markers")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.evidence\\.externalTargetVerificationLog\\.exists`: true"
        "report external target verification evidence exists")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.evidence\\.externalTargetVerificationLog\\.contentStatus`: match"
        "report external target verification evidence content status")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.evidence\\.externalTargetVerificationLog\\.missingContentMarker`: \\(none\\)"
        "report no missing external target verification evidence content markers")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.evidence\\.screenshot\\.exists`: true"
        "report screenshot evidence exists")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.evidence\\.screenshot\\.imageReadable`: true"
        "report readable screenshot image evidence")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.evidence\\.screenshot\\.imageFormat`: ppm"
        "report screenshot image format")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.evidence\\.screenshot\\.imageWidth`: 2"
        "report screenshot image width")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.evidence\\.screenshot\\.imageHeight`: 1"
        "report screenshot image height")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.evidence\\.screenshot\\.imagePixelsAtLeast4K`: false"
        "report screenshot 4K pixel candidate")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.evidence\\.screenshot\\.imageHasPixelVariation`: true"
        "report screenshot pixel variation")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.evidence\\.screenshot\\.readyFor4KAcceptance`: false"
        "report screenshot 4K acceptance readiness")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.evidence\\.ready`: true"
        "report complete acceptance evidence readiness")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.evidence\\.blocker`: \\(none\\)"
        "report no acceptance evidence blockers")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.evidenceSha256\\.ready`: true"
        "report acceptance evidence SHA-256 readiness")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.evidenceSha256\\.blocker`: \\(none\\)"
        "report no acceptance evidence SHA-256 blockers")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.recordTimestamp\\.ready`: true"
        "report target acceptance record timestamp readiness")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.recordTimestamp\\.blocker`: \\(none\\)"
        "report no target acceptance record timestamp blockers")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.evidenceTimestamp\\.ready`: true"
        "report acceptance evidence timestamp readiness")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.evidenceTimestamp\\.blocker`: \\(none\\)"
        "report no acceptance evidence timestamp blockers")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.linuxKdeWaylandNvidiaManualResult`: pass"
        "normalize Linux target manual acceptance result")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.windowsNvidiaManualResult`: pass"
        "normalize Windows target manual acceptance result")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.physicalDisplayManualResult`: pass"
        "normalize physical display manual acceptance result")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.externalTargetVerificationManualResult`: pass"
        "derive external target verification checklist manual result")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.checklistPassedRecord`: .*linuxKdeWaylandNvidia\\.linuxOs"
        "report passed Linux target checklist item record")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.checklistPassedRecord`: .*externalTargetVerification\\.rawKataGoUiComparison"
        "report passed external target checklist item record")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.checklistMissingRecord`: .*rawKataGoComparison\\.analysisJsonRawResponse"
        "report missing raw KataGo checklist item record")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.checklistMissingRecord`: .*manualUiInspection\\.mainWindow4KHighDpiLayout"
        "report missing manual UI checklist item record")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.manualFailedRecord`: \\(none\\)"
        "report no failed aggregate manual records for the package smoke record")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.manualInvalidRecord`: \\(none\\)"
        "report no invalid aggregate manual records for the package smoke record")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.checklist\\.ready`: false"
        "report incomplete target acceptance checklist readiness")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.checklist\\.blocker`: .*rawKataGoComparison\\.analysisJsonRawResponse"
        "report incomplete raw KataGo checklist blocker")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.checklist\\.blocker`: .*manualUiInspection\\.mainWindow4KHighDpiLayout"
        "report incomplete manual UI checklist blocker")
    require_package_target_acceptance_report(
        "- \\[x\\] linuxOs"
        "mark passed Linux checklist items")
    require_package_target_acceptance_report(
        "- \\[x\\] rawKataGoUiComparison"
        "mark passed external checklist items")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.externalTargetVerificationStatus`: pass"
        "derive external target verification pass status")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.rawKataGoComparisonStatus`: pass"
        "normalize raw KataGo comparison pass status")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.manualUiInspectionStatus`: pass"
        "normalize manual UI inspection pass status")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.finalAcceptanceBlocker`: targetPlatform, targetDisplay, multiDisplay, acceptanceChecklist, screenshotEvidence4K"
        "remove satisfied manual acceptance blockers while keeping incomplete checklist and weak screenshot evidence blockers")
    require_no_package_target_acceptance_report(
        "`plan\\.acceptance\\.finalAcceptanceBlocker`: [^\n]*(externalTargetVerification|manualRawEngineComparison|manualUiInspection)"
        "kept satisfied manual acceptance blockers")

    file(WRITE "${package_target_acceptance_report_record_file}" "
[metadata]
completedUtc=${package_target_acceptance_report_completed_utc}
appVersion=${package_expected_app_version}
appExecutableSha256=${package_target_acceptance_report_executable_sha256}
osPrettyName=${package_target_acceptance_report_os_pretty_name}
osKernelType=${package_target_acceptance_report_os_kernel_type}
osKernelVersion=${package_target_acceptance_report_os_kernel_version}
qtRuntimeVersion=${package_target_acceptance_report_qt_runtime_version}
qtBuildAbi=${package_target_acceptance_report_qt_build_abi}
cpuArchitecture=${package_target_acceptance_report_cpu_architecture}
reviewer=target tester
targetMachine=${package_target_acceptance_report_target_machine}
gpuDriver=NVIDIA 555.55
kataGoVersion=KataGo 1.15.3
notes=all target evidence attached

[evidence]
diagnostics=${package_target_acceptance_report_diagnostics_evidence}
targetAcceptanceReport=${package_target_acceptance_report_report_evidence}
engineLog=${package_target_acceptance_report_engine_log_evidence}
rawKataGoComparisonLog=${package_target_acceptance_report_raw_log_evidence}
manualUiInspectionLog=${package_target_acceptance_report_manual_ui_log_evidence}
externalTargetVerificationLog=${package_target_acceptance_report_external_verification_log_evidence}
screenshot=${package_target_acceptance_report_4k_screenshot_evidence}

[evidenceSha256]
diagnostics=${package_target_acceptance_report_diagnostics_evidence_sha256}
targetAcceptanceReport=${package_target_acceptance_report_report_evidence_sha256}
engineLog=${package_target_acceptance_report_engine_log_evidence_sha256}
rawKataGoComparisonLog=${package_target_acceptance_report_raw_log_evidence_sha256}
manualUiInspectionLog=${package_target_acceptance_report_manual_ui_log_evidence_sha256}
externalTargetVerificationLog=${package_target_acceptance_report_external_verification_log_evidence_sha256}
screenshot=${package_target_acceptance_report_4k_screenshot_evidence_sha256}

[evidenceContentMarkers]
diagnostics=LizzieYzy Qt Diagnostics; app.version; <record appVersion>; app.executable; <current app.executable>; plan.acceptance.recordFile.canonicalPath; <record canonical path>; plan.acceptance.recordFile.sha256; plan.acceptance.finalAcceptanceStatus; plan.acceptance.finalAcceptanceBlocker; plan.acceptance.finalAcceptanceOutstandingBlocker
targetAcceptanceReport=# Target Acceptance Report; app.version; <record appVersion>; app.executable; <current app.executable>; plan.acceptance.recordFile.canonicalPath; <record canonical path>; plan.acceptance.recordFile.sha256; plan.acceptance.finalAcceptanceStatus; plan.acceptance.finalAcceptanceBlocker; plan.acceptance.finalAcceptanceOutstandingBlocker
engineLog=KataGo; <record kataGoVersion>
engineLog.gpuOrStderrAny=CUDA; OpenCL; TensorRT; GPU; gpu; backend; Backend; stderr:; Stderr:
rawKataGoComparisonLog=KataGo; <record kataGoVersion>; kata-analyze; \"moveInfos\"; \"rootInfo\"; \"winrate\"; \"scoreMean\"; \"scoreStdev\"; \"visits\"; \"policy\"; \"pv\"; \"pvVisits\"; \"ownership\"; analysisJsonRawResponse; realtimeGtpRawLine; candidateTableRendering; pvPreview; winrateScorePerspective; ownershipDisplay; genmove; humanVsAi; selfPlay; engineVsEngine
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
    execute_process(
        COMMAND
            "${CMAKE_COMMAND}" -E env
            ${package_target_acceptance_report_recorded_environment}
            "${package_target_acceptance_report_executable_path}" --target-acceptance-report
            --target-acceptance-record "${package_target_acceptance_report_record_file}"
        WORKING_DIRECTORY "${PACKAGE_TARGET_ACCEPTANCE_REPORT_SMOKE_DIR}"
        RESULT_VARIABLE target_acceptance_4k_recorded_report_result
        OUTPUT_VARIABLE target_acceptance_4k_recorded_report_output
        ERROR_VARIABLE target_acceptance_4k_recorded_report_error
    )
    if(NOT target_acceptance_4k_recorded_report_result STREQUAL "0")
        message(
            FATAL_ERROR
            "Package target acceptance 4K recorded-result report smoke failed for '${package_executable_entry}': ${target_acceptance_4k_recorded_report_output}${target_acceptance_4k_recorded_report_error}")
    endif()
    set(
        package_target_acceptance_report_combined
        "${target_acceptance_4k_recorded_report_output}${target_acceptance_4k_recorded_report_error}")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.evidence\\.screenshot\\.sha256Status`: match"
        "report 4K screenshot evidence SHA-256 match status")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.evidence\\.screenshot\\.imageWidth`: 3840"
        "report 4K screenshot image width")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.evidence\\.screenshot\\.imageHeight`: 2160"
        "report 4K screenshot image height")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.evidence\\.screenshot\\.imagePixelsAtLeast4K`: true"
        "report complete 4K screenshot pixel envelope")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.evidence\\.screenshot\\.imageHasPixelVariation`: true"
        "report complete 4K screenshot pixel variation")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.evidence\\.screenshot\\.readyFor4KAcceptance`: true"
        "report complete 4K screenshot acceptance readiness")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.finalAcceptanceBlocker`: targetPlatform, targetDisplay, multiDisplay, acceptanceChecklist"
        "remove weak screenshot evidence blocker for complete installed 4K screenshots")
    require_no_package_target_acceptance_report(
        "`plan\\.acceptance\\.finalAcceptanceBlocker`: [^\n]*screenshotEvidence4K"
        "kept weak screenshot evidence blocker for complete installed 4K screenshots")

    package_target_acceptance_all_pass_checklists(
        package_target_acceptance_record_template_combined
        package_target_acceptance_report_all_pass_checklists)
    file(WRITE "${package_target_acceptance_report_record_file}" "
[metadata]
completedUtc=${package_target_acceptance_report_completed_utc}
appVersion=${package_expected_app_version}
appExecutableSha256=${package_target_acceptance_report_executable_sha256}
osPrettyName=${package_target_acceptance_report_os_pretty_name}
osKernelType=${package_target_acceptance_report_os_kernel_type}
osKernelVersion=${package_target_acceptance_report_os_kernel_version}
qtRuntimeVersion=${package_target_acceptance_report_qt_runtime_version}
qtBuildAbi=${package_target_acceptance_report_qt_build_abi}
cpuArchitecture=${package_target_acceptance_report_cpu_architecture}
reviewer=target tester
targetMachine=${package_target_acceptance_report_target_machine}
gpuDriver=NVIDIA 555.55
kataGoVersion=KataGo 1.15.3
notes=all target evidence and checklist records attached

[evidence]
diagnostics=${package_target_acceptance_report_diagnostics_evidence}
targetAcceptanceReport=${package_target_acceptance_report_report_evidence}
engineLog=${package_target_acceptance_report_engine_log_evidence}
rawKataGoComparisonLog=${package_target_acceptance_report_raw_log_evidence}
manualUiInspectionLog=${package_target_acceptance_report_manual_ui_log_evidence}
externalTargetVerificationLog=${package_target_acceptance_report_external_verification_log_evidence}
screenshot=${package_target_acceptance_report_4k_screenshot_evidence}

[evidenceSha256]
diagnostics=${package_target_acceptance_report_diagnostics_evidence_sha256}
targetAcceptanceReport=${package_target_acceptance_report_report_evidence_sha256}
engineLog=${package_target_acceptance_report_engine_log_evidence_sha256}
rawKataGoComparisonLog=${package_target_acceptance_report_raw_log_evidence_sha256}
manualUiInspectionLog=${package_target_acceptance_report_manual_ui_log_evidence_sha256}
externalTargetVerificationLog=${package_target_acceptance_report_external_verification_log_evidence_sha256}
screenshot=${package_target_acceptance_report_4k_screenshot_evidence_sha256}

[evidenceContentMarkers]
diagnostics=LizzieYzy Qt Diagnostics; app.version; <record appVersion>; app.executable; <current app.executable>; plan.acceptance.recordFile.canonicalPath; <record canonical path>; plan.acceptance.recordFile.sha256; plan.acceptance.finalAcceptanceStatus; plan.acceptance.finalAcceptanceBlocker; plan.acceptance.finalAcceptanceOutstandingBlocker
targetAcceptanceReport=# Target Acceptance Report; app.version; <record appVersion>; app.executable; <current app.executable>; plan.acceptance.recordFile.canonicalPath; <record canonical path>; plan.acceptance.recordFile.sha256; plan.acceptance.finalAcceptanceStatus; plan.acceptance.finalAcceptanceBlocker; plan.acceptance.finalAcceptanceOutstandingBlocker
engineLog=KataGo; <record kataGoVersion>
engineLog.gpuOrStderrAny=CUDA; OpenCL; TensorRT; GPU; gpu; backend; Backend; stderr:; Stderr:
rawKataGoComparisonLog=KataGo; <record kataGoVersion>; kata-analyze; \"moveInfos\"; \"rootInfo\"; \"winrate\"; \"scoreMean\"; \"scoreStdev\"; \"visits\"; \"policy\"; \"pv\"; \"pvVisits\"; \"ownership\"; analysisJsonRawResponse; realtimeGtpRawLine; candidateTableRendering; pvPreview; winrateScorePerspective; ownershipDisplay; genmove; humanVsAi; selfPlay; engineVsEngine
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
${package_target_acceptance_report_all_pass_checklists}")
    execute_process(
        COMMAND
            "${CMAKE_COMMAND}" -E env
            ${package_target_acceptance_report_recorded_environment}
            "${package_target_acceptance_report_executable_path}" --target-acceptance-report
            --target-acceptance-record "${package_target_acceptance_report_record_file}"
        WORKING_DIRECTORY "${PACKAGE_TARGET_ACCEPTANCE_REPORT_SMOKE_DIR}"
        RESULT_VARIABLE target_acceptance_complete_recorded_report_result
        OUTPUT_VARIABLE target_acceptance_complete_recorded_report_output
        ERROR_VARIABLE target_acceptance_complete_recorded_report_error
    )
    if(NOT target_acceptance_complete_recorded_report_result STREQUAL "0")
        message(
            FATAL_ERROR
            "Package target acceptance complete recorded-result report smoke failed for '${package_executable_entry}': ${target_acceptance_complete_recorded_report_output}${target_acceptance_complete_recorded_report_error}")
    endif()
    set(
        package_target_acceptance_report_combined
        "${target_acceptance_complete_recorded_report_output}${target_acceptance_complete_recorded_report_error}")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.externalTargetVerificationManualResult`: pass"
        "derive complete external target verification manual result")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.externalTargetVerificationStatus`: pass"
        "derive complete external target verification pass status")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.checklistMissingRecord`: \\(none\\)"
        "report no missing checklist records for a complete target acceptance record")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.manualFailedRecord`: \\(none\\)"
        "report no failed aggregate manual records for a complete target acceptance record")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.manualInvalidRecord`: \\(none\\)"
        "report no invalid aggregate manual records for a complete target acceptance record")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.checklist\\.ready`: true"
        "report complete target acceptance checklist readiness")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.checklist\\.blocker`: \\(none\\)"
        "report no target acceptance checklist blockers")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.checklistPassedRecord`: .*manualUiInspection\\.multiDisplayPlacement"
        "report dynamically generated all-pass manual UI checklist records")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.evidence\\.screenshot\\.readyFor4KAcceptance`: true"
        "keep complete 4K screenshot acceptance readiness with a complete checklist")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.finalAcceptanceBlocker`: targetPlatform, targetDisplay, multiDisplay"
        "remove satisfied non-environment final acceptance blockers for complete records")
    require_package_target_acceptance_report(
        "`plan\\.acceptance\\.finalAcceptanceOutstandingBlocker`: targetPlatform, targetDisplay, multiDisplay"
        "report final-section outstanding blockers for complete records")
    require_no_package_target_acceptance_report(
        "`plan\\.acceptance\\.finalAcceptanceBlocker`: [^\n]*(acceptanceChecklist|screenshotEvidence4K|externalTargetVerification|manualRawEngineComparison|manualUiInspection|acceptanceMetadata|acceptanceEvidence|acceptanceRecordTimestamp)"
        "kept non-environment final acceptance blockers for complete records")

    set(package_target_acceptance_report_mode_blocker_environment)
    if(DEFINED PACKAGE_TARGET_ACCEPTANCE_REPORT_QPA_PLATFORM
       AND NOT "${PACKAGE_TARGET_ACCEPTANCE_REPORT_QPA_PLATFORM}" STREQUAL "")
        list(
            APPEND
            package_target_acceptance_report_mode_blocker_environment
            "QT_QPA_PLATFORM=${PACKAGE_TARGET_ACCEPTANCE_REPORT_QPA_PLATFORM}")
    endif()
    list(
        APPEND
        package_target_acceptance_report_mode_blocker_environment
        "--unset=LIZZIE_KATAGO_EXECUTABLE"
        "--unset=LIZZIE_KATAGO_MODEL"
        "--unset=LIZZIE_KATAGO_ANALYSIS_CONFIG"
        "--unset=LIZZIE_KATAGO_GTP_CONFIG"
        "LIZZIE_DIAGNOSTICS_SETTINGS_FILE=${package_target_acceptance_report_mode_blocker_settings_file}")
    execute_process(
        COMMAND
            "${CMAKE_COMMAND}" -E env
            ${package_target_acceptance_report_mode_blocker_environment}
            "${package_target_acceptance_report_executable_path}" --target-acceptance-report
        WORKING_DIRECTORY "${PACKAGE_TARGET_ACCEPTANCE_REPORT_SMOKE_DIR}"
        RESULT_VARIABLE target_acceptance_report_mode_blocker_result
        OUTPUT_VARIABLE target_acceptance_report_mode_blocker_output
        ERROR_VARIABLE target_acceptance_report_mode_blocker_error
    )
    if(NOT target_acceptance_report_mode_blocker_result STREQUAL "0")
        message(
            FATAL_ERROR
            "Package target acceptance report mode-blocker smoke failed for '${package_executable_entry}': ${target_acceptance_report_mode_blocker_output}${target_acceptance_report_mode_blocker_error}")
    endif()
    set(
        package_target_acceptance_report_mode_blocker_combined
        "${target_acceptance_report_mode_blocker_output}${target_acceptance_report_mode_blocker_error}")
    if(NOT "${package_target_acceptance_report_mode_blocker_combined}" MATCHES
       "`plan\\.acceptance\\.finalAcceptanceBlocker`: targetPlatform, mainBatchAnalysisConfig, compareRealtimeGtpConfig, compareBatchAnalysisConfig, targetDisplay, multiDisplay, externalTargetVerification, manualRawEngineComparison, manualUiInspection")
        message(
            FATAL_ERROR
            "Package target acceptance report mode-blocker smoke did not report mode-specific final acceptance blockers: ${package_target_acceptance_report_mode_blocker_combined}")
    endif()
    if("${package_target_acceptance_report_mode_blocker_combined}" MATCHES
       "`plan\\.acceptance\\.finalAcceptanceBlocker`: [^\n]*(mainKataGoConfig|compareKataGoConfig)")
        message(
            FATAL_ERROR
            "Package target acceptance report mode-blocker smoke should not report coarse engine config blockers: ${package_target_acceptance_report_mode_blocker_combined}")
    endif()
endif()

if(PACKAGE_RUN_DIAGNOSTICS_SMOKE)
    set(package_diagnostics_smoke_dir_owned FALSE)
    if(NOT DEFINED PACKAGE_DIAGNOSTICS_SMOKE_DIR)
        get_filename_component(package_directory "${package_path}" DIRECTORY)
        set(PACKAGE_DIAGNOSTICS_SMOKE_DIR "${package_directory}/verify-package-diagnostics")
        set(package_diagnostics_smoke_dir_owned TRUE)
    endif()

    file(REMOVE_RECURSE "${PACKAGE_DIAGNOSTICS_SMOKE_DIR}")
    file(MAKE_DIRECTORY "${PACKAGE_DIAGNOSTICS_SMOKE_DIR}")
    execute_process(
        COMMAND "${CMAKE_COMMAND}" -E tar xf "${package_path}"
        WORKING_DIRECTORY "${PACKAGE_DIAGNOSTICS_SMOKE_DIR}"
        RESULT_VARIABLE diagnostics_extract_result
        ERROR_VARIABLE diagnostics_extract_error
    )
    if(NOT diagnostics_extract_result EQUAL 0)
        message(FATAL_ERROR "Could not extract package '${package_path}' for diagnostics smoke: ${diagnostics_extract_error}")
    endif()

    set(package_diagnostics_executable_path "${PACKAGE_DIAGNOSTICS_SMOKE_DIR}/${package_executable_entry}")
    if(NOT EXISTS "${package_diagnostics_executable_path}")
        message(FATAL_ERROR "Extracted package is missing executable '${package_executable_entry}'")
    endif()
    file(SHA256 "${package_diagnostics_executable_path}" package_diagnostics_executable_sha256)

    set(package_diagnostics_model_path "${PACKAGE_DIAGNOSTICS_SMOKE_DIR}/diagnostics-model.bin.gz")
    set(package_diagnostics_analysis_config_path "${PACKAGE_DIAGNOSTICS_SMOKE_DIR}/diagnostics-analysis.cfg")
    set(package_diagnostics_gtp_config_path "${PACKAGE_DIAGNOSTICS_SMOKE_DIR}/diagnostics-gtp.cfg")
    set(package_diagnostics_compare_model_path "${PACKAGE_DIAGNOSTICS_SMOKE_DIR}/diagnostics-compare-model.bin.gz")
    set(
        package_diagnostics_compare_analysis_config_path
        "${PACKAGE_DIAGNOSTICS_SMOKE_DIR}/diagnostics-compare-analysis.cfg")
    set(package_diagnostics_compare_gtp_config_path "${PACKAGE_DIAGNOSTICS_SMOKE_DIR}/diagnostics-compare-gtp.cfg")
    set(package_diagnostics_engine_working_dir "${PACKAGE_DIAGNOSTICS_SMOKE_DIR}/diagnostics-engine-working-dir")
    set(package_diagnostics_compare_working_dir "${PACKAGE_DIAGNOSTICS_SMOKE_DIR}/diagnostics-compare-working-dir")
    set(package_diagnostics_session_sgf_path "${PACKAGE_DIAGNOSTICS_SMOKE_DIR}/diagnostics-session.sgf")
    set(package_diagnostics_black_stone_texture_path "${PACKAGE_DIAGNOSTICS_SMOKE_DIR}/diagnostics-black-stone.png")
    set(package_diagnostics_white_stone_texture_path "${PACKAGE_DIAGNOSTICS_SMOKE_DIR}/diagnostics-white-stone.png")
    set(package_diagnostics_home_dir "${PACKAGE_DIAGNOSTICS_SMOKE_DIR}/diagnostics-home")
    set(package_diagnostics_settings_dir "${PACKAGE_DIAGNOSTICS_SMOKE_DIR}/diagnostics-settings/LizzieYzy")
    set(package_diagnostics_settings_file "${package_diagnostics_settings_dir}/LizzieYzy Qt.conf")
    set(package_diagnostics_xdg_data_home "${PACKAGE_DIAGNOSTICS_SMOKE_DIR}/diagnostics-xdg-data")
    set(package_diagnostics_xdg_cache_home "${PACKAGE_DIAGNOSTICS_SMOKE_DIR}/diagnostics-xdg-cache")
    set(package_diagnostics_xdg_runtime_dir "${PACKAGE_DIAGNOSTICS_SMOKE_DIR}/diagnostics-xdg-runtime")
    set(package_diagnostics_user_profile_dir "${PACKAGE_DIAGNOSTICS_SMOKE_DIR}/diagnostics-user-profile")
    set(package_diagnostics_appdata_dir "${PACKAGE_DIAGNOSTICS_SMOKE_DIR}/diagnostics-appdata")
    set(package_diagnostics_localappdata_dir "${PACKAGE_DIAGNOSTICS_SMOKE_DIR}/diagnostics-localappdata")
    set(package_diagnostics_temp_dir "${PACKAGE_DIAGNOSTICS_SMOKE_DIR}/diagnostics-temp")
    set(package_diagnostics_tmp_dir "${PACKAGE_DIAGNOSTICS_SMOKE_DIR}/diagnostics-tmp")
    set(package_diagnostics_kwin_drm_device_0 "${PACKAGE_DIAGNOSTICS_SMOKE_DIR}/diagnostics-dri-card0")
    set(package_diagnostics_kwin_drm_device_1 "${PACKAGE_DIAGNOSTICS_SMOKE_DIR}/diagnostics-dri-card1")
    set(package_diagnostics_vulkan_icd_0 "${PACKAGE_DIAGNOSTICS_SMOKE_DIR}/diagnostics-nvidia-icd-0.json")
    set(package_diagnostics_vulkan_icd_1 "${PACKAGE_DIAGNOSTICS_SMOKE_DIR}/diagnostics-nvidia-icd-1.json")
    set(package_diagnostics_vulkan_driver_0 "${PACKAGE_DIAGNOSTICS_SMOKE_DIR}/diagnostics-nvidia-driver-0.json")
    set(package_diagnostics_vulkan_driver_1 "${PACKAGE_DIAGNOSTICS_SMOKE_DIR}/diagnostics-nvidia-driver-1.json")
    set(package_diagnostics_egl_vendor_0 "${PACKAGE_DIAGNOSTICS_SMOKE_DIR}/diagnostics-egl-vendor-0.json")
    set(package_diagnostics_egl_vendor_1 "${PACKAGE_DIAGNOSTICS_SMOKE_DIR}/diagnostics-egl-vendor-1.json")
    set(package_diagnostics_qml_import_0 "${PACKAGE_DIAGNOSTICS_SMOKE_DIR}/diagnostics-qml-import-0")
    set(package_diagnostics_qml_import_1 "${PACKAGE_DIAGNOSTICS_SMOKE_DIR}/diagnostics-qml-import-1")
    set(package_diagnostics_qml2_import_0 "${PACKAGE_DIAGNOSTICS_SMOKE_DIR}/diagnostics-qml2-import-0")
    set(package_diagnostics_qml2_import_1 "${PACKAGE_DIAGNOSTICS_SMOKE_DIR}/diagnostics-qml2-import-1")
    set(
        package_diagnostics_quick_controls_style_0
        "${PACKAGE_DIAGNOSTICS_SMOKE_DIR}/diagnostics-quick-controls-style-0")
    set(
        package_diagnostics_quick_controls_style_1
        "${PACKAGE_DIAGNOSTICS_SMOKE_DIR}/diagnostics-quick-controls-style-1")
    set(package_diagnostics_quick_controls_conf "${PACKAGE_DIAGNOSTICS_SMOKE_DIR}/qtquickcontrols2.conf")
    set(package_diagnostics_record_file "${PACKAGE_DIAGNOSTICS_SMOKE_DIR}/diagnostics-target-acceptance-record.ini")
    set(package_diagnostics_diagnostics_evidence "${PACKAGE_DIAGNOSTICS_SMOKE_DIR}/diagnostics-output.txt")
    set(package_diagnostics_target_report_evidence "${PACKAGE_DIAGNOSTICS_SMOKE_DIR}/diagnostics-target-report.md")
    set(package_diagnostics_engine_log_evidence "${PACKAGE_DIAGNOSTICS_SMOKE_DIR}/diagnostics-engine.log")
    set(package_diagnostics_raw_log_evidence "${PACKAGE_DIAGNOSTICS_SMOKE_DIR}/diagnostics-raw-katago.log")
    set(package_diagnostics_manual_ui_log_evidence "${PACKAGE_DIAGNOSTICS_SMOKE_DIR}/diagnostics-manual-ui-inspection.log")
    set(package_diagnostics_external_verification_log_evidence "${PACKAGE_DIAGNOSTICS_SMOKE_DIR}/diagnostics-external-verification.log")
    set(package_diagnostics_screenshot_evidence "${PACKAGE_DIAGNOSTICS_SMOKE_DIR}/diagnostics-screenshot-4k.pgm")
    file(WRITE "${package_diagnostics_model_path}" "diagnostics model fixture\n")
    file(WRITE "${package_diagnostics_analysis_config_path}" "diagnostics analysis config fixture\n")
    file(WRITE "${package_diagnostics_gtp_config_path}" "diagnostics gtp config fixture\n")
    file(WRITE "${package_diagnostics_compare_model_path}" "diagnostics compare model fixture\n")
    file(WRITE "${package_diagnostics_compare_analysis_config_path}" "diagnostics compare analysis config fixture\n")
    file(WRITE "${package_diagnostics_compare_gtp_config_path}" "diagnostics compare gtp config fixture\n")
    file(WRITE "${package_diagnostics_session_sgf_path}" "(;GM[1]FF[4]SZ[9];B[dd];W[ee])\n")
    file(WRITE "${package_diagnostics_black_stone_texture_path}" "diagnostics black stone texture fixture\n")
    file(WRITE "${package_diagnostics_white_stone_texture_path}" "diagnostics white stone texture fixture\n")
    file(MAKE_DIRECTORY "${package_diagnostics_engine_working_dir}")
    file(MAKE_DIRECTORY "${package_diagnostics_compare_working_dir}")
    file(MAKE_DIRECTORY "${package_diagnostics_home_dir}")
    file(MAKE_DIRECTORY "${package_diagnostics_settings_dir}")
    file(MAKE_DIRECTORY "${package_diagnostics_xdg_data_home}")
    file(MAKE_DIRECTORY "${package_diagnostics_xdg_cache_home}")
    file(MAKE_DIRECTORY "${package_diagnostics_xdg_runtime_dir}")
    file(MAKE_DIRECTORY "${package_diagnostics_user_profile_dir}")
    file(MAKE_DIRECTORY "${package_diagnostics_appdata_dir}")
    file(MAKE_DIRECTORY "${package_diagnostics_localappdata_dir}")
    file(MAKE_DIRECTORY "${package_diagnostics_temp_dir}")
    file(MAKE_DIRECTORY "${package_diagnostics_tmp_dir}")
    file(CHMOD "${package_diagnostics_xdg_runtime_dir}" PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE)
    file(WRITE "${package_diagnostics_settings_file}" "
[startup]
firstRunComplete=true

[engine]
executable=${package_diagnostics_executable_path}
model=${package_diagnostics_model_path}
gtpConfig=${package_diagnostics_gtp_config_path}
analysisConfig=${package_diagnostics_analysis_config_path}
workingDirectory=${package_diagnostics_engine_working_dir}
extraArgs=--main-flag 123

[compare]
executable=${package_diagnostics_executable_path}
model=${package_diagnostics_compare_model_path}
gtpConfig=${package_diagnostics_compare_gtp_config_path}
analysisConfig=${package_diagnostics_compare_analysis_config_path}
workingDirectory=${package_diagnostics_compare_working_dir}
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
blackStoneTexture=${package_diagnostics_black_stone_texture_path}
whiteStoneTexture=${package_diagnostics_white_stone_texture_path}

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
lastFile=${package_diagnostics_session_sgf_path}
currentNodeId=2
collectionGameIndex=1
")
    file(WRITE "${package_diagnostics_kwin_drm_device_0}" "diagnostics drm fixture 0\n")
    file(WRITE "${package_diagnostics_kwin_drm_device_1}" "diagnostics drm fixture 1\n")
    file(WRITE "${package_diagnostics_vulkan_icd_0}" "diagnostics vulkan icd fixture 0\n")
    file(WRITE "${package_diagnostics_vulkan_icd_1}" "diagnostics vulkan icd fixture 1\n")
    file(WRITE "${package_diagnostics_vulkan_driver_0}" "diagnostics vulkan driver fixture 0\n")
    file(WRITE "${package_diagnostics_vulkan_driver_1}" "diagnostics vulkan driver fixture 1\n")
    file(WRITE "${package_diagnostics_egl_vendor_0}" "diagnostics egl vendor fixture 0\n")
    file(WRITE "${package_diagnostics_egl_vendor_1}" "diagnostics egl vendor fixture 1\n")
    file(WRITE "${package_diagnostics_quick_controls_conf}" "[Controls]\nStyle=Basic\n")
    file(MAKE_DIRECTORY "${package_diagnostics_qml_import_0}")
    file(MAKE_DIRECTORY "${package_diagnostics_qml_import_1}")
    file(MAKE_DIRECTORY "${package_diagnostics_qml2_import_0}")
    file(MAKE_DIRECTORY "${package_diagnostics_qml2_import_1}")
    file(MAKE_DIRECTORY "${package_diagnostics_quick_controls_style_0}")
    file(MAKE_DIRECTORY "${package_diagnostics_quick_controls_style_1}")
    set(package_diagnostics_kwin_drm_devices "${package_diagnostics_kwin_drm_device_0}" "${package_diagnostics_kwin_drm_device_1}")
    set(package_diagnostics_vulkan_icd_files "${package_diagnostics_vulkan_icd_0}" "${package_diagnostics_vulkan_icd_1}")
    set(
        package_diagnostics_vulkan_driver_files
        "${package_diagnostics_vulkan_driver_0}" "${package_diagnostics_vulkan_driver_1}")
    set(package_diagnostics_egl_vendor_files "${package_diagnostics_egl_vendor_0}" "${package_diagnostics_egl_vendor_1}")
    set(package_diagnostics_qml_import_paths "${package_diagnostics_qml_import_0}" "${package_diagnostics_qml_import_1}")
    set(package_diagnostics_qml2_import_paths "${package_diagnostics_qml2_import_0}" "${package_diagnostics_qml2_import_1}")
    set(
        package_diagnostics_quick_controls_style_paths
        "${package_diagnostics_quick_controls_style_0}" "${package_diagnostics_quick_controls_style_1}")
    cmake_path(CONVERT "${package_diagnostics_kwin_drm_devices}" TO_NATIVE_PATH_LIST package_diagnostics_kwin_env)
    cmake_path(CONVERT "${package_diagnostics_vulkan_icd_files}" TO_NATIVE_PATH_LIST package_diagnostics_vulkan_icd_env)
    cmake_path(
        CONVERT "${package_diagnostics_vulkan_driver_files}"
        TO_NATIVE_PATH_LIST package_diagnostics_vulkan_driver_env)
    cmake_path(
        CONVERT "${package_diagnostics_egl_vendor_files}"
        TO_NATIVE_PATH_LIST package_diagnostics_egl_vendor_env)
    cmake_path(CONVERT "${package_diagnostics_qml_import_paths}" TO_NATIVE_PATH_LIST package_diagnostics_qml_import_env)
    cmake_path(CONVERT "${package_diagnostics_qml2_import_paths}" TO_NATIVE_PATH_LIST package_diagnostics_qml2_import_env)
    cmake_path(
        CONVERT "${package_diagnostics_quick_controls_style_paths}"
        TO_NATIVE_PATH_LIST package_diagnostics_quick_controls_style_env)
    string(REPLACE ";" "\\;" package_diagnostics_kwin_env_arg "${package_diagnostics_kwin_env}")
    string(REPLACE ";" "\\;" package_diagnostics_vulkan_icd_env_arg "${package_diagnostics_vulkan_icd_env}")
    string(REPLACE ";" "\\;" package_diagnostics_vulkan_driver_env_arg "${package_diagnostics_vulkan_driver_env}")
    string(REPLACE ";" "\\;" package_diagnostics_egl_vendor_env_arg "${package_diagnostics_egl_vendor_env}")
    string(REPLACE ";" "\\;" package_diagnostics_qml_import_env_arg "${package_diagnostics_qml_import_env}")
    string(REPLACE ";" "\\;" package_diagnostics_qml2_import_env_arg "${package_diagnostics_qml2_import_env}")
    string(
        REPLACE ";" "\\;" package_diagnostics_quick_controls_style_env_arg
        "${package_diagnostics_quick_controls_style_env}")
    set(package_diagnostics_blank_path_list_entry "   ")

    set(package_diagnostics_qpa_environment)
    if(DEFINED PACKAGE_DIAGNOSTICS_QPA_PLATFORM AND NOT "${PACKAGE_DIAGNOSTICS_QPA_PLATFORM}" STREQUAL "")
        list(APPEND package_diagnostics_qpa_environment "QT_QPA_PLATFORM=${PACKAGE_DIAGNOSTICS_QPA_PLATFORM}")
    endif()
    set(
        diagnostics_environment
        "KWIN_DRM_DEVICES=${package_diagnostics_kwin_env_arg}"
        "QT_QPA_PLATFORM_PLUGIN_PATH="
        "QT_PLUGIN_PATH=${package_diagnostics_blank_path_list_entry}"
        "VK_ICD_FILENAMES=${package_diagnostics_vulkan_icd_env_arg}"
        "VK_DRIVER_FILES=${package_diagnostics_vulkan_driver_env_arg}"
        "__EGL_VENDOR_LIBRARY_FILENAMES=${package_diagnostics_egl_vendor_env_arg}"
        "QML_IMPORT_PATH=${package_diagnostics_qml_import_env_arg}"
        "QML2_IMPORT_PATH=${package_diagnostics_qml2_import_env_arg}"
        "QT_QUICK_CONTROLS_CONF=${package_diagnostics_quick_controls_conf}"
        "QT_QUICK_CONTROLS_STYLE_PATH=${package_diagnostics_quick_controls_style_env_arg}"
        "HOME=${package_diagnostics_home_dir}"
        "USERPROFILE=${package_diagnostics_user_profile_dir}"
        "APPDATA=${package_diagnostics_appdata_dir}"
        "LOCALAPPDATA=${package_diagnostics_localappdata_dir}"
        "XDG_CONFIG_HOME=${PACKAGE_DIAGNOSTICS_SMOKE_DIR}/diagnostics-settings"
        "XDG_DATA_HOME=${package_diagnostics_xdg_data_home}"
        "XDG_CACHE_HOME=${package_diagnostics_xdg_cache_home}"
        "XDG_RUNTIME_DIR=${package_diagnostics_xdg_runtime_dir}"
        "TEMP=${package_diagnostics_temp_dir}"
        "TMP=${package_diagnostics_tmp_dir}"
        "LIZZIE_DIAGNOSTICS_SETTINGS_FILE=${package_diagnostics_settings_file}"
        "LIZZIE_KATAGO_EXECUTABLE=${package_diagnostics_executable_path}"
        "LIZZIE_KATAGO_MODEL=${package_diagnostics_model_path}"
        "LIZZIE_KATAGO_ANALYSIS_CONFIG=${package_diagnostics_analysis_config_path}"
        "LIZZIE_KATAGO_GTP_CONFIG=${package_diagnostics_gtp_config_path}"
    )
    list(APPEND diagnostics_environment ${package_diagnostics_qpa_environment})
    execute_process(
        COMMAND "${CMAKE_COMMAND}" -E env ${diagnostics_environment} "${package_diagnostics_executable_path}" --diagnostics
        WORKING_DIRECTORY "${PACKAGE_DIAGNOSTICS_SMOKE_DIR}"
        RESULT_VARIABLE diagnostics_result
        OUTPUT_VARIABLE diagnostics_output
        ERROR_VARIABLE diagnostics_error
    )
    if(NOT diagnostics_result STREQUAL "0")
        message(
            FATAL_ERROR
            "Package diagnostics smoke failed for '${package_executable_entry}': ${diagnostics_output}${diagnostics_error}")
    endif()
    set(package_diagnostics_combined "${diagnostics_output}${diagnostics_error}")
    function(require_diagnostics_output output_variable pattern description)
        if(NOT "${${output_variable}}" MATCHES "${pattern}")
            message(
                FATAL_ERROR
                "Package diagnostics smoke did not ${description}: ${${output_variable}}")
        endif()
    endfunction()
    function(require_package_diagnostics pattern description)
        require_diagnostics_output(package_diagnostics_combined "${pattern}" "${description}")
    endfunction()
    if(NOT "${diagnostics_output}${diagnostics_error}" MATCHES "LizzieYzy Qt Diagnostics")
        message(FATAL_ERROR "Package diagnostics smoke did not print its header: ${diagnostics_output}${diagnostics_error}")
    endif()
    if(NOT "${diagnostics_output}${diagnostics_error}" MATCHES "app\\.executable: ")
        message(FATAL_ERROR "Package diagnostics smoke did not report the application executable: ${diagnostics_output}${diagnostics_error}")
    endif()
    require_package_diagnostics("app\\.executable\\.exists: true" "validate that the application executable exists")
    require_package_diagnostics("app\\.executable\\.file: true" "validate that the application executable is a file")
    require_package_diagnostics("app\\.executable\\.executable: true" "validate that the application executable is runnable")
    require_package_diagnostics("app\\.executable\\.size: [0-9]+" "report the application executable size")
    require_package_diagnostics("app\\.executable\\.lastModifiedUtc: " "report the application executable modification time")
    if(NOT "${diagnostics_output}${diagnostics_error}" MATCHES "app\\.dir: ")
        message(FATAL_ERROR "Package diagnostics smoke did not report the application directory: ${diagnostics_output}${diagnostics_error}")
    endif()
    require_package_diagnostics("app\\.dir\\.exists: true" "validate that the application directory exists")
    require_package_diagnostics("app\\.dir\\.dir: true" "validate that the application directory is a directory")
    require_package_diagnostics("app\\.dir\\.size: -?[0-9]+" "report the application directory size")
    require_package_diagnostics("app\\.dir\\.lastModifiedUtc: " "report the application directory modification time")
    require_package_diagnostics("qt\\.runtime\\.appLocal\\.dir: " "report the app-local Qt runtime directory")
    require_package_diagnostics("qt\\.runtime\\.appLocal\\.dir\\.exists: true" "validate the app-local runtime directory")
    require_package_diagnostics("qt\\.runtime\\.appLocal\\.dir\\.dir: true" "classify the app-local runtime directory")
    require_package_diagnostics(
        "qt\\.runtime\\.appLocal\\.expectedLibrary\\.count: 8"
        "report expected app-local Qt runtime library count")
    require_package_diagnostics(
        "qt\\.runtime\\.appLocal\\.expectedLibrary\\.0\\.module: Core"
        "report the app-local Qt Core runtime candidate")
    require_package_diagnostics(
        "qt\\.runtime\\.appLocal\\.expectedLibrary\\.0\\.fileName: "
        "report the app-local Qt Core runtime filename")
    require_package_diagnostics(
        "qt\\.runtime\\.appLocal\\.expectedLibrary\\.0\\.path: "
        "report the app-local Qt Core runtime path")
    require_package_diagnostics(
        "qt\\.runtime\\.appLocal\\.expectedLibrary\\.0\\.exists: (true|false)"
        "report whether the app-local Qt Core runtime exists")
    require_package_diagnostics(
        "qt\\.runtime\\.appLocal\\.expectedLibrary\\.0\\.size: -?[0-9]+"
        "report the app-local Qt Core runtime size")
    require_package_diagnostics(
        "qt\\.runtime\\.appLocal\\.expectedLibrary\\.7\\.module: OpenGL"
        "report the app-local Qt OpenGL runtime candidate")
    require_package_diagnostics(
        "qt\\.runtime\\.appLocal\\.platformPlugin\\.count: 6"
        "report expected app-local platform plugin count")
    require_package_diagnostics(
        "qt\\.runtime\\.appLocal\\.platformPlugin\\.0\\.baseName: qwindows"
        "report the app-local Windows platform plugin candidate")
    require_package_diagnostics(
        "qt\\.runtime\\.appLocal\\.platformPlugin\\.0\\.path: "
        "report the app-local Windows platform plugin path")
    require_package_diagnostics(
        "qt\\.runtime\\.appLocal\\.platformPlugin\\.0\\.exists: (true|false)"
        "report whether the app-local Windows platform plugin exists")
    require_package_diagnostics(
        "qt\\.runtime\\.appLocal\\.platformPlugin\\.5\\.baseName: qoffscreen"
        "report the app-local offscreen platform plugin candidate")
    require_package_diagnostics("process\\.currentWorkingDirectory: " "report the process working directory")
    require_package_diagnostics(
        "process\\.currentWorkingDirectory\\.exists: true" "validate the process working directory")
    require_package_diagnostics(
        "process\\.currentWorkingDirectory\\.dir: true" "classify the process working directory")
    require_package_diagnostics(
        "process\\.currentWorkingDirectory\\.writable: true" "validate the process working directory is writable")
    if(NOT "${diagnostics_output}${diagnostics_error}" MATCHES "qt\\.platform: ")
        message(FATAL_ERROR "Package diagnostics smoke did not report the Qt platform: ${diagnostics_output}${diagnostics_error}")
    endif()
    require_package_diagnostics("qt\\.version: " "report the Qt runtime version")
    require_package_diagnostics("qt\\.buildVersion: " "report the Qt build version")
    require_package_diagnostics("qt\\.runtimeVersion: " "report the explicit Qt runtime version")
    require_package_diagnostics(
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
        require_package_diagnostics("qt\\.install\\.${qt_install_path}: " "report Qt ${qt_install_path}")
        require_package_diagnostics(
            "qt\\.install\\.${qt_install_path}\\.absolutePath: "
            "report Qt ${qt_install_path} absolute path")
        require_package_diagnostics(
            "qt\\.install\\.${qt_install_path}\\.exists: (true|false)"
            "report whether Qt ${qt_install_path} exists")
        require_package_diagnostics(
            "qt\\.install\\.${qt_install_path}\\.size: -?[0-9]+"
            "report Qt ${qt_install_path} size")
        require_package_diagnostics(
            "qt\\.install\\.${qt_install_path}\\.lastModifiedUtc: "
            "report Qt ${qt_install_path} modification time")
    endforeach()
    require_package_diagnostics("qt\\.pluginPath\\.exists: true" "validate the Qt plugin path")
    require_package_diagnostics("qt\\.pluginPath\\.dir: true" "validate that the Qt plugin path is a directory")
    require_package_diagnostics("qt\\.pluginPath\\.size: -?[0-9]+" "report the Qt plugin path size")
    require_package_diagnostics("qt\\.pluginPath\\.lastModifiedUtc: " "report the Qt plugin path modification time")
    if(NOT "${diagnostics_output}${diagnostics_error}" MATCHES "qt\\.libraryPath\\.count: [0-9]+")
        message(FATAL_ERROR "Package diagnostics smoke did not report Qt library paths: ${diagnostics_output}${diagnostics_error}")
    endif()
    require_package_diagnostics("qt\\.libraryPath\\.0\\.exists: true" "validate the first Qt library path")
    require_package_diagnostics("qt\\.libraryPath\\.0\\.dir: true" "validate that the first Qt library path is a directory")
    require_package_diagnostics("qt\\.libraryPath\\.0\\.size: -?[0-9]+" "report the first Qt library path size")
    require_package_diagnostics("qt\\.libraryPath\\.0\\.lastModifiedUtc: " "report the first Qt library path modification time")
    require_package_diagnostics("qt\\.standardPath\\.config: " "report the writable config location")
    require_package_diagnostics(
        "qt\\.standardPath\\.config\\.absolutePath: "
        "report the writable config location absolute path")
    require_package_diagnostics(
        "qt\\.standardPath\\.config\\.exists: (true|false)"
        "report whether the writable config location exists")
    require_package_diagnostics(
        "qt\\.standardPath\\.config\\.dir: (true|false)"
        "report whether the writable config location is a directory")
    require_package_diagnostics("qt\\.standardPath\\.config\\.size: -?[0-9]+" "report the writable config location size")
    require_package_diagnostics(
        "qt\\.standardPath\\.config\\.lastModifiedUtc: "
        "report the writable config location modification time")
    require_package_diagnostics("qt\\.standardPath\\.appConfig: " "report the writable app config location")
    require_package_diagnostics(
        "qt\\.standardPath\\.appConfig\\.absolutePath: "
        "report the writable app config location absolute path")
    require_package_diagnostics(
        "qt\\.standardPath\\.appConfig\\.exists: (true|false)"
        "report whether the writable app config location exists")
    require_package_diagnostics(
        "qt\\.standardPath\\.appConfig\\.dir: (true|false)"
        "report whether the writable app config location is a directory")
    require_package_diagnostics(
        "qt\\.standardPath\\.appConfig\\.size: -?[0-9]+"
        "report the writable app config location size")
    require_package_diagnostics(
        "qt\\.standardPath\\.appConfig\\.lastModifiedUtc: "
        "report the writable app config location modification time")
    require_package_diagnostics("qt\\.standardPath\\.genericConfig: " "report the writable generic config location")
    require_package_diagnostics(
        "qt\\.standardPath\\.genericConfig\\.absolutePath: "
        "report the writable generic config location absolute path")
    require_package_diagnostics(
        "qt\\.standardPath\\.genericConfig\\.exists: (true|false)"
        "report whether the writable generic config location exists")
    require_package_diagnostics("qt\\.standardPath\\.genericData: " "report the writable generic data location")
    require_package_diagnostics("qt\\.standardPath\\.appData: " "report the writable app data location")
    require_package_diagnostics(
        "qt\\.standardPath\\.appData\\.absolutePath: "
        "report the writable app data location absolute path")
    require_package_diagnostics(
        "qt\\.standardPath\\.appData\\.exists: (true|false)"
        "report whether the writable app data location exists")
    require_package_diagnostics(
        "qt\\.standardPath\\.appData\\.dir: (true|false)"
        "report whether the writable app data location is a directory")
    require_package_diagnostics("qt\\.standardPath\\.appData\\.size: -?[0-9]+" "report the writable app data location size")
    require_package_diagnostics(
        "qt\\.standardPath\\.appData\\.lastModifiedUtc: "
        "report the writable app data location modification time")
    require_package_diagnostics("qt\\.standardPath\\.genericCache: " "report the writable generic cache location")
    require_package_diagnostics("qt\\.standardPath\\.cache: " "report the writable cache location")
    require_package_diagnostics("qt\\.standardPath\\.runtime: " "report the writable runtime location")
    require_package_diagnostics("qt\\.standardPath\\.runtime\\.exists: true" "validate the writable runtime location")
    require_package_diagnostics("qt\\.standardPath\\.runtime\\.dir: true" "classify the writable runtime location")
    require_package_diagnostics("qt\\.standardPath\\.runtime\\.writable: true" "validate the writable runtime location")
    require_package_diagnostics("qt\\.standardPath\\.temp: " "report the writable temporary location")
    require_package_diagnostics("qt\\.font\\.application\\.family: " "report the application font family")
    require_package_diagnostics("qt\\.font\\.application\\.styleName: " "report the application font style name")
    require_package_diagnostics(
        "qt\\.font\\.application\\.pointSizeF: -?[0-9]+(\\.[0-9]+)?"
        "report the application font point size")
    require_package_diagnostics("qt\\.font\\.application\\.pixelSize: -?[0-9]+" "report the application font pixel size")
    require_package_diagnostics("qt\\.font\\.application\\.weight: [0-9]+" "report the application font weight")
    require_package_diagnostics("qt\\.font\\.application\\.bold: (true|false)" "report whether the font is bold")
    require_package_diagnostics("qt\\.font\\.application\\.italic: (true|false)" "report whether the font is italic")
    require_package_diagnostics(
        "qt\\.font\\.application\\.fixedPitch: (true|false)" "report whether the font is fixed pitch")
    require_package_diagnostics("qt\\.font\\.application\\.kerning: (true|false)" "report whether the font uses kerning")
    require_package_diagnostics(
        "qt\\.font\\.application\\.hintingPreference: -?[0-9]+" "report the font hinting preference")
    require_package_diagnostics("qt\\.font\\.metrics\\.height: [0-9]+(\\.[0-9]+)?" "report font metrics height")
    require_package_diagnostics(
        "qt\\.font\\.metrics\\.lineSpacing: [0-9]+(\\.[0-9]+)?" "report font metrics line spacing")
    require_package_diagnostics("qt\\.font\\.metrics\\.ascent: [0-9]+(\\.[0-9]+)?" "report font metrics ascent")
    require_package_diagnostics("qt\\.font\\.metrics\\.descent: [0-9]+(\\.[0-9]+)?" "report font metrics descent")
    require_package_diagnostics(
        "qt\\.font\\.metrics\\.averageCharWidth: [0-9]+(\\.[0-9]+)?"
        "report font average character width")
    require_package_diagnostics(
        "qt\\.font\\.metrics\\.sampleAsciiWidth: [0-9]+(\\.[0-9]+)?" "report English sample text width")
    require_package_diagnostics(
        "qt\\.font\\.metrics\\.sampleCjkWidth: [0-9]+(\\.[0-9]+)?" "report CJK sample text width")
    require_package_diagnostics(
        "qt\\.appearance\\.style\\.objectName: " "report the application style object name")
    require_package_diagnostics(
        "qt\\.appearance\\.style\\.className: " "report the application style class name")
    require_package_diagnostics(
        "qt\\.appearance\\.colorScheme: (Unknown|Light|Dark|ColorScheme\\([0-9]+\\))"
        "report the runtime application color scheme")
    require_package_diagnostics(
        "qt\\.appearance\\.palette\\.window: #[0-9a-fA-F]+"
        "report the runtime window palette color")
    require_package_diagnostics(
        "qt\\.appearance\\.palette\\.windowText: #[0-9a-fA-F]+"
        "report the runtime window-text palette color")
    require_package_diagnostics(
        "qt\\.appearance\\.palette\\.base: #[0-9a-fA-F]+"
        "report the runtime base palette color")
    require_package_diagnostics(
        "qt\\.appearance\\.palette\\.text: #[0-9a-fA-F]+"
        "report the runtime text palette color")
    require_package_diagnostics(
        "qt\\.appearance\\.palette\\.button: #[0-9a-fA-F]+"
        "report the runtime button palette color")
    require_package_diagnostics(
        "qt\\.appearance\\.palette\\.buttonText: #[0-9a-fA-F]+"
        "report the runtime button-text palette color")
    require_package_diagnostics(
        "qt\\.appearance\\.palette\\.highlight: #[0-9a-fA-F]+"
        "report the runtime highlight palette color")
    require_package_diagnostics(
        "qt\\.appearance\\.palette\\.highlightedText: #[0-9a-fA-F]+"
        "report the runtime highlighted-text palette color")
    require_package_diagnostics(
        "qt\\.appearance\\.palette\\.windowLooksDark: (true|false)"
        "report whether the runtime window palette looks dark")
    require_package_diagnostics(
        "qt\\.appearance\\.styleSheet\\.present: (true|false)"
        "report whether a runtime application stylesheet is installed")
    require_package_diagnostics(
        "qt\\.appearance\\.styleSheet\\.length: [0-9]+"
        "report the runtime application stylesheet length")
    require_package_diagnostics(
        "qt\\.dialog\\.fileDialog\\.defaultDontUseNativeDialog: (true|false)"
        "report whether default file dialogs disable native dialogs")
    require_package_diagnostics(
        "qt\\.dialog\\.fileDialog\\.defaultMayUseNativeDialog: (true|false)"
        "report whether default file dialogs may use native dialogs")
    require_package_diagnostics(
        "qt\\.dialog\\.settings\\.pathBrowseButton\\.count: 12"
        "report Settings path browse button count")
    require_package_diagnostics(
        "qt\\.dialog\\.settings\\.pathBrowseButton\\.file\\.count: 10"
        "report Settings file-selector browse button count")
    require_package_diagnostics(
        "qt\\.dialog\\.settings\\.pathBrowseButton\\.directory\\.count: 2"
        "report Settings directory-selector browse button count")
    require_package_diagnostics(
        "qt\\.dialog\\.settings\\.pathBrowseButton\\.[0-9]+\\.rowKey: katago"
        "report Settings KataGo executable browse route")
    require_package_diagnostics(
        "qt\\.dialog\\.settings\\.pathBrowseButton\\.[0-9]+\\.rowKey: workDir"
        "report Settings main working-directory browse route")
    require_package_diagnostics(
        "qt\\.dialog\\.settings\\.pathBrowseButton\\.[0-9]+\\.rowKey: compareWorkDir"
        "report Settings compare working-directory browse route")
    require_package_diagnostics(
        "qt\\.dialog\\.settings\\.pathBrowseButton\\.[0-9]+\\.selectorKind: selectFile"
        "report Settings file selector routing")
    require_package_diagnostics(
        "qt\\.dialog\\.settings\\.pathBrowseButton\\.[0-9]+\\.selectorKind: selectDirectory"
        "report Settings directory selector routing")
    require_package_diagnostics("qt\\.ui\\.mainWindow\\.windowTitle: LizzieYzy Qt" "report the main window title")
    require_package_diagnostics(
        "qt\\.ui\\.mainWindow\\.centralWidget\\.className: QQuickWidget"
        "report the main board Quick central widget")
    require_package_diagnostics(
        "qt\\.ui\\.mainWindow\\.centralWidget\\.objectName: lizzieMainBoardQuickWidget"
        "report the main board Quick widget object name")
    require_package_diagnostics(
        "qt\\.ui\\.mainWindow\\.quickWidget\\.count: 2"
        "report main and ownership Quick widget count")
    require_package_diagnostics(
        "qt\\.ui\\.mainWindow\\.quickWidget\\.[0-9]+\\.objectName: lizzieMainBoardQuickWidget"
        "report the main board Quick widget")
    require_package_diagnostics(
        "qt\\.ui\\.mainWindow\\.quickWidget\\.[0-9]+\\.objectName: lizzieOwnershipQuickWidget"
        "report the ownership board Quick widget")
    require_package_diagnostics(
        "qt\\.ui\\.mainWindow\\.quickWidget\\.[0-9]+\\.rootObjectPresent: true"
        "report Quick widget root object presence")
    require_package_diagnostics(
        "qt\\.ui\\.mainWindow\\.toolBar\\.count: 1"
        "report the main toolbar count")
    require_package_diagnostics(
        "qt\\.ui\\.mainWindow\\.toolBar\\.[0-9]+\\.objectName: lizzieMainToolbar"
        "report the named main toolbar")
    require_package_diagnostics(
        "qt\\.ui\\.mainWindow\\.toolBar\\.[0-9]+\\.action\\.count: 11"
        "report the main toolbar action count")
    require_package_diagnostics(
        "qt\\.ui\\.mainWindow\\.toolBar\\.[0-9]+\\.commandAction\\.count: 8"
        "report the main toolbar command-action count")
    require_package_diagnostics(
        "qt\\.ui\\.mainWindow\\.dock\\.count: 8"
        "report main-window dock count")
    require_package_diagnostics(
        "qt\\.ui\\.mainWindow\\.dock\\.[0-9]+\\.objectName: lizzieCandidatesDock"
        "report the Candidates dock")
    require_package_diagnostics(
        "qt\\.ui\\.mainWindow\\.dock\\.[0-9]+\\.objectName: lizzieOwnershipDock"
        "report the Ownership dock")
    require_package_diagnostics(
        "qt\\.ui\\.mainWindow\\.dock\\.[0-9]+\\.objectName: lizzieCompareDock"
        "report the Compare dock")
    require_package_diagnostics(
        "qt\\.ui\\.mainWindow\\.dock\\.[0-9]+\\.objectName: lizzieConsoleDock"
        "report the GTP Console dock")
    require_package_diagnostics(
        "qt\\.ui\\.mainWindow\\.dock\\.[0-9]+\\.objectName: lizzieEngineLogDock"
        "report the Engine Log dock")
    require_package_diagnostics(
        "qt\\.ui\\.mainWindow\\.dock\\.[0-9]+\\.objectName: lizzieGraphDock"
        "report the graph dock")
    require_package_diagnostics(
        "qt\\.ui\\.mainWindow\\.dock\\.[0-9]+\\.objectName: lizzieTreeDock"
        "report the game-tree dock")
    require_package_diagnostics(
        "qt\\.ui\\.mainWindow\\.table\\.count: 2"
        "report candidate and compare table count")
    require_package_diagnostics(
        "qt\\.ui\\.mainWindow\\.table\\.[0-9]+\\.objectName: lizzieCandidatesTable"
        "report the candidate table")
    require_package_diagnostics(
        "qt\\.ui\\.mainWindow\\.table\\.[0-9]+\\.objectName: lizzieCompareTable"
        "report the compare table")
    require_package_diagnostics(
        "qt\\.ui\\.mainWindow\\.table\\.[0-9]+\\.column\\.count: 8"
        "report the candidate table column count")
    require_package_diagnostics(
        "qt\\.ui\\.mainWindow\\.table\\.[0-9]+\\.column\\.count: 9"
        "report the compare table column count")
    require_package_diagnostics(
        "qt\\.ui\\.mainWindow\\.tree\\.count: 1"
        "report the game tree count")
    require_package_diagnostics(
        "qt\\.ui\\.mainWindow\\.tree\\.[0-9]+\\.objectName: lizzieGameTree"
        "report the named game tree")
    require_package_diagnostics(
        "qt\\.ui\\.mainWindow\\.menu\\.count: 5"
        "report the main menu count")
    require_package_diagnostics("qt\\.locale\\.default\\.name: " "report the default locale name")
    require_package_diagnostics("qt\\.locale\\.default\\.bcp47Name: " "report the default locale BCP-47 name")
    require_package_diagnostics(
        "qt\\.locale\\.default\\.nativeLanguageName: " "report the default locale native language")
    require_package_diagnostics(
        "qt\\.locale\\.default\\.nativeTerritoryName: " "report the default locale native territory")
    require_package_diagnostics(
        "qt\\.locale\\.default\\.textDirection: (LeftToRight|RightToLeft|LayoutDirectionAuto|LayoutDirection\\(-?[0-9]+\\))"
        "report the default locale text direction")
    require_package_diagnostics(
        "qt\\.locale\\.default\\.measurementSystem: -?[0-9]+" "report the default locale measurement system")
    require_package_diagnostics("qt\\.locale\\.default\\.decimalPoint: " "report the default locale decimal point")
    require_package_diagnostics("qt\\.locale\\.default\\.groupSeparator: " "report the default locale group separator")
    require_package_diagnostics(
        "qt\\.locale\\.default\\.firstDayOfWeek: -?[0-9]+" "report the default locale first day of week")
    require_package_diagnostics("qt\\.locale\\.default\\.uiLanguage\\.count: [0-9]+" "report default UI languages")
    require_package_diagnostics("qt\\.locale\\.system\\.name: " "report the system locale name")
    require_package_diagnostics("qt\\.locale\\.system\\.bcp47Name: " "report the system locale BCP-47 name")
    require_package_diagnostics(
        "qt\\.locale\\.system\\.nativeLanguageName: " "report the system locale native language")
    require_package_diagnostics(
        "qt\\.locale\\.system\\.nativeTerritoryName: " "report the system locale native territory")
    require_package_diagnostics(
        "qt\\.locale\\.system\\.textDirection: (LeftToRight|RightToLeft|LayoutDirectionAuto|LayoutDirection\\(-?[0-9]+\\))"
        "report the system locale text direction")
    require_package_diagnostics(
        "qt\\.locale\\.system\\.measurementSystem: -?[0-9]+" "report the system locale measurement system")
    require_package_diagnostics("qt\\.locale\\.system\\.decimalPoint: " "report the system locale decimal point")
    require_package_diagnostics("qt\\.locale\\.system\\.groupSeparator: " "report the system locale group separator")
    require_package_diagnostics(
        "qt\\.locale\\.system\\.firstDayOfWeek: -?[0-9]+" "report the system locale first day of week")
    require_package_diagnostics("qt\\.locale\\.system\\.uiLanguage\\.count: [0-9]+" "report system UI languages")
    require_package_diagnostics("qt\\.settings\\.organizationName: LizzieYzy" "report the QSettings organization name")
    require_package_diagnostics("qt\\.settings\\.applicationName: LizzieYzy Qt" "report the QSettings application name")
    require_package_diagnostics("qt\\.settings\\.defaultFormat: " "report the QSettings default format")
    require_package_diagnostics("qt\\.settings\\.scope: UserScope" "report the QSettings user scope")
    require_package_diagnostics("qt\\.settings\\.fileName: " "report the QSettings storage location")
    require_package_diagnostics(
        "qt\\.settings\\.fileSystemBacked: (true|false)"
        "report whether the QSettings storage location is filesystem-backed")
    require_package_diagnostics(
        "qt\\.settings\\.file\\.absolutePath: "
        "report the QSettings storage absolute path or non-filesystem marker")
    require_package_diagnostics(
        "qt\\.settings\\.file\\.exists: (true|false)"
        "report whether the QSettings storage file exists")
    require_package_diagnostics(
        "qt\\.settings\\.file\\.file: (true|false)"
        "report whether the QSettings storage location is a file")
    require_package_diagnostics(
        "qt\\.settings\\.file\\.readable: (true|false)"
        "report whether the QSettings storage location is readable")
    require_package_diagnostics(
        "qt\\.settings\\.file\\.writable: true"
        "validate that the diagnostics QSettings storage location is writable")
    require_package_diagnostics("qt\\.settings\\.file\\.size: -?[0-9]+" "report the QSettings storage size")
    require_package_diagnostics(
        "qt\\.settings\\.file\\.lastModifiedUtc: "
        "report the QSettings storage modification time or non-filesystem marker")
    require_package_diagnostics(
        "qt\\.settings\\.startup\\.firstRunComplete\\.present: true"
        "report whether first-run completion is stored")
    require_package_diagnostics(
        "qt\\.settings\\.startup\\.firstRunComplete\\.value: true"
        "report the stored first-run completion value")
    require_package_diagnostics("qt\\.settings\\.engine\\.executable\\.present: true" "report stored main engine executable")
    require_package_diagnostics("qt\\.settings\\.engine\\.executable\\.value: " "report main engine executable value")
    require_package_diagnostics(
        "qt\\.settings\\.engine\\.executable\\.path\\.exists: true"
        "validate main engine executable path")
    require_package_diagnostics(
        "qt\\.settings\\.engine\\.executable\\.path\\.file: true"
        "classify main engine executable path")
    require_package_diagnostics(
        "qt\\.settings\\.engine\\.executable\\.path\\.executable: true"
        "validate main engine executable path executability")
    require_package_diagnostics("qt\\.settings\\.engine\\.model\\.present: true" "report stored main engine model")
    require_package_diagnostics("qt\\.settings\\.engine\\.model\\.path\\.exists: true" "validate main engine model path")
    require_package_diagnostics("qt\\.settings\\.engine\\.gtpConfig\\.present: true" "report stored main engine GTP config")
    require_package_diagnostics(
        "qt\\.settings\\.engine\\.gtpConfig\\.path\\.exists: true"
        "validate main engine GTP config path")
    require_package_diagnostics(
        "qt\\.settings\\.engine\\.analysisConfig\\.present: true"
        "report stored main engine analysis config")
    require_package_diagnostics(
        "qt\\.settings\\.engine\\.analysisConfig\\.path\\.exists: true"
        "validate main engine analysis config path")
    require_package_diagnostics(
        "qt\\.settings\\.engine\\.workingDirectory\\.present: true"
        "report stored main engine working directory")
    require_package_diagnostics(
        "qt\\.settings\\.engine\\.workingDirectory\\.path\\.dir: true"
        "validate main engine working directory")
    require_package_diagnostics("qt\\.settings\\.engine\\.extraArgs\\.present: true" "report stored main engine extra args")
    require_package_diagnostics(
        "qt\\.settings\\.engine\\.extraArgs\\.value: --main-flag 123"
        "report main engine extra args value")
    require_package_diagnostics(
        "qt\\.settings\\.engine\\.extraArgs\\.parsed\\.count: 2"
        "report parsed main extra arg count")
    require_package_diagnostics(
        "qt\\.settings\\.engine\\.extraArgs\\.parsed\\.0: --main-flag"
        "report parsed main extra arg 0")
    require_package_diagnostics(
        "qt\\.settings\\.engine\\.extraArgs\\.parsed\\.1: 123"
        "report parsed main extra arg 1")
    require_package_diagnostics(
        "qt\\.settings\\.engine\\.hasGtpMinimum: true"
        "report main GTP minimum readiness")
    require_package_diagnostics(
        "qt\\.settings\\.engine\\.hasAnalysisMinimum: true"
        "report main analysis minimum readiness")
    require_package_diagnostics(
        "qt\\.settings\\.engine\\.config\\.complete: true"
        "report complete main engine config")
    require_package_diagnostics("qt\\.settings\\.engine\\.config\\.ready: true" "report ready main engine config")
    require_package_diagnostics("qt\\.settings\\.engine\\.config\\.gtpReady: true" "report ready main GTP config")
    require_package_diagnostics(
        "qt\\.settings\\.engine\\.config\\.analysisReady: true"
        "report ready main analysis config")
    require_package_diagnostics("qt\\.settings\\.engine\\.config\\.status: ready" "report ready main config status")
    require_package_diagnostics(
        "qt\\.settings\\.engine\\.config\\.missing\\.count: 0"
        "report no missing main config paths")
    require_package_diagnostics(
        "qt\\.settings\\.engine\\.config\\.invalid\\.count: 0"
        "report no invalid main config paths")
    require_package_diagnostics(
        "qt\\.settings\\.compare\\.executable\\.present: true"
        "report stored compare engine executable")
    require_package_diagnostics(
        "qt\\.settings\\.compare\\.executable\\.path\\.exists: true"
        "validate compare engine executable path")
    require_package_diagnostics("qt\\.settings\\.compare\\.model\\.present: true" "report stored compare engine model")
    require_package_diagnostics(
        "qt\\.settings\\.compare\\.model\\.path\\.exists: true"
        "validate compare engine model path")
    require_package_diagnostics(
        "qt\\.settings\\.compare\\.gtpConfig\\.present: true"
        "report stored compare engine GTP config")
    require_package_diagnostics(
        "qt\\.settings\\.compare\\.gtpConfig\\.path\\.exists: true"
        "validate compare engine GTP config path")
    require_package_diagnostics(
        "qt\\.settings\\.compare\\.analysisConfig\\.present: true"
        "report stored compare engine analysis config")
    require_package_diagnostics(
        "qt\\.settings\\.compare\\.analysisConfig\\.path\\.exists: true"
        "validate compare engine analysis config path")
    require_package_diagnostics(
        "qt\\.settings\\.compare\\.workingDirectory\\.present: true"
        "report stored compare engine working directory")
    require_package_diagnostics(
        "qt\\.settings\\.compare\\.workingDirectory\\.path\\.dir: true"
        "validate compare engine working directory")
    require_package_diagnostics("qt\\.settings\\.compare\\.extraArgs\\.present: true" "report stored compare engine extra args")
    require_package_diagnostics(
        "qt\\.settings\\.compare\\.extraArgs\\.value: --compare-flag 456"
        "report compare engine extra args value")
    require_package_diagnostics(
        "qt\\.settings\\.compare\\.extraArgs\\.parsed\\.count: 2"
        "report parsed compare extra arg count")
    require_package_diagnostics(
        "qt\\.settings\\.compare\\.extraArgs\\.parsed\\.0: --compare-flag"
        "report parsed compare extra arg 0")
    require_package_diagnostics(
        "qt\\.settings\\.compare\\.extraArgs\\.parsed\\.1: 456"
        "report parsed compare extra arg 1")
    require_package_diagnostics(
        "qt\\.settings\\.compare\\.hasGtpMinimum: true"
        "report compare GTP minimum readiness")
    require_package_diagnostics(
        "qt\\.settings\\.compare\\.hasAnalysisMinimum: true"
        "report compare analysis minimum readiness")
    require_package_diagnostics(
        "qt\\.settings\\.compare\\.config\\.complete: true"
        "report complete compare engine config")
    require_package_diagnostics("qt\\.settings\\.compare\\.config\\.ready: true" "report ready compare engine config")
    require_package_diagnostics("qt\\.settings\\.compare\\.config\\.gtpReady: true" "report ready compare GTP config")
    require_package_diagnostics(
        "qt\\.settings\\.compare\\.config\\.analysisReady: true"
        "report ready compare analysis config")
    require_package_diagnostics("qt\\.settings\\.compare\\.config\\.status: ready" "report ready compare config status")
    require_package_diagnostics(
        "qt\\.settings\\.compare\\.config\\.missing\\.count: 0"
        "report no missing compare config paths")
    require_package_diagnostics(
        "qt\\.settings\\.compare\\.config\\.invalid\\.count: 0"
        "report no invalid compare config paths")
    require_package_diagnostics(
        "qt\\.settings\\.analysis\\.intervalCentiseconds\\.value: 250"
        "report stored analysis interval")
    require_package_diagnostics(
        "qt\\.settings\\.analysis\\.includeOwnership\\.value: false"
        "report stored ownership setting")
    require_package_diagnostics("qt\\.settings\\.analysis\\.maxVisits\\.value: 123" "report stored max visits")
    require_package_diagnostics("qt\\.settings\\.analysis\\.maxPlayouts\\.value: 456" "report stored max playouts")
    require_package_diagnostics("qt\\.settings\\.analysis\\.maxTimeSeconds\\.value: 7\\.5" "report stored max time")
    require_package_diagnostics(
        "qt\\.settings\\.analysis\\.playoutDoublingAdvantage\\.value: -1\\.25"
        "report stored playout doubling advantage")
    require_package_diagnostics(
        "qt\\.settings\\.analysis\\.analysisWideRootNoise\\.value: 0\\.35"
        "report stored wide root noise")
    require_package_diagnostics("qt\\.settings\\.board\\.showOwnership\\.present: true" "report stored ownership toggle")
    require_package_diagnostics(
        "qt\\.settings\\.board\\.showOwnership\\.value: false"
        "report stored ownership toggle value")
    require_package_diagnostics(
        "qt\\.settings\\.board\\.ownershipDisplayMode\\.present: true"
        "report stored ownership display mode")
    require_package_diagnostics(
        "qt\\.settings\\.board\\.ownershipDisplayMode\\.value: both"
        "report stored ownership display mode value")
    require_package_diagnostics(
        "qt\\.settings\\.board\\.ownershipOpacity\\.present: true"
        "report stored ownership opacity")
    require_package_diagnostics(
        "qt\\.settings\\.board\\.ownershipOpacity\\.value: 0\\.7"
        "report stored ownership opacity value")
    require_package_diagnostics(
        "qt\\.settings\\.board\\.blackStoneTexture\\.present: true"
        "report stored black stone texture path")
    require_package_diagnostics(
        "qt\\.settings\\.board\\.blackStoneTexture\\.value: .*diagnostics-black-stone\\.png"
        "report stored black stone texture value")
    require_package_diagnostics(
        "qt\\.settings\\.board\\.blackStoneTexture\\.path\\.exists: true"
        "validate black stone texture path")
    require_package_diagnostics(
        "qt\\.settings\\.board\\.blackStoneTexture\\.path\\.file: true"
        "classify black stone texture path")
    require_package_diagnostics(
        "qt\\.settings\\.board\\.blackStoneTexture\\.path\\.readable: true"
        "validate black stone texture readability")
    require_package_diagnostics(
        "qt\\.settings\\.board\\.blackStoneTexture\\.path\\.size: [0-9]+"
        "report black stone texture size")
    require_package_diagnostics(
        "qt\\.settings\\.board\\.blackStoneTexture\\.path\\.lastModifiedUtc: "
        "report black stone texture modification time")
    require_package_diagnostics(
        "qt\\.settings\\.board\\.whiteStoneTexture\\.present: true"
        "report stored white stone texture path")
    require_package_diagnostics(
        "qt\\.settings\\.board\\.whiteStoneTexture\\.value: .*diagnostics-white-stone\\.png"
        "report stored white stone texture value")
    require_package_diagnostics(
        "qt\\.settings\\.board\\.whiteStoneTexture\\.path\\.exists: true"
        "validate white stone texture path")
    require_package_diagnostics(
        "qt\\.settings\\.board\\.whiteStoneTexture\\.path\\.file: true"
        "classify white stone texture path")
    require_package_diagnostics(
        "qt\\.settings\\.board\\.whiteStoneTexture\\.path\\.readable: true"
        "validate white stone texture readability")
    require_package_diagnostics(
        "qt\\.settings\\.board\\.whiteStoneTexture\\.path\\.size: [0-9]+"
        "report white stone texture size")
    require_package_diagnostics(
        "qt\\.settings\\.board\\.whiteStoneTexture\\.path\\.lastModifiedUtc: "
        "report white stone texture modification time")
    require_package_diagnostics(
        "qt\\.settings\\.files\\.importLegacySgfAnalysis\\.present: true"
        "report legacy SGF import behavior")
    require_package_diagnostics(
        "qt\\.settings\\.files\\.importLegacySgfAnalysis\\.value: false"
        "report legacy SGF import behavior value")
    require_package_diagnostics(
        "qt\\.settings\\.files\\.loadAnalysisSidecar\\.present: true"
        "report sidecar load behavior")
    require_package_diagnostics(
        "qt\\.settings\\.files\\.loadAnalysisSidecar\\.value: false"
        "report sidecar load behavior value")
    require_package_diagnostics(
        "qt\\.settings\\.files\\.writeAnalysisSidecarAfterBatch\\.present: true"
        "report sidecar write behavior")
    require_package_diagnostics(
        "qt\\.settings\\.files\\.writeAnalysisSidecarAfterBatch\\.value: false"
        "report sidecar write behavior value")
    require_package_diagnostics("qt\\.settings\\.shortcuts\\.new\\.present: true" "report stored New shortcut")
    require_package_diagnostics(
        "qt\\.settings\\.shortcuts\\.new\\.value: Ctrl\\+Alt\\+N"
        "report stored New shortcut value")
    require_package_diagnostics("qt\\.settings\\.shortcuts\\.open\\.present: true" "report stored Open shortcut")
    require_package_diagnostics(
        "qt\\.settings\\.shortcuts\\.open\\.value: Ctrl\\+Alt\\+O"
        "report stored Open shortcut value")
    require_package_diagnostics("qt\\.settings\\.shortcuts\\.save\\.present: true" "report stored Save shortcut")
    require_package_diagnostics(
        "qt\\.settings\\.shortcuts\\.save\\.value: Ctrl\\+Alt\\+S"
        "report stored Save shortcut value")
    require_package_diagnostics("qt\\.settings\\.shortcuts\\.saveAs\\.present: true" "report stored Save As shortcut")
    require_package_diagnostics(
        "qt\\.settings\\.shortcuts\\.saveAs\\.value: Ctrl\\+Shift\\+S"
        "report stored Save As shortcut value")
    require_package_diagnostics("qt\\.settings\\.shortcuts\\.analyze\\.present: true" "report stored Analyze shortcut")
    require_package_diagnostics(
        "qt\\.settings\\.shortcuts\\.analyze\\.value: Ctrl\\+Alt\\+A"
        "report stored Analyze shortcut value")
    require_package_diagnostics("qt\\.settings\\.shortcuts\\.estimate\\.present: true" "report stored Estimate shortcut")
    require_package_diagnostics(
        "qt\\.settings\\.shortcuts\\.estimate\\.value: Ctrl\\+Alt\\+E"
        "report stored Estimate shortcut value")
    require_package_diagnostics("qt\\.settings\\.shortcuts\\.batch\\.present: true" "report stored Batch shortcut")
    require_package_diagnostics(
        "qt\\.settings\\.shortcuts\\.batch\\.value: Ctrl\\+Alt\\+B"
        "report stored Batch shortcut value")
    require_package_diagnostics(
        "qt\\.settings\\.shortcuts\\.restartEngine\\.present: true"
        "report stored Restart Engine shortcut")
    require_package_diagnostics(
        "qt\\.settings\\.shortcuts\\.restartEngine\\.value: Ctrl\\+Alt\\+R"
        "report stored Restart Engine shortcut value")
    require_package_diagnostics("qt\\.settings\\.shortcuts\\.compare\\.present: true" "report stored Compare shortcut")
    require_package_diagnostics(
        "qt\\.settings\\.shortcuts\\.compare\\.value: Ctrl\\+Alt\\+D"
        "report stored Compare shortcut value")
    require_package_diagnostics("qt\\.settings\\.shortcuts\\.aiMove\\.present: true" "report stored AI Move shortcut")
    require_package_diagnostics(
        "qt\\.settings\\.shortcuts\\.aiMove\\.value: Ctrl\\+Alt\\+G"
        "report stored AI Move shortcut value")
    require_package_diagnostics(
        "qt\\.settings\\.shortcuts\\.humanVsAi\\.present: true"
        "report stored Human vs AI shortcut")
    require_package_diagnostics(
        "qt\\.settings\\.shortcuts\\.humanVsAi\\.value: Ctrl\\+Alt\\+H"
        "report stored Human vs AI shortcut value")
    require_package_diagnostics(
        "qt\\.settings\\.shortcuts\\.selfPlay\\.present: true"
        "report stored Self Play shortcut")
    require_package_diagnostics(
        "qt\\.settings\\.shortcuts\\.selfPlay\\.value: Ctrl\\+Alt\\+Shift\\+G"
        "report stored Self Play shortcut value")
    require_package_diagnostics(
        "qt\\.settings\\.shortcuts\\.engineGame\\.present: true"
        "report stored Engine Game shortcut")
    require_package_diagnostics(
        "qt\\.settings\\.shortcuts\\.engineGame\\.value: Ctrl\\+Alt\\+Shift\\+E"
        "report stored Engine Game shortcut value")
    require_package_diagnostics(
        "qt\\.settings\\.shortcuts\\.previous\\.present: true"
        "report stored Previous shortcut")
    require_package_diagnostics(
        "qt\\.settings\\.shortcuts\\.previous\\.value: Alt\\+Left"
        "report stored Previous shortcut value")
    require_package_diagnostics("qt\\.settings\\.shortcuts\\.next\\.present: true" "report stored Next shortcut")
    require_package_diagnostics(
        "qt\\.settings\\.shortcuts\\.next\\.value: Alt\\+Right"
        "report stored Next shortcut value")
    require_package_diagnostics("qt\\.settings\\.shortcuts\\.undo\\.present: true" "report stored Undo shortcut")
    require_package_diagnostics(
        "qt\\.settings\\.shortcuts\\.undo\\.value: Ctrl\\+Alt\\+Z"
        "report stored Undo shortcut value")
    require_package_diagnostics(
        "qt\\.settings\\.shortcuts\\.pass\\.present: true"
        "report stored disabled Pass shortcut")
    require_package_diagnostics("qt\\.settings\\.shortcuts\\.pass\\.value: *\n" "report disabled Pass shortcut value")
    require_package_diagnostics("qt\\.settings\\.shortcuts\\.resign\\.present: true" "report stored Resign shortcut")
    require_package_diagnostics(
        "qt\\.settings\\.shortcuts\\.resign\\.value: Ctrl\\+Alt\\+P"
        "report stored Resign shortcut value")
    require_package_diagnostics(
        "qt\\.settings\\.shortcuts\\.settings\\.present: true"
        "report stored Settings shortcut")
    require_package_diagnostics(
        "qt\\.settings\\.shortcuts\\.settings\\.value: Ctrl\\+Alt\\+Comma"
        "report stored Settings shortcut value")
    require_package_diagnostics(
        "qt\\.settings\\.appearance\\.theme\\.present: true" "report stored theme")
    require_package_diagnostics("qt\\.settings\\.appearance\\.theme\\.value: dark" "report stored theme value")
    require_package_diagnostics(
        "qt\\.settings\\.appearance\\.language\\.present: true" "report stored language")
    require_package_diagnostics("qt\\.settings\\.appearance\\.language\\.value: zh" "report stored language value")
    require_package_diagnostics(
        "qt\\.settings\\.appearance\\.fontScale\\.present: true" "report stored font scale")
    require_package_diagnostics("qt\\.settings\\.appearance\\.fontScale\\.value: 1\\.25" "report stored font scale value")
    require_package_diagnostics(
        "qt\\.settings\\.appearance\\.normalized\\.theme: dark"
        "report normalized stored theme")
    require_package_diagnostics(
        "qt\\.settings\\.appearance\\.normalized\\.language: zh"
        "report normalized stored language")
    require_package_diagnostics(
        "qt\\.settings\\.appearance\\.normalized\\.fontScale: 1\\.25"
        "report normalized stored font scale")
    require_package_diagnostics(
        "qt\\.settings\\.session\\.lastFile\\.present: true"
        "report whether a session SGF path is stored")
    require_package_diagnostics(
        "qt\\.settings\\.session\\.lastFile\\.value: .*diagnostics-session\\.sgf"
        "report the stored session SGF path")
    require_package_diagnostics(
        "qt\\.settings\\.session\\.lastFile\\.path\\.absolutePath: "
        "report the stored session SGF absolute path")
    require_package_diagnostics(
        "qt\\.settings\\.session\\.lastFile\\.path\\.canonicalPath: "
        "report the stored session SGF canonical path")
    require_package_diagnostics(
        "qt\\.settings\\.session\\.lastFile\\.path\\.exists: true"
        "report whether the stored session SGF path exists")
    require_package_diagnostics(
        "qt\\.settings\\.session\\.lastFile\\.path\\.file: true"
        "validate the stored session SGF path is a file")
    require_package_diagnostics(
        "qt\\.settings\\.session\\.lastFile\\.path\\.readable: true"
        "validate the stored session SGF path is readable")
    require_package_diagnostics(
        "qt\\.settings\\.session\\.lastFile\\.path\\.size: -?[0-9]+"
        "report the stored session SGF path size")
    require_package_diagnostics(
        "qt\\.settings\\.session\\.lastFile\\.path\\.lastModifiedUtc: "
        "report the stored session SGF modification time")
    require_package_diagnostics(
        "qt\\.settings\\.session\\.currentNodeId\\.present: true"
        "report whether a session current node is stored")
    require_package_diagnostics(
        "qt\\.settings\\.session\\.currentNodeId\\.value: 2"
        "report the stored session current node")
    require_package_diagnostics(
        "qt\\.settings\\.session\\.collectionGameIndex\\.present: true"
        "report whether a collection game index is stored")
    require_package_diagnostics(
        "qt\\.settings\\.session\\.collectionGameIndex\\.value: 1"
        "report the stored collection game index")
    require_package_diagnostics(
        "qt\\.settings\\.window\\.geometry\\.present: (true|false)"
        "report whether saved window geometry is stored")
    require_package_diagnostics("qt\\.settings\\.window\\.geometry\\.size: [0-9]+" "report saved window geometry size")
    require_package_diagnostics(
        "qt\\.settings\\.window\\.geometry\\.restoreSucceeded: (true|false)"
        "report whether saved window geometry can be restored")
    require_package_diagnostics(
        "qt\\.settings\\.window\\.geometry\\.restoredGeometry: "
        "report the restored saved window geometry")
    require_package_diagnostics(
        "qt\\.settings\\.window\\.geometry\\.restoredFrameGeometry: "
        "report the restored saved window frame geometry")
    require_package_diagnostics(
        "qt\\.settings\\.window\\.geometry\\.visibleAreaOnAvailableScreens: [0-9]+"
        "report saved window geometry visible area")
    require_package_diagnostics(
        "qt\\.settings\\.window\\.geometry\\.requiredVisibleArea: [0-9]+"
        "report saved window geometry required visible area")
    require_package_diagnostics(
        "qt\\.settings\\.window\\.geometry\\.substantiallyVisible: (true|false)"
        "report whether saved window geometry is substantially visible")
    require_package_diagnostics(
        "qt\\.settings\\.window\\.geometry\\.wouldRecenter: (true|false)"
        "report whether saved window geometry would be recentered")
    require_package_diagnostics(
        "qt\\.settings\\.window\\.geometry\\.adjustedGeometry: "
        "report the adjusted saved window geometry")
    require_package_diagnostics(
        "qt\\.settings\\.window\\.state\\.present: (true|false)"
        "report whether saved window state is stored")
    require_package_diagnostics("qt\\.settings\\.window\\.state\\.size: [0-9]+" "report saved window state size")
    require_package_diagnostics(
        "qt\\.settings\\.window\\.state\\.restoreSucceeded: (true|false)"
        "report whether saved window state can be restored")
    require_package_diagnostics("qt\\.platformPlugin\\.current\\.name: " "report the current Qt platform plugin name")
    require_package_diagnostics(
        "qt\\.platformPlugin\\.current\\.expectedFile\\.count: [1-9][0-9]*"
        "report current Qt platform plugin expected file names")
    require_package_diagnostics(
        "qt\\.platformPlugin\\.current\\.candidate\\.count: [1-9][0-9]*"
        "report current Qt platform plugin candidates")
    require_package_diagnostics(
        "qt\\.platformPlugin\\.current\\.candidate\\.0\\.path: "
        "report the first current Qt platform plugin candidate path")
    require_package_diagnostics(
        "qt\\.platformPlugin\\.current\\.candidate\\.0\\.size: -?[0-9]+"
        "report the first current Qt platform plugin candidate size")
    require_package_diagnostics(
        "qt\\.platformPlugin\\.current\\.candidate\\.0\\.lastModifiedUtc: "
        "report the first current Qt platform plugin candidate modification time")
    require_package_diagnostics(
        "qt\\.platformPlugin\\.current\\.found: true"
        "find a readable current Qt platform plugin candidate")
    require_package_diagnostics(
        "qt\\.platformPlugin\\.commonWayland\\.expectedFile\\.count: 3"
        "report common Wayland Qt platform plugin expected file names")
    require_package_diagnostics(
        "qt\\.platformPlugin\\.commonWayland\\.candidate\\.count: [1-9][0-9]*"
        "report common Wayland Qt platform plugin candidates")
    require_package_diagnostics(
        "qt\\.platformPlugin\\.commonWayland\\.candidate\\.0\\.baseName: qwayland"
        "check qwayland before legacy Wayland plugin names")
    require_package_diagnostics(
        "qt\\.platformPlugin\\.commonWayland\\.candidate\\.0\\.path: "
        "report the first common Wayland Qt platform plugin candidate path")
    require_package_diagnostics(
        "qt\\.platformPlugin\\.commonWayland\\.candidate\\.0\\.size: -?[0-9]+"
        "report the first common Wayland Qt platform plugin candidate size")
    require_package_diagnostics(
        "qt\\.platformPlugin\\.commonWayland\\.candidate\\.0\\.lastModifiedUtc: "
        "report the first common Wayland Qt platform plugin candidate modification time")
    require_package_diagnostics(
        "qt\\.platformPlugin\\.commonWayland\\.found: (true|false)"
        "report whether a common Wayland Qt platform plugin candidate was found")
    require_package_diagnostics(
        "qt\\.platformPlugin\\.commonWindows\\.expectedFile\\.count: 1"
        "report common Windows Qt platform plugin expected file names")
    require_package_diagnostics(
        "qt\\.platformPlugin\\.commonWindows\\.candidate\\.count: [1-9][0-9]*"
        "report common Windows Qt platform plugin candidates")
    require_package_diagnostics(
        "qt\\.platformPlugin\\.commonWindows\\.candidate\\.0\\.baseName: qwindows"
        "check qwindows as the common Windows plugin name")
    require_package_diagnostics(
        "qt\\.platformPlugin\\.commonWindows\\.candidate\\.0\\.path: "
        "report the first common Windows Qt platform plugin candidate path")
    require_package_diagnostics(
        "qt\\.platformPlugin\\.commonWindows\\.candidate\\.0\\.size: -?[0-9]+"
        "report the first common Windows Qt platform plugin candidate size")
    require_package_diagnostics(
        "qt\\.platformPlugin\\.commonWindows\\.candidate\\.0\\.lastModifiedUtc: "
        "report the first common Windows Qt platform plugin candidate modification time")
    require_package_diagnostics(
        "qt\\.platformPlugin\\.commonWindows\\.found: (true|false)"
        "report whether a common Windows Qt platform plugin candidate was found")
    require_package_diagnostics(
        "qt\\.platformPlugin\\.availableRoot\\.count: [1-9][0-9]*"
        "report Qt platform plugin search roots")
    require_package_diagnostics(
        "qt\\.platformPlugin\\.availableRoot\\.0\\.platformsDir\\.exists: true"
        "validate the first Qt platform plugin directory")
    require_package_diagnostics(
        "qt\\.platformPlugin\\.availableRoot\\.0\\.platformsDir\\.size: -?[0-9]+"
        "report the first Qt platform plugin directory size")
    require_package_diagnostics(
        "qt\\.platformPlugin\\.availableRoot\\.0\\.platformsDir\\.lastModifiedUtc: "
        "report the first Qt platform plugin directory modification time")
    require_package_diagnostics(
        "qt\\.platformPlugin\\.availableRoot\\.0\\.plugin\\.count: [1-9][0-9]*"
        "report available Qt platform plugins")
    require_package_diagnostics(
        "qt\\.platformPlugin\\.availableRoot\\.0\\.plugin\\.0\\.path: "
        "report the first available Qt platform plugin path")
    require_package_diagnostics(
        "qt\\.platformPlugin\\.availableRoot\\.0\\.plugin\\.0\\.size: [0-9]+"
        "report the first available Qt platform plugin size")
    require_package_diagnostics(
        "qt\\.platformPlugin\\.availableRoot\\.0\\.plugin\\.0\\.lastModifiedUtc: "
        "report the first available Qt platform plugin modification time")
    if(NOT "${diagnostics_output}${diagnostics_error}" MATCHES "qt\\.quick\\.graphicsApi: ")
        message(
            FATAL_ERROR
            "Package diagnostics smoke did not report the Qt Quick graphics API: ${diagnostics_output}${diagnostics_error}")
    endif()
    require_package_diagnostics("qt\\.quick\\.sceneGraphBackend: " "report the Qt Quick scene graph backend")
    require_package_diagnostics("qt\\.qmlImportPath\\.count: [1-9][0-9]*" "report Qt QML import paths")
    require_package_diagnostics("qt\\.qmlImportPath\\.0: " "report the first Qt QML import path")
    require_package_diagnostics(
        "qt\\.qmlImportPath\\.0\\.resource: " "classify the first Qt QML import path")
    require_package_diagnostics(
        "qt\\.quick\\.rendererInterface\\.graphicsApi: " "report the renderer-interface graphics API")
    require_package_diagnostics(
        "qt\\.quick\\.rendererInterface\\.rhiBased: " "report whether the renderer-interface API is RHI based")
    require_package_diagnostics(
        "qt\\.quick\\.rendererInterface\\.shaderType: " "report the renderer-interface shader type")
    require_package_diagnostics(
        "qt\\.quick\\.rendererInterface\\.shaderCompilationTypes: " "report shader compilation types")
    require_package_diagnostics(
        "qt\\.quick\\.rendererInterface\\.shaderSourceTypes: " "report shader source types")
    require_package_diagnostics("qt\\.opengl\\.moduleType: " "report the runtime OpenGL module type")
    require_package_diagnostics(
        "qt\\.opengl\\.context\\.created: (true|false)"
        "report whether an OpenGL context can be created")
    require_package_diagnostics(
        "qt\\.opengl\\.context\\.valid: (true|false)"
        "report whether the OpenGL context is valid")
    require_package_diagnostics(
        "qt\\.opengl\\.surface\\.valid: (true|false)"
        "report whether the OpenGL probe surface is valid")
    require_package_diagnostics(
        "qt\\.opengl\\.context\\.makeCurrentSucceeded: (true|false)"
        "report whether the OpenGL context can be made current")
    require_package_diagnostics(
        "qt\\.opengl\\.context\\.format\\.renderableType: "
        "report the OpenGL context renderable type")
    require_package_diagnostics(
        "qt\\.opengl\\.context\\.format\\.majorVersion: -?[0-9]+"
        "report the OpenGL context major version")
    require_package_diagnostics(
        "qt\\.opengl\\.context\\.format\\.minorVersion: -?[0-9]+"
        "report the OpenGL context minor version")
    require_package_diagnostics(
        "qt\\.opengl\\.context\\.format\\.profile: "
        "report the OpenGL context profile")
    require_package_diagnostics(
        "qt\\.opengl\\.context\\.format\\.depthBufferSize: -?[0-9]+"
        "report the OpenGL context depth-buffer size")
    require_package_diagnostics(
        "qt\\.opengl\\.context\\.format\\.stencilBufferSize: -?[0-9]+"
        "report the OpenGL context stencil-buffer size")
    require_package_diagnostics(
        "qt\\.opengl\\.context\\.format\\.samples: -?[0-9]+"
        "report the OpenGL context sample count")
    require_package_diagnostics(
        "qt\\.opengl\\.functions\\.initialized: (true|false)"
        "report whether OpenGL functions initialized")
    require_package_diagnostics("qt\\.opengl\\.vendor: " "report the OpenGL vendor string")
    require_package_diagnostics("qt\\.opengl\\.renderer: " "report the OpenGL renderer string")
    require_package_diagnostics("qt\\.opengl\\.version: " "report the OpenGL version string")
    require_package_diagnostics(
        "qt\\.opengl\\.shadingLanguageVersion: "
        "report the OpenGL shading language version string")
    require_package_diagnostics("os\\.prettyName: " "report the OS pretty name")
    require_package_diagnostics("os\\.productType: " "report the OS product type")
    require_package_diagnostics("os\\.productVersion: " "report the OS product version")
    require_package_diagnostics("os\\.kernelType: " "report the OS kernel type")
    require_package_diagnostics("os\\.kernelVersion: " "report the OS kernel version")
    require_package_diagnostics("qt\\.buildAbi: " "report the Qt build ABI")
    require_package_diagnostics("cpu\\.buildArchitecture: " "report the build CPU architecture")
    require_package_diagnostics("cpu\\.architecture: " "report the current CPU architecture")
    require_package_diagnostics("env\\.PATH: " "report PATH")
    require_package_diagnostics("envPath\\.PATH\\.count: [0-9]+" "report PATH path-list entries")
    require_package_diagnostics("env\\.LD_LIBRARY_PATH: " "report LD_LIBRARY_PATH")
    require_package_diagnostics(
        "envPath\\.LD_LIBRARY_PATH\\.count: [0-9]+" "report LD_LIBRARY_PATH path-list entries")
    require_package_diagnostics("env\\.DYLD_LIBRARY_PATH: " "report DYLD_LIBRARY_PATH")
    require_package_diagnostics(
        "envPath\\.DYLD_LIBRARY_PATH\\.count: [0-9]+" "report DYLD_LIBRARY_PATH path-list entries")
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
        require_package_diagnostics("env\\.${single_path_env}: " "report ${single_path_env}")
        require_package_diagnostics("envPath\\.${single_path_env}\\.exists: true" "validate ${single_path_env}")
        require_package_diagnostics("envPath\\.${single_path_env}\\.dir: true" "classify ${single_path_env}")
        require_package_diagnostics(
            "envPath\\.${single_path_env}\\.readable: true" "validate ${single_path_env} readability")
        require_package_diagnostics(
            "envPath\\.${single_path_env}\\.writable: true" "validate ${single_path_env} writability")
    endforeach()
    require_package_diagnostics("env\\.XDG_SESSION_TYPE: " "report XDG_SESSION_TYPE")
    require_package_diagnostics("env\\.XDG_CURRENT_DESKTOP: " "report XDG_CURRENT_DESKTOP")
    require_package_diagnostics("env\\.XDG_SESSION_DESKTOP: " "report XDG_SESSION_DESKTOP")
    require_package_diagnostics("env\\.XDG_RUNTIME_DIR: " "report XDG_RUNTIME_DIR")
    require_package_diagnostics("envPath\\.XDG_RUNTIME_DIR\\.exists: true" "validate XDG_RUNTIME_DIR")
    require_package_diagnostics("envPath\\.XDG_RUNTIME_DIR\\.dir: true" "classify XDG_RUNTIME_DIR")
    require_package_diagnostics(
        "envPath\\.XDG_RUNTIME_DIR\\.readable: true" "validate XDG_RUNTIME_DIR readability")
    require_package_diagnostics(
        "envPath\\.XDG_RUNTIME_DIR\\.writable: true" "validate XDG_RUNTIME_DIR writability")
    require_package_diagnostics("env\\.DESKTOP_SESSION: " "report DESKTOP_SESSION")
    require_package_diagnostics("env\\.KDE_FULL_SESSION: " "report KDE_FULL_SESSION")
    require_package_diagnostics("env\\.KDE_SESSION_VERSION: " "report KDE_SESSION_VERSION")
    require_package_diagnostics("env\\.WAYLAND_DISPLAY: " "report WAYLAND_DISPLAY")
    require_package_diagnostics("env\\.DISPLAY: " "report DISPLAY")
    if(NOT "${diagnostics_output}${diagnostics_error}" MATCHES "env\\.QT_QPA_PLATFORM: ")
        message(FATAL_ERROR "Package diagnostics smoke did not report QT_QPA_PLATFORM: ${diagnostics_output}${diagnostics_error}")
    endif()
    if(NOT "${diagnostics_output}${diagnostics_error}" MATCHES "env\\.QT_QPA_PLATFORMTHEME: ")
        message(FATAL_ERROR "Package diagnostics smoke did not report QT_QPA_PLATFORMTHEME: ${diagnostics_output}${diagnostics_error}")
    endif()
    if(NOT "${diagnostics_output}${diagnostics_error}" MATCHES "env\\.QT_QPA_PLATFORM_PLUGIN_PATH: ")
        message(
            FATAL_ERROR
            "Package diagnostics smoke did not report QT_QPA_PLATFORM_PLUGIN_PATH: ${diagnostics_output}${diagnostics_error}")
    endif()
    require_package_diagnostics(
        "env\\.QT_QPA_PLATFORM_PLUGIN_PATH: \\(empty\\)"
        "distinguish empty QT_QPA_PLATFORM_PLUGIN_PATH from unset")
    if(NOT "${diagnostics_output}${diagnostics_error}" MATCHES "env\\.QT_PLUGIN_PATH: ")
        message(FATAL_ERROR "Package diagnostics smoke did not report QT_PLUGIN_PATH: ${diagnostics_output}${diagnostics_error}")
    endif()
    require_package_diagnostics(
        "envPath\\.QT_QPA_PLATFORM_PLUGIN_PATH\\.value: \\(empty\\)"
        "report empty QT_QPA_PLATFORM_PLUGIN_PATH path-list value")
    require_package_diagnostics(
        "envPath\\.QT_QPA_PLATFORM_PLUGIN_PATH\\.count: 1"
        "report empty QT_QPA_PLATFORM_PLUGIN_PATH as one blank path-list entry")
    require_package_diagnostics(
        "envPath\\.QT_QPA_PLATFORM_PLUGIN_PATH\\.0\\.hasText: false"
        "classify empty QT_QPA_PLATFORM_PLUGIN_PATH entries")
    require_package_diagnostics(
        "envPath\\.QT_QPA_PLATFORM_PLUGIN_PATH\\.0\\.absolutePath: \\(blank\\)"
        "avoid resolving empty QT_QPA_PLATFORM_PLUGIN_PATH entries")
    require_package_diagnostics("envPath\\.QT_PLUGIN_PATH\\.count: 1" "report QT_PLUGIN_PATH path-list entry count")
    require_package_diagnostics("envPath\\.QT_PLUGIN_PATH\\.0\\.hasText: false" "classify blank QT_PLUGIN_PATH entries")
    require_package_diagnostics(
        "envPath\\.QT_PLUGIN_PATH\\.0\\.absolutePath: \\(blank\\)" "avoid resolving blank QT_PLUGIN_PATH entries")
    require_package_diagnostics("envPath\\.QT_PLUGIN_PATH\\.0\\.exists: false" "reject blank QT_PLUGIN_PATH entries")
    if(NOT "${diagnostics_output}${diagnostics_error}" MATCHES "env\\.QT_DEBUG_PLUGINS: ")
        message(FATAL_ERROR "Package diagnostics smoke did not report QT_DEBUG_PLUGINS: ${diagnostics_output}${diagnostics_error}")
    endif()
    require_package_diagnostics("env\\.QT_LOGGING_RULES: " "report QT_LOGGING_RULES")
    require_package_diagnostics("env\\.QT_STYLE_OVERRIDE: " "report QT_STYLE_OVERRIDE")
    if(NOT "${diagnostics_output}${diagnostics_error}" MATCHES "env\\.QT_SCALE_FACTOR: ")
        message(FATAL_ERROR "Package diagnostics smoke did not report QT_SCALE_FACTOR: ${diagnostics_output}${diagnostics_error}")
    endif()
    if(NOT "${diagnostics_output}${diagnostics_error}" MATCHES "env\\.QT_SCREEN_SCALE_FACTORS: ")
        message(FATAL_ERROR "Package diagnostics smoke did not report QT_SCREEN_SCALE_FACTORS: ${diagnostics_output}${diagnostics_error}")
    endif()
    require_package_diagnostics("env\\.QT_AUTO_SCREEN_SCALE_FACTOR: " "report QT_AUTO_SCREEN_SCALE_FACTOR")
    require_package_diagnostics("env\\.QT_ENABLE_HIGHDPI_SCALING: " "report QT_ENABLE_HIGHDPI_SCALING")
    require_package_diagnostics(
        "env\\.QT_SCALE_FACTOR_ROUNDING_POLICY: " "report QT_SCALE_FACTOR_ROUNDING_POLICY")
    require_package_diagnostics("env\\.QT_USE_PHYSICAL_DPI: " "report QT_USE_PHYSICAL_DPI")
    require_package_diagnostics("env\\.QT_DPI_ADJUSTMENT_POLICY: " "report QT_DPI_ADJUSTMENT_POLICY")
    require_package_diagnostics("env\\.QT_FONT_DPI: " "report QT_FONT_DPI")
    if(NOT "${diagnostics_output}${diagnostics_error}" MATCHES "env\\.QSG_RHI_BACKEND: ")
        message(FATAL_ERROR "Package diagnostics smoke did not report QSG_RHI_BACKEND: ${diagnostics_output}${diagnostics_error}")
    endif()
    require_package_diagnostics("env\\.QT_RHI_BACKEND: " "report QT_RHI_BACKEND")
    require_package_diagnostics("env\\.QSG_INFO: " "report QSG_INFO")
    if(NOT "${diagnostics_output}${diagnostics_error}" MATCHES "env\\.QT_QUICK_BACKEND: ")
        message(FATAL_ERROR "Package diagnostics smoke did not report QT_QUICK_BACKEND: ${diagnostics_output}${diagnostics_error}")
    endif()
    require_package_diagnostics("env\\.QT_QUICK_CONTROLS_STYLE: " "report QT_QUICK_CONTROLS_STYLE")
    require_package_diagnostics("env\\.QT_QUICK_CONTROLS_CONF: " "report QT_QUICK_CONTROLS_CONF")
    require_package_diagnostics(
        "envPath\\.QT_QUICK_CONTROLS_CONF\\.exists: true" "validate QT_QUICK_CONTROLS_CONF")
    require_package_diagnostics(
        "envPath\\.QT_QUICK_CONTROLS_CONF\\.file: true" "classify QT_QUICK_CONTROLS_CONF")
    require_package_diagnostics(
        "envPath\\.QT_QUICK_CONTROLS_CONF\\.readable: true" "validate QT_QUICK_CONTROLS_CONF readability")
    require_package_diagnostics(
        "envPath\\.QT_QUICK_CONTROLS_CONF\\.writable: true" "validate QT_QUICK_CONTROLS_CONF writability")
    require_package_diagnostics(
        "envPath\\.QT_QUICK_CONTROLS_STYLE_PATH\\.count: 2" "report Qt Quick Controls style path entries")
    require_package_diagnostics(
        "envPath\\.QT_QUICK_CONTROLS_STYLE_PATH\\.0\\.exists: true"
        "validate the first Qt Quick Controls style path")
    require_package_diagnostics(
        "envPath\\.QT_QUICK_CONTROLS_STYLE_PATH\\.0\\.dir: true"
        "classify the first Qt Quick Controls style path")
    require_package_diagnostics("env\\.QML_IMPORT_PATH: " "report QML_IMPORT_PATH")
    require_package_diagnostics("envPath\\.QML_IMPORT_PATH\\.count: 2" "report QML import path entries")
    require_package_diagnostics("envPath\\.QML_IMPORT_PATH\\.0\\.exists: true" "validate the first QML import path")
    require_package_diagnostics("envPath\\.QML_IMPORT_PATH\\.0\\.dir: true" "classify the first QML import path")
    require_package_diagnostics("envPath\\.QML_IMPORT_PATH\\.1\\.exists: true" "validate the second QML import path")
    require_package_diagnostics("envPath\\.QML_IMPORT_PATH\\.1\\.dir: true" "classify the second QML import path")
    require_package_diagnostics("env\\.QML2_IMPORT_PATH: " "report QML2_IMPORT_PATH")
    require_package_diagnostics("envPath\\.QML2_IMPORT_PATH\\.count: 2" "report QML2 import path entries")
    require_package_diagnostics("envPath\\.QML2_IMPORT_PATH\\.0\\.exists: true" "validate the first QML2 import path")
    require_package_diagnostics("envPath\\.QML2_IMPORT_PATH\\.0\\.dir: true" "classify the first QML2 import path")
    require_package_diagnostics("envPath\\.QML2_IMPORT_PATH\\.1\\.exists: true" "validate the second QML2 import path")
    require_package_diagnostics("envPath\\.QML2_IMPORT_PATH\\.1\\.dir: true" "classify the second QML2 import path")
    require_package_diagnostics("env\\.QSG_RENDER_LOOP: " "report QSG_RENDER_LOOP")
    require_package_diagnostics("env\\.QSG_VISUALIZE: " "report QSG_VISUALIZE")
    require_package_diagnostics("env\\.QSG_RENDERER_DEBUG: " "report QSG_RENDERER_DEBUG")
    require_package_diagnostics("env\\.QSG_RHI_DEBUG_LAYER: " "report QSG_RHI_DEBUG_LAYER")
    require_package_diagnostics(
        "env\\.QSG_RHI_PREFER_SOFTWARE_RENDERER: " "report QSG_RHI_PREFER_SOFTWARE_RENDERER")
    if(NOT "${diagnostics_output}${diagnostics_error}" MATCHES "env\\.QT_OPENGL: ")
        message(FATAL_ERROR "Package diagnostics smoke did not report QT_OPENGL: ${diagnostics_output}${diagnostics_error}")
    endif()
    if(NOT "${diagnostics_output}${diagnostics_error}" MATCHES "env\\.QT_ANGLE_PLATFORM: ")
        message(FATAL_ERROR "Package diagnostics smoke did not report QT_ANGLE_PLATFORM: ${diagnostics_output}${diagnostics_error}")
    endif()
    require_package_diagnostics("env\\.QT_D3D_ADAPTER_INDEX: " "report QT_D3D_ADAPTER_INDEX")
    require_package_diagnostics("env\\.QT_D3D_DEBUG: " "report QT_D3D_DEBUG")
    require_package_diagnostics(
        "env\\.QT_WAYLAND_DISABLE_WINDOWDECORATION: " "report QT_WAYLAND_DISABLE_WINDOWDECORATION")
    require_package_diagnostics("env\\.QT_WAYLAND_SHELL_INTEGRATION: " "report QT_WAYLAND_SHELL_INTEGRATION")
    require_package_diagnostics("env\\.KWIN_DRM_DEVICES: " "report KWIN_DRM_DEVICES")
    require_package_diagnostics("envPath\\.KWIN_DRM_DEVICES\\.count: 2" "report KWin DRM path-list entries")
    require_package_diagnostics("envPath\\.KWIN_DRM_DEVICES\\.0\\.value: " "report the first KWin DRM path")
    require_package_diagnostics("envPath\\.KWIN_DRM_DEVICES\\.0\\.exists: true" "validate the first KWin DRM path")
    require_package_diagnostics("envPath\\.KWIN_DRM_DEVICES\\.0\\.file: true" "classify the first KWin DRM path")
    require_package_diagnostics(
        "envPath\\.KWIN_DRM_DEVICES\\.0\\.readable: true" "validate the first KWin DRM path readability")
    require_package_diagnostics("envPath\\.KWIN_DRM_DEVICES\\.0\\.size: [0-9]+" "report the first KWin DRM path size")
    require_package_diagnostics(
        "envPath\\.KWIN_DRM_DEVICES\\.0\\.lastModifiedUtc: "
        "report the first KWin DRM path modification time")
    require_package_diagnostics("envPath\\.KWIN_DRM_DEVICES\\.1\\.exists: true" "validate the second KWin DRM path")
    if(NOT "${diagnostics_output}${diagnostics_error}" MATCHES "env\\.GBM_BACKEND: ")
        message(FATAL_ERROR "Package diagnostics smoke did not report GBM_BACKEND: ${diagnostics_output}${diagnostics_error}")
    endif()
    if(NOT "${diagnostics_output}${diagnostics_error}" MATCHES "env\\.DRI_PRIME: ")
        message(FATAL_ERROR "Package diagnostics smoke did not report DRI_PRIME: ${diagnostics_output}${diagnostics_error}")
    endif()
    require_package_diagnostics("env\\.MESA_LOADER_DRIVER_OVERRIDE: " "report MESA_LOADER_DRIVER_OVERRIDE")
    require_package_diagnostics("env\\.LIBGL_ALWAYS_SOFTWARE: " "report LIBGL_ALWAYS_SOFTWARE")
    require_package_diagnostics("env\\.LIBGL_DEBUG: " "report LIBGL_DEBUG")
    require_package_diagnostics("env\\.LIBVA_DRIVER_NAME: " "report LIBVA_DRIVER_NAME")
    require_package_diagnostics("env\\.EGL_PLATFORM: " "report EGL_PLATFORM")
    require_package_diagnostics("env\\.__EGL_VENDOR_LIBRARY_FILENAMES: " "report EGL vendor library files")
    require_package_diagnostics(
        "envPath\\.__EGL_VENDOR_LIBRARY_FILENAMES\\.count: 2" "report EGL vendor path-list entries")
    require_package_diagnostics(
        "envPath\\.__EGL_VENDOR_LIBRARY_FILENAMES\\.0\\.value: " "report the first EGL vendor file")
    require_package_diagnostics(
        "envPath\\.__EGL_VENDOR_LIBRARY_FILENAMES\\.0\\.exists: true" "validate the first EGL vendor file")
    require_package_diagnostics(
        "envPath\\.__EGL_VENDOR_LIBRARY_FILENAMES\\.0\\.file: true" "classify the first EGL vendor file")
    require_package_diagnostics(
        "envPath\\.__EGL_VENDOR_LIBRARY_FILENAMES\\.0\\.readable: true"
        "validate the first EGL vendor file readability")
    require_package_diagnostics(
        "envPath\\.__EGL_VENDOR_LIBRARY_FILENAMES\\.0\\.size: [0-9]+"
        "report the first EGL vendor file size")
    require_package_diagnostics(
        "envPath\\.__EGL_VENDOR_LIBRARY_FILENAMES\\.0\\.lastModifiedUtc: "
        "report the first EGL vendor file modification time")
    require_package_diagnostics(
        "envPath\\.__EGL_VENDOR_LIBRARY_FILENAMES\\.1\\.exists: true" "validate the second EGL vendor file")
    if(NOT "${diagnostics_output}${diagnostics_error}" MATCHES "env\\.__GLX_VENDOR_LIBRARY_NAME: ")
        message(
            FATAL_ERROR
            "Package diagnostics smoke did not report GLX vendor selection: ${diagnostics_output}${diagnostics_error}")
    endif()
    if(NOT "${diagnostics_output}${diagnostics_error}" MATCHES "env\\.__NV_PRIME_RENDER_OFFLOAD: ")
        message(
            FATAL_ERROR
            "Package diagnostics smoke did not report NVIDIA PRIME offload env: ${diagnostics_output}${diagnostics_error}")
    endif()
    require_package_diagnostics(
        "env\\.__NV_PRIME_RENDER_OFFLOAD_PROVIDER: " "report NVIDIA PRIME offload provider")
    if(NOT "${diagnostics_output}${diagnostics_error}" MATCHES "env\\.__VK_LAYER_NV_optimus: ")
        message(
            FATAL_ERROR
            "Package diagnostics smoke did not report NVIDIA Vulkan Optimus env: ${diagnostics_output}${diagnostics_error}")
    endif()
    require_package_diagnostics("env\\.VK_INSTANCE_LAYERS: " "report Vulkan instance layers")
    require_package_diagnostics("env\\.VK_LOADER_DEBUG: " "report Vulkan loader debugging")
    if(NOT "${diagnostics_output}${diagnostics_error}" MATCHES "env\\.VK_ICD_FILENAMES: ")
        message(FATAL_ERROR "Package diagnostics smoke did not report VK_ICD_FILENAMES: ${diagnostics_output}${diagnostics_error}")
    endif()
    if(NOT "${diagnostics_output}${diagnostics_error}" MATCHES "env\\.VK_DRIVER_FILES: ")
        message(FATAL_ERROR "Package diagnostics smoke did not report VK_DRIVER_FILES: ${diagnostics_output}${diagnostics_error}")
    endif()
    require_package_diagnostics("envPath\\.VK_ICD_FILENAMES\\.count: 2" "report Vulkan ICD path-list entries")
    require_package_diagnostics("envPath\\.VK_ICD_FILENAMES\\.0\\.value: " "report the first Vulkan ICD path")
    require_package_diagnostics("envPath\\.VK_ICD_FILENAMES\\.0\\.exists: true" "validate the first Vulkan ICD path")
    require_package_diagnostics("envPath\\.VK_ICD_FILENAMES\\.0\\.file: true" "classify the first Vulkan ICD path")
    require_package_diagnostics(
        "envPath\\.VK_ICD_FILENAMES\\.0\\.readable: true" "validate the first Vulkan ICD path readability")
    require_package_diagnostics("envPath\\.VK_ICD_FILENAMES\\.0\\.size: [0-9]+" "report the first Vulkan ICD path size")
    require_package_diagnostics(
        "envPath\\.VK_ICD_FILENAMES\\.0\\.lastModifiedUtc: "
        "report the first Vulkan ICD path modification time")
    require_package_diagnostics("envPath\\.VK_ICD_FILENAMES\\.1\\.exists: true" "validate the second Vulkan ICD path")
    require_package_diagnostics("envPath\\.VK_DRIVER_FILES\\.count: 2" "report Vulkan driver path-list entries")
    require_package_diagnostics("envPath\\.VK_DRIVER_FILES\\.0\\.value: " "report the first Vulkan driver path")
    require_package_diagnostics("envPath\\.VK_DRIVER_FILES\\.0\\.exists: true" "validate the first Vulkan driver path")
    require_package_diagnostics("envPath\\.VK_DRIVER_FILES\\.0\\.file: true" "classify the first Vulkan driver path")
    require_package_diagnostics(
        "envPath\\.VK_DRIVER_FILES\\.0\\.readable: true" "validate the first Vulkan driver path readability")
    require_package_diagnostics("envPath\\.VK_DRIVER_FILES\\.0\\.size: [0-9]+" "report the first Vulkan driver path size")
    require_package_diagnostics(
        "envPath\\.VK_DRIVER_FILES\\.0\\.lastModifiedUtc: "
        "report the first Vulkan driver path modification time")
    require_package_diagnostics("envPath\\.VK_DRIVER_FILES\\.1\\.exists: true" "validate the second Vulkan driver path")
    if(NOT "${diagnostics_output}${diagnostics_error}" MATCHES "env\\.NVIDIA_VISIBLE_DEVICES: ")
        message(
            FATAL_ERROR
            "Package diagnostics smoke did not report NVIDIA_VISIBLE_DEVICES: ${diagnostics_output}${diagnostics_error}")
    endif()
    if(NOT "${diagnostics_output}${diagnostics_error}" MATCHES "env\\.CUDA_VISIBLE_DEVICES: ")
        message(FATAL_ERROR "Package diagnostics smoke did not report CUDA_VISIBLE_DEVICES: ${diagnostics_output}${diagnostics_error}")
    endif()
    if(NOT "${diagnostics_output}${diagnostics_error}" MATCHES "env\\.CUDA_DEVICE_ORDER: ")
        message(FATAL_ERROR "Package diagnostics smoke did not report CUDA_DEVICE_ORDER: ${diagnostics_output}${diagnostics_error}")
    endif()
    if(NOT "${diagnostics_output}${diagnostics_error}" MATCHES "env\\.NVIDIA_DRIVER_CAPABILITIES: ")
        message(
            FATAL_ERROR
            "Package diagnostics smoke did not report NVIDIA driver capabilities: ${diagnostics_output}${diagnostics_error}")
    endif()
    if(NOT "${diagnostics_output}${diagnostics_error}" MATCHES "katago\\.env\\.executable\\.value: ")
        message(
            FATAL_ERROR
            "Package diagnostics smoke did not report the KataGo executable env value: ${diagnostics_output}${diagnostics_error}")
    endif()
    require_package_diagnostics("katago\\.env\\.executable\\.hasText: true" "report executable env path text status")
    require_package_diagnostics("katago\\.env\\.executable\\.absolutePath: " "report the KataGo executable absolute path")
    if(NOT "${diagnostics_output}${diagnostics_error}" MATCHES "katago\\.env\\.executable\\.canonicalPath: ")
        message(
            FATAL_ERROR
            "Package diagnostics smoke did not report the KataGo executable canonical path: ${diagnostics_output}${diagnostics_error}")
    endif()
    require_package_diagnostics("katago\\.env\\.executable\\.exists: true" "validate the executable exists")
    require_package_diagnostics("katago\\.env\\.executable\\.file: true" "validate the executable is a file")
    require_package_diagnostics("katago\\.env\\.executable\\.readable: true" "validate the executable is readable")
    if(NOT "${diagnostics_output}${diagnostics_error}" MATCHES "katago\\.env\\.executable\\.executable: true")
        message(
            FATAL_ERROR
            "Package diagnostics smoke did not validate the executable path: ${diagnostics_output}${diagnostics_error}")
    endif()
    if(NOT "${diagnostics_output}${diagnostics_error}" MATCHES "katago\\.env\\.executable\\.size: [0-9]+")
        message(
            FATAL_ERROR
            "Package diagnostics smoke did not report the executable size: ${diagnostics_output}${diagnostics_error}")
    endif()
    if(NOT "${diagnostics_output}${diagnostics_error}" MATCHES "katago\\.env\\.executable\\.lastModifiedUtc: ")
        message(
            FATAL_ERROR
            "Package diagnostics smoke did not report the executable modification time: ${diagnostics_output}${diagnostics_error}")
    endif()
    if(NOT "${diagnostics_output}${diagnostics_error}" MATCHES "katago\\.env\\.model\\.exists: true")
        message(
            FATAL_ERROR
            "Package diagnostics smoke did not validate the model path: ${diagnostics_output}${diagnostics_error}")
    endif()
    require_package_diagnostics("katago\\.env\\.model\\.hasText: true" "report model env path text status")
    require_package_diagnostics("katago\\.env\\.model\\.absolutePath: " "report the model absolute path")
    if(NOT "${diagnostics_output}${diagnostics_error}" MATCHES "katago\\.env\\.model\\.canonicalPath: ")
        message(
            FATAL_ERROR
            "Package diagnostics smoke did not report the model canonical path: ${diagnostics_output}${diagnostics_error}")
    endif()
    require_package_diagnostics("katago\\.env\\.model\\.file: true" "validate the model is a file")
    require_package_diagnostics("katago\\.env\\.model\\.readable: true" "validate the model is readable")
    require_package_diagnostics("katago\\.env\\.model\\.writable: true" "validate the model is writable")
    if(NOT "${diagnostics_output}${diagnostics_error}" MATCHES "katago\\.env\\.model\\.size: [0-9]+")
        message(
            FATAL_ERROR
            "Package diagnostics smoke did not report the model size: ${diagnostics_output}${diagnostics_error}")
    endif()
    if(NOT "${diagnostics_output}${diagnostics_error}" MATCHES "katago\\.env\\.model\\.lastModifiedUtc: ")
        message(
            FATAL_ERROR
            "Package diagnostics smoke did not report the model modification time: ${diagnostics_output}${diagnostics_error}")
    endif()
    if(NOT "${diagnostics_output}${diagnostics_error}" MATCHES "katago\\.env\\.analysisConfig\\.exists: true")
        message(
            FATAL_ERROR
            "Package diagnostics smoke did not validate the analysis config path: ${diagnostics_output}${diagnostics_error}")
    endif()
    require_package_diagnostics(
        "katago\\.env\\.analysisConfig\\.hasText: true" "report analysis config env path text status")
    require_package_diagnostics(
        "katago\\.env\\.analysisConfig\\.absolutePath: " "report the analysis config absolute path")
    if(NOT "${diagnostics_output}${diagnostics_error}" MATCHES "katago\\.env\\.analysisConfig\\.canonicalPath: ")
        message(
            FATAL_ERROR
            "Package diagnostics smoke did not report the analysis config canonical path: ${diagnostics_output}${diagnostics_error}")
    endif()
    require_package_diagnostics("katago\\.env\\.analysisConfig\\.file: true" "validate the analysis config is a file")
    require_package_diagnostics(
        "katago\\.env\\.analysisConfig\\.readable: true" "validate the analysis config is readable")
    require_package_diagnostics(
        "katago\\.env\\.analysisConfig\\.writable: true" "validate the analysis config is writable")
    if(NOT "${diagnostics_output}${diagnostics_error}" MATCHES "katago\\.env\\.analysisConfig\\.size: [0-9]+")
        message(
            FATAL_ERROR
            "Package diagnostics smoke did not report the analysis config size: ${diagnostics_output}${diagnostics_error}")
    endif()
    if(NOT "${diagnostics_output}${diagnostics_error}" MATCHES "katago\\.env\\.analysisConfig\\.lastModifiedUtc: ")
        message(
            FATAL_ERROR
            "Package diagnostics smoke did not report the analysis config modification time: ${diagnostics_output}${diagnostics_error}")
    endif()
    if(NOT "${diagnostics_output}${diagnostics_error}" MATCHES "katago\\.env\\.gtpConfig\\.exists: ")
        message(
            FATAL_ERROR
            "Package diagnostics smoke did not report the KataGo GTP config env status: ${diagnostics_output}${diagnostics_error}")
    endif()
    if(NOT "${diagnostics_output}${diagnostics_error}" MATCHES "katago\\.env\\.gtpConfig\\.exists: true")
        message(
            FATAL_ERROR
            "Package diagnostics smoke did not validate the GTP config path: ${diagnostics_output}${diagnostics_error}")
    endif()
    require_package_diagnostics("katago\\.env\\.gtpConfig\\.hasText: true" "report GTP config env path text status")
    require_package_diagnostics("katago\\.env\\.gtpConfig\\.absolutePath: " "report the GTP config absolute path")
    if(NOT "${diagnostics_output}${diagnostics_error}" MATCHES "katago\\.env\\.gtpConfig\\.canonicalPath: ")
        message(
            FATAL_ERROR
            "Package diagnostics smoke did not report the GTP config canonical path: ${diagnostics_output}${diagnostics_error}")
    endif()
    require_package_diagnostics("katago\\.env\\.gtpConfig\\.file: true" "validate the GTP config is a file")
    require_package_diagnostics("katago\\.env\\.gtpConfig\\.readable: true" "validate the GTP config is readable")
    require_package_diagnostics("katago\\.env\\.gtpConfig\\.writable: true" "validate the GTP config is writable")
    if(NOT "${diagnostics_output}${diagnostics_error}" MATCHES "katago\\.env\\.gtpConfig\\.size: [0-9]+")
        message(
            FATAL_ERROR
            "Package diagnostics smoke did not report the GTP config size: ${diagnostics_output}${diagnostics_error}")
    endif()
    if(NOT "${diagnostics_output}${diagnostics_error}" MATCHES "katago\\.env\\.gtpConfig\\.lastModifiedUtc: ")
        message(
            FATAL_ERROR
            "Package diagnostics smoke did not report the GTP config modification time: ${diagnostics_output}${diagnostics_error}")
    endif()
    require_package_diagnostics("katago\\.env\\.complete: true" "summarize complete KataGo env configuration")
    require_package_diagnostics("katago\\.env\\.ready: true" "summarize ready KataGo env configuration")
    require_package_diagnostics("katago\\.env\\.status: ready" "report ready KataGo env status")
    require_package_diagnostics("katago\\.env\\.missing\\.count: 0" "report no missing KataGo env paths")
    require_package_diagnostics("katago\\.env\\.invalid\\.count: 0" "report no invalid KataGo env paths")
    require_package_diagnostics("plan\\.target\\.osFamily: " "report the target-platform OS family")
    require_package_diagnostics(
        "plan\\.target\\.inFirstReleaseScope: (true|false)"
        "summarize whether the OS is in the first-release scope")
    require_package_diagnostics(
        "plan\\.target\\.linuxKdeWayland\\.kdeSession: (true|false)"
        "summarize KDE session detection for Linux target acceptance")
    require_package_diagnostics(
        "plan\\.target\\.linuxKdeWayland\\.waylandSession: (true|false)"
        "summarize Wayland session detection for Linux target acceptance")
    require_package_diagnostics(
        "plan\\.target\\.linuxKdeWayland\\.qtPlatformWayland: (true|false)"
        "summarize Qt Wayland platform detection for Linux target acceptance")
    require_package_diagnostics(
        "plan\\.target\\.linuxKdeWayland\\.detected: (true|false)"
        "summarize Linux KDE Wayland target detection")
    require_package_diagnostics(
        "plan\\.target\\.windows\\.nativePlatformPlugin: (true|false)"
        "summarize Windows Qt platform plugin detection")
    require_package_diagnostics(
        "plan\\.target\\.windows\\.detected: (true|false)"
        "summarize Windows target detection")
    require_package_diagnostics(
        "plan\\.target\\.nvidiaEnvironmentHint\\.present: true"
        "summarize NVIDIA environment hints from graphics path-list fixtures")
    require_package_diagnostics(
        "plan\\.target\\.nvidiaEnvironmentHint\\.source\\.count: [1-9][0-9]*"
        "report NVIDIA environment hint source count")
    require_package_diagnostics(
        "plan\\.target\\.nvidiaEnvironmentHint\\.source\\.[0-9]+: (VK_ICD_FILENAMES|VK_DRIVER_FILES)"
        "report a Vulkan NVIDIA environment hint source")
    require_package_diagnostics(
        "plan\\.target\\.display\\.screenCount: [0-9]+"
        "summarize screen count for target display acceptance")
    require_package_diagnostics(
        "plan\\.target\\.display\\.multiScreen: (true|false)"
        "summarize multi-display target detection")
    require_package_diagnostics(
        "plan\\.target\\.display\\.primaryDevicePixelsAtLeast4K: (true|false)"
        "summarize 4K primary-screen detection")
    require_package_diagnostics(
        "plan\\.target\\.display\\.anyDevicePixelsAtLeast4K: (true|false)"
        "summarize 4K detection across all screens")
    require_package_diagnostics(
        "plan\\.target\\.display\\.primaryScaleAtLeast1_5: (true|false)"
        "summarize high-DPI primary-screen detection")
    require_package_diagnostics(
        "plan\\.target\\.display\\.anyScaleAtLeast1_5: (true|false)"
        "summarize high-DPI detection across all screens")
    require_package_diagnostics(
        "plan\\.target\\.display\\.primaryTargetScreenCandidate: (true|false)"
        "summarize same-screen primary target display detection")
    require_package_diagnostics(
        "plan\\.target\\.display\\.anyTargetScreenCandidate: (true|false)"
        "summarize same-screen target display detection across all screens")
    require_package_diagnostics(
        "plan\\.target\\.acceptance\\.linuxKdeWaylandNvidiaCandidate: (true|false)"
        "summarize Linux KDE Wayland NVIDIA target candidate detection")
    require_package_diagnostics(
        "plan\\.target\\.acceptance\\.linuxKdeWaylandNvidiaBlocker\\.count: [1-9][0-9]*"
        "report package Linux KDE Wayland NVIDIA target blocker count")
    require_package_diagnostics(
        "plan\\.target\\.acceptance\\.linuxKdeWaylandNvidiaBlocker\\.[0-9]+: qtWaylandPlatform"
        "report package offscreen Qt platform as a Linux KDE Wayland target blocker")
    require_package_diagnostics(
        "plan\\.target\\.acceptance\\.windowsNvidiaCandidate: (true|false)"
        "summarize Windows NVIDIA target candidate detection")
    require_package_diagnostics(
        "plan\\.target\\.acceptance\\.windowsNvidiaBlocker\\.count: [1-9][0-9]*"
        "report package Windows NVIDIA target blocker count")
    require_package_diagnostics(
        "plan\\.target\\.acceptance\\.firstReleaseNvidiaPlatformCandidate: (true|false)"
        "summarize first-release NVIDIA target candidate detection")
    require_package_diagnostics(
        "plan\\.target\\.acceptance\\.firstReleaseNvidiaPlatformBlocker\\.count: 2"
        "report package first-release target platform blocker count")
    require_package_diagnostics(
        "plan\\.target\\.acceptance\\.firstReleaseNvidiaPlatformBlocker\\.0: linuxKdeWaylandNvidia"
        "report package Linux target platform blocker")
    require_package_diagnostics(
        "plan\\.target\\.acceptance\\.firstReleaseNvidiaPlatformBlocker\\.1: windowsNvidia"
        "report package Windows target platform blocker")
    require_package_diagnostics(
        "plan\\.target\\.acceptance\\.display4KCandidate: (true|false)"
        "summarize 4K display candidate detection")
    require_package_diagnostics(
        "plan\\.target\\.acceptance\\.highDpiCandidate: (true|false)"
        "summarize high-DPI display candidate detection")
    require_package_diagnostics(
        "plan\\.target\\.acceptance\\.targetDisplayCandidate: (true|false)"
        "summarize combined target display candidate detection")
    require_package_diagnostics(
        "plan\\.target\\.acceptance\\.targetDisplayBlocker\\.count: [1-9][0-9]*"
        "report package target display blocker count")
    require_package_diagnostics(
        "plan\\.target\\.acceptance\\.targetDisplayBlocker\\.[0-9]+: display4K"
        "report package missing 4K display target blocker")
    require_package_diagnostics(
        "plan\\.target\\.acceptance\\.targetDisplayBlocker\\.[0-9]+: highDpiScale"
        "report package missing high-DPI display target blocker")
    require_package_diagnostics(
        "plan\\.target\\.acceptance\\.multiDisplayBlocker\\.0: multiDisplay"
        "report package missing multi-display target blocker")
    require_package_diagnostics(
        "plan\\.acceptance\\.linuxKdeWaylandNvidiaCandidate: (true|false)"
        "summarize Linux KDE Wayland NVIDIA PLAN acceptance candidate status")
    require_package_diagnostics(
        "plan\\.acceptance\\.windowsNvidiaCandidate: (true|false)"
        "summarize Windows NVIDIA PLAN acceptance candidate status")
    require_package_diagnostics(
        "plan\\.acceptance\\.targetPlatformCandidate: false"
        "summarize that offscreen package diagnostics still need a target machine")
    require_package_diagnostics(
        "plan\\.acceptance\\.targetPlatformBlocker\\.count: 2"
        "report package PLAN acceptance target-platform blocker count")
    require_package_diagnostics(
        "plan\\.acceptance\\.targetPlatformBlocker\\.0: linuxKdeWaylandNvidia"
        "report package PLAN acceptance Linux target-platform blocker")
    require_package_diagnostics(
        "plan\\.acceptance\\.targetPlatformBlocker\\.1: windowsNvidia"
        "report package PLAN acceptance Windows target-platform blocker")
    require_package_diagnostics(
        "plan\\.acceptance\\.realKataGoEnvReady: true"
        "summarize ready package KataGo env fixtures")
    require_package_diagnostics(
        "plan\\.acceptance\\.realKataGoEnvStatus: ready"
        "report package KataGo env readiness status")
    require_package_diagnostics(
        "plan\\.acceptance\\.realKataGoTargetRunCandidate: false"
        "avoid treating offscreen package fixtures as a target-machine real-KataGo run")
    require_package_diagnostics(
        "plan\\.acceptance\\.realKataGoManualVerificationCandidate: false"
        "avoid treating offscreen package fixtures as fully ready for real-KataGo manual verification")
    require_package_diagnostics(
        "plan\\.acceptance\\.realKataGoManualVerificationBlocker\\.0: targetPlatform"
        "report package target-platform blocker for offscreen real-KataGo manual verification")
    require_package_diagnostics(
        "plan\\.acceptance\\.savedMainConfigReady: true"
        "summarize ready saved main engine config for package PLAN acceptance")
    require_package_diagnostics(
        "plan\\.acceptance\\.savedMainGtpReady: true"
        "summarize ready saved main GTP config for package PLAN acceptance")
    require_package_diagnostics(
        "plan\\.acceptance\\.savedMainAnalysisReady: true"
        "summarize ready saved main analysis config for package PLAN acceptance")
    require_package_diagnostics(
        "plan\\.acceptance\\.savedMainConfigStatus: ready"
        "report package saved main engine config readiness status")
    require_package_diagnostics(
        "plan\\.acceptance\\.savedCompareConfigReady: true"
        "summarize ready saved compare engine config for package PLAN acceptance")
    require_package_diagnostics(
        "plan\\.acceptance\\.savedCompareGtpReady: true"
        "summarize ready saved compare GTP config for package PLAN acceptance")
    require_package_diagnostics(
        "plan\\.acceptance\\.savedCompareAnalysisReady: true"
        "summarize ready saved compare analysis config for package PLAN acceptance")
    require_package_diagnostics(
        "plan\\.acceptance\\.savedCompareConfigStatus: ready"
        "report package saved compare engine config readiness status")
    require_package_diagnostics(
        "plan\\.acceptance\\.envOrSavedMainConfigReady: true"
        "summarize ready package env-or-saved main engine config")
    require_package_diagnostics(
        "plan\\.acceptance\\.envOrSavedMainGtpReady: true"
        "summarize ready package env-or-saved main GTP config")
    require_package_diagnostics(
        "plan\\.acceptance\\.envOrSavedMainAnalysisReady: true"
        "summarize ready package env-or-saved main analysis config")
    require_package_diagnostics(
        "plan\\.acceptance\\.configuredMainConfigSource: env-and-saved"
        "report package env-and-saved main config source")
    require_package_diagnostics(
        "plan\\.acceptance\\.configuredMainGtpSource: env-and-saved"
        "report package env-and-saved main GTP source")
    require_package_diagnostics(
        "plan\\.acceptance\\.configuredMainAnalysisSource: env-and-saved"
        "report package env-and-saved main analysis source")
    require_package_diagnostics(
        "plan\\.acceptance\\.configuredCompareConfigSource: saved"
        "report package saved compare config source")
    require_package_diagnostics(
        "plan\\.acceptance\\.configuredCompareGtpSource: saved"
        "report package saved compare GTP source")
    require_package_diagnostics(
        "plan\\.acceptance\\.configuredCompareAnalysisSource: saved"
        "report package saved compare analysis source")
    require_package_diagnostics(
        "plan\\.acceptance\\.configuredDualEngineConfigReady: true"
        "summarize ready package configured dual-engine config")
    require_package_diagnostics(
        "plan\\.acceptance\\.configuredDualEngineGtpReady: true"
        "summarize ready package configured dual-engine GTP config")
    require_package_diagnostics(
        "plan\\.acceptance\\.configuredDualEngineAnalysisReady: true"
        "summarize ready package configured dual-engine analysis config")
    require_package_diagnostics(
        "plan\\.acceptance\\.configuredKataGoTargetRunCandidate: false"
        "avoid treating offscreen package saved config as a target-machine run")
    require_package_diagnostics(
        "plan\\.acceptance\\.configuredManualVerificationCandidate: false"
        "avoid treating offscreen package saved config as fully ready for manual verification")
    require_package_diagnostics(
        "plan\\.acceptance\\.configuredManualVerificationBlocker\\.0: targetPlatform"
        "report package target-platform blocker for offscreen configured manual verification")
    require_package_diagnostics(
        "plan\\.acceptance\\.configuredRealtimeGtpTargetRunCandidate: false"
        "avoid treating offscreen package realtime GTP config as a target-machine run")
    require_package_diagnostics(
        "plan\\.acceptance\\.configuredRealtimeGtpManualVerificationCandidate: false"
        "avoid treating offscreen package realtime GTP config as fully ready for manual verification")
    require_package_diagnostics(
        "plan\\.acceptance\\.configuredBatchAnalysisTargetRunCandidate: false"
        "avoid treating offscreen package batch analysis config as a target-machine run")
    require_package_diagnostics(
        "plan\\.acceptance\\.configuredBatchAnalysisManualVerificationCandidate: false"
        "avoid treating offscreen package batch analysis config as fully ready for manual verification")
    require_package_diagnostics(
        "plan\\.acceptance\\.configuredDualEngineTargetRunCandidate: false"
        "avoid treating offscreen package dual-engine config as a target-machine run")
    require_package_diagnostics(
        "plan\\.acceptance\\.configuredDualEngineManualVerificationCandidate: false"
        "avoid treating offscreen package dual-engine config as fully ready for manual verification")
    require_package_diagnostics(
        "plan\\.acceptance\\.configuredDualEngineManualVerificationBlocker\\.0: targetPlatform"
        "report package target-platform blocker for offscreen dual-engine manual verification")
    require_package_diagnostics(
        "plan\\.acceptance\\.configuredDualRealtimeGtpTargetRunCandidate: false"
        "avoid treating offscreen package dual-engine realtime GTP config as a target-machine run")
    require_package_diagnostics(
        "plan\\.acceptance\\.configuredDualRealtimeGtpManualVerificationCandidate: false"
        "avoid treating offscreen package dual-engine realtime GTP config as fully ready for manual verification")
    require_package_diagnostics(
        "plan\\.acceptance\\.configuredDualBatchAnalysisTargetRunCandidate: false"
        "avoid treating offscreen package dual-engine batch analysis config as a target-machine run")
    require_package_diagnostics(
        "plan\\.acceptance\\.configuredDualBatchAnalysisManualVerificationCandidate: false"
        "avoid treating offscreen package dual-engine batch analysis config as fully ready for manual verification")
    require_package_diagnostics(
        "plan\\.acceptance\\.configuredStatus: katago-config-ready-needs-target-machine"
        "summarize package offscreen configured-KataGo diagnostics acceptance status")
    require_package_diagnostics(
        "plan\\.acceptance\\.recordFile: \\(unset\\)"
        "report package unset target acceptance record file")
    require_package_diagnostics(
        "plan\\.acceptance\\.recordFile\\.set: false"
        "report package unset target acceptance record file set flag")
    require_package_diagnostics(
        "plan\\.acceptance\\.recordFile\\.exists: false"
        "report package unset target acceptance record file exists flag")
    require_package_diagnostics(
        "plan\\.acceptance\\.recordFile\\.readable: false"
        "report package unset target acceptance record file readable flag")
    require_package_diagnostics(
        "plan\\.acceptance\\.record\\.completedUtc: \\(unset\\)"
        "report package missing target acceptance completion time")
    require_package_diagnostics(
        "plan\\.acceptance\\.record\\.appVersion: \\(unset\\)"
        "report package missing target acceptance app version")
    require_package_diagnostics(
        "plan\\.acceptance\\.record\\.appVersionStatus: missing"
        "report package missing target acceptance app version status")
    require_package_diagnostics(
        "plan\\.acceptance\\.record\\.appExecutableSha256: \\(unset\\)"
        "report package missing target acceptance app executable SHA-256")
    require_package_diagnostics(
        "plan\\.acceptance\\.record\\.appExecutableSha256Status: missing"
        "report package missing target acceptance app executable SHA-256 status")
    require_package_diagnostics(
        "plan\\.acceptance\\.record\\.osPrettyName: \\(unset\\)"
        "report package missing target acceptance OS pretty name")
    require_package_diagnostics(
        "plan\\.acceptance\\.record\\.osPrettyNameStatus: missing"
        "report package missing target acceptance OS pretty-name status")
    require_package_diagnostics(
        "plan\\.acceptance\\.record\\.osKernelType: \\(unset\\)"
        "report package missing target acceptance OS kernel type")
    require_package_diagnostics(
        "plan\\.acceptance\\.record\\.osKernelTypeStatus: missing"
        "report package missing target acceptance OS kernel-type status")
    require_package_diagnostics(
        "plan\\.acceptance\\.record\\.osKernelVersion: \\(unset\\)"
        "report package missing target acceptance OS kernel version")
    require_package_diagnostics(
        "plan\\.acceptance\\.record\\.osKernelVersionStatus: missing"
        "report package missing target acceptance OS kernel-version status")
    require_package_diagnostics(
        "plan\\.acceptance\\.record\\.qtRuntimeVersion: \\(unset\\)"
        "report package missing target acceptance Qt runtime version")
    require_package_diagnostics(
        "plan\\.acceptance\\.record\\.qtRuntimeVersionStatus: missing"
        "report package missing target acceptance Qt runtime version status")
    require_package_diagnostics(
        "plan\\.acceptance\\.record\\.qtBuildAbi: \\(unset\\)"
        "report package missing target acceptance Qt build ABI")
    require_package_diagnostics(
        "plan\\.acceptance\\.record\\.qtBuildAbiStatus: missing"
        "report package missing target acceptance Qt build ABI status")
    require_package_diagnostics(
        "plan\\.acceptance\\.record\\.cpuArchitecture: \\(unset\\)"
        "report package missing target acceptance CPU architecture")
    require_package_diagnostics(
        "plan\\.acceptance\\.record\\.cpuArchitectureStatus: missing"
        "report package missing target acceptance CPU architecture status")
    require_package_diagnostics(
        "plan\\.acceptance\\.record\\.reviewer: \\(unset\\)"
        "report package missing target acceptance reviewer")
    require_package_diagnostics(
        "plan\\.acceptance\\.record\\.targetMachine: \\(unset\\)"
        "report package missing target acceptance machine")
    require_package_diagnostics(
        "plan\\.acceptance\\.record\\.targetMachineStatus: missing"
        "report package missing target acceptance machine status")
    require_package_diagnostics(
        "plan\\.acceptance\\.record\\.gpuDriver: \\(unset\\)"
        "report package missing target acceptance GPU driver")
    require_package_diagnostics(
        "plan\\.acceptance\\.record\\.gpuDriverStatus: missing"
        "report package missing target acceptance GPU driver status")
    require_package_diagnostics(
        "plan\\.acceptance\\.record\\.kataGoVersion: \\(unset\\)"
        "report package missing target acceptance KataGo version")
    require_package_diagnostics(
        "plan\\.acceptance\\.record\\.kataGoVersionStatus: missing"
        "report package missing target acceptance KataGo version status")
    require_package_diagnostics(
        "plan\\.acceptance\\.recordMetadata\\.ready: false"
        "report package incomplete target acceptance metadata by default")
    require_package_diagnostics(
        "plan\\.acceptance\\.recordMetadata\\.blocker\\.count: 13"
        "report package default target acceptance metadata blocker count")
    require_package_diagnostics(
        "plan\\.acceptance\\.recordMetadata\\.blocker\\.0: completedUtc"
        "report package missing target acceptance completion metadata blocker")
    require_package_diagnostics(
        "plan\\.acceptance\\.recordHint\\.passValues: pass; passed; accepted; true; yes"
        "report package accepted pass value hints")
    require_package_diagnostics(
        "plan\\.acceptance\\.recordHint\\.failValues: fail; failed; rejected; false; no"
        "report package accepted fail value hints")
    require_package_diagnostics(
        "plan\\.acceptance\\.recordHint\\.metadataKeysRequired: completedUtc; appVersion; appExecutableSha256; osPrettyName; osKernelType; osKernelVersion; qtRuntimeVersion; qtBuildAbi; cpuArchitecture; reviewer; targetMachine; gpuDriver; kataGoVersion"
        "report package required metadata key hints")
    require_package_diagnostics(
        "plan\\.acceptance\\.recordHint\\.metadataExactMatchKeys: appVersion; appExecutableSha256; osPrettyName; osKernelType; osKernelVersion; qtRuntimeVersion; qtBuildAbi; cpuArchitecture; targetMachine"
        "report package exact-match metadata key hints")
    require_package_diagnostics(
        "plan\\.acceptance\\.recordHint\\.metadataValueCheckedKeys: completedUtc; reviewer; gpuDriver; kataGoVersion"
        "report package value-checked metadata key hints")
    require_package_diagnostics(
        "plan\\.acceptance\\.recordHint\\.completedUtcRequired: ISO-UTC-not-future"
        "report package completedUtc requirement hint")
    require_package_diagnostics(
        "plan\\.acceptance\\.recordHint\\.aggregateAcceptanceKeysRequired: pass"
        "report package aggregate pass requirement hint")
    require_package_diagnostics(
        "plan\\.acceptance\\.recordHint\\.checklistItemsRequired: pass"
        "report package checklist pass requirement hint")
    require_package_diagnostics(
        "plan\\.acceptance\\.recordHint\\.evidencePathsRequired: readable-non-empty-distinct"
        "report package evidence path requirement hint")
    require_package_diagnostics(
        "plan\\.acceptance\\.recordHint\\.evidenceSha256Required: true"
        "report package evidence SHA-256 requirement hint")
    require_package_diagnostics(
        "plan\\.acceptance\\.recordHint\\.evidenceContentMarkersRequired: true"
        "report package evidence content marker requirement hint")
    require_package_diagnostics(
        "plan\\.acceptance\\.recordHint\\.recordAndEvidenceTimestampsMustNotAfterCompletedUtc: true"
        "report package timestamp requirement hint")
    require_package_diagnostics(
        "plan\\.acceptance\\.evidence\\.diagnostics\\.value: \\(unset\\)"
        "report package missing diagnostics evidence path")
    require_package_diagnostics(
        "plan\\.acceptance\\.evidence\\.diagnostics\\.hasText: false"
        "report package missing diagnostics evidence path text flag")
    require_package_diagnostics(
        "plan\\.acceptance\\.evidence\\.ready: false"
        "report package incomplete acceptance evidence readiness by default")
    require_package_diagnostics(
        "plan\\.acceptance\\.evidence\\.blocker\\.count: 14"
        "report package default acceptance evidence blocker count")
    require_package_diagnostics(
        "plan\\.acceptance\\.evidence\\.blocker\\.0: diagnosticsEvidence"
        "report package missing diagnostics evidence blocker")
    require_package_diagnostics(
        "plan\\.acceptance\\.evidence\\.blocker\\.10: engineLogGpuOrStderrEvidenceContent"
        "report package missing engine log GPU/stderr evidence content blocker")
    require_package_diagnostics(
        "plan\\.acceptance\\.linuxKdeWaylandNvidiaManualResult: manual-record-required"
        "report package missing Linux target manual result")
    require_package_diagnostics(
        "plan\\.acceptance\\.windowsNvidiaManualResult: manual-record-required"
        "report package missing Windows target manual result")
    require_package_diagnostics(
        "plan\\.acceptance\\.physicalDisplayManualResult: manual-record-required"
        "report package missing physical display manual result")
    require_package_diagnostics(
        "plan\\.acceptance\\.externalTargetVerificationManualResult: manual-record-required"
        "report package missing external target manual result")
    require_package_diagnostics(
        "plan\\.acceptance\\.checklistPassedRecord\\.count: 0"
        "report package no passed target checklist records by default")
    require_package_diagnostics(
        "plan\\.acceptance\\.checklistFailedRecord\\.count: 0"
        "report package no failed target checklist records by default")
    require_package_diagnostics(
        "plan\\.acceptance\\.checklistInvalidRecord\\.count: 0"
        "report package no invalid target checklist records by default")
    require_package_diagnostics(
        "plan\\.acceptance\\.manualFailedRecord\\.count: 0"
        "report package no failed aggregate manual records by default")
    require_package_diagnostics(
        "plan\\.acceptance\\.manualInvalidRecord\\.count: 0"
        "report package no invalid aggregate manual records by default")
    require_package_diagnostics(
        "plan\\.acceptance\\.display4KCandidate: (true|false)"
        "summarize package PLAN 4K display acceptance candidate status")
    require_package_diagnostics(
        "plan\\.acceptance\\.highDpiCandidate: (true|false)"
        "summarize package PLAN high-DPI acceptance candidate status")
    require_package_diagnostics(
        "plan\\.acceptance\\.targetDisplayCandidate: (true|false)"
        "summarize package PLAN combined target display acceptance candidate status")
    require_package_diagnostics(
        "plan\\.acceptance\\.sameScreenTargetDisplayCandidate: (true|false)"
        "summarize package same-screen 4K/high-DPI PLAN acceptance candidate status")
    require_package_diagnostics(
        "plan\\.acceptance\\.multiDisplayCandidate: (true|false)"
        "summarize package PLAN multi-display acceptance candidate status")
    require_package_diagnostics(
        "plan\\.acceptance\\.targetDisplayBlocker\\.[0-9]+: display4K"
        "report package PLAN acceptance missing 4K display blocker")
    require_package_diagnostics(
        "plan\\.acceptance\\.targetDisplayBlocker\\.[0-9]+: highDpiScale"
        "report package PLAN acceptance missing high-DPI display blocker")
    require_package_diagnostics(
        "plan\\.acceptance\\.multiDisplayBlocker\\.0: multiDisplay"
        "report package PLAN acceptance missing multi-display blocker")
    require_package_diagnostics(
        "plan\\.acceptance\\.manualUiInspectionRequired: true"
        "make package manual target UI inspection explicit")
    require_package_diagnostics(
        "plan\\.acceptance\\.manualUiInspectionStatus: manual-record-required"
        "report package manual UI inspection status")
    require_package_diagnostics(
        "plan\\.acceptance\\.manualUiInspectionChecklist\\.count: 12"
        "report package manual UI inspection checklist size")
    require_package_diagnostics(
        "plan\\.acceptance\\.manualUiInspectionChecklist\\.0: mainWindow4KHighDpiLayout"
        "report package main-window high-DPI layout inspection item")
    require_package_diagnostics(
        "plan\\.acceptance\\.manualUiInspectionChecklist\\.1: boardLinesCoordinatesAndStarPoints"
        "report package board line and coordinate inspection item")
    require_package_diagnostics(
        "plan\\.acceptance\\.manualUiInspectionChecklist\\.2: stoneRenderingAndCandidateLabels"
        "report package stone and candidate-label inspection item")
    require_package_diagnostics(
        "plan\\.acceptance\\.manualUiInspectionChecklist\\.3: candidateTableColumns"
        "report package candidate-table inspection item")
    require_package_diagnostics(
        "plan\\.acceptance\\.manualUiInspectionChecklist\\.4: pvPreviewStones"
        "report package PV preview inspection item")
    require_package_diagnostics(
        "plan\\.acceptance\\.manualUiInspectionChecklist\\.5: ownershipMainBoardOverlay"
        "report package main-board ownership inspection item")
    require_package_diagnostics(
        "plan\\.acceptance\\.manualUiInspectionChecklist\\.6: ownershipMiniBoardDock"
        "report package mini-board ownership inspection item")
    require_package_diagnostics(
        "plan\\.acceptance\\.manualUiInspectionChecklist\\.7: winrateScoreGraph"
        "report package graph inspection item")
    require_package_diagnostics(
        "plan\\.acceptance\\.manualUiInspectionChecklist\\.8: toolbarDockAndMenuLayout"
        "report package toolbar, dock, and menu layout inspection item")
    require_package_diagnostics(
        "plan\\.acceptance\\.manualUiInspectionChecklist\\.9: bilingualTextFit"
        "report package bilingual text-fit inspection item")
    require_package_diagnostics(
        "plan\\.acceptance\\.manualUiInspectionChecklist\\.10: savedWindowRestore"
        "report package saved-window restore inspection item")
    require_package_diagnostics(
        "plan\\.acceptance\\.manualUiInspectionChecklist\\.11: multiDisplayPlacement"
        "report package multi-display placement inspection item")
    require_package_diagnostics(
        "plan\\.acceptance\\.manualRawEngineComparisonRequired: true"
        "make package manual raw-engine comparison explicit")
    require_package_diagnostics(
        "plan\\.acceptance\\.rawKataGoComparisonStatus: manual-record-required"
        "report package raw KataGo comparison status")
    require_package_diagnostics(
        "plan\\.acceptance\\.rawKataGoComparisonChecklist\\.count: 10"
        "report package raw KataGo comparison checklist size")
    require_package_diagnostics(
        "plan\\.acceptance\\.rawKataGoComparisonChecklist\\.0: analysisJsonRawResponse"
        "report package Analysis JSON raw-response comparison item")
    require_package_diagnostics(
        "plan\\.acceptance\\.rawKataGoComparisonChecklist\\.1: realtimeGtpRawLine"
        "report package realtime GTP raw-line comparison item")
    require_package_diagnostics(
        "plan\\.acceptance\\.rawKataGoComparisonChecklist\\.2: candidateTableRendering"
        "report package candidate-table raw comparison item")
    require_package_diagnostics(
        "plan\\.acceptance\\.rawKataGoComparisonChecklist\\.3: pvPreview"
        "report package PV preview raw comparison item")
    require_package_diagnostics(
        "plan\\.acceptance\\.rawKataGoComparisonChecklist\\.4: winrateScorePerspective"
        "report package winrate and score perspective raw comparison item")
    require_package_diagnostics(
        "plan\\.acceptance\\.rawKataGoComparisonChecklist\\.5: ownershipDisplay"
        "report package ownership raw comparison item")
    require_package_diagnostics(
        "plan\\.acceptance\\.rawKataGoComparisonChecklist\\.6: genmove"
        "report package genmove raw comparison item")
    require_package_diagnostics(
        "plan\\.acceptance\\.rawKataGoComparisonChecklist\\.7: humanVsAi"
        "report package Human-vs-AI raw comparison item")
    require_package_diagnostics(
        "plan\\.acceptance\\.rawKataGoComparisonChecklist\\.8: selfPlay"
        "report package self-play raw comparison item")
    require_package_diagnostics(
        "plan\\.acceptance\\.rawKataGoComparisonChecklist\\.9: engineVsEngine"
        "report package engine-vs-engine raw comparison item")
    require_package_diagnostics(
        "plan\\.acceptance\\.externalTargetVerificationRequired: true"
        "make package external target verification explicit")
    require_package_diagnostics(
        "plan\\.acceptance\\.externalTargetVerificationStatus: manual-record-required"
        "report package external target verification status")
    require_package_diagnostics(
        "plan\\.acceptance\\.externalTargetVerificationChecklist\\.count: 4"
        "report package external target verification checklist size")
    require_package_diagnostics(
        "plan\\.acceptance\\.externalTargetVerificationChecklist\\.0: linuxKdeWaylandNvidiaRealtimeKataGo"
        "report package Linux KDE Wayland NVIDIA external verification item")
    require_package_diagnostics(
        "plan\\.acceptance\\.externalTargetVerificationChecklist\\.1: windowsNvidiaRealtimeKataGo"
        "report package Windows NVIDIA external verification item")
    require_package_diagnostics(
        "plan\\.acceptance\\.externalTargetVerificationChecklist\\.2: physical4KHighDpiMultiDisplayUi"
        "report package physical 4K/high-DPI/multi-display external verification item")
    require_package_diagnostics(
        "plan\\.acceptance\\.externalTargetVerificationChecklist\\.3: rawKataGoUiComparison"
        "report package raw KataGo UI comparison external verification item")
    require_package_diagnostics(
        "plan\\.acceptance\\.finalAcceptanceStatus: needs-readiness-and-final-manual-acceptance"
        "report package final acceptance status")
    require_package_diagnostics(
        "plan\\.acceptance\\.finalAcceptanceBlocker\\.count: 6"
        "report package final acceptance blocker count")
    require_package_diagnostics(
        "plan\\.acceptance\\.finalAcceptanceBlocker\\.0: targetPlatform"
        "report package final target-platform blocker")
    require_package_diagnostics(
        "plan\\.acceptance\\.finalAcceptanceBlocker\\.1: targetDisplay"
        "report package final target-display blocker")
    require_package_diagnostics(
        "plan\\.acceptance\\.finalAcceptanceBlocker\\.2: multiDisplay"
        "report package final multi-display blocker")
    require_package_diagnostics(
        "plan\\.acceptance\\.finalAcceptanceBlocker\\.3: externalTargetVerification"
        "report package final external target verification blocker")
    require_package_diagnostics(
        "plan\\.acceptance\\.finalAcceptanceBlocker\\.4: manualRawEngineComparison"
        "report package final raw-engine comparison blocker")
    require_package_diagnostics(
        "plan\\.acceptance\\.finalAcceptanceBlocker\\.5: manualUiInspection"
        "report package final manual UI inspection blocker")
    require_package_diagnostics(
        "plan\\.acceptance\\.finalAcceptanceOutstandingBlocker\\.count: 6"
        "report package final outstanding blocker count")
    require_package_diagnostics(
        "plan\\.acceptance\\.finalAcceptanceOutstandingBlocker\\.0: targetPlatform"
        "report package final outstanding target-platform blocker")
    require_package_diagnostics(
        "plan\\.acceptance\\.finalAcceptanceOutstandingBlocker\\.1: targetDisplay"
        "report package final outstanding target-display blocker")
    require_package_diagnostics(
        "plan\\.acceptance\\.finalAcceptanceOutstandingBlocker\\.2: multiDisplay"
        "report package final outstanding multi-display blocker")
    require_package_diagnostics(
        "plan\\.acceptance\\.finalAcceptanceOutstandingBlocker\\.3: externalTargetVerification"
        "report package final outstanding external target verification blocker")
    require_package_diagnostics(
        "plan\\.acceptance\\.finalAcceptanceOutstandingBlocker\\.4: manualRawEngineComparison"
        "report package final outstanding raw-engine comparison blocker")
    require_package_diagnostics(
        "plan\\.acceptance\\.finalAcceptanceOutstandingBlocker\\.5: manualUiInspection"
        "report package final outstanding manual UI inspection blocker")
    require_package_diagnostics(
        "plan\\.acceptance\\.linuxKdeWaylandNvidiaVerificationChecklist\\.count: 10"
        "report package Linux KDE Wayland NVIDIA verification checklist size")
    require_package_diagnostics(
        "plan\\.acceptance\\.linuxKdeWaylandNvidiaVerificationChecklist\\.0: linuxOs"
        "report package Linux OS verification item")
    require_package_diagnostics(
        "plan\\.acceptance\\.linuxKdeWaylandNvidiaVerificationChecklist\\.1: kdeSession"
        "report package KDE session verification item")
    require_package_diagnostics(
        "plan\\.acceptance\\.linuxKdeWaylandNvidiaVerificationChecklist\\.2: waylandSession"
        "report package Wayland session verification item")
    require_package_diagnostics(
        "plan\\.acceptance\\.linuxKdeWaylandNvidiaVerificationChecklist\\.3: qwaylandPlatformPlugin"
        "report package qwayland platform plugin verification item")
    require_package_diagnostics(
        "plan\\.acceptance\\.linuxKdeWaylandNvidiaVerificationChecklist\\.4: nvidiaEnvironmentHint"
        "report package Linux NVIDIA environment verification item")
    require_package_diagnostics(
        "plan\\.acceptance\\.linuxKdeWaylandNvidiaVerificationChecklist\\.5: packageStarts"
        "report package Linux package startup verification item")
    require_package_diagnostics(
        "plan\\.acceptance\\.linuxKdeWaylandNvidiaVerificationChecklist\\.6: configureKataGoPaths"
        "report package Linux KataGo path configuration verification item")
    require_package_diagnostics(
        "plan\\.acceptance\\.linuxKdeWaylandNvidiaVerificationChecklist\\.7: realtimeAnalyzeCurrentPosition"
        "report package Linux realtime analysis verification item")
    require_package_diagnostics(
        "plan\\.acceptance\\.linuxKdeWaylandNvidiaVerificationChecklist\\.8: branchResync"
        "report package Linux branch resync verification item")
    require_package_diagnostics(
        "plan\\.acceptance\\.linuxKdeWaylandNvidiaVerificationChecklist\\.9: gpuStderrCaptured"
        "report package Linux GPU stderr verification item")
    require_package_diagnostics(
        "plan\\.acceptance\\.linuxKdeWaylandNvidiaVerificationStatus: katago-gtp-config-ready-needs-linux-kde-wayland-nvidia"
        "report package Linux target verification readiness status")
    require_package_diagnostics(
        "plan\\.acceptance\\.linuxKdeWaylandNvidiaVerificationReadinessBlocker\\.count: 1"
        "report package Linux target verification readiness blocker count")
    require_package_diagnostics(
        "plan\\.acceptance\\.linuxKdeWaylandNvidiaVerificationReadinessBlocker\\.0: qtWaylandPlatform"
        "report package Linux target qwayland readiness blocker")
    require_package_diagnostics(
        "plan\\.acceptance\\.windowsNvidiaVerificationChecklist\\.count: 9"
        "report package Windows NVIDIA verification checklist size")
    require_package_diagnostics(
        "plan\\.acceptance\\.windowsNvidiaVerificationChecklist\\.0: windowsOs"
        "report package Windows OS verification item")
    require_package_diagnostics(
        "plan\\.acceptance\\.windowsNvidiaVerificationChecklist\\.1: qwindowsPlatformPlugin"
        "report package qwindows platform plugin verification item")
    require_package_diagnostics(
        "plan\\.acceptance\\.windowsNvidiaVerificationChecklist\\.2: packageExtracts"
        "report package Windows package extraction verification item")
    require_package_diagnostics(
        "plan\\.acceptance\\.windowsNvidiaVerificationChecklist\\.3: appLocalQtRuntime"
        "report package Windows app-local Qt runtime verification item")
    require_package_diagnostics(
        "plan\\.acceptance\\.windowsNvidiaVerificationChecklist\\.4: nvidiaEnvironmentHint"
        "report package Windows NVIDIA environment verification item")
    require_package_diagnostics(
        "plan\\.acceptance\\.windowsNvidiaVerificationChecklist\\.5: configureKataGoPaths"
        "report package Windows KataGo path configuration verification item")
    require_package_diagnostics(
        "plan\\.acceptance\\.windowsNvidiaVerificationChecklist\\.6: nativeSettingsPathDialog"
        "report package Windows native settings path-dialog verification item")
    require_package_diagnostics(
        "plan\\.acceptance\\.windowsNvidiaVerificationChecklist\\.7: realtimeAnalyzeCurrentPosition"
        "report package Windows realtime analysis verification item")
    require_package_diagnostics(
        "plan\\.acceptance\\.windowsNvidiaVerificationChecklist\\.8: engineDiagnosticsCaptured"
        "report package Windows engine diagnostics verification item")
    require_package_diagnostics(
        "plan\\.acceptance\\.windowsNvidiaVerificationStatus: katago-gtp-config-ready-needs-windows-nvidia"
        "report package Windows target verification readiness status")
    require_package_diagnostics(
        "plan\\.acceptance\\.windowsNvidiaVerificationReadinessBlocker\\.count: 2"
        "report package Windows target verification readiness blocker count")
    require_package_diagnostics(
        "plan\\.acceptance\\.windowsNvidiaVerificationReadinessBlocker\\.0: windowsOs"
        "report package Windows target OS readiness blocker")
    require_package_diagnostics(
        "plan\\.acceptance\\.windowsNvidiaVerificationReadinessBlocker\\.1: windowsPlatformPlugin"
        "report package Windows target platform plugin readiness blocker")
    require_package_diagnostics(
        "plan\\.acceptance\\.physicalDisplayVerificationChecklist\\.count: 9"
        "report package physical display verification checklist size")
    require_package_diagnostics(
        "plan\\.acceptance\\.physicalDisplayVerificationChecklist\\.0: physical4KPanel"
        "report package physical 4K panel verification item")
    require_package_diagnostics(
        "plan\\.acceptance\\.physicalDisplayVerificationChecklist\\.1: highDpiScale150Or200"
        "report package high-DPI scale verification item")
    require_package_diagnostics(
        "plan\\.acceptance\\.physicalDisplayVerificationChecklist\\.2: multiDisplayLayout"
        "report package multi-display layout verification item")
    require_package_diagnostics(
        "plan\\.acceptance\\.physicalDisplayVerificationChecklist\\.3: boardTextSharpness"
        "report package board text sharpness verification item")
    require_package_diagnostics(
        "plan\\.acceptance\\.physicalDisplayVerificationChecklist\\.4: candidateLabelsNoOverlap"
        "report package candidate label overlap verification item")
    require_package_diagnostics(
        "plan\\.acceptance\\.physicalDisplayVerificationChecklist\\.5: pvPreviewNoOverlap"
        "report package PV preview overlap verification item")
    require_package_diagnostics(
        "plan\\.acceptance\\.physicalDisplayVerificationChecklist\\.6: ownershipOverlayReadable"
        "report package ownership overlay readability verification item")
    require_package_diagnostics(
        "plan\\.acceptance\\.physicalDisplayVerificationChecklist\\.7: winrateScoreGraphReadable"
        "report package winrate and score graph readability verification item")
    require_package_diagnostics(
        "plan\\.acceptance\\.physicalDisplayVerificationChecklist\\.8: restoredWindowVisible"
        "report package restored window visibility verification item")
    require_package_diagnostics(
        "plan\\.acceptance\\.physicalDisplayVerificationStatus: needs-target-display-and-multi-display"
        "report package physical display verification readiness status")
    require_package_diagnostics(
        "plan\\.acceptance\\.physicalDisplayVerificationReadinessBlocker\\.count: 3"
        "report package physical display verification readiness blocker count")
    require_package_diagnostics(
        "plan\\.acceptance\\.physicalDisplayVerificationReadinessBlocker\\.0: display4K"
        "report package physical display 4K readiness blocker")
    require_package_diagnostics(
        "plan\\.acceptance\\.physicalDisplayVerificationReadinessBlocker\\.1: highDpiScale"
        "report package physical display high-DPI readiness blocker")
    require_package_diagnostics(
        "plan\\.acceptance\\.physicalDisplayVerificationReadinessBlocker\\.2: multiDisplay"
        "report package physical display multi-display readiness blocker")
    require_package_diagnostics(
        "plan\\.acceptance\\.status: katago-env-ready-needs-target-machine"
        "summarize package offscreen diagnostics acceptance status")

    function(extract_package_diagnostics_record_template_value source_var key output_var)
        string(REGEX MATCH "(^|\n)${key}=([^\n]*)" template_match "${${source_var}}")
        if(NOT template_match)
            message(FATAL_ERROR "Package diagnostics record template did not include ${key}: ${${source_var}}")
        endif()
        set("${output_var}" "${CMAKE_MATCH_2}" PARENT_SCOPE)
    endfunction()
    function(package_diagnostics_record_template_section source_var section_name output_var)
        set(section_marker "\n[${section_name}]\n")
        string(FIND "${${source_var}}" "${section_marker}" section_marker_index)
        if(section_marker_index EQUAL -1)
            message(FATAL_ERROR "Package diagnostics record template did not include [${section_name}]: ${${source_var}}")
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
    function(package_diagnostics_pass_checklist_section source_var section_name output_var)
        package_diagnostics_record_template_section("${source_var}" "${section_name}" section_content)
        set(section_output "\n[${section_name}]\n")
        string(REPLACE "\n" ";" section_lines "${section_content}")
        foreach(section_line IN LISTS section_lines)
            if(section_line MATCHES "^([^=]+)=")
                string(APPEND section_output "${CMAKE_MATCH_1}=pass\n")
            endif()
        endforeach()
        set("${output_var}" "${section_output}" PARENT_SCOPE)
    endfunction()
    function(package_diagnostics_all_pass_checklists source_var output_var)
        set(all_checklists "")
        foreach(section_name IN ITEMS
            checklist.linuxKdeWaylandNvidia
            checklist.windowsNvidia
            checklist.physicalDisplay
            checklist.rawKataGoComparison
            checklist.externalTargetVerification
            checklist.manualUiInspection)
            package_diagnostics_pass_checklist_section("${source_var}" "${section_name}" section_text)
            string(APPEND all_checklists "${section_text}")
        endforeach()
        set("${output_var}" "${all_checklists}" PARENT_SCOPE)
    endfunction()

    execute_process(
        COMMAND
            "${CMAKE_COMMAND}" -E env
            "QT_QPA_PLATFORM=missing-qpa-for-diagnostics-template-smoke"
            "${package_diagnostics_executable_path}" --target-acceptance-record-template
        WORKING_DIRECTORY "${PACKAGE_DIAGNOSTICS_SMOKE_DIR}"
        RESULT_VARIABLE package_diagnostics_record_template_result
        OUTPUT_VARIABLE package_diagnostics_record_template_output
        ERROR_VARIABLE package_diagnostics_record_template_error
    )
    set(
        package_diagnostics_record_template_combined
        "${package_diagnostics_record_template_output}${package_diagnostics_record_template_error}")
    if(NOT package_diagnostics_record_template_result STREQUAL "0")
        message(
            FATAL_ERROR
            "Package diagnostics record template smoke failed for '${package_executable_entry}': ${package_diagnostics_record_template_combined}")
    endif()
    extract_package_diagnostics_record_template_value(
        package_diagnostics_record_template_combined
        "osPrettyName"
        package_diagnostics_record_os_pretty_name)
    extract_package_diagnostics_record_template_value(
        package_diagnostics_record_template_combined
        "osKernelType"
        package_diagnostics_record_os_kernel_type)
    extract_package_diagnostics_record_template_value(
        package_diagnostics_record_template_combined
        "osKernelVersion"
        package_diagnostics_record_os_kernel_version)
    extract_package_diagnostics_record_template_value(
        package_diagnostics_record_template_combined
        "qtRuntimeVersion"
        package_diagnostics_record_qt_runtime_version)
    extract_package_diagnostics_record_template_value(
        package_diagnostics_record_template_combined
        "qtBuildAbi"
        package_diagnostics_record_qt_build_abi)
    extract_package_diagnostics_record_template_value(
        package_diagnostics_record_template_combined
        "cpuArchitecture"
        package_diagnostics_record_cpu_architecture)
    extract_package_diagnostics_record_template_value(
        package_diagnostics_record_template_combined
        "targetMachine"
        package_diagnostics_record_target_machine)
    package_diagnostics_all_pass_checklists(
        package_diagnostics_record_template_combined
        package_diagnostics_all_pass_checklists)

    set(
        package_diagnostics_diagnostics_evidence_content
        "LizzieYzy Qt Diagnostics\napp.version: ${package_expected_app_version}\napp.executable: ${package_diagnostics_executable_path}\nplan.acceptance.recordFile.canonicalPath: ${package_diagnostics_record_file}\nplan.acceptance.recordFile.sha256: fixture\nplan.acceptance.finalAcceptanceStatus: fixture\nplan.acceptance.finalAcceptanceBlocker.count: 0\nplan.acceptance.finalAcceptanceOutstandingBlocker.count: 0\n${package_diagnostics_record_file}\n")
    set(
        package_diagnostics_target_report_evidence_content
        "# Target Acceptance Report\napp.version: ${package_expected_app_version}\napp.executable: ${package_diagnostics_executable_path}\n`plan.acceptance.recordFile.canonicalPath`: ${package_diagnostics_record_file}\n`plan.acceptance.recordFile.sha256`: fixture\n`plan.acceptance.finalAcceptanceStatus`: fixture\n`plan.acceptance.finalAcceptanceBlocker`: fixture\n`plan.acceptance.finalAcceptanceOutstandingBlocker`: fixture\n${package_diagnostics_record_file}\n")
    set(
        package_diagnostics_engine_log_evidence_content
        "diagnostics KataGo engine log evidence fixture\nKataGo 1.15.3\nstderr: CUDA fake backend ready\n")
    set(
        package_diagnostics_raw_log_evidence_content
        "diagnostics raw KataGo evidence fixture\nKataGo 1.15.3\nkata-analyze raw line\n{\"rootInfo\":{\"winrate\":0.5,\"scoreMean\":0.0,\"ownership\":[0.1,-0.1]},\"moveInfos\":[{\"move\":\"D4\",\"visits\":10,\"winrate\":0.5,\"scoreMean\":0.0,\"scoreStdev\":4.2,\"policy\":0.12,\"pv\":[\"D4\",\"Q16\"],\"pvVisits\":[10,5],\"ownership\":[0.1,-0.1]}]}\n${package_raw_katago_comparison_checklist_content}")
    set(
        package_diagnostics_manual_ui_log_evidence_content
        "diagnostics manual UI inspection evidence fixture\nmainWindow4KHighDpiLayout\nboardLinesCoordinatesAndStarPoints\nstoneRenderingAndCandidateLabels\ncandidateTableColumns\npvPreviewStones\nownershipMainBoardOverlay\nownershipMiniBoardDock\nwinrateScoreGraph\ntoolbarDockAndMenuLayout\nbilingualTextFit\nsavedWindowRestore\nmultiDisplayPlacement\n")
    set(
        package_diagnostics_external_verification_log_evidence_content
        "diagnostics external verification evidence fixture\nlinuxKdeWaylandNvidiaRealtimeKataGo\nwindowsNvidiaRealtimeKataGo\nphysical4KHighDpiMultiDisplayUi\nrawKataGoUiComparison\n")
    file(WRITE "${package_diagnostics_screenshot_evidence}" "P2\n3840 2160\n1\n0 ")
    string(REPEAT "1 " 3839 package_diagnostics_screenshot_first_row_tail)
    file(APPEND "${package_diagnostics_screenshot_evidence}" "${package_diagnostics_screenshot_first_row_tail}\n")
    string(REPEAT "1 " 3840 package_diagnostics_screenshot_row)
    foreach(_package_diagnostics_screenshot_row_index RANGE 2 2160)
        file(APPEND "${package_diagnostics_screenshot_evidence}" "${package_diagnostics_screenshot_row}\n")
    endforeach()
    string(SHA256 package_diagnostics_diagnostics_evidence_sha256 "${package_diagnostics_diagnostics_evidence_content}")
    string(SHA256 package_diagnostics_target_report_evidence_sha256 "${package_diagnostics_target_report_evidence_content}")
    string(SHA256 package_diagnostics_engine_log_evidence_sha256 "${package_diagnostics_engine_log_evidence_content}")
    string(SHA256 package_diagnostics_raw_log_evidence_sha256 "${package_diagnostics_raw_log_evidence_content}")
    string(SHA256 package_diagnostics_manual_ui_log_evidence_sha256 "${package_diagnostics_manual_ui_log_evidence_content}")
    string(SHA256 package_diagnostics_external_verification_log_evidence_sha256 "${package_diagnostics_external_verification_log_evidence_content}")
    file(SHA256 "${package_diagnostics_screenshot_evidence}" package_diagnostics_screenshot_evidence_sha256)
    file(WRITE "${package_diagnostics_diagnostics_evidence}" "${package_diagnostics_diagnostics_evidence_content}")
    file(WRITE "${package_diagnostics_target_report_evidence}" "${package_diagnostics_target_report_evidence_content}")
    file(WRITE "${package_diagnostics_engine_log_evidence}" "${package_diagnostics_engine_log_evidence_content}")
    file(WRITE "${package_diagnostics_raw_log_evidence}" "${package_diagnostics_raw_log_evidence_content}")
    file(WRITE "${package_diagnostics_manual_ui_log_evidence}" "${package_diagnostics_manual_ui_log_evidence_content}")
    file(WRITE "${package_diagnostics_external_verification_log_evidence}" "${package_diagnostics_external_verification_log_evidence_content}")
    string(TIMESTAMP package_diagnostics_record_completed_utc "%Y-%m-%dT%H:%M:%SZ" UTC)
    file(WRITE "${package_diagnostics_record_file}" "
[metadata]
completedUtc=${package_diagnostics_record_completed_utc}
appVersion=${package_expected_app_version}
appExecutableSha256=${package_diagnostics_executable_sha256}
osPrettyName=${package_diagnostics_record_os_pretty_name}
osKernelType=${package_diagnostics_record_os_kernel_type}
osKernelVersion=${package_diagnostics_record_os_kernel_version}
qtRuntimeVersion=${package_diagnostics_record_qt_runtime_version}
qtBuildAbi=${package_diagnostics_record_qt_build_abi}
cpuArchitecture=${package_diagnostics_record_cpu_architecture}
reviewer=target tester
targetMachine=${package_diagnostics_record_target_machine}
gpuDriver=NVIDIA 555.55
kataGoVersion=KataGo 1.15.3
notes=all target evidence and checklist records attached to package diagnostics smoke

[evidence]
diagnostics=${package_diagnostics_diagnostics_evidence}
targetAcceptanceReport=${package_diagnostics_target_report_evidence}
engineLog=${package_diagnostics_engine_log_evidence}
rawKataGoComparisonLog=${package_diagnostics_raw_log_evidence}
manualUiInspectionLog=${package_diagnostics_manual_ui_log_evidence}
externalTargetVerificationLog=${package_diagnostics_external_verification_log_evidence}
screenshot=${package_diagnostics_screenshot_evidence}

[evidenceSha256]
diagnostics=${package_diagnostics_diagnostics_evidence_sha256}
targetAcceptanceReport=${package_diagnostics_target_report_evidence_sha256}
engineLog=${package_diagnostics_engine_log_evidence_sha256}
rawKataGoComparisonLog=${package_diagnostics_raw_log_evidence_sha256}
manualUiInspectionLog=${package_diagnostics_manual_ui_log_evidence_sha256}
externalTargetVerificationLog=${package_diagnostics_external_verification_log_evidence_sha256}
screenshot=${package_diagnostics_screenshot_evidence_sha256}

[evidenceContentMarkers]
diagnostics=LizzieYzy Qt Diagnostics; app.version; <record appVersion>; app.executable; <current app.executable>; plan.acceptance.recordFile.canonicalPath; <record canonical path>; plan.acceptance.recordFile.sha256; plan.acceptance.finalAcceptanceStatus; plan.acceptance.finalAcceptanceBlocker; plan.acceptance.finalAcceptanceOutstandingBlocker
targetAcceptanceReport=# Target Acceptance Report; app.version; <record appVersion>; app.executable; <current app.executable>; plan.acceptance.recordFile.canonicalPath; <record canonical path>; plan.acceptance.recordFile.sha256; plan.acceptance.finalAcceptanceStatus; plan.acceptance.finalAcceptanceBlocker; plan.acceptance.finalAcceptanceOutstandingBlocker
engineLog=KataGo; <record kataGoVersion>
engineLog.gpuOrStderrAny=CUDA; OpenCL; TensorRT; GPU; gpu; backend; Backend; stderr:; Stderr:
rawKataGoComparisonLog=KataGo; <record kataGoVersion>; kata-analyze; \"moveInfos\"; \"rootInfo\"; \"winrate\"; \"scoreMean\"; \"scoreStdev\"; \"visits\"; \"policy\"; \"pv\"; \"pvVisits\"; \"ownership\"; analysisJsonRawResponse; realtimeGtpRawLine; candidateTableRendering; pvPreview; winrateScorePerspective; ownershipDisplay; genmove; humanVsAi; selfPlay; engineVsEngine
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
${package_diagnostics_all_pass_checklists}")
    set(package_complete_record_diagnostics_environment ${diagnostics_environment})
    list(APPEND package_complete_record_diagnostics_environment
        "LIZZIE_TARGET_ACCEPTANCE_RECORD_FILE=${package_diagnostics_record_file}")
    execute_process(
        COMMAND
            "${CMAKE_COMMAND}" -E env
            ${package_complete_record_diagnostics_environment}
            "${package_diagnostics_executable_path}" --diagnostics
        WORKING_DIRECTORY "${PACKAGE_DIAGNOSTICS_SMOKE_DIR}"
        RESULT_VARIABLE complete_record_diagnostics_result
        OUTPUT_VARIABLE complete_record_diagnostics_output
        ERROR_VARIABLE complete_record_diagnostics_error
    )
    set(
        package_complete_record_diagnostics_combined
        "${complete_record_diagnostics_output}${complete_record_diagnostics_error}")
    if(NOT complete_record_diagnostics_result STREQUAL "0")
        message(
            FATAL_ERROR
            "Package complete-record diagnostics smoke failed for '${package_executable_entry}': ${package_complete_record_diagnostics_combined}")
    endif()
    require_diagnostics_output(
        package_complete_record_diagnostics_combined
        "plan\\.acceptance\\.recordFile\\.set: true"
        "report package complete target acceptance record file")
    require_diagnostics_output(
        package_complete_record_diagnostics_combined
        "plan\\.acceptance\\.recordMetadata\\.ready: true"
        "report complete target acceptance metadata readiness in diagnostics")
    require_diagnostics_output(
        package_complete_record_diagnostics_combined
        "plan\\.acceptance\\.evidence\\.ready: true"
        "report complete target acceptance evidence readiness in diagnostics")
    require_diagnostics_output(
        package_complete_record_diagnostics_combined
        "plan\\.acceptance\\.evidence\\.engineLog\\.gpuOrStderrContentStatus: match"
        "report complete diagnostics engine log GPU/stderr evidence status")
    require_diagnostics_output(
        package_complete_record_diagnostics_combined
        "plan\\.acceptance\\.evidence\\.engineLog\\.missingGpuOrStderrContentMarker\\.count: 0"
        "report no complete diagnostics engine log GPU/stderr missing markers")
    require_diagnostics_output(
        package_complete_record_diagnostics_combined
        "plan\\.acceptance\\.evidence\\.manualUiInspectionLog\\.exists: true"
        "report complete diagnostics manual UI inspection evidence exists")
    require_diagnostics_output(
        package_complete_record_diagnostics_combined
        "plan\\.acceptance\\.evidence\\.manualUiInspectionLog\\.contentStatus: match"
        "report complete diagnostics manual UI inspection evidence content status")
    require_diagnostics_output(
        package_complete_record_diagnostics_combined
        "plan\\.acceptance\\.evidence\\.manualUiInspectionLog\\.missingContentMarker\\.count: 0"
        "report no complete diagnostics manual UI inspection missing content markers")
    require_diagnostics_output(
        package_complete_record_diagnostics_combined
        "plan\\.acceptance\\.evidence\\.externalTargetVerificationLog\\.exists: true"
        "report complete diagnostics external target verification evidence exists")
    require_diagnostics_output(
        package_complete_record_diagnostics_combined
        "plan\\.acceptance\\.evidence\\.externalTargetVerificationLog\\.contentStatus: match"
        "report complete diagnostics external target verification evidence content status")
    require_diagnostics_output(
        package_complete_record_diagnostics_combined
        "plan\\.acceptance\\.evidence\\.externalTargetVerificationLog\\.missingContentMarker\\.count: 0"
        "report no complete diagnostics external target verification missing content markers")
    require_diagnostics_output(
        package_complete_record_diagnostics_combined
        "plan\\.acceptance\\.evidenceSha256\\.ready: true"
        "report complete target acceptance evidence SHA-256 readiness in diagnostics")
    require_diagnostics_output(
        package_complete_record_diagnostics_combined
        "plan\\.acceptance\\.recordTimestamp\\.ready: true"
        "report complete target acceptance record timestamp readiness in diagnostics")
    require_diagnostics_output(
        package_complete_record_diagnostics_combined
        "plan\\.acceptance\\.evidenceTimestamp\\.ready: true"
        "report complete target acceptance evidence timestamp readiness in diagnostics")
    require_diagnostics_output(
        package_complete_record_diagnostics_combined
        "plan\\.acceptance\\.evidence\\.screenshot\\.imageWidth: 3840"
        "report complete diagnostics screenshot image width")
    require_diagnostics_output(
        package_complete_record_diagnostics_combined
        "plan\\.acceptance\\.evidence\\.screenshot\\.imageHeight: 2160"
        "report complete diagnostics screenshot image height")
    require_diagnostics_output(
        package_complete_record_diagnostics_combined
        "plan\\.acceptance\\.evidence\\.screenshot\\.imagePixelsAtLeast4K: true"
        "report complete diagnostics screenshot 4K pixel envelope")
    require_diagnostics_output(
        package_complete_record_diagnostics_combined
        "plan\\.acceptance\\.evidence\\.screenshot\\.imageHasPixelVariation: true"
        "report complete diagnostics screenshot pixel variation")
    require_diagnostics_output(
        package_complete_record_diagnostics_combined
        "plan\\.acceptance\\.evidence\\.screenshot\\.readyFor4KAcceptance: true"
        "report complete diagnostics screenshot 4K readiness")
    require_diagnostics_output(
        package_complete_record_diagnostics_combined
        "plan\\.acceptance\\.externalTargetVerificationManualResult: pass"
        "derive complete diagnostics external target verification manual result")
    require_diagnostics_output(
        package_complete_record_diagnostics_combined
        "plan\\.acceptance\\.externalTargetVerificationStatus: pass"
        "derive complete diagnostics external target verification pass status")
    require_diagnostics_output(
        package_complete_record_diagnostics_combined
        "plan\\.acceptance\\.checklistMissingRecord\\.count: 0"
        "report no missing diagnostics checklist records for complete target acceptance records")
    require_diagnostics_output(
        package_complete_record_diagnostics_combined
        "plan\\.acceptance\\.manualFailedRecord\\.count: 0"
        "report no failed aggregate diagnostics manual records for complete target acceptance records")
    require_diagnostics_output(
        package_complete_record_diagnostics_combined
        "plan\\.acceptance\\.manualInvalidRecord\\.count: 0"
        "report no invalid aggregate diagnostics manual records for complete target acceptance records")
    require_diagnostics_output(
        package_complete_record_diagnostics_combined
        "plan\\.acceptance\\.checklist\\.ready: true"
        "report complete diagnostics checklist readiness")
    require_diagnostics_output(
        package_complete_record_diagnostics_combined
        "plan\\.acceptance\\.checklist\\.blocker\\.count: 0"
        "report no diagnostics checklist blockers")
    require_diagnostics_output(
        package_complete_record_diagnostics_combined
        "plan\\.acceptance\\.checklistPassedRecord\\.[0-9]+: manualUiInspection\\.multiDisplayPlacement"
        "report dynamically generated all-pass diagnostics manual UI checklist records")
    require_diagnostics_output(
        package_complete_record_diagnostics_combined
        "plan\\.acceptance\\.finalAcceptanceBlocker\\.count: 3"
        "report only target environment final blockers for complete diagnostics records")
    require_diagnostics_output(
        package_complete_record_diagnostics_combined
        "plan\\.acceptance\\.finalAcceptanceBlocker\\.0: targetPlatform"
        "keep target-platform final blocker for offscreen complete diagnostics records")
    require_diagnostics_output(
        package_complete_record_diagnostics_combined
        "plan\\.acceptance\\.finalAcceptanceBlocker\\.1: targetDisplay"
        "keep target-display final blocker for offscreen complete diagnostics records")
    require_diagnostics_output(
        package_complete_record_diagnostics_combined
        "plan\\.acceptance\\.finalAcceptanceBlocker\\.2: multiDisplay"
        "keep multi-display final blocker for offscreen complete diagnostics records")
    require_diagnostics_output(
        package_complete_record_diagnostics_combined
        "plan\\.acceptance\\.finalAcceptanceOutstandingBlocker\\.count: 3"
        "report complete-record diagnostics final outstanding blocker count")
    require_diagnostics_output(
        package_complete_record_diagnostics_combined
        "plan\\.acceptance\\.finalAcceptanceOutstandingBlocker\\.0: targetPlatform"
        "keep target-platform final outstanding blocker for offscreen complete diagnostics records")
    require_diagnostics_output(
        package_complete_record_diagnostics_combined
        "plan\\.acceptance\\.finalAcceptanceOutstandingBlocker\\.1: targetDisplay"
        "keep target-display final outstanding blocker for offscreen complete diagnostics records")
    require_diagnostics_output(
        package_complete_record_diagnostics_combined
        "plan\\.acceptance\\.finalAcceptanceOutstandingBlocker\\.2: multiDisplay"
        "keep multi-display final outstanding blocker for offscreen complete diagnostics records")
    if("${package_complete_record_diagnostics_combined}" MATCHES
       "plan\\.acceptance\\.finalAcceptanceBlocker\\.[0-9]+: (acceptanceChecklist|screenshotEvidence4K|externalTargetVerification|manualRawEngineComparison|manualUiInspection|acceptanceMetadata|acceptanceEvidence|acceptanceEvidenceSha256|acceptanceRecordTimestamp|acceptanceEvidenceTimestamp)")
        message(
            FATAL_ERROR
            "Package complete-record diagnostics smoke kept a non-environment final blocker: ${package_complete_record_diagnostics_combined}")
    endif()

    set(package_missing_diagnostics_environment
        ${package_diagnostics_qpa_environment}
        "LIZZIE_DIAGNOSTICS_SETTINGS_FILE=${package_diagnostics_settings_file}"
    )
    execute_process(
        COMMAND
            "${CMAKE_COMMAND}" -E env
            "--unset=LIZZIE_KATAGO_EXECUTABLE"
            "--unset=LIZZIE_KATAGO_MODEL"
            "--unset=LIZZIE_KATAGO_ANALYSIS_CONFIG"
            "--unset=LIZZIE_KATAGO_GTP_CONFIG"
            ${package_missing_diagnostics_environment}
            "${package_diagnostics_executable_path}" --diagnostics
        WORKING_DIRECTORY "${PACKAGE_DIAGNOSTICS_SMOKE_DIR}"
        RESULT_VARIABLE missing_diagnostics_result
        OUTPUT_VARIABLE missing_diagnostics_output
        ERROR_VARIABLE missing_diagnostics_error
    )
    set(package_missing_diagnostics_combined "${missing_diagnostics_output}${missing_diagnostics_error}")
    if(NOT missing_diagnostics_result STREQUAL "0")
        message(
            FATAL_ERROR
            "Package missing-env diagnostics smoke failed for '${package_executable_entry}': ${package_missing_diagnostics_combined}")
    endif()
    require_diagnostics_output(
        package_missing_diagnostics_combined
        "katago\\.env\\.complete: false"
        "summarize incomplete missing KataGo env configuration")
    require_diagnostics_output(
        package_missing_diagnostics_combined
        "katago\\.env\\.ready: false"
        "summarize unready missing KataGo env configuration")
    require_diagnostics_output(
        package_missing_diagnostics_combined
        "katago\\.env\\.status: missing"
        "report missing KataGo env status")
    require_diagnostics_output(
        package_missing_diagnostics_combined
        "katago\\.env\\.missing\\.count: 4"
        "report all missing KataGo env paths")
    require_diagnostics_output(
        package_missing_diagnostics_combined
        "katago\\.env\\.missing\\.0: executable"
        "name the missing KataGo executable")
    require_diagnostics_output(
        package_missing_diagnostics_combined
        "katago\\.env\\.missing\\.1: model"
        "name the missing KataGo model")
    require_diagnostics_output(
        package_missing_diagnostics_combined
        "katago\\.env\\.missing\\.2: analysisConfig"
        "name the missing KataGo analysis config")
    require_diagnostics_output(
        package_missing_diagnostics_combined
        "katago\\.env\\.missing\\.3: gtpConfig"
        "name the missing KataGo GTP config")
    require_diagnostics_output(
        package_missing_diagnostics_combined
        "katago\\.env\\.invalid\\.count: 0"
        "avoid invalid paths when KataGo env vars are missing")
    require_diagnostics_output(
        package_missing_diagnostics_combined
        "katago\\.env\\.executable\\.hasText: false"
        "report unset KataGo executable as lacking path text")

    set(package_invalid_diagnostics_executable_path "${PACKAGE_DIAGNOSTICS_SMOKE_DIR}/diagnostics-not-executable")
    set(package_invalid_diagnostics_missing_analysis_config_path
        "${PACKAGE_DIAGNOSTICS_SMOKE_DIR}/diagnostics-missing-analysis.cfg")
    set(package_invalid_diagnostics_gtp_config_path "${PACKAGE_DIAGNOSTICS_SMOKE_DIR}/diagnostics-gtp-config-dir")
    file(WRITE "${package_invalid_diagnostics_executable_path}" "not executable\n")
    file(
        CHMOD "${package_invalid_diagnostics_executable_path}"
        PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ)
    file(MAKE_DIRECTORY "${package_invalid_diagnostics_gtp_config_path}")
    set(
        package_invalid_diagnostics_environment
        ${package_diagnostics_qpa_environment}
        "LIZZIE_DIAGNOSTICS_SETTINGS_FILE=${package_diagnostics_settings_file}"
        "LIZZIE_KATAGO_EXECUTABLE=${package_invalid_diagnostics_executable_path}"
        "LIZZIE_KATAGO_MODEL=${package_diagnostics_model_path}"
        "LIZZIE_KATAGO_ANALYSIS_CONFIG=${package_invalid_diagnostics_missing_analysis_config_path}"
        "LIZZIE_KATAGO_GTP_CONFIG=${package_invalid_diagnostics_gtp_config_path}"
    )
    execute_process(
        COMMAND
            "${CMAKE_COMMAND}" -E env
            ${package_invalid_diagnostics_environment}
            "${package_diagnostics_executable_path}" --diagnostics
        WORKING_DIRECTORY "${PACKAGE_DIAGNOSTICS_SMOKE_DIR}"
        RESULT_VARIABLE invalid_diagnostics_result
        OUTPUT_VARIABLE invalid_diagnostics_output
        ERROR_VARIABLE invalid_diagnostics_error
    )
    set(package_invalid_diagnostics_combined "${invalid_diagnostics_output}${invalid_diagnostics_error}")
    if(NOT invalid_diagnostics_result STREQUAL "0")
        message(
            FATAL_ERROR
            "Package invalid-env diagnostics smoke failed for '${package_executable_entry}': ${package_invalid_diagnostics_combined}")
    endif()
    require_diagnostics_output(
        package_invalid_diagnostics_combined
        "katago\\.env\\.complete: true"
        "summarize complete invalid KataGo env configuration")
    require_diagnostics_output(
        package_invalid_diagnostics_combined
        "katago\\.env\\.ready: false"
        "summarize unready invalid KataGo env configuration")
    require_diagnostics_output(
        package_invalid_diagnostics_combined
        "katago\\.env\\.status: invalid"
        "report invalid KataGo env status")
    require_diagnostics_output(
        package_invalid_diagnostics_combined
        "katago\\.env\\.missing\\.count: 0"
        "avoid missing paths when all KataGo env vars are set")
    require_diagnostics_output(
        package_invalid_diagnostics_combined
        "katago\\.env\\.invalid\\.count: 3"
        "report invalid KataGo env paths")
    require_diagnostics_output(
        package_invalid_diagnostics_combined
        "katago\\.env\\.invalid\\.0: executable"
        "name the invalid KataGo executable")
    require_diagnostics_output(
        package_invalid_diagnostics_combined
        "katago\\.env\\.invalid\\.1: analysisConfig"
        "name the invalid KataGo analysis config")
    require_diagnostics_output(
        package_invalid_diagnostics_combined
        "katago\\.env\\.invalid\\.2: gtpConfig"
        "name the invalid KataGo GTP config")
    require_diagnostics_output(
        package_invalid_diagnostics_combined
        "katago\\.env\\.executable\\.hasText: true"
        "report invalid executable path text presence")
    require_diagnostics_output(
        package_invalid_diagnostics_combined
        "katago\\.env\\.model\\.hasText: true"
        "report valid model path text presence during invalid env smoke")
    require_diagnostics_output(
        package_invalid_diagnostics_combined
        "katago\\.env\\.executable\\.executable: false"
        "validate non-executable KataGo env path")
    require_diagnostics_output(
        package_invalid_diagnostics_combined
        "katago\\.env\\.analysisConfig\\.exists: false"
        "validate missing analysis-config env path")
    require_diagnostics_output(
        package_invalid_diagnostics_combined
        "katago\\.env\\.gtpConfig\\.file: false"
        "reject a directory GTP-config env path")

    set(package_blank_env_text "   ")
    set(
        package_blank_diagnostics_environment
        ${package_diagnostics_qpa_environment}
        "LIZZIE_DIAGNOSTICS_SETTINGS_FILE=${package_diagnostics_settings_file}"
        "LIZZIE_KATAGO_EXECUTABLE=${package_blank_env_text}"
        "LIZZIE_KATAGO_MODEL=${package_blank_env_text}"
        "LIZZIE_KATAGO_ANALYSIS_CONFIG=${package_blank_env_text}"
        "LIZZIE_KATAGO_GTP_CONFIG=${package_blank_env_text}"
    )
    execute_process(
        COMMAND
            "${CMAKE_COMMAND}" -E env
            ${package_blank_diagnostics_environment}
            "${package_diagnostics_executable_path}" --diagnostics
        WORKING_DIRECTORY "${PACKAGE_DIAGNOSTICS_SMOKE_DIR}"
        RESULT_VARIABLE blank_diagnostics_result
        OUTPUT_VARIABLE blank_diagnostics_output
        ERROR_VARIABLE blank_diagnostics_error
    )
    set(package_blank_diagnostics_combined "${blank_diagnostics_output}${blank_diagnostics_error}")
    if(NOT blank_diagnostics_result STREQUAL "0")
        message(
            FATAL_ERROR
            "Package blank-env diagnostics smoke failed for '${package_executable_entry}': ${package_blank_diagnostics_combined}")
    endif()
    require_diagnostics_output(
        package_blank_diagnostics_combined
        "katago\\.env\\.complete: false"
        "treat blank KataGo env paths as incomplete")
    require_diagnostics_output(
        package_blank_diagnostics_combined
        "katago\\.env\\.ready: false"
        "treat blank KataGo env paths as unready")
    require_diagnostics_output(
        package_blank_diagnostics_combined
        "katago\\.env\\.status: missing"
        "report blank KataGo env paths as missing")
    require_diagnostics_output(
        package_blank_diagnostics_combined
        "katago\\.env\\.missing\\.count: 4"
        "report all blank KataGo env paths as missing")
    require_diagnostics_output(
        package_blank_diagnostics_combined
        "katago\\.env\\.invalid\\.count: 0"
        "avoid invalid paths when KataGo env vars are blank")
    require_diagnostics_output(
        package_blank_diagnostics_combined
        "katago\\.env\\.executable\\.hasText: false"
        "report blank executable path text status")
    require_diagnostics_output(
        package_blank_diagnostics_combined
        "katago\\.env\\.model\\.hasText: false"
        "report blank model path text status")
    require_diagnostics_output(
        package_blank_diagnostics_combined
        "katago\\.env\\.analysisConfig\\.hasText: false"
        "report blank analysis config path text status")
    require_diagnostics_output(
        package_blank_diagnostics_combined
        "katago\\.env\\.gtpConfig\\.hasText: false"
        "report blank GTP config path text status")
    require_diagnostics_output(
        package_blank_diagnostics_combined
        "katago\\.env\\.executable\\.absolutePath: \\(blank\\)"
        "avoid resolving blank executable env paths")

    set(package_missing_settings_file "${PACKAGE_DIAGNOSTICS_SMOKE_DIR}/diagnostics-missing-settings.ini")
    set(
        package_missing_settings_environment
        ${package_diagnostics_qpa_environment}
        "LIZZIE_DIAGNOSTICS_SETTINGS_FILE=${package_missing_settings_file}"
        "LIZZIE_KATAGO_EXECUTABLE=${package_diagnostics_executable_path}"
        "LIZZIE_KATAGO_MODEL=${package_diagnostics_model_path}"
        "LIZZIE_KATAGO_ANALYSIS_CONFIG=${package_diagnostics_analysis_config_path}"
        "LIZZIE_KATAGO_GTP_CONFIG=${package_diagnostics_gtp_config_path}"
    )
    execute_process(
        COMMAND
            "${CMAKE_COMMAND}" -E env
            ${package_missing_settings_environment}
            "${package_diagnostics_executable_path}" --diagnostics
        WORKING_DIRECTORY "${PACKAGE_DIAGNOSTICS_SMOKE_DIR}"
        RESULT_VARIABLE missing_settings_diagnostics_result
        OUTPUT_VARIABLE missing_settings_diagnostics_output
        ERROR_VARIABLE missing_settings_diagnostics_error
    )
    set(
        package_missing_settings_diagnostics_combined
        "${missing_settings_diagnostics_output}${missing_settings_diagnostics_error}")
    if(NOT missing_settings_diagnostics_result STREQUAL "0")
        message(
            FATAL_ERROR
            "Package missing-settings diagnostics smoke failed for '${package_executable_entry}': ${package_missing_settings_diagnostics_combined}")
    endif()
    foreach(settings_prefix engine compare)
        require_diagnostics_output(
            package_missing_settings_diagnostics_combined
            "qt\\.settings\\.${settings_prefix}\\.config\\.complete: false"
            "report incomplete missing ${settings_prefix} settings config")
        require_diagnostics_output(
            package_missing_settings_diagnostics_combined
            "qt\\.settings\\.${settings_prefix}\\.config\\.ready: false"
            "report unready missing ${settings_prefix} settings config")
        require_diagnostics_output(
            package_missing_settings_diagnostics_combined
            "qt\\.settings\\.${settings_prefix}\\.config\\.gtpReady: false"
            "report unready missing ${settings_prefix} GTP settings config")
        require_diagnostics_output(
            package_missing_settings_diagnostics_combined
            "qt\\.settings\\.${settings_prefix}\\.config\\.analysisReady: false"
            "report unready missing ${settings_prefix} analysis settings config")
        require_diagnostics_output(
            package_missing_settings_diagnostics_combined
            "qt\\.settings\\.${settings_prefix}\\.config\\.status: missing"
            "report missing ${settings_prefix} settings config status")
        require_diagnostics_output(
            package_missing_settings_diagnostics_combined
            "qt\\.settings\\.${settings_prefix}\\.config\\.missing\\.count: 4"
            "report all missing ${settings_prefix} settings config paths")
        require_diagnostics_output(
            package_missing_settings_diagnostics_combined
            "qt\\.settings\\.${settings_prefix}\\.config\\.invalid\\.count: 0"
            "avoid invalid ${settings_prefix} settings paths when config is missing")
    endforeach()
    require_diagnostics_output(
        package_missing_settings_diagnostics_combined
        "plan\\.acceptance\\.configuredDualEngineConfigReady: false"
        "summarize missing package saved dual-engine config readiness")
    require_diagnostics_output(
        package_missing_settings_diagnostics_combined
        "plan\\.acceptance\\.configuredMainConfigSource: env"
        "report package missing-settings main config source from env")
    require_diagnostics_output(
        package_missing_settings_diagnostics_combined
        "plan\\.acceptance\\.configuredCompareConfigSource: none"
        "report package missing-settings compare config source as none")
    require_diagnostics_output(
        package_missing_settings_diagnostics_combined
        "plan\\.acceptance\\.configuredManualVerificationBlocker\\.0: targetPlatform"
        "report package target-platform blocker for missing-settings configured manual verification")
    require_diagnostics_output(
        package_missing_settings_diagnostics_combined
        "plan\\.acceptance\\.configuredDualEngineManualVerificationBlocker\\.1: compareKataGoConfig"
        "report package missing compare config manual-verification blocker")
    require_diagnostics_output(
        package_missing_settings_diagnostics_combined
        "plan\\.acceptance\\.configuredDualEngineTargetRunCandidate: false"
        "avoid missing package saved dual-engine target-run readiness")
    require_diagnostics_output(
        package_missing_settings_diagnostics_combined
        "plan\\.acceptance\\.configuredRealtimeGtpTargetRunCandidate: false"
        "avoid missing package saved realtime GTP target-run readiness")
    require_diagnostics_output(
        package_missing_settings_diagnostics_combined
        "plan\\.acceptance\\.configuredBatchAnalysisTargetRunCandidate: false"
        "avoid missing package saved batch-analysis target-run readiness")
    require_diagnostics_output(
        package_missing_settings_diagnostics_combined
        "plan\\.acceptance\\.configuredDualRealtimeGtpTargetRunCandidate: false"
        "avoid missing package saved dual-engine realtime GTP target-run readiness")
    require_diagnostics_output(
        package_missing_settings_diagnostics_combined
        "plan\\.acceptance\\.configuredDualBatchAnalysisTargetRunCandidate: false"
        "avoid missing package saved dual-engine batch-analysis target-run readiness")
    require_diagnostics_output(
        package_missing_settings_diagnostics_combined
        "plan\\.acceptance\\.finalAcceptanceBlocker\\.count: 8"
        "report missing package saved final acceptance blocker count")
    require_diagnostics_output(
        package_missing_settings_diagnostics_combined
        "plan\\.acceptance\\.finalAcceptanceBlocker\\.0: targetPlatform"
        "report missing package saved target-platform final blocker")
    require_diagnostics_output(
        package_missing_settings_diagnostics_combined
        "plan\\.acceptance\\.finalAcceptanceBlocker\\.1: compareRealtimeGtpConfig"
        "report missing package saved compare realtime GTP final blocker")
    require_diagnostics_output(
        package_missing_settings_diagnostics_combined
        "plan\\.acceptance\\.finalAcceptanceBlocker\\.2: compareBatchAnalysisConfig"
        "report missing package saved compare batch-analysis final blocker")
    require_diagnostics_output(
        package_missing_settings_diagnostics_combined
        "plan\\.acceptance\\.finalAcceptanceBlocker\\.3: targetDisplay"
        "report missing package saved target-display final blocker")
    require_diagnostics_output(
        package_missing_settings_diagnostics_combined
        "plan\\.acceptance\\.finalAcceptanceBlocker\\.4: multiDisplay"
        "report missing package saved multi-display final blocker")
    require_diagnostics_output(
        package_missing_settings_diagnostics_combined
        "plan\\.acceptance\\.finalAcceptanceBlocker\\.5: externalTargetVerification"
        "report missing package saved external target final blocker")
    require_diagnostics_output(
        package_missing_settings_diagnostics_combined
        "plan\\.acceptance\\.finalAcceptanceBlocker\\.6: manualRawEngineComparison"
        "report missing package saved raw comparison final blocker")
    require_diagnostics_output(
        package_missing_settings_diagnostics_combined
        "plan\\.acceptance\\.finalAcceptanceBlocker\\.7: manualUiInspection"
        "report missing package saved manual UI final blocker")

    set(package_invalid_settings_file "${PACKAGE_DIAGNOSTICS_SMOKE_DIR}/diagnostics-invalid-settings.ini")
    file(WRITE "${package_invalid_settings_file}" "
[engine]
executable=${package_invalid_diagnostics_executable_path}
model=${package_diagnostics_model_path}
gtpConfig=${package_invalid_diagnostics_gtp_config_path}
analysisConfig=${package_invalid_diagnostics_missing_analysis_config_path}

[compare]
executable=${package_diagnostics_executable_path}
model=${package_diagnostics_model_path}
gtpConfig=${package_invalid_diagnostics_missing_analysis_config_path}
analysisConfig=${package_diagnostics_analysis_config_path}
")
    set(
        package_invalid_settings_environment
        ${package_diagnostics_qpa_environment}
        "LIZZIE_DIAGNOSTICS_SETTINGS_FILE=${package_invalid_settings_file}"
        "LIZZIE_KATAGO_EXECUTABLE=${package_diagnostics_executable_path}"
        "LIZZIE_KATAGO_MODEL=${package_diagnostics_model_path}"
        "LIZZIE_KATAGO_ANALYSIS_CONFIG=${package_diagnostics_analysis_config_path}"
        "LIZZIE_KATAGO_GTP_CONFIG=${package_diagnostics_gtp_config_path}"
    )
    execute_process(
        COMMAND
            "${CMAKE_COMMAND}" -E env
            ${package_invalid_settings_environment}
            "${package_diagnostics_executable_path}" --diagnostics
        WORKING_DIRECTORY "${PACKAGE_DIAGNOSTICS_SMOKE_DIR}"
        RESULT_VARIABLE invalid_settings_diagnostics_result
        OUTPUT_VARIABLE invalid_settings_diagnostics_output
        ERROR_VARIABLE invalid_settings_diagnostics_error
    )
    set(
        package_invalid_settings_diagnostics_combined
        "${invalid_settings_diagnostics_output}${invalid_settings_diagnostics_error}")
    if(NOT invalid_settings_diagnostics_result STREQUAL "0")
        message(
            FATAL_ERROR
            "Package invalid-settings diagnostics smoke failed for '${package_executable_entry}': ${package_invalid_settings_diagnostics_combined}")
    endif()
    require_diagnostics_output(
        package_invalid_settings_diagnostics_combined
        "qt\\.settings\\.engine\\.config\\.complete: true"
        "report complete invalid main settings config")
    require_diagnostics_output(
        package_invalid_settings_diagnostics_combined
        "qt\\.settings\\.engine\\.config\\.ready: false"
        "report unready invalid main settings config")
    require_diagnostics_output(
        package_invalid_settings_diagnostics_combined
        "qt\\.settings\\.engine\\.config\\.gtpReady: false"
        "report invalid main GTP settings config")
    require_diagnostics_output(
        package_invalid_settings_diagnostics_combined
        "qt\\.settings\\.engine\\.config\\.analysisReady: false"
        "report invalid main analysis settings config")
    require_diagnostics_output(
        package_invalid_settings_diagnostics_combined
        "qt\\.settings\\.engine\\.config\\.status: invalid"
        "report invalid main settings status")
    require_diagnostics_output(
        package_invalid_settings_diagnostics_combined
        "qt\\.settings\\.engine\\.config\\.missing\\.count: 0"
        "avoid missing main settings paths when all are set")
    require_diagnostics_output(
        package_invalid_settings_diagnostics_combined
        "qt\\.settings\\.engine\\.config\\.invalid\\.count: 3"
        "report invalid main settings paths")
    require_diagnostics_output(
        package_invalid_settings_diagnostics_combined
        "qt\\.settings\\.engine\\.config\\.invalid\\.0: executable"
        "name invalid main executable")
    require_diagnostics_output(
        package_invalid_settings_diagnostics_combined
        "qt\\.settings\\.engine\\.config\\.invalid\\.1: gtpConfig"
        "name invalid main GTP config")
    require_diagnostics_output(
        package_invalid_settings_diagnostics_combined
        "qt\\.settings\\.engine\\.config\\.invalid\\.2: analysisConfig"
        "name invalid main analysis config")
    require_diagnostics_output(
        package_invalid_settings_diagnostics_combined
        "qt\\.settings\\.compare\\.config\\.complete: true"
        "report complete invalid compare settings config")
    require_diagnostics_output(
        package_invalid_settings_diagnostics_combined
        "qt\\.settings\\.compare\\.config\\.ready: false"
        "report unready invalid compare settings config")
    require_diagnostics_output(
        package_invalid_settings_diagnostics_combined
        "qt\\.settings\\.compare\\.config\\.gtpReady: false"
        "report invalid compare GTP settings config")
    require_diagnostics_output(
        package_invalid_settings_diagnostics_combined
        "qt\\.settings\\.compare\\.config\\.analysisReady: true"
        "keep compare analysis settings ready when only GTP config is invalid")
    require_diagnostics_output(
        package_invalid_settings_diagnostics_combined
        "qt\\.settings\\.compare\\.config\\.status: invalid"
        "report invalid compare settings status")
    require_diagnostics_output(
        package_invalid_settings_diagnostics_combined
        "qt\\.settings\\.compare\\.config\\.missing\\.count: 0"
        "avoid missing compare settings paths when all are set")
    require_diagnostics_output(
        package_invalid_settings_diagnostics_combined
        "qt\\.settings\\.compare\\.config\\.invalid\\.count: 1"
        "report one invalid compare settings path")
    require_diagnostics_output(
        package_invalid_settings_diagnostics_combined
        "qt\\.settings\\.compare\\.config\\.invalid\\.0: gtpConfig"
        "name invalid compare GTP config")
    require_diagnostics_output(
        package_invalid_settings_diagnostics_combined
        "plan\\.acceptance\\.configuredDualEngineConfigReady: false"
        "summarize invalid package saved dual-engine config readiness")
    require_diagnostics_output(
        package_invalid_settings_diagnostics_combined
        "plan\\.acceptance\\.configuredMainConfigSource: env"
        "report package invalid-settings main config source from env")
    require_diagnostics_output(
        package_invalid_settings_diagnostics_combined
        "plan\\.acceptance\\.configuredCompareConfigSource: none"
        "report package invalid-settings compare config source as none")
    require_diagnostics_output(
        package_invalid_settings_diagnostics_combined
        "plan\\.acceptance\\.configuredManualVerificationBlocker\\.0: targetPlatform"
        "report package target-platform blocker for invalid-settings configured manual verification")
    require_diagnostics_output(
        package_invalid_settings_diagnostics_combined
        "plan\\.acceptance\\.configuredDualEngineManualVerificationBlocker\\.1: compareKataGoConfig"
        "report package invalid compare config manual-verification blocker")
    require_diagnostics_output(
        package_invalid_settings_diagnostics_combined
        "plan\\.acceptance\\.configuredDualEngineTargetRunCandidate: false"
        "avoid invalid package saved dual-engine target-run readiness")
    require_diagnostics_output(
        package_invalid_settings_diagnostics_combined
        "plan\\.acceptance\\.configuredRealtimeGtpTargetRunCandidate: false"
        "avoid invalid package saved realtime GTP target-run readiness")
    require_diagnostics_output(
        package_invalid_settings_diagnostics_combined
        "plan\\.acceptance\\.configuredBatchAnalysisTargetRunCandidate: false"
        "avoid invalid package saved batch-analysis target-run readiness")
    require_diagnostics_output(
        package_invalid_settings_diagnostics_combined
        "plan\\.acceptance\\.configuredDualRealtimeGtpTargetRunCandidate: false"
        "avoid invalid package saved dual-engine realtime GTP target-run readiness")
    require_diagnostics_output(
        package_invalid_settings_diagnostics_combined
        "plan\\.acceptance\\.configuredDualBatchAnalysisTargetRunCandidate: false"
        "avoid invalid package saved dual-engine batch-analysis target-run readiness")
    require_diagnostics_output(
        package_invalid_settings_diagnostics_combined
        "plan\\.acceptance\\.finalAcceptanceBlocker\\.count: 7"
        "report invalid package saved final acceptance blocker count")
    require_diagnostics_output(
        package_invalid_settings_diagnostics_combined
        "plan\\.acceptance\\.finalAcceptanceBlocker\\.0: targetPlatform"
        "report invalid package saved target-platform final blocker")
    require_diagnostics_output(
        package_invalid_settings_diagnostics_combined
        "plan\\.acceptance\\.finalAcceptanceBlocker\\.1: compareRealtimeGtpConfig"
        "report invalid package saved compare realtime GTP final blocker")
    require_diagnostics_output(
        package_invalid_settings_diagnostics_combined
        "plan\\.acceptance\\.finalAcceptanceBlocker\\.2: targetDisplay"
        "report invalid package saved target-display final blocker")
    require_diagnostics_output(
        package_invalid_settings_diagnostics_combined
        "plan\\.acceptance\\.finalAcceptanceBlocker\\.3: multiDisplay"
        "report invalid package saved multi-display final blocker")
    require_diagnostics_output(
        package_invalid_settings_diagnostics_combined
        "plan\\.acceptance\\.finalAcceptanceBlocker\\.4: externalTargetVerification"
        "report invalid package saved external target final blocker")
    require_diagnostics_output(
        package_invalid_settings_diagnostics_combined
        "plan\\.acceptance\\.finalAcceptanceBlocker\\.5: manualRawEngineComparison"
        "report invalid package saved raw comparison final blocker")
    require_diagnostics_output(
        package_invalid_settings_diagnostics_combined
        "plan\\.acceptance\\.finalAcceptanceBlocker\\.6: manualUiInspection"
        "report invalid package saved manual UI final blocker")
    if(NOT "${diagnostics_output}${diagnostics_error}" MATCHES "screen\\.count: [0-9]+")
        message(FATAL_ERROR "Package diagnostics smoke did not report screen count: ${diagnostics_output}${diagnostics_error}")
    endif()
    if(NOT "${diagnostics_output}${diagnostics_error}" MATCHES "screen\\.primary\\.index: -?[0-9]+")
        message(FATAL_ERROR "Package diagnostics smoke did not report the primary screen index: ${diagnostics_output}${diagnostics_error}")
    endif()
    require_package_diagnostics("screen\\.primary\\.name: " "report the primary screen name")
    require_package_diagnostics("screen\\.primary\\.geometry: " "report primary screen geometry")
    require_package_diagnostics("screen\\.primary\\.availableGeometry: " "report primary screen available geometry")
    require_package_diagnostics("screen\\.primary\\.virtualGeometry: " "report primary screen virtual geometry")
    require_package_diagnostics(
        "screen\\.primary\\.availableVirtualGeometry: " "report primary screen available virtual geometry")
    require_package_diagnostics(
        "screen\\.primary\\.geometryDevicePixels: " "report primary screen geometry in device pixels")
    require_package_diagnostics(
        "screen\\.primary\\.availableGeometryDevicePixels: "
        "report primary screen available geometry in device pixels")
    require_package_diagnostics(
        "screen\\.primary\\.virtualGeometryDevicePixels: "
        "report primary screen virtual geometry in device pixels")
    require_package_diagnostics(
        "screen\\.primary\\.availableVirtualGeometryDevicePixels: "
        "report primary screen available virtual geometry in device pixels")
    require_package_diagnostics("screen\\.primary\\.orientation: " "report primary screen orientation")
    require_package_diagnostics("screen\\.primary\\.primaryOrientation: " "report primary screen primary orientation")
    require_package_diagnostics("screen\\.primary\\.nativeOrientation: " "report primary screen native orientation")
    require_package_diagnostics("screen\\.primary\\.devicePixelRatio: " "report primary screen device-pixel ratio")
    require_package_diagnostics("screen\\.primary\\.logicalDpi: " "report primary screen logical DPI")
    require_package_diagnostics("screen\\.primary\\.logicalDpiX: " "report primary screen horizontal logical DPI")
    require_package_diagnostics("screen\\.primary\\.logicalDpiY: " "report primary screen vertical logical DPI")
    require_package_diagnostics("screen\\.primary\\.physicalDpi: " "report primary screen physical DPI")
    require_package_diagnostics("screen\\.primary\\.physicalDpiX: " "report primary screen horizontal physical DPI")
    require_package_diagnostics("screen\\.primary\\.physicalDpiY: " "report primary screen vertical physical DPI")
    require_package_diagnostics("screen\\.primary\\.physicalSizeMm: " "report primary screen physical size")
    require_package_diagnostics("screen\\.primary\\.refreshRate: " "report primary screen refresh rate")
    require_package_diagnostics("screen\\.primary\\.depth: -?[0-9]+" "report primary screen color depth")
    require_package_diagnostics("screen\\.primary\\.manufacturer: " "report primary screen manufacturer")
    require_package_diagnostics("screen\\.primary\\.model: " "report primary screen model")
    require_package_diagnostics("screen\\.primary\\.serialNumber: " "report primary screen serial number")
    if(NOT "${diagnostics_output}${diagnostics_error}" MATCHES "screen\\.0\\.virtualGeometry: ")
        message(FATAL_ERROR "Package diagnostics smoke did not report screen virtual geometry: ${diagnostics_output}${diagnostics_error}")
    endif()
    require_package_diagnostics("screen\\.0\\.geometry: " "report screen geometry")
    require_package_diagnostics("screen\\.0\\.availableGeometry: " "report screen available geometry")
    require_package_diagnostics("screen\\.0\\.availableVirtualGeometry: " "report screen available virtual geometry")
    require_package_diagnostics("screen\\.0\\.geometryDevicePixels: " "report screen geometry in device pixels")
    require_package_diagnostics(
        "screen\\.0\\.availableGeometryDevicePixels: " "report screen available geometry in device pixels")
    require_package_diagnostics(
        "screen\\.0\\.virtualGeometryDevicePixels: " "report screen virtual geometry in device pixels")
    require_package_diagnostics(
        "screen\\.0\\.availableVirtualGeometryDevicePixels: " "report screen available virtual geometry in device pixels")
    require_package_diagnostics("screen\\.0\\.orientation: " "report screen orientation")
    require_package_diagnostics("screen\\.0\\.primaryOrientation: " "report screen primary orientation")
    require_package_diagnostics("screen\\.0\\.nativeOrientation: " "report screen native orientation")
    require_package_diagnostics("screen\\.0\\.devicePixelRatio: " "report screen device-pixel ratio")
    require_package_diagnostics("screen\\.0\\.logicalDpi: " "report screen logical DPI")
    require_package_diagnostics("screen\\.0\\.logicalDpiX: " "report screen horizontal logical DPI")
    require_package_diagnostics("screen\\.0\\.logicalDpiY: " "report screen vertical logical DPI")
    require_package_diagnostics("screen\\.0\\.physicalDpi: " "report screen physical DPI")
    require_package_diagnostics("screen\\.0\\.physicalDpiX: " "report screen horizontal physical DPI")
    require_package_diagnostics("screen\\.0\\.physicalDpiY: " "report screen vertical physical DPI")
    require_package_diagnostics("screen\\.0\\.physicalSizeMm: " "report screen physical size")
    require_package_diagnostics("screen\\.0\\.refreshRate: " "report screen refresh rate")
    require_package_diagnostics("screen\\.0\\.depth: [0-9]+" "report screen color depth")
    require_package_diagnostics("screen\\.0\\.manufacturer: " "report screen manufacturer")
    require_package_diagnostics("screen\\.0\\.model: " "report screen model")
    require_package_diagnostics("screen\\.0\\.serialNumber: " "report screen serial number")
    require_package_diagnostics("screen\\.0\\.virtualSibling\\.count: [0-9]+" "report screen virtual sibling count")
    require_package_diagnostics("screen\\.0\\.virtualSibling\\.0\\.index: -?[0-9]+" "report screen virtual sibling index")
    require_package_diagnostics("screen\\.0\\.virtualSibling\\.0\\.name: " "report screen virtual sibling name")
endif()

if(NOT PACKAGE_KEEP_VERIFY_DIRS)
    if(package_doc_smoke_dir_owned)
        file(REMOVE_RECURSE "${PACKAGE_DOC_SMOKE_DIR}")
    endif()
    if(PACKAGE_RUN_VERSION_SMOKE AND package_smoke_dir_owned)
        file(REMOVE_RECURSE "${PACKAGE_SMOKE_DIR}")
    endif()
    if(PACKAGE_RUN_TARGET_ACCEPTANCE_REPORT_SMOKE AND package_target_acceptance_report_smoke_dir_owned)
        file(REMOVE_RECURSE "${PACKAGE_TARGET_ACCEPTANCE_REPORT_SMOKE_DIR}")
    endif()
    if(PACKAGE_RUN_TARGET_ACCEPTANCE_RECORD_TEMPLATE_SMOKE AND package_target_acceptance_record_template_smoke_dir_owned)
        file(REMOVE_RECURSE "${PACKAGE_TARGET_ACCEPTANCE_RECORD_TEMPLATE_SMOKE_DIR}")
    endif()
    if(PACKAGE_RUN_DIAGNOSTICS_SMOKE AND package_diagnostics_smoke_dir_owned)
        file(REMOVE_RECURSE "${PACKAGE_DIAGNOSTICS_SMOKE_DIR}")
    endif()
endif()

message(STATUS "Verified package contents: ${package_path}")
