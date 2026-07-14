if(NOT DEFINED ROOT)
    message(FATAL_ERROR "ROOT is required")
endif()

if(NOT DEFINED TEST_BINARY_DIR)
    message(FATAL_ERROR "TEST_BINARY_DIR is required")
endif()

set(work_dir "${TEST_BINARY_DIR}/verify-package-script")
file(REMOVE_RECURSE "${work_dir}")

function(write_package_file path)
    get_filename_component(parent "${path}" DIRECTORY)
    file(MAKE_DIRECTORY "${parent}")
    if(ARGC GREATER 1)
        file(WRITE "${path}" "${ARGV1}")
    else()
        file(WRITE "${path}" "test")
    endif()
endfunction()

set(valid_readme
    "repository-root PLAN.md\nConfigure Engine\nGTP config, and analysis config\nplatform-specific package filenames\n0 or 2 through 9 handicap choices\nNo Engine Mode\ndocs/Migration.md\ndocs/PlanRequirementAudit.md\ndocs/Verification.md\ndocs/TargetAcceptanceReport.md\nLIZZIE_KATAGO_EXECUTABLE\nKataGo env readiness summary\n--diagnostics\n--target-acceptance-report\n--target-acceptance-record-template\ntemplate command runs before Qt platform initialization\n[recordHints]\nplan.acceptance.recordHint.*\nrecordHint.metadataKeysRequired\n--target-acceptance-record\n--target-acceptance-record -- <ini>\nrecord-file\ncanonical path, size, SHA-256, and modification time\n4K pixel envelope\nscreenshotEvidence4K\nacceptanceChecklist\nchecklistMissingRecord\nacceptanceEvidenceSha256\nappVersion\nappExecutableSha256\nosPrettyName\nqtRuntimeVersion\nacceptanceEvidenceTimestamp\ncommon config file extensions\napp directory\napp-local Qt runtime artifact diagnostics\nQt library paths\nQt plugin path status\nPath-list environment diagnostics\nQStandardPaths writable locations\napplication font/text metrics\nlocale/UI-language diagnostics\nruntime appearance style/palette diagnostics\nnative file dialog/settings path selector diagnostics\nmain-window UI structure diagnostics\nstored and normalized appearance settings\nfirst-run completion diagnostics\nsaved engine setting path diagnostics\nstored engine path-readiness diagnostics\nstored analysis option diagnostics\nstored board display diagnostics\nstored shortcut diagnostics\nQSettings storage location\nQSettings session/window restore keys\nsaved session SGF path-status\nsaved window-geometry visibility checks\ncurrent platform plugin candidates\ncommon Wayland platform plugin candidates\ncommon Windows platform plugin candidates\navailable platform plugin listing\ntarget-platform summary diagnostics\ntarget-platform blocker diagnostics\ndisplay blocker diagnostics\nsame-screen target display summaries\nPLAN acceptance candidate flags\nPLAN acceptance summary flags\nenv-or-saved main config\nconfigured source diagnostics\nconfigured acceptance status\nmanual verification candidate flags\nmanual verification blocker diagnostics\nmode-specific final acceptance blocker diagnostics\nmanual UI inspection checklist diagnostics\nraw KataGo comparison checklist diagnostics\nexternal target verification checklist\ntarget-specific verification checklist diagnostics\ntarget-specific verification status diagnostics\nrealtime GTP and batch-analysis acceptance candidate flags\ndual-engine acceptance candidate flags\nscene graph\nQML import paths\nrenderer-interface\nOpenGL runtime context diagnostics\nnative `windows` QPA plugin\nQt build ABI\nQt build/runtime version match\nQt install paths\nVulkan\nextended graphics environment diagnostics\n__EGL_VENDOR_LIBRARY_FILENAMES\nscreen orientation\ndevice-pixel geometry\nper-axis DPI\nsize, and modification time\nout-of-scope artifacts\nnon-KataGo engine artifacts\nmacOS artifacts\nWindows runtime artifacts in Linux packages\nLinux runtime\nartifacts in Windows packages\n")
string(APPEND valid_readme "stored file behavior diagnostics\n")
set(valid_plan "LizzieYzy Qt6/C++ plan\nAcceptance Criteria\nKDE Wayland\nGTP config 和 analysis config\n")
set(valid_plan_coverage [=[
repository-root PLAN.md
Automated Evidence
External Acceptance Still Required
PlanRequirementAudit.md
Verification.md
PLAN.md-triggered CI
Windows native `windows` QPA
Qt build ABI
Qt build/runtime version match
Qt install-path diagnostics
installed diagnostics smoke
installed target acceptance report smoke
installed target acceptance record template smoke
target acceptance report CLI smoke
packaged target acceptance record template smoke
PACKAGE_RUN_TARGET_ACCEPTANCE_RECORD_TEMPLATE_SMOKE
--target-acceptance-record-template
--target-acceptance-record
ready/missing/invalid KataGo env
KataGo env readiness summary
missing, blank, and invalid path coverage
all-unset KataGo integration skip path-status diagnostics
analysis-config and
GTP-config diagnostics
strict realtime GTP visit-count parsing
strict realtime GTP PV/PV-visit validation and length consistency
finite realtime GTP numeric parsing
per-field realtime rootInfo fallback
invalid-board realtime success-update rejection
realtime candidate ownership size filtering
finite realtime direct-update ownership filtering
finite SGF/Analysis/GTP komi handling
trimmed SGF numeric metadata parsing with positive `HA` and nonnegative `MN` filtering
invalid setup-position diagnostics across core, GTP sync, and batch analysis
invalid in-memory SGF property-name filtering
legacy `Lizzie.*` global-state markers
LayerBoundaryCheck.cmake
lizzie_layer_boundary_check
core/engine/app layer boundary violations
executable-entry separation from the app library
non-engine QProcess/QThread dependency leaks
app/UI engine-private process header leaks
engine public QThread API leaks
FirstReleaseScopeCheck.cmake
lizzie_first_release_scope_check
first-release out-of-scope implementation markers
finite legacy Java analysis option import
bounded legacy Java analysis option import
strict legacy Java boolean option import
rootInfo ownership fallback
top-level realtime ownership precedence
movesOwnership realtime candidate precedence
visits-sorted sidecar candidate reload/export
sidecar scoreLead/prior alias import
sidecar invalid root field fallback
sidecar success/error exclusivity
sidecar top-level nodes diagnostics
sidecar malformed node-entry diagnostics
strict sidecar nodeId diagnostics
strict sidecar visit-count parsing
finite sidecar numeric parsing
strict sidecar ownership array validation
trimmed numeric-string sidecar scalar/ownership parsing
finite sidecar/SGF/readable analysis export
nonnegative sidecar/SGF visit export
visits-sorted legacy analysis import
finite legacy/generated analysis numeric parsing
normalized readable analysis rate export
legacy/generated visit-count overflow rejection
strict legacy/generated PV validation
strict legacy/generated PV-visit validation
strict sidecar/Analysis JSON PV validation
strict sidecar/Analysis JSON PV-visit validation
PV/PV-visit length consistency
legacy candidate ownership alias import
strict legacy candidate ownership validation
generated-comment root/candidate ownership import/write-back
strict generated-comment ownership validation
persisted analysis import ownership filtering
sidecar/SGF ownership write-back size filtering
numeric-string Analysis JSON parsing
newline-safe GTP command serialization
positive GTP command id serialization/reservation
threaded GTP command-id alignment
engine-level outbound GTP command-name validation
strict GTP response id parsing
numeric GTP response payload preservation
invalid GTP board-size coordinate rejection
Analysis JSON request point filtering
Analysis JSON request
board-size fallback
visits-sorted Analysis JSON candidates
per-field Analysis JSON root fallback
zero Analysis JSON root visit fallback/rejection
complete Analysis JSON ownership filtering
strict Analysis JSON ownership array validation
strict Analysis JSON response id parsing
strict Analysis JSON visit-count parsing
isolated Analysis JSON error payload parsing
bounded realtime `kata-analyze` interval serialization
finite Analysis JSON request option serialization
finite realtime search-parameter serialization
positive Analysis JSON/realtime search-limit serialization
empty Analysis JSON success response rejection
unknown/duplicate Analysis JSON response ignoring
invalid-board Analysis JSON success-response rejection
UI-level unknown/duplicate Analysis JSON response diagnostics
incomplete ownership filtering
finite applied-response ownership filtering
finite applied-response scalar/ownership filtering
visits-sorted applied-response candidates
with root fallback
platform-specific package filename verification
common config file extensions
localized main/compare clean-exit diagnostics
zero-value unlimited/off semantics
available platform plugin listing
path-list environment diagnostics
extended graphics environment diagnostics
OpenGL runtime context diagnostics
__EGL_VENDOR_LIBRARY_FILENAMES
QML import paths
device-pixel geometry
primary screen geometry
per-axis DPI
single-token command-name validation
balanced-quote validation
invalid raw GTP console diagnostics
out-of-scope artifacts
non-KataGo engine artifacts
macOS artifacts
Windows runtime artifacts in Linux packages
Linux runtime artifacts in Windows packages
app/Qt filesystem path size/mtime
app-local Qt runtime artifact diagnostics
QStandardPaths writable locations
application font/text metrics
locale/UI-language diagnostics
runtime appearance style/palette diagnostics
native file dialog/settings path selector diagnostics
main-window UI structure diagnostics
main-window 4K/high-DPI layout geometry smoke
stored and normalized appearance settings
finite stored/saved UI settings normalization
strict stored boolean settings parsing
first-run completion diagnostics
saved engine setting path diagnostics
stored extra-args parsed diagnostics
stored engine minimum-readiness diagnostics
stored engine path-readiness diagnostics
missing/invalid saved settings coverage
stored analysis option diagnostics
stored board display diagnostics
stored file behavior diagnostics
stored shortcut diagnostics
QSettings storage location
QSettings session/window restore keys
saved session SGF path-status
saved window-geometry visibility checks
saved window-state restore checks
invalid SGF collection-index restore diagnostics
shared window placement helper
standard handicap option gating
core/editor duplicate/conflicting setup, comment, and markup canonicalization
always-reported common Wayland
always-reported common Windows
target-platform summary diagnostics
target-platform blocker diagnostics
display blocker diagnostics
same-screen target-display candidate diagnostics
acceptance candidate flags
PLAN acceptance summary diagnostics
env-or-saved main config
configured source diagnostics
configured acceptance status
manual
verification candidate flags
manual verification blocker diagnostics
mode-specific final acceptance blocker diagnostics
final-section outstanding blocker diagnostics
manual UI inspection checklist diagnostics
raw KataGo comparison checklist diagnostics
external target verification checklist
target-specific verification checklist diagnostics
target-specific verification status diagnostics
target-machine acceptance report full evidence/SHA-256/checklist template coverage with evidence-content-marker guidance, record-hint metadata requirement manifest, and record-hint guidance
installed strict CLI and target acceptance record argument validation smoke
target acceptance report manual-record, record-file path/size/mtime/SHA-256 diagnostics, checklist-item, checklist missing-record diagnostics, checklist readiness/final `acceptanceChecklist` blocker enforcement, UTC metadata-readiness with `completedUtcStatus` future-timestamp rejection, record-file timestamp diagnostics/final `acceptanceRecordTimestamp` blocker enforcement, app-version/app-executable SHA-256/OS/Qt-runtime/CPU/GPU-driver/KataGo-version metadata match diagnostics/final `acceptanceMetadata` blocker enforcement, non-empty evidence-path ingestion with distinct-path rejection, diagnostics/report/engine-log/raw-log/manual-UI-log/external-target-log content-marker checks and missing-marker diagnostics including diagnostics app-version/executable-path/record-path/record-sha/final-blocker binding, target-report app-version/executable-path/record-path/record-sha/final-blocker binding, engine-log and raw-log recorded KataGo-version binding, engine-log GPU/stderr marker evidence gates, raw comparison checklist-item evidence markers, manual UI inspection log evidence markers, external target verification log evidence markers, and raw `kata-analyze`/`rootInfo`/`moveInfos`/`winrate`/`scoreMean`/`scoreStdev`/`visits`/`policy`/`pv`/`pvVisits`/`ownership` evidence markers with targeted winrate/scoreMean/scoreStdev/policy/visits/pv/pvVisits/ownership missing-marker diagnostics, and SHA-256 evidence digests, required evidence SHA-256 pin missing/mismatch diagnostics/final `acceptanceEvidenceSha256` blocker enforcement, evidence timestamp diagnostics/final `acceptanceEvidenceTimestamp` blocker enforcement, screenshot evidence image format/dimension/4K-pixel/variation diagnostics, complete 4K screenshot evidence blocker removal, complete checklist final blocker removal, diagnostics complete checklist final blocker removal, installed diagnostics complete checklist final blocker smoke
target-component/checklist failed/invalid manual final status priority
aggregate manual failed/invalid record diagnostics
target acceptance report mode-specific final blocker smoke output
target acceptance report CLI smoke
target acceptance report configured-path and argument summaries
screenshot evidence image format/dimension/4K-pixel/variation diagnostics
complete 4K screenshot evidence blocker removal
complete checklist final blocker removal
diagnostics complete checklist final blocker removal
installed diagnostics complete checklist final blocker smoke
screenshotEvidence4K
acceptanceChecklist
realtime GTP and batch-analysis acceptance
candidate flags
dual-engine acceptance candidate flags
human-vs-AI, self-play, and engine-vs-engine paths
cold and already-running
missing-genmove capability shutdown
no stale
follow-up or auto-restart
pending automated replies
]=])
set(valid_plan_requirement_audit [=[
PLAN.md Requirement Audit
Evidence Legend
Automated
Optional real KataGo
External acceptance
External acceptance record
KDE Wayland NVIDIA
Windows NVIDIA
4K pixel envelope
Architecture
Core Model
KataGo Engine
UI/UX
Feature Scope
Acceptance Criteria
No macOS first-release support
No Java Swing/AWT dependency
LayerBoundaryCheck.cmake
executable-entry separation from the app library
non-engine `QProcess`/`QThread` API leaks
app/UI engine-private process header leaks
engine public `QThread` API leaks are rejected
FirstReleaseScopeCheck.cmake
first-release out-of-scope implementation markers
legacy `Lizzie.*` global-state markers
Human-vs-AI, self-play, dual-engine comparison, and engine-vs-engine play
Completion Gate
finalAcceptanceStatus
]=])
set(valid_migration "Java Config Import\nGTP config, and\nanalysis config\nquoted arguments\nempty quoted arguments\nWindows-style paths\nsidecar\nGame Info\n0 or 2 through 9 handicap choices\n")
set(
    valid_verification
    "Real KataGo\nGTP config, and analysis config\nKDE Wayland\nWindows\nHigh DPI And Multi-Display\n--diagnostics\n--target-acceptance-report\n--target-acceptance-record-template\n--target-acceptance-record\n--target-acceptance-record -- <ini>\ninstalled diagnostics smoke\ninstalled target acceptance\nreport smoke\ninstalled target acceptance record template smoke\nmissing-qpa-for-installed-record-template-smoke\nPACKAGE_EXPECT_PLATFORM\nPACKAGE_DIAGNOSTICS_QPA_PLATFORM=windows\nPACKAGE_RUN_TARGET_ACCEPTANCE_REPORT_SMOKE\nPACKAGE_RUN_TARGET_ACCEPTANCE_RECORD_TEMPLATE_SMOKE\nPACKAGE_TARGET_ACCEPTANCE_REPORT_QPA_PLATFORM\npackaged target acceptance report CLI smoke\ninvalid QPA platform\nready KataGo env status\nready saved config status\ndual-engine config\nreadiness\nmode-specific final blocker package smoke\nraw diagnostics attached\nplatform-specific package filenames\nQt Quick graphics API\nscene graph\nQML import paths\nrenderer-interface\nOpenGL runtime context diagnostics\nQt build ABI\nQt build/runtime version match\nQt install paths\nQt library paths\napp-local Qt runtime artifact diagnostics\npath-status fields\nwritability\nPath-list environment diagnostics\nQStandardPaths writable locations\napplication font/text metrics\nlocale/UI-language diagnostics\nruntime appearance style/palette diagnostics\nnative file dialog/settings path selector diagnostics\nmain-window UI structure diagnostics\nmain-window 4K/high-DPI layout geometry smoke\nstored and normalized appearance settings\nfirst-run completion diagnostics\nsaved engine setting path diagnostics\nstored engine path-readiness diagnostics\nmissing/invalid saved settings coverage\nstored analysis option diagnostics\nstored board display diagnostics\nstored file behavior diagnostics\nstored shortcut diagnostics\nQSettings storage location\nQSettings session/window restore keys\nsaved session SGF path-status\nsaved window-geometry visibility checks\ncurrent platform plugin candidates\ncommon Wayland platform plugin candidates\ncommon Windows platform plugin candidates\navailable platform plugin listing\ntarget-platform summary diagnostics\ntarget-platform blocker diagnostics\ndisplay blocker diagnostics\nplan.target.acceptance\nplan.acceptance.*\nenv-or-saved main config\nconfiguredStatus\nconfiguredMainConfigSource\nconfiguredCompareConfigSource\nManualVerificationCandidate\nManualVerificationBlocker\nmode-specific final acceptance blocker diagnostics\nfinalAcceptanceBlocker\nfinalAcceptanceOutstandingBlocker\nmanualUiInspectionChecklist\nrawKataGoComparisonChecklist\nexternalTargetVerificationChecklist\nlinuxKdeWaylandNvidiaVerificationChecklist\nwindowsNvidiaVerificationChecklist\nphysicalDisplayVerificationChecklist\nVerificationStatus\nVerificationReadinessBlocker\nconfiguredDualEngine\nconfiguredRealtimeGtp\nconfiguredBatchAnalysis\nsame-screen target-display candidates\nQT_PLUGIN_PATH\nQT_QPA_PLATFORMTHEME\nQSG_RHI_BACKEND\nVK_INSTANCE_LAYERS\nVK_DRIVER_FILES\nNVIDIA_VISIBLE_DEVICES\nextended graphics environment diagnostics\n__EGL_VENDOR_LIBRARY_FILENAMES\ncommon config file extensions\nscreen.primary.index\nscreen.0.virtualSibling.count\ndevice-pixel geometry\nprimary screen geometry\nper-axis DPI\nout-of-scope artifacts\nnon-KataGo engine artifacts\nmacOS artifacts\nWindows runtime\nartifacts\nLinux runtime\nartifacts\nsize/modification time\nkatago.env.executable\nKataGo env readiness summary\n")
string(
    APPEND
    valid_verification
    "complete 4K\nscreenshot evidence blocker removal\ncomplete checklist final blocker\nremoval\n")
string(
    APPEND
    valid_verification
    "recordFile\nrecordFile.sha256\nrecordFile.lastModifiedUtc\nrecordFile.timestampStatus\nrecord.completedUtc\ncompletedUtcStatus\nfuture\nrecord.appVersion\nrecord.appVersionStatus\nrecord.appExecutableSha256\nrecord.appExecutableSha256Status\nrecord.osPrettyName\nrecord.osPrettyNameStatus\nrecord.osKernelType\nrecord.osKernelTypeStatus\nrecord.osKernelVersion\nrecord.osKernelVersionStatus\nrecord.qtRuntimeVersion\nrecord.qtRuntimeVersionStatus\nrecord.qtBuildAbi\nrecord.qtBuildAbiStatus\nrecord.cpuArchitecture\nrecord.cpuArchitectureStatus\nrecord.reviewer\nrecord.targetMachine\nrecord.targetMachineStatus\nrecord.gpuDriver\nrecord.gpuDriverStatus\nrecord.kataGoVersion\nrecord.kataGoVersionStatus\nrecordMetadata.ready\nrecordMetadata.blocker\nrecordHint.passValues\nrecordHint.metadataKeysRequired\nrecordHint.completedUtcRequired\nrecordHint.checklistItemsRequired\nrecordHint.recordAndEvidenceTimestampsMustNotAfterCompletedUtc\nISO UTC\nnon-empty file\nSHA-256\nevidence.diagnostics\nevidence.engineLog\nevidence.rawKataGoComparisonLog\nevidence.manualUiInspectionLog\nevidence.externalTargetVerificationLog\nevidence.*.lastModifiedUtc\nevidence.*.timestampStatus\nevidence.diagnostics.contentStatus\nevidence.targetAcceptanceReport.contentStatus\nevidence.engineLog.contentStatus\nengineLog.gpuOrStderrContentStatus\nengineLog.missingGpuOrStderrContentMarker\nevidence.rawKataGoComparisonLog.contentStatus\nevidence.manualUiInspectionLog.contentStatus\nevidence.externalTargetVerificationLog.contentStatus\nevidence.*.missingContentMarker\nraw comparison checklist item names\nmanual UI inspection checklist item names\nexternal target verification checklist item names\nfinal blocker fields\nLizzieYzy Qt Diagnostics\nkata-analyze\nmoveInfos\nevidence.ready\nevidence.blocker\ndistinct canonical files\nexpectedSha256\nsha256Status\nevidenceSha256.ready\nevidenceSha256.blocker\nrecordTimestamp.ready\nrecordTimestamp.blocker\nevidenceTimestamp.ready\nevidenceTimestamp.blocker\nimagePixelsAtLeast4K\nimageHasPixelVariation\nreadyFor4KAcceptance\nscreenshotEvidence4K\nacceptanceChecklist\nacceptanceEvidenceSha256\nacceptanceRecordTimestamp\nacceptanceEvidenceTimestamp\nlinuxKdeWaylandNvidiaManualResult\nwindowsNvidiaManualResult\nphysicalDisplayManualResult\nexternalTargetVerificationManualResult\nchecklistPassedRecord\nchecklistFailedRecord\nchecklistInvalidRecord\nchecklistMissingRecord\nmanualFailedRecord\nmanualInvalidRecord\nplan.acceptance.checklist.ready\nexternalTargetVerificationStatus\nrawKataGoComparisonStatus\nmanualUiInspectionStatus\nfinalAcceptanceStatus\nfinalAcceptanceOutstandingBlocker\n")
string(APPEND valid_verification "rootInfo\nwinrate\nscoreMean\nscoreStdev\nvisits\npolicy\n`pv`\npvVisits\nownership\n")
string(
    APPEND
    valid_verification
    "[evidenceSha256]\n[checklist.linuxKdeWaylandNvidia]\n[checklist.windowsNvidia]\n[checklist.physicalDisplay]\n[checklist.rawKataGoComparison]\n[checklist.externalTargetVerification]\n[checklist.manualUiInspection]\nwindowsOs=pass\nphysical4KPanel=pass\nlinuxKdeWaylandNvidiaRealtimeKataGo=pass\nmainWindow4KHighDpiLayout=pass\n")
string(
    APPEND
    valid_verification
    "[evidenceContentMarkers]\ndiagnostics=LizzieYzy Qt Diagnostics\ntargetAcceptanceReport=# Target Acceptance Report\n[recordHints]\npassValues=pass; passed; accepted; true; yes\nmetadataKeysRequired=completedUtc; appVersion\ncompletedUtcRequired=ISO-UTC-not-future\nchecklistItemsRequired=pass\n")
set(
    valid_target_acceptance
    "Target Acceptance Report\n--target-acceptance-report\n--target-acceptance-record-template\nrecord-template command runs before Qt platform initialization\n--target-acceptance-record\n--target-acceptance-record -- <ini>\nimagePixelsAtLeast4K\nreadyFor4KAcceptance\nscreenshotEvidence4K\nacceptanceChecklist\napp.version\ngeneratedUtc\napp.executable\napp.dir\nprocess.currentWorkingDirectory\nqt.version\nqt.platform\nos.prettyName\nos.kernelType\nos.kernelVersion\nplan.target.osFamily\nplan.acceptance.status\nconfiguredStatus\nrealKataGoEnvReady\nplan.target.inFirstReleaseScope\ntargetPlatformCandidate\nrealKataGoTargetRunCandidate\nrealKataGoManualVerificationCandidate\nsavedMainConfigReady\nsavedMainGtpReady\nsavedMainAnalysisReady\nsavedCompareConfigReady\nsavedCompareGtpReady\nsavedCompareAnalysisReady\nenvOrSavedMainConfigReady\nenvOrSavedMainGtpReady\nenvOrSavedMainAnalysisReady\nconfiguredMainConfigSource\nconfiguredMainGtpSource\nconfiguredMainAnalysisSource\nconfiguredCompareConfigSource\nconfiguredCompareGtpSource\nconfiguredCompareAnalysisSource\nconfiguredKataGoTargetRunCandidate\nconfiguredManualVerificationCandidate\nconfiguredRealtimeGtpTargetRunCandidate\nconfiguredBatchAnalysisTargetRunCandidate\nconfiguredRealtimeGtpManualVerificationCandidate\nconfiguredBatchAnalysisManualVerificationCandidate\nconfiguredDualEngineConfigReady\nconfiguredDualEngineTargetRunCandidate\nconfiguredDualEngineManualVerificationCandidate\nconfiguredDualEngineGtpReady\nconfiguredDualEngineAnalysisReady\nconfiguredDualRealtimeGtpTargetRunCandidate\nconfiguredDualBatchAnalysisTargetRunCandidate\nconfiguredDualRealtimeGtpManualVerificationCandidate\nconfiguredDualBatchAnalysisManualVerificationCandidate\nconfiguredDualEngineManualVerificationBlocker\nfirstReleaseNvidiaPlatformBlocker\nlinuxKdeWayland.detected\nlinuxKdeWaylandNvidiaBlocker\nwindows.detected\nwindowsNvidiaBlocker\ntargetDisplayBlocker\nmultiDisplayBlocker\nprimaryTargetScreenCandidate\ndisplay4KCandidate\nhighDpiCandidate\nsameScreenTargetDisplayCandidate\nexternalTargetVerificationRequired\nmanualRawEngineComparisonRequired\nmanualUiInspectionRequired\nfinalAcceptanceBlocker\nfinalAcceptanceOutstandingBlocker\nmainBatchAnalysisConfig\nlinuxKdeWaylandNvidiaVerificationStatus\nwindowsNvidiaVerificationStatus\nphysicalDisplayVerificationStatus\nrawKataGoComparisonChecklist\nexternalTargetVerificationChecklist\nmanualUiInspectionChecklist\nKDE Wayland + NVIDIA\nWindows + NVIDIA\nHigh DPI And Multi-Display\nRaw KataGo Comparison\nExternal Target Verification\nManual UI Inspection\nFinal Acceptance\n")
string(
    APPEND
    valid_target_acceptance
    "Configured Paths\nkatago.env.executable\nkatago.env.model\nkatago.env.gtpConfig\nkatago.env.analysisConfig\nqt.settings.diagnosticsOverrideFile\nqt.settings.fileName\nqt.settings.engine.executable.value\nqt.settings.engine.model.value\nqt.settings.engine.gtpConfig.value\nqt.settings.engine.analysisConfig.value\nqt.settings.engine.workingDirectory.value\nqt.settings.engine.extraArgs.value\nqt.settings.engine.extraArgs.parsed.count\nqt.settings.engine.extraArgs.parsed.*\nqt.settings.compare.executable.value\nqt.settings.compare.model.value\nqt.settings.compare.gtpConfig.value\nqt.settings.compare.analysisConfig.value\nqt.settings.compare.workingDirectory.value\nqt.settings.compare.extraArgs.value\nqt.settings.compare.extraArgs.parsed.count\nqt.settings.compare.extraArgs.parsed.*\nrecordFile\nrecordFile.sha256\nrecordFile.lastModifiedUtc\nrecordFile.timestampStatus\nrecord.completedUtc\ncompletedUtcStatus\nfuture\nrecord.appVersion\nrecord.appVersionStatus\nrecord.appExecutableSha256\nrecord.appExecutableSha256Status\nrecord.osPrettyName\nrecord.osPrettyNameStatus\nrecord.osKernelType\nrecord.osKernelTypeStatus\nrecord.osKernelVersion\nrecord.osKernelVersionStatus\nrecord.qtRuntimeVersion\nrecord.qtRuntimeVersionStatus\nrecord.qtBuildAbi\nrecord.qtBuildAbiStatus\nrecord.cpuArchitecture\nrecord.cpuArchitectureStatus\nrecord.reviewer\nrecord.targetMachine\nrecord.targetMachineStatus\nrecord.gpuDriver\nrecord.gpuDriverStatus\nrecord.kataGoVersion\nrecord.kataGoVersionStatus\nrecordMetadata.ready\nrecordMetadata.blocker\nrecordHint.passValues\nrecordHint.metadataKeysRequired\nrecordHint.completedUtcRequired\nrecordHint.checklistItemsRequired\nrecordHint.recordAndEvidenceTimestampsMustNotAfterCompletedUtc\nISO UTC\nnon-empty file\nSHA-256\nevidence.diagnostics\nevidence.engineLog\nevidence.rawKataGoComparisonLog\nevidence.manualUiInspectionLog\nevidence.externalTargetVerificationLog\nevidence.*.lastModifiedUtc\nevidence.*.timestampStatus\nevidence.diagnostics.contentStatus\nevidence.targetAcceptanceReport.contentStatus\nevidence.engineLog.contentStatus\nengineLog.gpuOrStderrContentStatus\nengineLog.missingGpuOrStderrContentMarker\nevidence.rawKataGoComparisonLog.contentStatus\nevidence.manualUiInspectionLog.contentStatus\nevidence.externalTargetVerificationLog.contentStatus\nevidence.*.missingContentMarker\nraw comparison checklist item names\nmanual UI inspection checklist item names\nexternal target verification checklist item names\nfinal blocker fields\nLizzieYzy Qt Diagnostics\nkata-analyze\nmoveInfos\nevidence.ready\nevidence.blocker\ndistinct canonical files\nexpectedSha256\nsha256Status\nevidenceSha256.ready\nevidenceSha256.blocker\nrecordTimestamp.ready\nrecordTimestamp.blocker\nevidenceTimestamp.ready\nevidenceTimestamp.blocker\nimagePixelsAtLeast4K\nimageHasPixelVariation\nreadyFor4KAcceptance\nscreenshotEvidence4K\nacceptanceChecklist\nacceptanceEvidenceSha256\nacceptanceRecordTimestamp\nacceptanceEvidenceTimestamp\nlinuxKdeWaylandNvidiaManualResult\nwindowsNvidiaManualResult\nphysicalDisplayManualResult\nexternalTargetVerificationManualResult\nchecklistPassedRecord\nchecklistFailedRecord\nchecklistInvalidRecord\nchecklistMissingRecord\nmanualFailedRecord\nmanualInvalidRecord\nplan.acceptance.checklist.ready\nexternalTargetVerificationStatus\nrawKataGoComparisonStatus\nmanualUiInspectionStatus\nfinalAcceptanceStatus\nfinalAcceptanceOutstandingBlocker\nResult: pass / fail\nNotes:\n")
string(APPEND valid_target_acceptance "rootInfo\nwinrate\nscoreMean\nscoreStdev\nvisits\npolicy\n`pv`\npvVisits\nownership\n")
string(
    APPEND
    valid_target_acceptance
    "[checklist.linuxKdeWaylandNvidia]\n[checklist.windowsNvidia]\n[checklist.physicalDisplay]\n[checklist.rawKataGoComparison]\n[checklist.externalTargetVerification]\n[checklist.manualUiInspection]\nwindowsOs=pass\nphysical4KPanel=pass\nlinuxKdeWaylandNvidiaRealtimeKataGo=pass\nmainWindow4KHighDpiLayout=pass\n")
string(
    APPEND
    valid_target_acceptance
    "seven evidence paths\n[evidenceContentMarkers]\ndiagnostics=LizzieYzy Qt Diagnostics\ntargetAcceptanceReport=# Target Acceptance Report\n[recordHints]\npassValues=pass; passed; accepted; true; yes\nmetadataKeysRequired=completedUtc; appVersion\ncompletedUtcRequired=ISO-UTC-not-future\nchecklistItemsRequired=pass\n")

function(write_standard_package_docs package_root)
    write_package_file("${package_root}/LizzieYzyQt/share/doc/LizzieYzyQt/README.md" "${valid_readme}")
    write_package_file("${package_root}/LizzieYzyQt/share/doc/LizzieYzyQt/PLAN.md" "${valid_plan}")
    write_package_file("${package_root}/LizzieYzyQt/share/doc/LizzieYzyQt/docs/PlanCoverage.md" "${valid_plan_coverage}")
    write_package_file(
        "${package_root}/LizzieYzyQt/share/doc/LizzieYzyQt/docs/PlanRequirementAudit.md"
        "${valid_plan_requirement_audit}")
    write_package_file("${package_root}/LizzieYzyQt/share/doc/LizzieYzyQt/docs/Migration.md" "${valid_migration}")
    write_package_file("${package_root}/LizzieYzyQt/share/doc/LizzieYzyQt/docs/Verification.md" "${valid_verification}")
    write_package_file(
        "${package_root}/LizzieYzyQt/share/doc/LizzieYzyQt/docs/TargetAcceptanceReport.md"
        "${valid_target_acceptance}")
endfunction()

function(create_fake_package package_root package_path include_qml)
    file(REMOVE_RECURSE "${package_root}")
    write_package_file("${package_root}/LizzieYzyQt/bin/lizzieyzy_qt.exe")
    write_standard_package_docs("${package_root}")
    write_package_file("${package_root}/LizzieYzyQt/bin/Qt6Core.dll")
    write_package_file("${package_root}/LizzieYzyQt/bin/Qt6Gui.dll")
    write_package_file("${package_root}/LizzieYzyQt/bin/Qt6Widgets.dll")
    write_package_file("${package_root}/LizzieYzyQt/bin/Qt6Quick.dll")
    write_package_file("${package_root}/LizzieYzyQt/bin/Qt6QuickWidgets.dll")
    if(include_qml)
        write_package_file("${package_root}/LizzieYzyQt/bin/Qt6Qml.dll")
    endif()
    write_package_file("${package_root}/LizzieYzyQt/bin/Qt6Network.dll")
    write_package_file("${package_root}/LizzieYzyQt/bin/Qt6OpenGL.dll")
    write_package_file("${package_root}/LizzieYzyQt/bin/platforms/qwindows.dll")
    foreach(extra_path ${ARGN})
        write_package_file("${package_root}/${extra_path}")
    endforeach()

    execute_process(
        COMMAND "${CMAKE_COMMAND}" -E tar cf "${package_path}" --format=zip LizzieYzyQt
        WORKING_DIRECTORY "${package_root}"
        RESULT_VARIABLE tar_result
        ERROR_VARIABLE tar_error
    )
    if(NOT tar_result EQUAL 0)
        message(FATAL_ERROR "Could not create fake package: ${tar_error}")
    endif()
endfunction()

function(create_fake_linux_package package_root package_path include_executable include_verification_doc)
    file(REMOVE_RECURSE "${package_root}")
    if(include_executable)
        write_package_file("${package_root}/LizzieYzyQt/bin/lizzieyzy_qt")
    endif()
    write_standard_package_docs("${package_root}")
    if(NOT include_verification_doc)
        file(REMOVE "${package_root}/LizzieYzyQt/share/doc/LizzieYzyQt/docs/Verification.md")
    endif()
    foreach(extra_path ${ARGN})
        write_package_file("${package_root}/${extra_path}")
    endforeach()

    execute_process(
        COMMAND "${CMAKE_COMMAND}" -E tar czf "${package_path}" LizzieYzyQt
        WORKING_DIRECTORY "${package_root}"
        RESULT_VARIABLE tar_result
        ERROR_VARIABLE tar_error
    )
    if(NOT tar_result EQUAL 0)
        message(FATAL_ERROR "Could not create fake Linux package: ${tar_error}")
    endif()
endfunction()

file(MAKE_DIRECTORY "${work_dir}")
set(linux_root "${work_dir}/linux-root")
set(linux_package "${work_dir}/linux.tar.gz")
create_fake_linux_package("${linux_root}" "${linux_package}" TRUE TRUE)

execute_process(
    COMMAND
        "${CMAKE_COMMAND}"
        "-DPACKAGE_GLOB=${linux_package}"
        "-DPACKAGE_EXECUTABLE=lizzieyzy_qt"
        -P "${ROOT}/cmake/VerifyPackage.cmake"
    RESULT_VARIABLE linux_result
    OUTPUT_VARIABLE linux_output
    ERROR_VARIABLE linux_error
)
if(NOT linux_result EQUAL 0)
    message(FATAL_ERROR "Minimal fake Linux package should pass verification: ${linux_output}${linux_error}")
endif()
if(EXISTS "${work_dir}/verify-package-docs")
    message(FATAL_ERROR "Default documentation smoke directory should be removed after successful verification")
endif()

set(named_linux_root "${work_dir}/named-linux-root")
set(named_linux_package "${work_dir}/LizzieYzyQt-0.1.0-Linux.tar.gz")
create_fake_linux_package("${named_linux_root}" "${named_linux_package}" TRUE TRUE)

execute_process(
    COMMAND
        "${CMAKE_COMMAND}"
        "-DPACKAGE_GLOB=${named_linux_package}"
        "-DPACKAGE_EXECUTABLE=lizzieyzy_qt"
        "-DPACKAGE_EXPECT_PLATFORM=Linux"
        -P "${ROOT}/cmake/VerifyPackage.cmake"
    RESULT_VARIABLE named_linux_result
    OUTPUT_VARIABLE named_linux_output
    ERROR_VARIABLE named_linux_error
)
if(NOT named_linux_result EQUAL 0)
    message(FATAL_ERROR "Platform-named fake Linux package should pass verification: ${named_linux_output}${named_linux_error}")
endif()

set(misnamed_linux_package "${work_dir}/LizzieYzyQt-0.1.0-Windows.zip")
file(WRITE "${misnamed_linux_package}" "not a package\n")
execute_process(
    COMMAND
        "${CMAKE_COMMAND}"
        "-DPACKAGE_GLOB=${misnamed_linux_package}"
        "-DPACKAGE_EXECUTABLE=lizzieyzy_qt"
        "-DPACKAGE_EXPECT_PLATFORM=Linux"
        -P "${ROOT}/cmake/VerifyPackage.cmake"
    RESULT_VARIABLE misnamed_linux_result
    OUTPUT_VARIABLE misnamed_linux_output
    ERROR_VARIABLE misnamed_linux_error
)
if(misnamed_linux_result EQUAL 0)
    message(FATAL_ERROR "Misnamed Linux package should fail platform-specific filename verification")
endif()
if(NOT "${misnamed_linux_output}${misnamed_linux_error}" MATCHES "platform-specific package filename")
    message(
        FATAL_ERROR
        "Misnamed Linux package failure should explain platform-specific filename verification: ${misnamed_linux_output}${misnamed_linux_error}")
endif()

execute_process(
    COMMAND
        "${CMAKE_COMMAND}"
        "-DPACKAGE_GLOB=${linux_package}"
        "-DPACKAGE_EXECUTABLE=lizzieyzy_qt"
        "-DPACKAGE_KEEP_VERIFY_DIRS=ON"
        -P "${ROOT}/cmake/VerifyPackage.cmake"
    RESULT_VARIABLE keep_dirs_result
    OUTPUT_VARIABLE keep_dirs_output
    ERROR_VARIABLE keep_dirs_error
)
if(NOT keep_dirs_result EQUAL 0)
    message(FATAL_ERROR "Package verification with PACKAGE_KEEP_VERIFY_DIRS should pass: ${keep_dirs_output}${keep_dirs_error}")
endif()
if(NOT IS_DIRECTORY "${work_dir}/verify-package-docs")
    message(FATAL_ERROR "PACKAGE_KEEP_VERIFY_DIRS should preserve the default documentation smoke directory")
endif()
file(REMOVE_RECURSE "${work_dir}/verify-package-docs")

set(missing_analysis_config_doc_root "${work_dir}/missing-analysis-config-doc-root")
set(missing_analysis_config_doc_package "${work_dir}/missing-analysis-config-doc.tar.gz")
file(REMOVE_RECURSE "${missing_analysis_config_doc_root}")
write_package_file("${missing_analysis_config_doc_root}/LizzieYzyQt/bin/lizzieyzy_qt")
write_standard_package_docs("${missing_analysis_config_doc_root}")
write_package_file(
    "${missing_analysis_config_doc_root}/LizzieYzyQt/share/doc/LizzieYzyQt/README.md"
    "repository-root PLAN.md\nConfigure Engine\nGTP config\nplatform-specific package filenames\n0 or 2 through 9 handicap choices\nNo Engine Mode\ndocs/Migration.md\ndocs/Verification.md\nLIZZIE_KATAGO_EXECUTABLE\nKataGo env readiness summary\n--diagnostics\n--target-acceptance-record-template\ntemplate command runs before Qt platform initialization\ncommon config file extensions\napp directory\nQt library paths\nQt plugin path status\nPath-list environment diagnostics\nQStandardPaths writable locations\napplication font/text metrics\nlocale/UI-language diagnostics\nstored appearance settings\nfirst-run completion diagnostics\nsaved engine setting path diagnostics\nstored analysis option diagnostics\nstored board display diagnostics\nstored file behavior diagnostics\nstored shortcut diagnostics\nQSettings storage location\nQSettings session/window restore keys\nsaved session SGF path-status\nsaved window-geometry visibility checks\ncurrent platform plugin candidates\ncommon Wayland platform plugin candidates\ncommon Windows platform plugin candidates\navailable platform plugin listing\ntarget-platform summary diagnostics\nscene graph\nQML import paths\nrenderer-interface\nVulkan\nextended graphics environment diagnostics\n__EGL_VENDOR_LIBRARY_FILENAMES\nscreen orientation\ndevice-pixel geometry\nper-axis DPI\nsize, and modification time\nout-of-scope artifacts\nnon-KataGo engine artifacts\nmacOS artifacts\nWindows runtime artifacts in Linux packages\nLinux runtime\nartifacts in Windows packages\n")
execute_process(
    COMMAND "${CMAKE_COMMAND}" -E tar czf "${missing_analysis_config_doc_package}" LizzieYzyQt
    WORKING_DIRECTORY "${missing_analysis_config_doc_root}"
    RESULT_VARIABLE missing_analysis_config_doc_tar_result
    ERROR_VARIABLE missing_analysis_config_doc_tar_error
)
if(NOT missing_analysis_config_doc_tar_result EQUAL 0)
    message(FATAL_ERROR "Could not create fake missing-analysis-config-doc package: ${missing_analysis_config_doc_tar_error}")
endif()
execute_process(
    COMMAND
        "${CMAKE_COMMAND}"
        "-DPACKAGE_GLOB=${missing_analysis_config_doc_package}"
        "-DPACKAGE_EXECUTABLE=lizzieyzy_qt"
        -P "${ROOT}/cmake/VerifyPackage.cmake"
    RESULT_VARIABLE missing_analysis_config_doc_result
    OUTPUT_VARIABLE missing_analysis_config_doc_output
    ERROR_VARIABLE missing_analysis_config_doc_error
)
if(missing_analysis_config_doc_result EQUAL 0)
    message(FATAL_ERROR "Package docs without first-run analysis config should fail verification")
endif()
if(NOT "${missing_analysis_config_doc_output}${missing_analysis_config_doc_error}" MATCHES "README\\.md should document first-run analysis config")
    message(
        FATAL_ERROR
        "Missing first-run analysis config doc failure should name README.md: ${missing_analysis_config_doc_output}${missing_analysis_config_doc_error}")
endif()

execute_process(
    COMMAND
        "${CMAKE_COMMAND}"
        "-DPACKAGE_GLOB=${linux_package}"
        "-DPACKAGE_EXECUTABLE=lizzieyzy_qt"
        "-DPACKAGE_RUN_VERSION_SMOKE=ON"
        "-DPACKAGE_SMOKE_DIR=${work_dir}/fake-linux-smoke"
        -P "${ROOT}/cmake/VerifyPackage.cmake"
    RESULT_VARIABLE fake_smoke_result
    OUTPUT_VARIABLE fake_smoke_output
    ERROR_VARIABLE fake_smoke_error
)
if(fake_smoke_result EQUAL 0)
    message(FATAL_ERROR "Version smoke should fail for a fake text executable")
endif()
if(NOT "${fake_smoke_output}${fake_smoke_error}" MATCHES "Package version smoke failed")
    message(FATAL_ERROR "Fake executable smoke failure should name the version smoke: ${fake_smoke_output}${fake_smoke_error}")
endif()

if(UNIX)
    set(non_strict_cli_root "${work_dir}/non-strict-cli-root")
    set(non_strict_cli_package "${work_dir}/non-strict-cli.tar.gz")
    file(REMOVE_RECURSE "${non_strict_cli_root}")
    write_standard_package_docs("${non_strict_cli_root}")
    write_package_file(
        "${non_strict_cli_root}/LizzieYzyQt/bin/lizzieyzy_qt"
        "#!/bin/sh\nif [ \"$1\" = \"--version\" ]; then\n  echo \"LizzieYzy Qt 0.1.0\"\n  exit 0\nfi\nif [ \"$1\" = \"--unknown-target-acceptance-option\" ]; then\n  echo \"ignored unknown option\"\n  exit 0\nfi\nexit 0\n")
    file(
        CHMOD
        "${non_strict_cli_root}/LizzieYzyQt/bin/lizzieyzy_qt"
        PERMISSIONS
            OWNER_READ
            OWNER_WRITE
            OWNER_EXECUTE
            GROUP_READ
            GROUP_EXECUTE
            WORLD_READ
            WORLD_EXECUTE)
    execute_process(
        COMMAND "${CMAKE_COMMAND}" -E tar czf "${non_strict_cli_package}" LizzieYzyQt
        WORKING_DIRECTORY "${non_strict_cli_root}"
        RESULT_VARIABLE non_strict_cli_tar_result
        ERROR_VARIABLE non_strict_cli_tar_error
    )
    if(NOT non_strict_cli_tar_result EQUAL 0)
        message(FATAL_ERROR "Could not create fake non-strict CLI package: ${non_strict_cli_tar_error}")
    endif()
    execute_process(
        COMMAND
            "${CMAKE_COMMAND}"
            "-DPACKAGE_GLOB=${non_strict_cli_package}"
            "-DPACKAGE_EXECUTABLE=lizzieyzy_qt"
            "-DPACKAGE_RUN_VERSION_SMOKE=ON"
            "-DPACKAGE_SMOKE_DIR=${work_dir}/non-strict-cli-smoke"
            -P "${ROOT}/cmake/VerifyPackage.cmake"
        RESULT_VARIABLE non_strict_cli_result
        OUTPUT_VARIABLE non_strict_cli_output
        ERROR_VARIABLE non_strict_cli_error
    )
    if(non_strict_cli_result EQUAL 0)
        message(FATAL_ERROR "Package version smoke should fail when unknown long options are accepted")
    endif()
    if(NOT "${non_strict_cli_output}${non_strict_cli_error}" MATCHES "strict CLI argument validation smoke")
        message(
            FATAL_ERROR
            "Non-strict CLI failure should name strict CLI argument validation smoke: ${non_strict_cli_output}${non_strict_cli_error}")
    endif()

    set(non_strict_record_cli_root "${work_dir}/non-strict-record-cli-root")
    set(non_strict_record_cli_package "${work_dir}/non-strict-record-cli.tar.gz")
    file(REMOVE_RECURSE "${non_strict_record_cli_root}")
    write_standard_package_docs("${non_strict_record_cli_root}")
    write_package_file(
        "${non_strict_record_cli_root}/LizzieYzyQt/bin/lizzieyzy_qt"
        "#!/bin/sh\nif [ \"$1\" = \"--version\" ]; then\n  echo \"LizzieYzy Qt 0.1.0\"\n  exit 0\nfi\nif [ \"$1\" = \"--unknown-target-acceptance-option\" ]; then\n  echo \"Unknown option: --unknown-target-acceptance-option\" >&2\n  exit 2\nfi\nif [ \"$1\" = \"--target-acceptance-record\" ]; then\n  echo \"accepted option-like record path\"\n  exit 0\nfi\nexit 0\n")
    file(
        CHMOD
        "${non_strict_record_cli_root}/LizzieYzyQt/bin/lizzieyzy_qt"
        PERMISSIONS
            OWNER_READ
            OWNER_WRITE
            OWNER_EXECUTE
            GROUP_READ
            GROUP_EXECUTE
            WORLD_READ
            WORLD_EXECUTE)
    execute_process(
        COMMAND "${CMAKE_COMMAND}" -E tar czf "${non_strict_record_cli_package}" LizzieYzyQt
        WORKING_DIRECTORY "${non_strict_record_cli_root}"
        RESULT_VARIABLE non_strict_record_cli_tar_result
        ERROR_VARIABLE non_strict_record_cli_tar_error
    )
    if(NOT non_strict_record_cli_tar_result EQUAL 0)
        message(FATAL_ERROR "Could not create fake non-strict record CLI package: ${non_strict_record_cli_tar_error}")
    endif()
    execute_process(
        COMMAND
            "${CMAKE_COMMAND}"
            "-DPACKAGE_GLOB=${non_strict_record_cli_package}"
            "-DPACKAGE_EXECUTABLE=lizzieyzy_qt"
            "-DPACKAGE_RUN_VERSION_SMOKE=ON"
            "-DPACKAGE_SMOKE_DIR=${work_dir}/non-strict-record-cli-smoke"
            -P "${ROOT}/cmake/VerifyPackage.cmake"
        RESULT_VARIABLE non_strict_record_cli_result
        OUTPUT_VARIABLE non_strict_record_cli_output
        ERROR_VARIABLE non_strict_record_cli_error
    )
    if(non_strict_record_cli_result EQUAL 0)
        message(FATAL_ERROR "Package version smoke should fail when option-like record paths are accepted")
    endif()
    if(NOT "${non_strict_record_cli_output}${non_strict_record_cli_error}" MATCHES "target acceptance record argument validation smoke")
        message(
            FATAL_ERROR
            "Non-strict record CLI failure should name target acceptance record argument validation smoke: ${non_strict_record_cli_output}${non_strict_record_cli_error}")
    endif()
endif()

execute_process(
    COMMAND
        "${CMAKE_COMMAND}"
        "-DPACKAGE_GLOB=${linux_package}"
        "-DPACKAGE_EXECUTABLE=lizzieyzy_qt"
        "-DPACKAGE_RUN_DIAGNOSTICS_SMOKE=ON"
        "-DPACKAGE_DIAGNOSTICS_SMOKE_DIR=${work_dir}/fake-linux-diagnostics-smoke"
        "-DPACKAGE_DIAGNOSTICS_QPA_PLATFORM=offscreen"
        -P "${ROOT}/cmake/VerifyPackage.cmake"
    RESULT_VARIABLE fake_diagnostics_smoke_result
    OUTPUT_VARIABLE fake_diagnostics_smoke_output
    ERROR_VARIABLE fake_diagnostics_smoke_error
)
if(fake_diagnostics_smoke_result EQUAL 0)
    message(FATAL_ERROR "Diagnostics smoke should fail for a fake text executable")
endif()
if(NOT "${fake_diagnostics_smoke_output}${fake_diagnostics_smoke_error}" MATCHES "Package diagnostics smoke failed")
    message(
        FATAL_ERROR
        "Fake executable diagnostics failure should name the diagnostics smoke: ${fake_diagnostics_smoke_output}${fake_diagnostics_smoke_error}")
endif()

execute_process(
    COMMAND
        "${CMAKE_COMMAND}"
        "-DPACKAGE_GLOB=${linux_package}"
        "-DPACKAGE_EXECUTABLE=lizzieyzy_qt"
        "-DPACKAGE_RUN_TARGET_ACCEPTANCE_REPORT_SMOKE=ON"
        "-DPACKAGE_TARGET_ACCEPTANCE_REPORT_SMOKE_DIR=${work_dir}/fake-linux-target-acceptance-report-smoke"
        "-DPACKAGE_TARGET_ACCEPTANCE_REPORT_QPA_PLATFORM=offscreen"
        -P "${ROOT}/cmake/VerifyPackage.cmake"
    RESULT_VARIABLE fake_target_acceptance_report_smoke_result
    OUTPUT_VARIABLE fake_target_acceptance_report_smoke_output
    ERROR_VARIABLE fake_target_acceptance_report_smoke_error
)
if(fake_target_acceptance_report_smoke_result EQUAL 0)
    message(FATAL_ERROR "Target acceptance report smoke should fail for a fake text executable")
endif()
if(NOT "${fake_target_acceptance_report_smoke_output}${fake_target_acceptance_report_smoke_error}" MATCHES "Package target acceptance record metadata template probe failed")
    message(
        FATAL_ERROR
        "Fake executable target acceptance report failure should name the metadata template probe: ${fake_target_acceptance_report_smoke_output}${fake_target_acceptance_report_smoke_error}")
endif()

execute_process(
    COMMAND
        "${CMAKE_COMMAND}"
        "-DPACKAGE_GLOB=${linux_package}"
        "-DPACKAGE_EXECUTABLE=lizzieyzy_qt"
        "-DPACKAGE_RUN_TARGET_ACCEPTANCE_RECORD_TEMPLATE_SMOKE=ON"
        "-DPACKAGE_TARGET_ACCEPTANCE_RECORD_TEMPLATE_SMOKE_DIR=${work_dir}/fake-linux-target-acceptance-record-template-smoke"
        -P "${ROOT}/cmake/VerifyPackage.cmake"
    RESULT_VARIABLE fake_target_acceptance_record_template_smoke_result
    OUTPUT_VARIABLE fake_target_acceptance_record_template_smoke_output
    ERROR_VARIABLE fake_target_acceptance_record_template_smoke_error
)
if(fake_target_acceptance_record_template_smoke_result EQUAL 0)
    message(FATAL_ERROR "Target acceptance record template smoke should fail for a fake text executable")
endif()
if(NOT "${fake_target_acceptance_record_template_smoke_output}${fake_target_acceptance_record_template_smoke_error}" MATCHES
   "Package target acceptance record template smoke failed")
    message(
        FATAL_ERROR
        "Fake executable target acceptance record template failure should name the record template smoke: ${fake_target_acceptance_record_template_smoke_output}${fake_target_acceptance_record_template_smoke_error}")
endif()

set(missing_executable_root "${work_dir}/missing-executable-root")
set(missing_executable_package "${work_dir}/missing-executable.tar.gz")
create_fake_linux_package("${missing_executable_root}" "${missing_executable_package}" FALSE TRUE)

execute_process(
    COMMAND
        "${CMAKE_COMMAND}"
        "-DPACKAGE_GLOB=${missing_executable_package}"
        "-DPACKAGE_EXECUTABLE=lizzieyzy_qt"
        -P "${ROOT}/cmake/VerifyPackage.cmake"
    RESULT_VARIABLE missing_executable_result
    OUTPUT_VARIABLE missing_executable_output
    ERROR_VARIABLE missing_executable_error
)
if(missing_executable_result EQUAL 0)
    message(FATAL_ERROR "Package missing the app executable should fail verification")
endif()
if(NOT "${missing_executable_output}${missing_executable_error}" MATCHES "bin/lizzieyzy_qt")
    message(FATAL_ERROR "Missing-executable failure should name bin/lizzieyzy_qt: ${missing_executable_output}${missing_executable_error}")
endif()

set(missing_doc_root "${work_dir}/missing-doc-root")
set(missing_doc_package "${work_dir}/missing-doc.tar.gz")
create_fake_linux_package("${missing_doc_root}" "${missing_doc_package}" TRUE FALSE)

execute_process(
    COMMAND
        "${CMAKE_COMMAND}"
        "-DPACKAGE_GLOB=${missing_doc_package}"
        "-DPACKAGE_EXECUTABLE=lizzieyzy_qt"
        -P "${ROOT}/cmake/VerifyPackage.cmake"
    RESULT_VARIABLE missing_doc_result
    OUTPUT_VARIABLE missing_doc_output
    ERROR_VARIABLE missing_doc_error
)
if(missing_doc_result EQUAL 0)
    message(FATAL_ERROR "Package missing Verification.md should fail verification")
endif()
if(NOT "${missing_doc_output}${missing_doc_error}" MATCHES "Verification\\.md")
    message(FATAL_ERROR "Missing-doc failure should name Verification.md: ${missing_doc_output}${missing_doc_error}")
endif()

set(missing_target_acceptance_root "${work_dir}/missing-target-acceptance-root")
set(missing_target_acceptance_package "${work_dir}/missing-target-acceptance.tar.gz")
file(REMOVE_RECURSE "${missing_target_acceptance_root}")
write_package_file("${missing_target_acceptance_root}/LizzieYzyQt/bin/lizzieyzy_qt")
write_standard_package_docs("${missing_target_acceptance_root}")
file(REMOVE "${missing_target_acceptance_root}/LizzieYzyQt/share/doc/LizzieYzyQt/docs/TargetAcceptanceReport.md")
execute_process(
    COMMAND "${CMAKE_COMMAND}" -E tar czf "${missing_target_acceptance_package}" LizzieYzyQt
    WORKING_DIRECTORY "${missing_target_acceptance_root}"
    RESULT_VARIABLE missing_target_acceptance_tar_result
    ERROR_VARIABLE missing_target_acceptance_tar_error
)
if(NOT missing_target_acceptance_tar_result EQUAL 0)
    message(FATAL_ERROR "Could not create fake missing-target-acceptance package: ${missing_target_acceptance_tar_error}")
endif()

execute_process(
    COMMAND
        "${CMAKE_COMMAND}"
        "-DPACKAGE_GLOB=${missing_target_acceptance_package}"
        "-DPACKAGE_EXECUTABLE=lizzieyzy_qt"
        -P "${ROOT}/cmake/VerifyPackage.cmake"
    RESULT_VARIABLE missing_target_acceptance_result
    OUTPUT_VARIABLE missing_target_acceptance_output
    ERROR_VARIABLE missing_target_acceptance_error
)
if(missing_target_acceptance_result EQUAL 0)
    message(FATAL_ERROR "Package missing TargetAcceptanceReport.md should fail verification")
endif()
if(NOT "${missing_target_acceptance_output}${missing_target_acceptance_error}" MATCHES "TargetAcceptanceReport\\.md")
    message(FATAL_ERROR "Missing-target-acceptance failure should name TargetAcceptanceReport.md: ${missing_target_acceptance_output}${missing_target_acceptance_error}")
endif()

set(missing_plan_root "${work_dir}/missing-plan-root")
set(missing_plan_package "${work_dir}/missing-plan.tar.gz")
file(REMOVE_RECURSE "${missing_plan_root}")
write_package_file("${missing_plan_root}/LizzieYzyQt/bin/lizzieyzy_qt")
write_standard_package_docs("${missing_plan_root}")
file(REMOVE "${missing_plan_root}/LizzieYzyQt/share/doc/LizzieYzyQt/PLAN.md")
execute_process(
    COMMAND "${CMAKE_COMMAND}" -E tar czf "${missing_plan_package}" LizzieYzyQt
    WORKING_DIRECTORY "${missing_plan_root}"
    RESULT_VARIABLE missing_plan_tar_result
    ERROR_VARIABLE missing_plan_tar_error
)
if(NOT missing_plan_tar_result EQUAL 0)
    message(FATAL_ERROR "Could not create fake missing-PLAN package: ${missing_plan_tar_error}")
endif()

execute_process(
    COMMAND
        "${CMAKE_COMMAND}"
        "-DPACKAGE_GLOB=${missing_plan_package}"
        "-DPACKAGE_EXECUTABLE=lizzieyzy_qt"
        -P "${ROOT}/cmake/VerifyPackage.cmake"
    RESULT_VARIABLE missing_plan_result
    OUTPUT_VARIABLE missing_plan_output
    ERROR_VARIABLE missing_plan_error
)
if(missing_plan_result EQUAL 0)
    message(FATAL_ERROR "Package missing PLAN.md should fail verification")
endif()
if(NOT "${missing_plan_output}${missing_plan_error}" MATCHES "PLAN\\.md")
    message(FATAL_ERROR "Missing-PLAN failure should name PLAN.md: ${missing_plan_output}${missing_plan_error}")
endif()

set(missing_plan_doc_root "${work_dir}/missing-plan-doc-root")
set(missing_plan_doc_package "${work_dir}/missing-plan-doc.tar.gz")
file(REMOVE_RECURSE "${missing_plan_doc_root}")
write_package_file("${missing_plan_doc_root}/LizzieYzyQt/bin/lizzieyzy_qt")
write_standard_package_docs("${missing_plan_doc_root}")
file(REMOVE "${missing_plan_doc_root}/LizzieYzyQt/share/doc/LizzieYzyQt/docs/PlanCoverage.md")
execute_process(
    COMMAND "${CMAKE_COMMAND}" -E tar czf "${missing_plan_doc_package}" LizzieYzyQt
    WORKING_DIRECTORY "${missing_plan_doc_root}"
    RESULT_VARIABLE missing_plan_tar_result
    ERROR_VARIABLE missing_plan_tar_error
)
if(NOT missing_plan_tar_result EQUAL 0)
    message(FATAL_ERROR "Could not create fake missing-PlanCoverage package: ${missing_plan_tar_error}")
endif()

execute_process(
    COMMAND
        "${CMAKE_COMMAND}"
        "-DPACKAGE_GLOB=${missing_plan_doc_package}"
        "-DPACKAGE_EXECUTABLE=lizzieyzy_qt"
        -P "${ROOT}/cmake/VerifyPackage.cmake"
    RESULT_VARIABLE missing_plan_doc_result
    OUTPUT_VARIABLE missing_plan_doc_output
    ERROR_VARIABLE missing_plan_doc_error
)
if(missing_plan_doc_result EQUAL 0)
    message(FATAL_ERROR "Package missing PlanCoverage.md should fail verification")
endif()
if(NOT "${missing_plan_doc_output}${missing_plan_doc_error}" MATCHES "PlanCoverage\\.md")
    message(FATAL_ERROR "Missing-plan-doc failure should name PlanCoverage.md: ${missing_plan_doc_output}${missing_plan_doc_error}")
endif()

set(missing_plan_requirement_audit_root "${work_dir}/missing-plan-requirement-audit-root")
set(missing_plan_requirement_audit_package "${work_dir}/missing-plan-requirement-audit.tar.gz")
file(REMOVE_RECURSE "${missing_plan_requirement_audit_root}")
write_package_file("${missing_plan_requirement_audit_root}/LizzieYzyQt/bin/lizzieyzy_qt")
write_standard_package_docs("${missing_plan_requirement_audit_root}")
file(REMOVE "${missing_plan_requirement_audit_root}/LizzieYzyQt/share/doc/LizzieYzyQt/docs/PlanRequirementAudit.md")
execute_process(
    COMMAND "${CMAKE_COMMAND}" -E tar czf "${missing_plan_requirement_audit_package}" LizzieYzyQt
    WORKING_DIRECTORY "${missing_plan_requirement_audit_root}"
    RESULT_VARIABLE missing_plan_requirement_audit_tar_result
    ERROR_VARIABLE missing_plan_requirement_audit_tar_error
)
if(NOT missing_plan_requirement_audit_tar_result EQUAL 0)
    message(
        FATAL_ERROR
        "Could not create fake missing-PlanRequirementAudit package: ${missing_plan_requirement_audit_tar_error}")
endif()

execute_process(
    COMMAND
        "${CMAKE_COMMAND}"
        "-DPACKAGE_GLOB=${missing_plan_requirement_audit_package}"
        "-DPACKAGE_EXECUTABLE=lizzieyzy_qt"
        -P "${ROOT}/cmake/VerifyPackage.cmake"
    RESULT_VARIABLE missing_plan_requirement_audit_result
    OUTPUT_VARIABLE missing_plan_requirement_audit_output
    ERROR_VARIABLE missing_plan_requirement_audit_error
)
if(missing_plan_requirement_audit_result EQUAL 0)
    message(FATAL_ERROR "Package missing PlanRequirementAudit.md should fail verification")
endif()
if(NOT "${missing_plan_requirement_audit_output}${missing_plan_requirement_audit_error}" MATCHES "PlanRequirementAudit\\.md")
    message(
        FATAL_ERROR
        "Missing-plan-requirement-audit failure should name PlanRequirementAudit.md: ${missing_plan_requirement_audit_output}${missing_plan_requirement_audit_error}")
endif()

set(incomplete_plan_requirement_audit_root "${work_dir}/incomplete-plan-requirement-audit-root")
set(incomplete_plan_requirement_audit_package "${work_dir}/incomplete-plan-requirement-audit.tar.gz")
file(REMOVE_RECURSE "${incomplete_plan_requirement_audit_root}")
write_package_file("${incomplete_plan_requirement_audit_root}/LizzieYzyQt/bin/lizzieyzy_qt")
write_standard_package_docs("${incomplete_plan_requirement_audit_root}")
write_package_file(
    "${incomplete_plan_requirement_audit_root}/LizzieYzyQt/share/doc/LizzieYzyQt/docs/PlanRequirementAudit.md"
    "PLAN.md Requirement Audit\nEvidence Legend\nAutomated\nOptional real KataGo\nExternal acceptance\nCore Model\nKataGo Engine\nUI/UX\nHuman-vs-AI, self-play, dual-engine comparison, and engine-vs-engine play\nNo macOS first-release support\nCompletion Gate\nfinalAcceptanceStatus\n")
execute_process(
    COMMAND "${CMAKE_COMMAND}" -E tar czf "${incomplete_plan_requirement_audit_package}" LizzieYzyQt
    WORKING_DIRECTORY "${incomplete_plan_requirement_audit_root}"
    RESULT_VARIABLE incomplete_plan_requirement_audit_tar_result
    ERROR_VARIABLE incomplete_plan_requirement_audit_tar_error
)
if(NOT incomplete_plan_requirement_audit_tar_result EQUAL 0)
    message(
        FATAL_ERROR
        "Could not create fake incomplete-PlanRequirementAudit package: ${incomplete_plan_requirement_audit_tar_error}")
endif()

execute_process(
    COMMAND
        "${CMAKE_COMMAND}"
        "-DPACKAGE_GLOB=${incomplete_plan_requirement_audit_package}"
        "-DPACKAGE_EXECUTABLE=lizzieyzy_qt"
        -P "${ROOT}/cmake/VerifyPackage.cmake"
    RESULT_VARIABLE incomplete_plan_requirement_audit_result
    OUTPUT_VARIABLE incomplete_plan_requirement_audit_output
    ERROR_VARIABLE incomplete_plan_requirement_audit_error
)
if(incomplete_plan_requirement_audit_result EQUAL 0)
    message(FATAL_ERROR "Package incomplete PlanRequirementAudit.md should fail verification")
endif()
if(NOT "${incomplete_plan_requirement_audit_output}${incomplete_plan_requirement_audit_error}" MATCHES "PlanRequirementAudit\\.md should tie target-machine criteria to acceptance[ \n]+records")
    message(
        FATAL_ERROR
        "Incomplete PlanRequirementAudit failure should name missing external acceptance record coverage: ${incomplete_plan_requirement_audit_output}${incomplete_plan_requirement_audit_error}")
endif()

set(incomplete_plan_coverage_root "${work_dir}/incomplete-plan-coverage-root")
set(incomplete_plan_coverage_package "${work_dir}/incomplete-plan-coverage.tar.gz")
file(REMOVE_RECURSE "${incomplete_plan_coverage_root}")
write_package_file("${incomplete_plan_coverage_root}/LizzieYzyQt/bin/lizzieyzy_qt")
write_standard_package_docs("${incomplete_plan_coverage_root}")
write_package_file(
    "${incomplete_plan_coverage_root}/LizzieYzyQt/share/doc/LizzieYzyQt/docs/PlanCoverage.md"
    "repository-root PLAN.md\nAutomated Evidence\nExternal Acceptance Still Required\nPlanRequirementAudit.md\nVerification.md\nPLAN.md-triggered CI\nWindows native `windows` QPA\nQt build ABI\nQt build/runtime version match\nQt install-path diagnostics\ninstalled diagnostics smoke\ninstalled target acceptance report smoke\ninstalled target acceptance record template smoke\ntarget acceptance report CLI smoke\npackaged target acceptance record template smoke\nPACKAGE_RUN_TARGET_ACCEPTANCE_RECORD_TEMPLATE_SMOKE\n--target-acceptance-record-template\n--target-acceptance-record\ntarget acceptance report configured-path and argument summaries\ntarget acceptance report manual-record, record-file path/size/mtime/SHA-256 diagnostics, checklist-item, checklist missing-record diagnostics, checklist readiness/final `acceptanceChecklist` blocker enforcement, UTC metadata-readiness with `completedUtcStatus` future-timestamp rejection, non-empty evidence-path ingestion with distinct-path rejection, diagnostics/report content-marker checks, and SHA-256 evidence digests, required evidence SHA-256 pin missing/mismatch diagnostics/final `acceptanceEvidenceSha256` blocker enforcement\nscreenshot evidence image format/dimension/4K-pixel/variation diagnostics\ncomplete 4K screenshot evidence blocker removal\ncomplete checklist final blocker removal\ndiagnostics complete checklist final blocker removal\ninstalled diagnostics complete checklist final blocker smoke\nscreenshotEvidence4K\nacceptanceChecklist\nready/missing/invalid KataGo env\nKataGo env readiness summary\nmissing, blank, and invalid path coverage\nall-unset KataGo integration skip path-status diagnostics\nfirst-run completion diagnostics\nstored engine path-readiness diagnostics\nmissing/invalid saved settings coverage\nenv-or-saved main config\nconfigured source diagnostics\nconfigured acceptance status\nmanual\nverification candidate flags\nmanual verification blocker diagnostics\nmanual UI inspection checklist diagnostics\nraw KataGo comparison checklist diagnostics\nexternal target verification checklist\ntarget-specific verification checklist diagnostics\ntarget-specific verification status diagnostics\ntarget-machine acceptance report full evidence/SHA-256/checklist template coverage with evidence-content-marker guidance, record-hint metadata requirement manifest, and record-hint guidance\ninstalled strict CLI and target acceptance record argument validation smoke\nraw `kata-analyze`/`rootInfo`/`moveInfos`/`winrate`/`scoreMean`/`policy`/`pv`/`ownership` evidence markers\nrealtime GTP and batch-analysis acceptance\ncandidate flags\ndual-engine acceptance candidate flags\nhuman-vs-AI, self-play, and engine-vs-engine paths\ncold and already-running\nmissing-genmove capability shutdown\nno stale\nfollow-up or auto-restart\npending automated replies\nstored board display diagnostics\nstored file behavior diagnostics\nstored shortcut diagnostics\nsaved session SGF path-status\ntarget-platform summary diagnostics\ntarget-platform blocker diagnostics\ndisplay blocker diagnostics\nlegacy `Lizzie.*` global-state markers\nLayerBoundaryCheck.cmake\nlizzie_layer_boundary_check\ncore/engine layer boundary violations\nFirstReleaseScopeCheck.cmake\nlizzie_first_release_scope_check\nfirst-release out-of-scope implementation markers\nanalysis-config diagnostics\n")
file(
    APPEND
    "${incomplete_plan_coverage_root}/LizzieYzyQt/share/doc/LizzieYzyQt/docs/PlanCoverage.md"
    "core/engine/app layer boundary violations\nexecutable-entry separation from the app library\nnon-engine QProcess/QThread dependency leaks\napp/UI engine-private process header leaks\nengine public QThread API leaks\n")
execute_process(
    COMMAND "${CMAKE_COMMAND}" -E tar czf "${incomplete_plan_coverage_package}" LizzieYzyQt
    WORKING_DIRECTORY "${incomplete_plan_coverage_root}"
    RESULT_VARIABLE incomplete_plan_coverage_tar_result
    ERROR_VARIABLE incomplete_plan_coverage_tar_error
)
if(NOT incomplete_plan_coverage_tar_result EQUAL 0)
    message(FATAL_ERROR "Could not create fake incomplete-PlanCoverage package: ${incomplete_plan_coverage_tar_error}")
endif()

execute_process(
    COMMAND
        "${CMAKE_COMMAND}"
        "-DPACKAGE_GLOB=${incomplete_plan_coverage_package}"
        "-DPACKAGE_EXECUTABLE=lizzieyzy_qt"
        -P "${ROOT}/cmake/VerifyPackage.cmake"
    RESULT_VARIABLE incomplete_plan_coverage_result
    OUTPUT_VARIABLE incomplete_plan_coverage_output
    ERROR_VARIABLE incomplete_plan_coverage_error
)
if(incomplete_plan_coverage_result EQUAL 0)
    message(FATAL_ERROR "Package PlanCoverage.md without GTP-config diagnostic coverage should fail verification")
endif()
if(NOT "${incomplete_plan_coverage_output}${incomplete_plan_coverage_error}" MATCHES "PlanCoverage\\.md should document mixed config diagnostic coverage")
    message(
        FATAL_ERROR
        "Incomplete PlanCoverage failure should name mixed config diagnostic coverage: ${incomplete_plan_coverage_output}${incomplete_plan_coverage_error}")
endif()

set(missing_migration_doc_root "${work_dir}/missing-migration-doc-root")
set(missing_migration_doc_package "${work_dir}/missing-migration-doc.tar.gz")
file(REMOVE_RECURSE "${missing_migration_doc_root}")
write_package_file("${missing_migration_doc_root}/LizzieYzyQt/bin/lizzieyzy_qt")
write_standard_package_docs("${missing_migration_doc_root}")
file(REMOVE "${missing_migration_doc_root}/LizzieYzyQt/share/doc/LizzieYzyQt/docs/Migration.md")
execute_process(
    COMMAND "${CMAKE_COMMAND}" -E tar czf "${missing_migration_doc_package}" LizzieYzyQt
    WORKING_DIRECTORY "${missing_migration_doc_root}"
    RESULT_VARIABLE missing_migration_tar_result
    ERROR_VARIABLE missing_migration_tar_error
)
if(NOT missing_migration_tar_result EQUAL 0)
    message(FATAL_ERROR "Could not create fake missing-Migration package: ${missing_migration_tar_error}")
endif()

execute_process(
    COMMAND
        "${CMAKE_COMMAND}"
        "-DPACKAGE_GLOB=${missing_migration_doc_package}"
        "-DPACKAGE_EXECUTABLE=lizzieyzy_qt"
        -P "${ROOT}/cmake/VerifyPackage.cmake"
    RESULT_VARIABLE missing_migration_doc_result
    OUTPUT_VARIABLE missing_migration_doc_output
    ERROR_VARIABLE missing_migration_doc_error
)
if(missing_migration_doc_result EQUAL 0)
    message(FATAL_ERROR "Package missing Migration.md should fail verification")
endif()
if(NOT "${missing_migration_doc_output}${missing_migration_doc_error}" MATCHES "Migration\\.md")
    message(FATAL_ERROR "Missing-migration-doc failure should name Migration.md: ${missing_migration_doc_output}${missing_migration_doc_error}")
endif()

set(incomplete_readme_root "${work_dir}/incomplete-readme-root")
set(incomplete_readme_package "${work_dir}/incomplete-readme.tar.gz")
file(REMOVE_RECURSE "${incomplete_readme_root}")
write_package_file("${incomplete_readme_root}/LizzieYzyQt/bin/lizzieyzy_qt")
write_standard_package_docs("${incomplete_readme_root}")
write_package_file("${incomplete_readme_root}/LizzieYzyQt/share/doc/LizzieYzyQt/README.md" "Incomplete README\n")
execute_process(
    COMMAND "${CMAKE_COMMAND}" -E tar czf "${incomplete_readme_package}" LizzieYzyQt
    WORKING_DIRECTORY "${incomplete_readme_root}"
    RESULT_VARIABLE incomplete_readme_tar_result
    ERROR_VARIABLE incomplete_readme_tar_error
)
if(NOT incomplete_readme_tar_result EQUAL 0)
    message(FATAL_ERROR "Could not create fake incomplete-README package: ${incomplete_readme_tar_error}")
endif()

execute_process(
    COMMAND
        "${CMAKE_COMMAND}"
        "-DPACKAGE_GLOB=${incomplete_readme_package}"
        "-DPACKAGE_EXECUTABLE=lizzieyzy_qt"
        -P "${ROOT}/cmake/VerifyPackage.cmake"
    RESULT_VARIABLE incomplete_readme_result
    OUTPUT_VARIABLE incomplete_readme_output
    ERROR_VARIABLE incomplete_readme_error
)
if(incomplete_readme_result EQUAL 0)
    message(FATAL_ERROR "Package with an incomplete README should fail verification")
endif()
if(NOT "${incomplete_readme_output}${incomplete_readme_error}" MATCHES "README\\.md")
    message(FATAL_ERROR "Incomplete-README failure should name README.md: ${incomplete_readme_output}${incomplete_readme_error}")
endif()

set(source_path_readme_root "${work_dir}/source-path-readme-root")
set(source_path_readme_package "${work_dir}/source-path-readme.tar.gz")
file(REMOVE_RECURSE "${source_path_readme_root}")
write_package_file("${source_path_readme_root}/LizzieYzyQt/bin/lizzieyzy_qt")
write_standard_package_docs("${source_path_readme_root}")
write_package_file(
    "${source_path_readme_root}/LizzieYzyQt/share/doc/LizzieYzyQt/README.md"
    "${valid_readme}\nThis source tree is described in ../PLAN.md\n")
execute_process(
    COMMAND "${CMAKE_COMMAND}" -E tar czf "${source_path_readme_package}" LizzieYzyQt
    WORKING_DIRECTORY "${source_path_readme_root}"
    RESULT_VARIABLE source_path_readme_tar_result
    ERROR_VARIABLE source_path_readme_tar_error
)
if(NOT source_path_readme_tar_result EQUAL 0)
    message(FATAL_ERROR "Could not create fake source-path README package: ${source_path_readme_tar_error}")
endif()

execute_process(
    COMMAND
        "${CMAKE_COMMAND}"
        "-DPACKAGE_GLOB=${source_path_readme_package}"
        "-DPACKAGE_EXECUTABLE=lizzieyzy_qt"
        -P "${ROOT}/cmake/VerifyPackage.cmake"
    RESULT_VARIABLE source_path_readme_result
    OUTPUT_VARIABLE source_path_readme_output
    ERROR_VARIABLE source_path_readme_error
)
if(source_path_readme_result EQUAL 0)
    message(FATAL_ERROR "Package README with a source-only PLAN.md path should fail verification")
endif()
if(NOT "${source_path_readme_output}${source_path_readme_error}" MATCHES "README\\.md")
    message(
        FATAL_ERROR
        "Source-path README failure should name README.md: ${source_path_readme_output}${source_path_readme_error}")
endif()

set(source_path_plan_coverage_root "${work_dir}/source-path-plan-coverage-root")
set(source_path_plan_coverage_package "${work_dir}/source-path-plan-coverage.tar.gz")
file(REMOVE_RECURSE "${source_path_plan_coverage_root}")
write_package_file("${source_path_plan_coverage_root}/LizzieYzyQt/bin/lizzieyzy_qt")
write_standard_package_docs("${source_path_plan_coverage_root}")
write_package_file(
    "${source_path_plan_coverage_root}/LizzieYzyQt/share/doc/LizzieYzyQt/docs/PlanCoverage.md"
    "${valid_plan_coverage}\nSee ../PLAN.md and docs/Verification.md\n")
execute_process(
    COMMAND "${CMAKE_COMMAND}" -E tar czf "${source_path_plan_coverage_package}" LizzieYzyQt
    WORKING_DIRECTORY "${source_path_plan_coverage_root}"
    RESULT_VARIABLE source_path_plan_coverage_tar_result
    ERROR_VARIABLE source_path_plan_coverage_tar_error
)
if(NOT source_path_plan_coverage_tar_result EQUAL 0)
    message(FATAL_ERROR "Could not create fake source-path PlanCoverage package: ${source_path_plan_coverage_tar_error}")
endif()

execute_process(
    COMMAND
        "${CMAKE_COMMAND}"
        "-DPACKAGE_GLOB=${source_path_plan_coverage_package}"
        "-DPACKAGE_EXECUTABLE=lizzieyzy_qt"
        -P "${ROOT}/cmake/VerifyPackage.cmake"
    RESULT_VARIABLE source_path_plan_coverage_result
    OUTPUT_VARIABLE source_path_plan_coverage_output
    ERROR_VARIABLE source_path_plan_coverage_error
)
if(source_path_plan_coverage_result EQUAL 0)
    message(FATAL_ERROR "Package PlanCoverage.md with source-only paths should fail verification")
endif()
if(NOT "${source_path_plan_coverage_output}${source_path_plan_coverage_error}" MATCHES "PlanCoverage\\.md")
    message(
        FATAL_ERROR
        "Source-path PlanCoverage failure should name PlanCoverage.md: ${source_path_plan_coverage_output}${source_path_plan_coverage_error}")
endif()

set(incomplete_migration_root "${work_dir}/incomplete-migration-root")
set(incomplete_migration_package "${work_dir}/incomplete-migration.tar.gz")
file(REMOVE_RECURSE "${incomplete_migration_root}")
write_package_file("${incomplete_migration_root}/LizzieYzyQt/bin/lizzieyzy_qt")
write_standard_package_docs("${incomplete_migration_root}")
write_package_file(
    "${incomplete_migration_root}/LizzieYzyQt/share/doc/LizzieYzyQt/docs/Migration.md"
    "Java Config Import\nsidecar\nGame Info\n")
execute_process(
    COMMAND "${CMAKE_COMMAND}" -E tar czf "${incomplete_migration_package}" LizzieYzyQt
    WORKING_DIRECTORY "${incomplete_migration_root}"
    RESULT_VARIABLE incomplete_migration_tar_result
    ERROR_VARIABLE incomplete_migration_tar_error
)
if(NOT incomplete_migration_tar_result EQUAL 0)
    message(FATAL_ERROR "Could not create fake incomplete-Migration package: ${incomplete_migration_tar_error}")
endif()

execute_process(
    COMMAND
        "${CMAKE_COMMAND}"
        "-DPACKAGE_GLOB=${incomplete_migration_package}"
        "-DPACKAGE_EXECUTABLE=lizzieyzy_qt"
        -P "${ROOT}/cmake/VerifyPackage.cmake"
    RESULT_VARIABLE incomplete_migration_result
    OUTPUT_VARIABLE incomplete_migration_output
    ERROR_VARIABLE incomplete_migration_error
)
if(incomplete_migration_result EQUAL 0)
    message(FATAL_ERROR "Package with incomplete Migration.md command-argument notes should fail verification")
endif()
if(NOT "${incomplete_migration_output}${incomplete_migration_error}" MATCHES "Migration\\.md")
    message(
        FATAL_ERROR
        "Incomplete-Migration failure should name Migration.md: ${incomplete_migration_output}${incomplete_migration_error}")
endif()

set(bundled_katago_root "${work_dir}/bundled-katago-root")
set(bundled_katago_package "${work_dir}/bundled-katago.tar.gz")
create_fake_linux_package(
    "${bundled_katago_root}"
    "${bundled_katago_package}"
    TRUE
    TRUE
    "LizzieYzyQt/bin/katago")

execute_process(
    COMMAND
        "${CMAKE_COMMAND}"
        "-DPACKAGE_GLOB=${bundled_katago_package}"
        "-DPACKAGE_EXECUTABLE=lizzieyzy_qt"
        -P "${ROOT}/cmake/VerifyPackage.cmake"
    RESULT_VARIABLE bundled_katago_result
    OUTPUT_VARIABLE bundled_katago_output
    ERROR_VARIABLE bundled_katago_error
)
if(bundled_katago_result EQUAL 0)
    message(FATAL_ERROR "Package bundling a KataGo executable should fail verification")
endif()
if(NOT "${bundled_katago_output}${bundled_katago_error}" MATCHES "KataGo executables")
    message(FATAL_ERROR "Bundled-KataGo failure should explain the forbidden executable: ${bundled_katago_output}${bundled_katago_error}")
endif()

set(bundled_katago_tool_root "${work_dir}/bundled-katago-tool-root")
set(bundled_katago_tool_package "${work_dir}/bundled-katago-tool.tar.gz")
create_fake_linux_package(
    "${bundled_katago_tool_root}"
    "${bundled_katago_tool_package}"
    TRUE
    TRUE
    "LizzieYzyQt/tools/katago.exe")

execute_process(
    COMMAND
        "${CMAKE_COMMAND}"
        "-DPACKAGE_GLOB=${bundled_katago_tool_package}"
        "-DPACKAGE_EXECUTABLE=lizzieyzy_qt"
        -P "${ROOT}/cmake/VerifyPackage.cmake"
    RESULT_VARIABLE bundled_katago_tool_result
    OUTPUT_VARIABLE bundled_katago_tool_output
    ERROR_VARIABLE bundled_katago_tool_error
)
if(bundled_katago_tool_result EQUAL 0)
    message(FATAL_ERROR "Package bundling a KataGo executable outside bin should fail verification")
endif()
if(NOT "${bundled_katago_tool_output}${bundled_katago_tool_error}" MATCHES "KataGo executables")
    message(FATAL_ERROR "Bundled-KataGo tool failure should explain the forbidden executable: ${bundled_katago_tool_output}${bundled_katago_tool_error}")
endif()

set(bundled_katago_variant_root "${work_dir}/bundled-katago-variant-root")
set(bundled_katago_variant_package "${work_dir}/bundled-katago-variant.tar.gz")
create_fake_linux_package(
    "${bundled_katago_variant_root}"
    "${bundled_katago_variant_package}"
    TRUE
    TRUE
    "LizzieYzyQt/bin/katago-opencl.exe")

execute_process(
    COMMAND
        "${CMAKE_COMMAND}"
        "-DPACKAGE_GLOB=${bundled_katago_variant_package}"
        "-DPACKAGE_EXECUTABLE=lizzieyzy_qt"
        -P "${ROOT}/cmake/VerifyPackage.cmake"
    RESULT_VARIABLE bundled_katago_variant_result
    OUTPUT_VARIABLE bundled_katago_variant_output
    ERROR_VARIABLE bundled_katago_variant_error
)
if(bundled_katago_variant_result EQUAL 0)
    message(FATAL_ERROR "Package bundling a variant-named KataGo executable should fail verification")
endif()
if(NOT "${bundled_katago_variant_output}${bundled_katago_variant_error}" MATCHES "KataGo executables")
    message(FATAL_ERROR "Bundled-KataGo variant failure should explain the forbidden executable: ${bundled_katago_variant_output}${bundled_katago_variant_error}")
endif()

set(bundled_katago_uppercase_root "${work_dir}/bundled-katago-uppercase-root")
set(bundled_katago_uppercase_package "${work_dir}/bundled-katago-uppercase.tar.gz")
create_fake_linux_package(
    "${bundled_katago_uppercase_root}"
    "${bundled_katago_uppercase_package}"
    TRUE
    TRUE
    "LizzieYzyQt/bin/KataGo-CUDA.EXE")

execute_process(
    COMMAND
        "${CMAKE_COMMAND}"
        "-DPACKAGE_GLOB=${bundled_katago_uppercase_package}"
        "-DPACKAGE_EXECUTABLE=lizzieyzy_qt"
        -P "${ROOT}/cmake/VerifyPackage.cmake"
    RESULT_VARIABLE bundled_katago_uppercase_result
    OUTPUT_VARIABLE bundled_katago_uppercase_output
    ERROR_VARIABLE bundled_katago_uppercase_error
)
if(bundled_katago_uppercase_result EQUAL 0)
    message(FATAL_ERROR "Package bundling a case-varied KataGo executable should fail verification")
endif()
if(NOT "${bundled_katago_uppercase_output}${bundled_katago_uppercase_error}" MATCHES "KataGo executables")
    message(FATAL_ERROR "Bundled-KataGo uppercase failure should explain the forbidden executable: ${bundled_katago_uppercase_output}${bundled_katago_uppercase_error}")
endif()

set(bundled_katago_config_root "${work_dir}/bundled-katago-config-root")
set(bundled_katago_config_package "${work_dir}/bundled-katago-config.tar.gz")
create_fake_linux_package(
    "${bundled_katago_config_root}"
    "${bundled_katago_config_package}"
    TRUE
    TRUE
    "LizzieYzyQt/share/config/analysis.cfg")

execute_process(
    COMMAND
        "${CMAKE_COMMAND}"
        "-DPACKAGE_GLOB=${bundled_katago_config_package}"
        "-DPACKAGE_EXECUTABLE=lizzieyzy_qt"
        -P "${ROOT}/cmake/VerifyPackage.cmake"
    RESULT_VARIABLE bundled_katago_config_result
    OUTPUT_VARIABLE bundled_katago_config_output
    ERROR_VARIABLE bundled_katago_config_error
)
if(bundled_katago_config_result EQUAL 0)
    message(FATAL_ERROR "Package bundling a KataGo config should fail verification")
endif()
if(NOT "${bundled_katago_config_output}${bundled_katago_config_error}" MATCHES "KataGo config files")
    message(FATAL_ERROR "Bundled-KataGo config failure should explain the forbidden config: ${bundled_katago_config_output}${bundled_katago_config_error}")
endif()

set(bundled_katago_yaml_config_root "${work_dir}/bundled-katago-yaml-config-root")
set(bundled_katago_yaml_config_package "${work_dir}/bundled-katago-yaml-config.tar.gz")
create_fake_linux_package(
    "${bundled_katago_yaml_config_root}"
    "${bundled_katago_yaml_config_package}"
    TRUE
    TRUE
    "LizzieYzyQt/share/config/katago-analysis.yaml")

execute_process(
    COMMAND
        "${CMAKE_COMMAND}"
        "-DPACKAGE_GLOB=${bundled_katago_yaml_config_package}"
        "-DPACKAGE_EXECUTABLE=lizzieyzy_qt"
        -P "${ROOT}/cmake/VerifyPackage.cmake"
    RESULT_VARIABLE bundled_katago_yaml_config_result
    OUTPUT_VARIABLE bundled_katago_yaml_config_output
    ERROR_VARIABLE bundled_katago_yaml_config_error
)
if(bundled_katago_yaml_config_result EQUAL 0)
    message(FATAL_ERROR "Package bundling a YAML KataGo config should fail verification")
endif()
if(NOT "${bundled_katago_yaml_config_output}${bundled_katago_yaml_config_error}" MATCHES "KataGo config files")
    message(
        FATAL_ERROR
        "Bundled-KataGo YAML config failure should explain the forbidden config: ${bundled_katago_yaml_config_output}${bundled_katago_yaml_config_error}")
endif()

set(bundled_katago_json_config_root "${work_dir}/bundled-katago-json-config-root")
set(bundled_katago_json_config_package "${work_dir}/bundled-katago-json-config.tar.gz")
create_fake_linux_package(
    "${bundled_katago_json_config_root}"
    "${bundled_katago_json_config_package}"
    TRUE
    TRUE
    "LizzieYzyQt/share/config/analysis-config.json")

execute_process(
    COMMAND
        "${CMAKE_COMMAND}"
        "-DPACKAGE_GLOB=${bundled_katago_json_config_package}"
        "-DPACKAGE_EXECUTABLE=lizzieyzy_qt"
        -P "${ROOT}/cmake/VerifyPackage.cmake"
    RESULT_VARIABLE bundled_katago_json_config_result
    OUTPUT_VARIABLE bundled_katago_json_config_output
    ERROR_VARIABLE bundled_katago_json_config_error
)
if(bundled_katago_json_config_result EQUAL 0)
    message(FATAL_ERROR "Package bundling a JSON KataGo config should fail verification")
endif()
if(NOT "${bundled_katago_json_config_output}${bundled_katago_json_config_error}" MATCHES "KataGo config files")
    message(
        FATAL_ERROR
        "Bundled-KataGo JSON config failure should explain the forbidden config: ${bundled_katago_json_config_output}${bundled_katago_json_config_error}")
endif()

set(bundled_model_root "${work_dir}/bundled-model-root")
set(bundled_model_package "${work_dir}/bundled-model.tar.gz")
create_fake_linux_package(
    "${bundled_model_root}"
    "${bundled_model_package}"
    TRUE
    TRUE
    "LizzieYzyQt/share/models/default.bin.gz")

execute_process(
    COMMAND
        "${CMAKE_COMMAND}"
        "-DPACKAGE_GLOB=${bundled_model_package}"
        "-DPACKAGE_EXECUTABLE=lizzieyzy_qt"
        -P "${ROOT}/cmake/VerifyPackage.cmake"
    RESULT_VARIABLE bundled_model_result
    OUTPUT_VARIABLE bundled_model_output
    ERROR_VARIABLE bundled_model_error
)
if(bundled_model_result EQUAL 0)
    message(FATAL_ERROR "Package bundling a KataGo model should fail verification")
endif()
if(NOT "${bundled_model_output}${bundled_model_error}" MATCHES "model files")
    message(FATAL_ERROR "Bundled-model failure should explain the forbidden model: ${bundled_model_output}${bundled_model_error}")
endif()

set(bundled_uncompressed_model_root "${work_dir}/bundled-uncompressed-model-root")
set(bundled_uncompressed_model_package "${work_dir}/bundled-uncompressed-model.tar.gz")
create_fake_linux_package(
    "${bundled_uncompressed_model_root}"
    "${bundled_uncompressed_model_package}"
    TRUE
    TRUE
    "LizzieYzyQt/share/models/default.bin")

execute_process(
    COMMAND
        "${CMAKE_COMMAND}"
        "-DPACKAGE_GLOB=${bundled_uncompressed_model_package}"
        "-DPACKAGE_EXECUTABLE=lizzieyzy_qt"
        -P "${ROOT}/cmake/VerifyPackage.cmake"
    RESULT_VARIABLE bundled_uncompressed_model_result
    OUTPUT_VARIABLE bundled_uncompressed_model_output
    ERROR_VARIABLE bundled_uncompressed_model_error
)
if(bundled_uncompressed_model_result EQUAL 0)
    message(FATAL_ERROR "Package bundling an uncompressed KataGo model should fail verification")
endif()
if(NOT "${bundled_uncompressed_model_output}${bundled_uncompressed_model_error}" MATCHES "model files")
    message(FATAL_ERROR "Bundled-uncompressed-model failure should explain the forbidden model: ${bundled_uncompressed_model_output}${bundled_uncompressed_model_error}")
endif()

set(bundled_onnx_model_root "${work_dir}/bundled-onnx-model-root")
set(bundled_onnx_model_package "${work_dir}/bundled-onnx-model.tar.gz")
create_fake_linux_package(
    "${bundled_onnx_model_root}"
    "${bundled_onnx_model_package}"
    TRUE
    TRUE
    "LizzieYzyQt/share/networks/default.onnx")

execute_process(
    COMMAND
        "${CMAKE_COMMAND}"
        "-DPACKAGE_GLOB=${bundled_onnx_model_package}"
        "-DPACKAGE_EXECUTABLE=lizzieyzy_qt"
        -P "${ROOT}/cmake/VerifyPackage.cmake"
    RESULT_VARIABLE bundled_onnx_model_result
    OUTPUT_VARIABLE bundled_onnx_model_output
    ERROR_VARIABLE bundled_onnx_model_error
)
if(bundled_onnx_model_result EQUAL 0)
    message(FATAL_ERROR "Package bundling an ONNX neural-network model should fail verification")
endif()
if(NOT "${bundled_onnx_model_output}${bundled_onnx_model_error}" MATCHES "model files")
    message(FATAL_ERROR "Bundled-ONNX-model failure should explain the forbidden model: ${bundled_onnx_model_output}${bundled_onnx_model_error}")
endif()

set(bundled_safetensors_model_root "${work_dir}/bundled-safetensors-model-root")
set(bundled_safetensors_model_package "${work_dir}/bundled-safetensors-model.tar.gz")
create_fake_linux_package(
    "${bundled_safetensors_model_root}"
    "${bundled_safetensors_model_package}"
    TRUE
    TRUE
    "LizzieYzyQt/share/resources/katago-model.safetensors.gz")

execute_process(
    COMMAND
        "${CMAKE_COMMAND}"
        "-DPACKAGE_GLOB=${bundled_safetensors_model_package}"
        "-DPACKAGE_EXECUTABLE=lizzieyzy_qt"
        -P "${ROOT}/cmake/VerifyPackage.cmake"
    RESULT_VARIABLE bundled_safetensors_model_result
    OUTPUT_VARIABLE bundled_safetensors_model_output
    ERROR_VARIABLE bundled_safetensors_model_error
)
if(bundled_safetensors_model_result EQUAL 0)
    message(FATAL_ERROR "Package bundling a common neural-network model outside model directories should fail verification")
endif()
if(NOT "${bundled_safetensors_model_output}${bundled_safetensors_model_error}" MATCHES "model files")
    message(
        FATAL_ERROR
        "Bundled-safetensors-model failure should explain the forbidden model: ${bundled_safetensors_model_output}${bundled_safetensors_model_error}")
endif()

set(bundled_leelaz_engine_root "${work_dir}/bundled-leelaz-engine-root")
set(bundled_leelaz_engine_package "${work_dir}/bundled-leelaz-engine.tar.gz")
create_fake_linux_package(
    "${bundled_leelaz_engine_root}"
    "${bundled_leelaz_engine_package}"
    TRUE
    TRUE
    "LizzieYzyQt/bin/leelaz")

execute_process(
    COMMAND
        "${CMAKE_COMMAND}"
        "-DPACKAGE_GLOB=${bundled_leelaz_engine_package}"
        "-DPACKAGE_EXECUTABLE=lizzieyzy_qt"
        -P "${ROOT}/cmake/VerifyPackage.cmake"
    RESULT_VARIABLE bundled_leelaz_engine_result
    OUTPUT_VARIABLE bundled_leelaz_engine_output
    ERROR_VARIABLE bundled_leelaz_engine_error
)
if(bundled_leelaz_engine_result EQUAL 0)
    message(FATAL_ERROR "Package bundling a Leela Zero engine should fail verification")
endif()
if(NOT "${bundled_leelaz_engine_output}${bundled_leelaz_engine_error}" MATCHES "non-KataGo engine artifacts")
    message(
        FATAL_ERROR
        "Bundled-LeelaZero failure should explain the forbidden engine: ${bundled_leelaz_engine_output}${bundled_leelaz_engine_error}")
endif()

set(bundled_gnugo_engine_root "${work_dir}/bundled-gnugo-engine-root")
set(bundled_gnugo_engine_package "${work_dir}/bundled-gnugo-engine.tar.gz")
create_fake_linux_package(
    "${bundled_gnugo_engine_root}"
    "${bundled_gnugo_engine_package}"
    TRUE
    TRUE
    "LizzieYzyQt/tools/GnuGo.EXE")

execute_process(
    COMMAND
        "${CMAKE_COMMAND}"
        "-DPACKAGE_GLOB=${bundled_gnugo_engine_package}"
        "-DPACKAGE_EXECUTABLE=lizzieyzy_qt"
        -P "${ROOT}/cmake/VerifyPackage.cmake"
    RESULT_VARIABLE bundled_gnugo_engine_result
    OUTPUT_VARIABLE bundled_gnugo_engine_output
    ERROR_VARIABLE bundled_gnugo_engine_error
)
if(bundled_gnugo_engine_result EQUAL 0)
    message(FATAL_ERROR "Package bundling a GnuGo engine should fail verification")
endif()
if(NOT "${bundled_gnugo_engine_output}${bundled_gnugo_engine_error}" MATCHES "non-KataGo engine artifacts")
    message(
        FATAL_ERROR
        "Bundled-GnuGo failure should explain the forbidden engine: ${bundled_gnugo_engine_output}${bundled_gnugo_engine_error}")
endif()

set(bundled_pachi_engine_root "${work_dir}/bundled-pachi-engine-root")
set(bundled_pachi_engine_package "${work_dir}/bundled-pachi-engine.tar.gz")
create_fake_linux_package(
    "${bundled_pachi_engine_root}"
    "${bundled_pachi_engine_package}"
    TRUE
    TRUE
    "LizzieYzyQt/share/engines/pachi-fuego.cfg")

execute_process(
    COMMAND
        "${CMAKE_COMMAND}"
        "-DPACKAGE_GLOB=${bundled_pachi_engine_package}"
        "-DPACKAGE_EXECUTABLE=lizzieyzy_qt"
        -P "${ROOT}/cmake/VerifyPackage.cmake"
    RESULT_VARIABLE bundled_pachi_engine_result
    OUTPUT_VARIABLE bundled_pachi_engine_output
    ERROR_VARIABLE bundled_pachi_engine_error
)
if(bundled_pachi_engine_result EQUAL 0)
    message(FATAL_ERROR "Package bundling a Pachi/Fuego engine config should fail verification")
endif()
if(NOT "${bundled_pachi_engine_output}${bundled_pachi_engine_error}" MATCHES "non-KataGo engine artifacts")
    message(
        FATAL_ERROR
        "Bundled-Pachi/Fuego failure should explain the forbidden engine: ${bundled_pachi_engine_output}${bundled_pachi_engine_error}")
endif()

set(bundled_java_archive_root "${work_dir}/bundled-java-archive-root")
set(bundled_java_archive_package "${work_dir}/bundled-java-archive.tar.gz")
create_fake_linux_package(
    "${bundled_java_archive_root}"
    "${bundled_java_archive_package}"
    TRUE
    TRUE
    "LizzieYzyQt/lib/lizzie-java-ui.jar")

execute_process(
    COMMAND
        "${CMAKE_COMMAND}"
        "-DPACKAGE_GLOB=${bundled_java_archive_package}"
        "-DPACKAGE_EXECUTABLE=lizzieyzy_qt"
        -P "${ROOT}/cmake/VerifyPackage.cmake"
    RESULT_VARIABLE bundled_java_archive_result
    OUTPUT_VARIABLE bundled_java_archive_output
    ERROR_VARIABLE bundled_java_archive_error
)
if(bundled_java_archive_result EQUAL 0)
    message(FATAL_ERROR "Package bundling a Java archive should fail verification")
endif()
if(NOT "${bundled_java_archive_output}${bundled_java_archive_error}" MATCHES "Java archives or bytecode")
    message(FATAL_ERROR "Bundled-Java archive failure should explain the forbidden artifact: ${bundled_java_archive_output}${bundled_java_archive_error}")
endif()

set(bundled_java_archive_uppercase_root "${work_dir}/bundled-java-archive-uppercase-root")
set(bundled_java_archive_uppercase_package "${work_dir}/bundled-java-archive-uppercase.tar.gz")
create_fake_linux_package(
    "${bundled_java_archive_uppercase_root}"
    "${bundled_java_archive_uppercase_package}"
    TRUE
    TRUE
    "LizzieYzyQt/lib/OldSwingUI.JAR")

execute_process(
    COMMAND
        "${CMAKE_COMMAND}"
        "-DPACKAGE_GLOB=${bundled_java_archive_uppercase_package}"
        "-DPACKAGE_EXECUTABLE=lizzieyzy_qt"
        -P "${ROOT}/cmake/VerifyPackage.cmake"
    RESULT_VARIABLE bundled_java_archive_uppercase_result
    OUTPUT_VARIABLE bundled_java_archive_uppercase_output
    ERROR_VARIABLE bundled_java_archive_uppercase_error
)
if(bundled_java_archive_uppercase_result EQUAL 0)
    message(FATAL_ERROR "Package bundling a case-varied Java archive should fail verification")
endif()
if(NOT "${bundled_java_archive_uppercase_output}${bundled_java_archive_uppercase_error}" MATCHES "Java archives or bytecode")
    message(FATAL_ERROR "Bundled-Java archive uppercase failure should explain the forbidden artifact: ${bundled_java_archive_uppercase_output}${bundled_java_archive_uppercase_error}")
endif()

set(bundled_java_executable_root "${work_dir}/bundled-java-executable-root")
set(bundled_java_executable_package "${work_dir}/bundled-java-executable.tar.gz")
create_fake_linux_package(
    "${bundled_java_executable_root}"
    "${bundled_java_executable_package}"
    TRUE
    TRUE
    "LizzieYzyQt/bin/java")

execute_process(
    COMMAND
        "${CMAKE_COMMAND}"
        "-DPACKAGE_GLOB=${bundled_java_executable_package}"
        "-DPACKAGE_EXECUTABLE=lizzieyzy_qt"
        -P "${ROOT}/cmake/VerifyPackage.cmake"
    RESULT_VARIABLE bundled_java_executable_result
    OUTPUT_VARIABLE bundled_java_executable_output
    ERROR_VARIABLE bundled_java_executable_error
)
if(bundled_java_executable_result EQUAL 0)
    message(FATAL_ERROR "Package bundling a Java runtime executable should fail verification")
endif()
if(NOT "${bundled_java_executable_output}${bundled_java_executable_error}" MATCHES "Java runtime executables")
    message(FATAL_ERROR "Bundled-Java executable failure should explain the forbidden executable: ${bundled_java_executable_output}${bundled_java_executable_error}")
endif()

set(bundled_javaw_executable_root "${work_dir}/bundled-javaw-executable-root")
set(bundled_javaw_executable_package "${work_dir}/bundled-javaw-executable.tar.gz")
create_fake_linux_package(
    "${bundled_javaw_executable_root}"
    "${bundled_javaw_executable_package}"
    TRUE
    TRUE
    "LizzieYzyQt/bin/javaw.exe")

execute_process(
    COMMAND
        "${CMAKE_COMMAND}"
        "-DPACKAGE_GLOB=${bundled_javaw_executable_package}"
        "-DPACKAGE_EXECUTABLE=lizzieyzy_qt"
        -P "${ROOT}/cmake/VerifyPackage.cmake"
    RESULT_VARIABLE bundled_javaw_executable_result
    OUTPUT_VARIABLE bundled_javaw_executable_output
    ERROR_VARIABLE bundled_javaw_executable_error
)
if(bundled_javaw_executable_result EQUAL 0)
    message(FATAL_ERROR "Package bundling a Java windowed runtime executable should fail verification")
endif()
if(NOT "${bundled_javaw_executable_output}${bundled_javaw_executable_error}" MATCHES "Java runtime executables")
    message(FATAL_ERROR "Bundled-Javaw executable failure should explain the forbidden executable: ${bundled_javaw_executable_output}${bundled_javaw_executable_error}")
endif()

set(bundled_java_executable_uppercase_root "${work_dir}/bundled-java-executable-uppercase-root")
set(bundled_java_executable_uppercase_package "${work_dir}/bundled-java-executable-uppercase.tar.gz")
create_fake_linux_package(
    "${bundled_java_executable_uppercase_root}"
    "${bundled_java_executable_uppercase_package}"
    TRUE
    TRUE
    "LizzieYzyQt/bin/JAVA.EXE")

execute_process(
    COMMAND
        "${CMAKE_COMMAND}"
        "-DPACKAGE_GLOB=${bundled_java_executable_uppercase_package}"
        "-DPACKAGE_EXECUTABLE=lizzieyzy_qt"
        -P "${ROOT}/cmake/VerifyPackage.cmake"
    RESULT_VARIABLE bundled_java_executable_uppercase_result
    OUTPUT_VARIABLE bundled_java_executable_uppercase_output
    ERROR_VARIABLE bundled_java_executable_uppercase_error
)
if(bundled_java_executable_uppercase_result EQUAL 0)
    message(FATAL_ERROR "Package bundling a case-varied Java runtime executable should fail verification")
endif()
if(NOT "${bundled_java_executable_uppercase_output}${bundled_java_executable_uppercase_error}" MATCHES "Java runtime executables")
    message(FATAL_ERROR "Bundled-Java executable uppercase failure should explain the forbidden executable: ${bundled_java_executable_uppercase_output}${bundled_java_executable_uppercase_error}")
endif()

set(bundled_jre_root "${work_dir}/bundled-jre-root")
set(bundled_jre_package "${work_dir}/bundled-jre.tar.gz")
create_fake_linux_package(
    "${bundled_jre_root}"
    "${bundled_jre_package}"
    TRUE
    TRUE
    "LizzieYzyQt/jre/bin/java")

execute_process(
    COMMAND
        "${CMAKE_COMMAND}"
        "-DPACKAGE_GLOB=${bundled_jre_package}"
        "-DPACKAGE_EXECUTABLE=lizzieyzy_qt"
        -P "${ROOT}/cmake/VerifyPackage.cmake"
    RESULT_VARIABLE bundled_jre_result
    OUTPUT_VARIABLE bundled_jre_output
    ERROR_VARIABLE bundled_jre_error
)
if(bundled_jre_result EQUAL 0)
    message(FATAL_ERROR "Package bundling a Java runtime directory should fail verification")
endif()
if(NOT "${bundled_jre_output}${bundled_jre_error}" MATCHES "Java runtime directories")
    message(FATAL_ERROR "Bundled-JRE failure should explain the forbidden directory: ${bundled_jre_output}${bundled_jre_error}")
endif()

set(bundled_jdk_root "${work_dir}/bundled-jdk-root")
set(bundled_jdk_package "${work_dir}/bundled-jdk.tar.gz")
create_fake_linux_package(
    "${bundled_jdk_root}"
    "${bundled_jdk_package}"
    TRUE
    TRUE
    "LizzieYzyQt/jdk/bin/java")

execute_process(
    COMMAND
        "${CMAKE_COMMAND}"
        "-DPACKAGE_GLOB=${bundled_jdk_package}"
        "-DPACKAGE_EXECUTABLE=lizzieyzy_qt"
        -P "${ROOT}/cmake/VerifyPackage.cmake"
    RESULT_VARIABLE bundled_jdk_result
    OUTPUT_VARIABLE bundled_jdk_output
    ERROR_VARIABLE bundled_jdk_error
)
if(bundled_jdk_result EQUAL 0)
    message(FATAL_ERROR "Package bundling a Java Development Kit directory should fail verification")
endif()
if(NOT "${bundled_jdk_output}${bundled_jdk_error}" MATCHES "Java runtime directories")
    message(FATAL_ERROR "Bundled-JDK failure should explain the forbidden directory: ${bundled_jdk_output}${bundled_jdk_error}")
endif()

set(bundled_lib_jvm_root "${work_dir}/bundled-lib-jvm-root")
set(bundled_lib_jvm_package "${work_dir}/bundled-lib-jvm.tar.gz")
create_fake_linux_package(
    "${bundled_lib_jvm_root}"
    "${bundled_lib_jvm_package}"
    TRUE
    TRUE
    "LizzieYzyQt/lib/jvm/java-21/lib/server/libjvm.so")

execute_process(
    COMMAND
        "${CMAKE_COMMAND}"
        "-DPACKAGE_GLOB=${bundled_lib_jvm_package}"
        "-DPACKAGE_EXECUTABLE=lizzieyzy_qt"
        -P "${ROOT}/cmake/VerifyPackage.cmake"
    RESULT_VARIABLE bundled_lib_jvm_result
    OUTPUT_VARIABLE bundled_lib_jvm_output
    ERROR_VARIABLE bundled_lib_jvm_error
)
if(bundled_lib_jvm_result EQUAL 0)
    message(FATAL_ERROR "Package bundling a lib/jvm runtime tree should fail verification")
endif()
if(NOT "${bundled_lib_jvm_output}${bundled_lib_jvm_error}" MATCHES "Java runtime directories")
    message(FATAL_ERROR "Bundled-lib-jvm failure should explain the forbidden directory: ${bundled_lib_jvm_output}${bundled_lib_jvm_error}")
endif()

set(bundled_jvm_library_root "${work_dir}/bundled-jvm-library-root")
set(bundled_jvm_library_package "${work_dir}/bundled-jvm-library.tar.gz")
create_fake_linux_package(
    "${bundled_jvm_library_root}"
    "${bundled_jvm_library_package}"
    TRUE
    TRUE
    "LizzieYzyQt/runtime/bin/server/jvm.dll")

execute_process(
    COMMAND
        "${CMAKE_COMMAND}"
        "-DPACKAGE_GLOB=${bundled_jvm_library_package}"
        "-DPACKAGE_EXECUTABLE=lizzieyzy_qt"
        -P "${ROOT}/cmake/VerifyPackage.cmake"
    RESULT_VARIABLE bundled_jvm_library_result
    OUTPUT_VARIABLE bundled_jvm_library_output
    ERROR_VARIABLE bundled_jvm_library_error
)
if(bundled_jvm_library_result EQUAL 0)
    message(FATAL_ERROR "Package bundling a Java runtime library should fail verification")
endif()
if(NOT "${bundled_jvm_library_output}${bundled_jvm_library_error}" MATCHES "Java runtime libraries")
    message(FATAL_ERROR "Bundled-JVM-library failure should explain the forbidden library: ${bundled_jvm_library_output}${bundled_jvm_library_error}")
endif()

set(bundled_jvm_library_uppercase_root "${work_dir}/bundled-jvm-library-uppercase-root")
set(bundled_jvm_library_uppercase_package "${work_dir}/bundled-jvm-library-uppercase.tar.gz")
create_fake_linux_package(
    "${bundled_jvm_library_uppercase_root}"
    "${bundled_jvm_library_uppercase_package}"
    TRUE
    TRUE
    "LizzieYzyQt/runtime/bin/server/JVM.DLL")

execute_process(
    COMMAND
        "${CMAKE_COMMAND}"
        "-DPACKAGE_GLOB=${bundled_jvm_library_uppercase_package}"
        "-DPACKAGE_EXECUTABLE=lizzieyzy_qt"
        -P "${ROOT}/cmake/VerifyPackage.cmake"
    RESULT_VARIABLE bundled_jvm_library_uppercase_result
    OUTPUT_VARIABLE bundled_jvm_library_uppercase_output
    ERROR_VARIABLE bundled_jvm_library_uppercase_error
)
if(bundled_jvm_library_uppercase_result EQUAL 0)
    message(FATAL_ERROR "Package bundling a case-varied Java runtime library should fail verification")
endif()
if(NOT "${bundled_jvm_library_uppercase_output}${bundled_jvm_library_uppercase_error}" MATCHES "Java runtime libraries")
    message(FATAL_ERROR "Bundled-JVM-library uppercase failure should explain the forbidden library: ${bundled_jvm_library_uppercase_output}${bundled_jvm_library_uppercase_error}")
endif()

set(bundled_java_support_library_root "${work_dir}/bundled-java-support-library-root")
set(bundled_java_support_library_package "${work_dir}/bundled-java-support-library.tar.gz")
create_fake_linux_package(
    "${bundled_java_support_library_root}"
    "${bundled_java_support_library_package}"
    TRUE
    TRUE
    "LizzieYzyQt/runtime/bin/java.dll")

execute_process(
    COMMAND
        "${CMAKE_COMMAND}"
        "-DPACKAGE_GLOB=${bundled_java_support_library_package}"
        "-DPACKAGE_EXECUTABLE=lizzieyzy_qt"
        -P "${ROOT}/cmake/VerifyPackage.cmake"
    RESULT_VARIABLE bundled_java_support_library_result
    OUTPUT_VARIABLE bundled_java_support_library_output
    ERROR_VARIABLE bundled_java_support_library_error
)
if(bundled_java_support_library_result EQUAL 0)
    message(FATAL_ERROR "Package bundling a Java support runtime library should fail verification")
endif()
if(NOT "${bundled_java_support_library_output}${bundled_java_support_library_error}" MATCHES "Java runtime libraries")
    message(FATAL_ERROR "Bundled-Java-support-library failure should explain the forbidden library: ${bundled_java_support_library_output}${bundled_java_support_library_error}")
endif()

set(bundled_awt_library_root "${work_dir}/bundled-awt-library-root")
set(bundled_awt_library_package "${work_dir}/bundled-awt-library.tar.gz")
create_fake_linux_package(
    "${bundled_awt_library_root}"
    "${bundled_awt_library_package}"
    TRUE
    TRUE
    "LizzieYzyQt/runtime/lib/libjawt.so")

execute_process(
    COMMAND
        "${CMAKE_COMMAND}"
        "-DPACKAGE_GLOB=${bundled_awt_library_package}"
        "-DPACKAGE_EXECUTABLE=lizzieyzy_qt"
        -P "${ROOT}/cmake/VerifyPackage.cmake"
    RESULT_VARIABLE bundled_awt_library_result
    OUTPUT_VARIABLE bundled_awt_library_output
    ERROR_VARIABLE bundled_awt_library_error
)
if(bundled_awt_library_result EQUAL 0)
    message(FATAL_ERROR "Package bundling a Java AWT runtime library should fail verification")
endif()
if(NOT "${bundled_awt_library_output}${bundled_awt_library_error}" MATCHES "Java runtime libraries")
    message(FATAL_ERROR "Bundled-AWT-library failure should explain the forbidden library: ${bundled_awt_library_output}${bundled_awt_library_error}")
endif()

set(bundled_remote_ssh_root "${work_dir}/bundled-remote-ssh-root")
set(bundled_remote_ssh_package "${work_dir}/bundled-remote-ssh.tar.gz")
create_fake_linux_package(
    "${bundled_remote_ssh_root}"
    "${bundled_remote_ssh_package}"
    TRUE
    TRUE
    "LizzieYzyQt/plugins/remote-ssh-engine.dll")

execute_process(
    COMMAND
        "${CMAKE_COMMAND}"
        "-DPACKAGE_GLOB=${bundled_remote_ssh_package}"
        "-DPACKAGE_EXECUTABLE=lizzieyzy_qt"
        -P "${ROOT}/cmake/VerifyPackage.cmake"
    RESULT_VARIABLE bundled_remote_ssh_result
    OUTPUT_VARIABLE bundled_remote_ssh_output
    ERROR_VARIABLE bundled_remote_ssh_error
)
if(bundled_remote_ssh_result EQUAL 0)
    message(FATAL_ERROR "Package bundling a remote SSH engine artifact should fail verification")
endif()
if(NOT "${bundled_remote_ssh_output}${bundled_remote_ssh_error}" MATCHES "remote SSH engine artifacts")
    message(
        FATAL_ERROR
        "Bundled-remote-SSH failure should explain the forbidden artifact: ${bundled_remote_ssh_output}${bundled_remote_ssh_error}")
endif()

set(bundled_online_board_root "${work_dir}/bundled-online-board-root")
set(bundled_online_board_package "${work_dir}/bundled-online-board.tar.gz")
create_fake_linux_package(
    "${bundled_online_board_root}"
    "${bundled_online_board_package}"
    TRUE
    TRUE
    "LizzieYzyQt/plugins/fox-online-board.dll")

execute_process(
    COMMAND
        "${CMAKE_COMMAND}"
        "-DPACKAGE_GLOB=${bundled_online_board_package}"
        "-DPACKAGE_EXECUTABLE=lizzieyzy_qt"
        -P "${ROOT}/cmake/VerifyPackage.cmake"
    RESULT_VARIABLE bundled_online_board_result
    OUTPUT_VARIABLE bundled_online_board_output
    ERROR_VARIABLE bundled_online_board_error
)
if(bundled_online_board_result EQUAL 0)
    message(FATAL_ERROR "Package bundling an online board artifact should fail verification")
endif()
if(NOT "${bundled_online_board_output}${bundled_online_board_error}" MATCHES "online board artifacts")
    message(
        FATAL_ERROR
        "Bundled-online-board failure should explain the forbidden artifact: ${bundled_online_board_output}${bundled_online_board_error}")
endif()

set(bundled_screen_sync_root "${work_dir}/bundled-screen-sync-root")
set(bundled_screen_sync_package "${work_dir}/bundled-screen-sync.tar.gz")
create_fake_linux_package(
    "${bundled_screen_sync_root}"
    "${bundled_screen_sync_package}"
    TRUE
    TRUE
    "LizzieYzyQt/plugins/screen-board-sync.so")

execute_process(
    COMMAND
        "${CMAKE_COMMAND}"
        "-DPACKAGE_GLOB=${bundled_screen_sync_package}"
        "-DPACKAGE_EXECUTABLE=lizzieyzy_qt"
        -P "${ROOT}/cmake/VerifyPackage.cmake"
    RESULT_VARIABLE bundled_screen_sync_result
    OUTPUT_VARIABLE bundled_screen_sync_output
    ERROR_VARIABLE bundled_screen_sync_error
)
if(bundled_screen_sync_result EQUAL 0)
    message(FATAL_ERROR "Package bundling a screen board synchronization artifact should fail verification")
endif()
if(NOT "${bundled_screen_sync_output}${bundled_screen_sync_error}" MATCHES "screen board synchronization artifacts")
    message(
        FATAL_ERROR
        "Bundled-screen-sync failure should explain the forbidden artifact: ${bundled_screen_sync_output}${bundled_screen_sync_error}")
endif()

set(bundled_training_root "${work_dir}/bundled-training-root")
set(bundled_training_package "${work_dir}/bundled-training.tar.gz")
create_fake_linux_package(
    "${bundled_training_root}"
    "${bundled_training_package}"
    TRUE
    TRUE
    "LizzieYzyQt/tools/distributed-training-worker")

execute_process(
    COMMAND
        "${CMAKE_COMMAND}"
        "-DPACKAGE_GLOB=${bundled_training_package}"
        "-DPACKAGE_EXECUTABLE=lizzieyzy_qt"
        -P "${ROOT}/cmake/VerifyPackage.cmake"
    RESULT_VARIABLE bundled_training_result
    OUTPUT_VARIABLE bundled_training_output
    ERROR_VARIABLE bundled_training_error
)
if(bundled_training_result EQUAL 0)
    message(FATAL_ERROR "Package bundling a distributed training artifact should fail verification")
endif()
if(NOT "${bundled_training_output}${bundled_training_error}" MATCHES "distributed training artifacts")
    message(
        FATAL_ERROR
        "Bundled-training failure should explain the forbidden artifact: ${bundled_training_output}${bundled_training_error}")
endif()

set(bundled_macos_app_root "${work_dir}/bundled-macos-app-root")
set(bundled_macos_app_package "${work_dir}/bundled-macos-app.tar.gz")
create_fake_linux_package(
    "${bundled_macos_app_root}"
    "${bundled_macos_app_package}"
    TRUE
    TRUE
    "LizzieYzyQt/LizzieYzyQt.app/Contents/MacOS/lizzieyzy_qt")

execute_process(
    COMMAND
        "${CMAKE_COMMAND}"
        "-DPACKAGE_GLOB=${bundled_macos_app_package}"
        "-DPACKAGE_EXECUTABLE=lizzieyzy_qt"
        -P "${ROOT}/cmake/VerifyPackage.cmake"
    RESULT_VARIABLE bundled_macos_app_result
    OUTPUT_VARIABLE bundled_macos_app_output
    ERROR_VARIABLE bundled_macos_app_error
)
if(bundled_macos_app_result EQUAL 0)
    message(FATAL_ERROR "Package bundling a macOS app bundle should fail verification")
endif()
if(NOT "${bundled_macos_app_output}${bundled_macos_app_error}" MATCHES "macOS app bundles")
    message(
        FATAL_ERROR
        "Bundled-macOS-app failure should explain the forbidden bundle: ${bundled_macos_app_output}${bundled_macos_app_error}")
endif()

set(bundled_macos_dylib_root "${work_dir}/bundled-macos-dylib-root")
set(bundled_macos_dylib_package "${work_dir}/bundled-macos-dylib.tar.gz")
create_fake_linux_package(
    "${bundled_macos_dylib_root}"
    "${bundled_macos_dylib_package}"
    TRUE
    TRUE
    "LizzieYzyQt/lib/libqcocoa.dylib")

execute_process(
    COMMAND
        "${CMAKE_COMMAND}"
        "-DPACKAGE_GLOB=${bundled_macos_dylib_package}"
        "-DPACKAGE_EXECUTABLE=lizzieyzy_qt"
        -P "${ROOT}/cmake/VerifyPackage.cmake"
    RESULT_VARIABLE bundled_macos_dylib_result
    OUTPUT_VARIABLE bundled_macos_dylib_output
    ERROR_VARIABLE bundled_macos_dylib_error
)
if(bundled_macos_dylib_result EQUAL 0)
    message(FATAL_ERROR "Package bundling a macOS dynamic library should fail verification")
endif()
if(NOT "${bundled_macos_dylib_output}${bundled_macos_dylib_error}" MATCHES "macOS dynamic libraries")
    message(
        FATAL_ERROR
        "Bundled-macOS-dylib failure should explain the forbidden library: ${bundled_macos_dylib_output}${bundled_macos_dylib_error}")
endif()

set(bundled_macos_framework_root "${work_dir}/bundled-macos-framework-root")
set(bundled_macos_framework_package "${work_dir}/bundled-macos-framework.tar.gz")
create_fake_linux_package(
    "${bundled_macos_framework_root}"
    "${bundled_macos_framework_package}"
    TRUE
    TRUE
    "LizzieYzyQt/Frameworks/QtCore.framework/QtCore")

execute_process(
    COMMAND
        "${CMAKE_COMMAND}"
        "-DPACKAGE_GLOB=${bundled_macos_framework_package}"
        "-DPACKAGE_EXECUTABLE=lizzieyzy_qt"
        -P "${ROOT}/cmake/VerifyPackage.cmake"
    RESULT_VARIABLE bundled_macos_framework_result
    OUTPUT_VARIABLE bundled_macos_framework_output
    ERROR_VARIABLE bundled_macos_framework_error
)
if(bundled_macos_framework_result EQUAL 0)
    message(FATAL_ERROR "Package bundling a macOS framework should fail verification")
endif()
if(NOT "${bundled_macos_framework_output}${bundled_macos_framework_error}" MATCHES "macOS framework bundles")
    message(
        FATAL_ERROR
        "Bundled-macOS-framework failure should explain the forbidden framework: ${bundled_macos_framework_output}${bundled_macos_framework_error}")
endif()

set(bundled_windows_dll_linux_root "${work_dir}/bundled-windows-dll-linux-root")
set(bundled_windows_dll_linux_package "${work_dir}/bundled-windows-dll-linux.tar.gz")
create_fake_linux_package(
    "${bundled_windows_dll_linux_root}"
    "${bundled_windows_dll_linux_package}"
    TRUE
    TRUE
    "LizzieYzyQt/bin/qwindows.dll")
execute_process(
    COMMAND
        "${CMAKE_COMMAND}"
        "-DPACKAGE_GLOB=${bundled_windows_dll_linux_package}"
        "-DPACKAGE_EXECUTABLE=lizzieyzy_qt"
        -P "${ROOT}/cmake/VerifyPackage.cmake"
    RESULT_VARIABLE bundled_windows_dll_linux_result
    OUTPUT_VARIABLE bundled_windows_dll_linux_output
    ERROR_VARIABLE bundled_windows_dll_linux_error
)
if(bundled_windows_dll_linux_result EQUAL 0)
    message(FATAL_ERROR "Linux package bundling a Windows DLL should fail verification")
endif()
if(NOT "${bundled_windows_dll_linux_output}${bundled_windows_dll_linux_error}" MATCHES "Windows runtime artifacts in Linux packages")
    message(
        FATAL_ERROR
        "Bundled-Windows-DLL Linux failure should explain the forbidden artifact: ${bundled_windows_dll_linux_output}${bundled_windows_dll_linux_error}")
endif()

set(bundled_windows_exe_linux_root "${work_dir}/bundled-windows-exe-linux-root")
set(bundled_windows_exe_linux_package "${work_dir}/bundled-windows-exe-linux.tar.gz")
create_fake_linux_package(
    "${bundled_windows_exe_linux_root}"
    "${bundled_windows_exe_linux_package}"
    TRUE
    TRUE
    "LizzieYzyQt/bin/helper.exe")
execute_process(
    COMMAND
        "${CMAKE_COMMAND}"
        "-DPACKAGE_GLOB=${bundled_windows_exe_linux_package}"
        "-DPACKAGE_EXECUTABLE=lizzieyzy_qt"
        -P "${ROOT}/cmake/VerifyPackage.cmake"
    RESULT_VARIABLE bundled_windows_exe_linux_result
    OUTPUT_VARIABLE bundled_windows_exe_linux_output
    ERROR_VARIABLE bundled_windows_exe_linux_error
)
if(bundled_windows_exe_linux_result EQUAL 0)
    message(FATAL_ERROR "Linux package bundling a Windows executable should fail verification")
endif()
if(NOT "${bundled_windows_exe_linux_output}${bundled_windows_exe_linux_error}" MATCHES "Windows runtime artifacts in Linux packages")
    message(
        FATAL_ERROR
        "Bundled-Windows-executable Linux failure should explain the forbidden artifact: ${bundled_windows_exe_linux_output}${bundled_windows_exe_linux_error}")
endif()

set(bundled_linux_so_windows_root "${work_dir}/bundled-linux-so-windows-root")
set(bundled_linux_so_windows_package "${work_dir}/bundled-linux-so-windows.zip")
create_fake_package(
    "${bundled_linux_so_windows_root}"
    "${bundled_linux_so_windows_package}"
    TRUE
    "LizzieYzyQt/bin/libQt6Core.so.6")
execute_process(
    COMMAND
        "${CMAKE_COMMAND}"
        "-DPACKAGE_GLOB=${bundled_linux_so_windows_package}"
        "-DPACKAGE_EXECUTABLE=lizzieyzy_qt.exe"
        "-DPACKAGE_REQUIRE_QT_DEPLOY=ON"
        -P "${ROOT}/cmake/VerifyPackage.cmake"
    RESULT_VARIABLE bundled_linux_so_windows_result
    OUTPUT_VARIABLE bundled_linux_so_windows_output
    ERROR_VARIABLE bundled_linux_so_windows_error
)
if(bundled_linux_so_windows_result EQUAL 0)
    message(FATAL_ERROR "Windows package bundling a Linux shared object should fail verification")
endif()
if(NOT "${bundled_linux_so_windows_output}${bundled_linux_so_windows_error}" MATCHES "Linux runtime artifacts in Windows packages")
    message(
        FATAL_ERROR
        "Bundled-Linux-shared-object Windows failure should explain the forbidden artifact: ${bundled_linux_so_windows_output}${bundled_linux_so_windows_error}")
endif()

set(bundled_linux_executable_windows_root "${work_dir}/bundled-linux-executable-windows-root")
set(bundled_linux_executable_windows_package "${work_dir}/bundled-linux-executable-windows.zip")
create_fake_package(
    "${bundled_linux_executable_windows_root}"
    "${bundled_linux_executable_windows_package}"
    TRUE
    "LizzieYzyQt/bin/lizzieyzy_qt")
execute_process(
    COMMAND
        "${CMAKE_COMMAND}"
        "-DPACKAGE_GLOB=${bundled_linux_executable_windows_package}"
        "-DPACKAGE_EXECUTABLE=lizzieyzy_qt.exe"
        "-DPACKAGE_REQUIRE_QT_DEPLOY=ON"
        -P "${ROOT}/cmake/VerifyPackage.cmake"
    RESULT_VARIABLE bundled_linux_executable_windows_result
    OUTPUT_VARIABLE bundled_linux_executable_windows_output
    ERROR_VARIABLE bundled_linux_executable_windows_error
)
if(bundled_linux_executable_windows_result EQUAL 0)
    message(FATAL_ERROR "Windows package bundling a Linux executable should fail verification")
endif()
if(NOT "${bundled_linux_executable_windows_output}${bundled_linux_executable_windows_error}" MATCHES "Linux runtime artifacts in Windows packages")
    message(
        FATAL_ERROR
        "Bundled-Linux-executable Windows failure should explain the forbidden artifact: ${bundled_linux_executable_windows_output}${bundled_linux_executable_windows_error}")
endif()

set(valid_root "${work_dir}/valid-root")
set(valid_package "${work_dir}/valid.zip")
create_fake_package("${valid_root}" "${valid_package}" TRUE)

execute_process(
    COMMAND
        "${CMAKE_COMMAND}"
        "-DPACKAGE_GLOB=${valid_package}"
        "-DPACKAGE_EXECUTABLE=lizzieyzy_qt.exe"
        "-DPACKAGE_REQUIRE_QT_DEPLOY=ON"
        -P "${ROOT}/cmake/VerifyPackage.cmake"
    RESULT_VARIABLE valid_result
    OUTPUT_VARIABLE valid_output
    ERROR_VARIABLE valid_error
)
if(NOT valid_result EQUAL 0)
    message(FATAL_ERROR "Valid fake Windows package should pass verification: ${valid_output}${valid_error}")
endif()

set(named_windows_root "${work_dir}/named-windows-root")
set(named_windows_package "${work_dir}/LizzieYzyQt-0.1.0-Windows.zip")
create_fake_package("${named_windows_root}" "${named_windows_package}" TRUE)

execute_process(
    COMMAND
        "${CMAKE_COMMAND}"
        "-DPACKAGE_GLOB=${named_windows_package}"
        "-DPACKAGE_EXECUTABLE=lizzieyzy_qt.exe"
        "-DPACKAGE_EXPECT_PLATFORM=Windows"
        "-DPACKAGE_REQUIRE_QT_DEPLOY=ON"
        -P "${ROOT}/cmake/VerifyPackage.cmake"
    RESULT_VARIABLE named_windows_result
    OUTPUT_VARIABLE named_windows_output
    ERROR_VARIABLE named_windows_error
)
if(NOT named_windows_result EQUAL 0)
    message(FATAL_ERROR "Platform-named fake Windows package should pass verification: ${named_windows_output}${named_windows_error}")
endif()

set(minimal_windows_root "${work_dir}/minimal-windows-root")
set(minimal_windows_package "${work_dir}/minimal-windows.zip")
create_fake_package("${minimal_windows_root}" "${minimal_windows_package}" FALSE)

execute_process(
    COMMAND
        "${CMAKE_COMMAND}"
        "-DPACKAGE_GLOB=${minimal_windows_package}"
        "-DPACKAGE_EXECUTABLE=lizzieyzy_qt.exe"
        "-DPACKAGE_REQUIRE_QT_DEPLOY=OFF"
        -P "${ROOT}/cmake/VerifyPackage.cmake"
    RESULT_VARIABLE minimal_windows_result
    OUTPUT_VARIABLE minimal_windows_output
    ERROR_VARIABLE minimal_windows_error
)
if(NOT minimal_windows_result EQUAL 0)
    message(FATAL_ERROR "Windows package verification without deploy requirement should not require Qt DLLs: ${minimal_windows_output}${minimal_windows_error}")
endif()

set(missing_qml_root "${work_dir}/missing-qml-root")
set(missing_qml_package "${work_dir}/missing-qml.zip")
create_fake_package("${missing_qml_root}" "${missing_qml_package}" FALSE)

execute_process(
    COMMAND
        "${CMAKE_COMMAND}"
        "-DPACKAGE_GLOB=${missing_qml_package}"
        "-DPACKAGE_EXECUTABLE=lizzieyzy_qt.exe"
        "-DPACKAGE_REQUIRE_QT_DEPLOY=ON"
        -P "${ROOT}/cmake/VerifyPackage.cmake"
    RESULT_VARIABLE missing_qml_result
    OUTPUT_VARIABLE missing_qml_output
    ERROR_VARIABLE missing_qml_error
)
if(missing_qml_result EQUAL 0)
    message(FATAL_ERROR "Package missing Qt6Qml.dll should fail verification")
endif()
if(NOT "${missing_qml_output}${missing_qml_error}" MATCHES "Qt6Qml\\.dll")
    message(FATAL_ERROR "Missing-QML failure should name Qt6Qml.dll: ${missing_qml_output}${missing_qml_error}")
endif()

message(STATUS "verify_package_script_tests passed")
