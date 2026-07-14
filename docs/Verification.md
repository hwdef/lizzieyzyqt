# Verification Checklist

Use this checklist for the PLAN.md acceptance items that require a real target
machine, a real KataGo installation, or a packaging toolchain.
The first Qt release targets Linux and Windows only; macOS is intentionally not
supported. After running the target-machine checks, fill in
`TargetAcceptanceReport.md` with the relevant diagnostics fields, raw KataGo
comparison notes, and final pass/fail results.

## Build, Test, Install, Package

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --parallel 2 --output-on-failure
cmake --install build --prefix "$PWD/install"
./install/bin/lizzieyzy_qt --version
QT_QPA_PLATFORM=offscreen ./install/bin/lizzieyzy_qt --diagnostics
QT_QPA_PLATFORM=offscreen ./install/bin/lizzieyzy_qt --target-acceptance-report
cmake -E env QT_QPA_PLATFORM=missing-qpa-for-installed-record-template-smoke ./install/bin/lizzieyzy_qt --target-acceptance-record-template
cmake --build build --target package
cmake -DPACKAGE_GLOB="build/LizzieYzyQt-*-Linux.tar.gz" -DPACKAGE_EXECUTABLE=lizzieyzy_qt -DPACKAGE_EXPECT_PLATFORM=Linux -DPACKAGE_RUN_VERSION_SMOKE=ON -P cmake/VerifyPackage.cmake
cmake -DPACKAGE_GLOB="build/LizzieYzyQt-*-Linux.tar.gz" -DPACKAGE_EXECUTABLE=lizzieyzy_qt -DPACKAGE_EXPECT_PLATFORM=Linux -DPACKAGE_RUN_DIAGNOSTICS_SMOKE=ON -DPACKAGE_DIAGNOSTICS_QPA_PLATFORM=offscreen -P cmake/VerifyPackage.cmake
cmake -DPACKAGE_GLOB="build/LizzieYzyQt-*-Linux.tar.gz" -DPACKAGE_EXECUTABLE=lizzieyzy_qt -DPACKAGE_EXPECT_PLATFORM=Linux -DPACKAGE_RUN_TARGET_ACCEPTANCE_RECORD_TEMPLATE_SMOKE=ON -P cmake/VerifyPackage.cmake
cmake -DPACKAGE_GLOB="build/LizzieYzyQt-*-Linux.tar.gz" -DPACKAGE_EXECUTABLE=lizzieyzy_qt -DPACKAGE_EXPECT_PLATFORM=Linux -DPACKAGE_RUN_TARGET_ACCEPTANCE_REPORT_SMOKE=ON -DPACKAGE_TARGET_ACCEPTANCE_REPORT_QPA_PLATFORM=offscreen -P cmake/VerifyPackage.cmake
```

After the real target-machine checks pass, generate an INI skeleton with
`lizzieyzy_qt --target-acceptance-record-template`, store the manual results in
that INI file, and rerun the report with `--target-acceptance-record <ini>` so
`externalTargetVerificationStatus`, `rawKataGoComparisonStatus`,
`manualUiInspectionStatus`, `finalAcceptanceStatus`, and
`finalAcceptanceBlocker` reflect the recorded acceptance state. Scripted runs
can also set `LIZZIE_TARGET_ACCEPTANCE_RECORD_FILE` to the same path.
`finalAcceptanceStatus` reports invalid or failed manual records, including
target component results and checklist item records, before generic readiness
gaps so a bad acceptance record is visible even when the current run also lacks
a target platform or display.
The generated diagnostics include the record INI canonical path, size,
SHA-256 digest, and modification time so archived reports can be tied back to
the exact record file.
Fill each `[checklist.*]` item in addition to any aggregate `[acceptance]`
status; final acceptance keeps `acceptanceChecklist` in
`finalAcceptanceBlocker` until every checklist item is recorded as pass.
`plan.acceptance.checklistMissingRecord` lists unfilled checklist items
separately from failed or invalid records.
The generated `[recordHints]` section repeats the accepted pass/fail values,
required metadata keys, metadata matching rules, and the evidence, SHA-256,
content-marker, and timestamp requirements; it is guidance for filling the
record and is ignored by the acceptance parser. Generated reports and
diagnostics also print the same guidance as
`plan.acceptance.recordHint.*` fields for grep-friendly review logs.
Fill `[evidenceSha256]` with the generated evidence SHA-256 values after
collecting the evidence files. Final acceptance keeps
`acceptanceEvidenceSha256` in `finalAcceptanceBlocker` until every expected
hash is present, valid, and matches the current evidence file.
The evidence paths for diagnostics, target report, engine log, raw KataGo
comparison log, and screenshot must resolve to distinct canonical files; duplicate
paths keep `acceptanceEvidence` in `finalAcceptanceBlocker` even when the hashes
match.
Diagnostics evidence must contain `LizzieYzy Qt Diagnostics`, `app.version`,
`app.executable`, the current app version and executable path,
`plan.acceptance.recordFile.canonicalPath`, the current record canonical path,
and `plan.acceptance.recordFile.sha256`. It must also contain
`plan.acceptance.finalAcceptanceStatus` and the final blocker fields
`plan.acceptance.finalAcceptanceBlocker` and
`plan.acceptance.finalAcceptanceOutstandingBlocker`. Target report evidence
must contain `# Target Acceptance Report`, `app.version`, `app.executable`, the
current app version and executable path,
`plan.acceptance.recordFile.canonicalPath`, the current record canonical path,
`plan.acceptance.recordFile.sha256`, `plan.acceptance.finalAcceptanceStatus`,
and the same final blocker fields.
Engine-log evidence must contain `KataGo` and, when the record `kataGoVersion`
matches the current app's KataGo requirement, the recorded version string. It
must also include at least one GPU/stderr marker such as `CUDA`, `OpenCL`,
`TensorRT`, `GPU`, `backend`, or `stderr:`; the
`plan.acceptance.evidence.engineLog.gpuOrStderrContentStatus` and
`plan.acceptance.evidence.engineLog.missingGpuOrStderrContentMarker` fields
report that target-machine evidence gate. Raw KataGo comparison log evidence must contain `KataGo`, `kata-analyze`,
`rootInfo`, `moveInfos`, `winrate`, `scoreMean`, `scoreStdev`, `visits`,
`policy`, `pv`, `pvVisits`, and `ownership`, and, when the record
`kataGoVersion` matches the current app's KataGo requirement, the recorded
version string. Missing markers keep
`acceptanceEvidence` in `finalAcceptanceBlocker` and are reported through
`contentStatus`.
Set `completedUtc` after collecting the evidence files. Final acceptance keeps
`acceptanceRecordTimestamp` in `finalAcceptanceBlocker` if the record file was
modified later than `completedUtc` plus the built-in clock-skew tolerance, and
keeps `acceptanceEvidenceTimestamp` there if any evidence file was modified
later than the same threshold.

```ini
[acceptance]
linuxKdeWaylandNvidia=pass
windowsNvidia=pass
physicalDisplay=pass
externalTargetVerification=pass
rawKataGoComparison=pass
manualUiInspection=pass

[evidence]
diagnostics=/path/to/lizzieyzy-diagnostics.txt
targetAcceptanceReport=/path/to/target-acceptance-report.md
engineLog=/path/to/katago-engine.log
rawKataGoComparisonLog=/path/to/raw-katago-comparison.log
manualUiInspectionLog=/path/to/manual-ui-inspection.log
externalTargetVerificationLog=/path/to/external-target-verification.log
screenshot=/path/to/4k-high-dpi-ui.png

[evidenceSha256]
diagnostics=
targetAcceptanceReport=
engineLog=
rawKataGoComparisonLog=
manualUiInspectionLog=
externalTargetVerificationLog=
screenshot=

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

[checklist.linuxKdeWaylandNvidia]
linuxOs=pass
kdeSession=pass
waylandSession=pass
qwaylandPlatformPlugin=pass
nvidiaEnvironmentHint=pass
packageStarts=pass
configureKataGoPaths=pass
realtimeAnalyzeCurrentPosition=pass
branchResync=pass
gpuStderrCaptured=pass

[checklist.windowsNvidia]
windowsOs=pass
qwindowsPlatformPlugin=pass
packageExtracts=pass
appLocalQtRuntime=pass
nvidiaEnvironmentHint=pass
configureKataGoPaths=pass
nativeSettingsPathDialog=pass
realtimeAnalyzeCurrentPosition=pass
engineDiagnosticsCaptured=pass

[checklist.physicalDisplay]
physical4KPanel=pass
highDpiScale150Or200=pass
multiDisplayLayout=pass
boardTextSharpness=pass
candidateLabelsNoOverlap=pass
pvPreviewNoOverlap=pass
ownershipOverlayReadable=pass
winrateScoreGraphReadable=pass
restoredWindowVisible=pass

[checklist.rawKataGoComparison]
analysisJsonRawResponse=pass
realtimeGtpRawLine=pass
candidateTableRendering=pass
pvPreview=pass
winrateScorePerspective=pass
ownershipDisplay=pass
genmove=pass
humanVsAi=pass
selfPlay=pass
engineVsEngine=pass

[checklist.externalTargetVerification]
linuxKdeWaylandNvidiaRealtimeKataGo=pass
windowsNvidiaRealtimeKataGo=pass
physical4KHighDpiMultiDisplayUi=pass
rawKataGoUiComparison=pass

[checklist.manualUiInspection]
mainWindow4KHighDpiLayout=pass
boardLinesCoordinatesAndStarPoints=pass
stoneRenderingAndCandidateLabels=pass
candidateTableColumns=pass
pvPreviewStones=pass
ownershipMainBoardOverlay=pass
ownershipMiniBoardDock=pass
winrateScoreGraph=pass
toolbarDockAndMenuLayout=pass
bilingualTextFit=pass
savedWindowRestore=pass
multiDisplayPlacement=pass
```

Evidence paths can be absolute or relative; relative paths are resolved from the
directory containing the target acceptance record file, and each evidence path
must point at a readable, non-empty file.
If the record path starts with `-`, pass it as
`--target-acceptance-record -- <ini>` so it is not confused with a CLI option.
The target acceptance report and diagnostics include evidence file sizes and
SHA-256 digests so attached artifacts can be matched to the recorded result.
For screenshot evidence they also report image readability, detected format,
pixel width/height, `imagePixelsAtLeast4K`, and `imageHasPixelVariation` so 4K
display evidence is auditable from the saved logs. The final acceptance gate
keeps `screenshotEvidence4K` in `finalAcceptanceBlocker` until the screenshot
evidence is readable, reaches that 4K pixel envelope, and is not a uniform image.
The final acceptance gate requires the key metadata fields `completedUtc`,
`appVersion`, `appExecutableSha256`, `osPrettyName`, `osKernelType`,
`osKernelVersion`, `qtRuntimeVersion`, `qtBuildAbi`, `cpuArchitecture`,
`reviewer`, `targetMachine`, `gpuDriver`, and `kataGoVersion` to be present.
`completedUtc` must be an ISO UTC timestamp ending in `Z` and must not be in
the future; `plan.acceptance.record.completedUtcStatus` reports `ready`,
`missing`, `invalid-format`, or `future`. `appVersion` must match the generated
`app.version`; `plan.acceptance.record.appVersionStatus` reports `match`,
`missing`, or `mismatch`. `appExecutableSha256` must match the SHA-256 digest of
the current `app.executable`; `plan.acceptance.record.appExecutableSha256Status`
reports `match`, `missing`, `invalid`, `current-unavailable`, or `mismatch`.
The OS, Qt runtime, CPU, and target-machine metadata must match the current
`os.*`, `qt.*`, `cpu.*`, and machine host-name values; their status fields
report `match`, `missing`, `current-unavailable`, or `mismatch`.
`gpuDriver` must identify NVIDIA; `plan.acceptance.record.gpuDriverStatus`
reports `match`, `missing`, or `mismatch`.
`kataGoVersion` must identify KataGo; `plan.acceptance.record.kataGoVersionStatus`
reports `match`, `missing`, or `mismatch`.
The record-template command runs before Qt platform initialization, so it can
still generate the INI skeleton if display or platform-plugin setup is broken.

```bash
./install/bin/lizzieyzy_qt --target-acceptance-report --target-acceptance-record target-acceptance-record.ini
./install/bin/lizzieyzy_qt --diagnostics --target-acceptance-record target-acceptance-record.ini
```

The local CTest suite includes `lizzie_engine_worker_tests`, which verifies that
the UI-facing engine proxies create realtime GTP and batch analysis worker
objects on dedicated threads while preserving startup probes, command ids, and
analysis responses through signals.
CI also runs an installed diagnostics smoke, an installed target acceptance
report smoke, and an installed target acceptance record template smoke before
packaging so install-tree layout problems are caught before the archive is
created. The installed target acceptance record template smoke uses
`missing-qpa-for-installed-record-template-smoke` to prove the template remains
available when display initialization is broken.

Expected package output:

- Linux: `build/LizzieYzyQt-<version>-Linux.tar.gz`
- Windows: `build/LizzieYzyQt-<version>-Windows.zip`

Pass `-DPACKAGE_EXPECT_PLATFORM=Linux` or
`-DPACKAGE_EXPECT_PLATFORM=Windows` so `VerifyPackage.cmake` rejects archives
whose platform-specific package filenames do not match the target package.
On Windows, use `-DPACKAGE_GLOB="build/LizzieYzyQt-*-Windows.zip"` and
`-DPACKAGE_EXECUTABLE=lizzieyzy_qt.exe -DPACKAGE_EXPECT_PLATFORM=Windows
-DPACKAGE_REQUIRE_QT_DEPLOY=ON
-DPACKAGE_RUN_VERSION_SMOKE=ON` for the package contents and executable smoke
check. Add `-DPACKAGE_RUN_DIAGNOSTICS_SMOKE=ON` to match CI's packaged
diagnostics smoke; CI runs Windows diagnostics with
`-DPACKAGE_DIAGNOSTICS_QPA_PLATFORM=windows` so the native platform plugin is
exercised explicitly. Add
`-DPACKAGE_RUN_TARGET_ACCEPTANCE_RECORD_TEMPLATE_SMOKE=ON` for the independent
packaged target acceptance record template smoke; it runs
`--target-acceptance-record-template` with a deliberately invalid QPA platform
so the acceptance record skeleton stays usable when display initialization is
broken. Add `-DPACKAGE_RUN_TARGET_ACCEPTANCE_REPORT_SMOKE=ON` with
`-DPACKAGE_TARGET_ACCEPTANCE_REPORT_QPA_PLATFORM=windows` for the packaged
target acceptance report CLI smoke. Run
`.\install\qt\bin\lizzieyzy_qt.exe --version`,
`.\install\qt\bin\lizzieyzy_qt.exe --diagnostics`, and
`.\install\qt\bin\lizzieyzy_qt.exe --target-acceptance-report` after
`cmake --install` for the installed-executable smoke. Also run
`cmake -E env QT_QPA_PLATFORM=missing-qpa-for-installed-record-template-smoke
.\install\qt\bin\lizzieyzy_qt.exe --target-acceptance-record-template` so the
installed acceptance record skeleton path is checked independently of the
native display plugin.

The Linux archive contains the application binary and expects compatible system
Qt/KDE runtime packages on the target machine. The Windows archive should include
the Qt runtime libraries and plugins selected by Qt's deploy tooling; the
package verifier checks the app executable, documentation, the Qt Core/Gui/
Widgets/Quick/QuickWidgets/Qml/Network/OpenGL DLLs, and
`platforms/qwindows.dll`. Linux package verification rejects Windows runtime
artifacts so the tarball cannot accidentally carry `.dll`, `.pdb`, `.lib`, or
`bin/*.exe` files from a Windows build. Windows package verification rejects
Linux runtime artifacts so the zip cannot accidentally carry `.so`, AppImage, or
the extensionless Linux executable from a Linux build.
For both platforms, package verification also rejects bundled KataGo binaries,
KataGo config files, KataGo neural-network model files, Java archives or
bytecode, and bundled Java runtime directories, executables, and runtime
libraries, non-KataGo engine artifacts, plus out-of-scope artifacts for remote
SSH engines, online board integrations, screen board synchronization, distributed training, and macOS artifacts. Model rejection covers KataGo `.bin/.txt` weights plus common
neural-network weight formats throughout the package, with uncompressed
`.bin/.txt` weights also rejected under model, net, or network directories.
KataGo config rejection covers common config file extensions plus config-named
JSON files. The
packaged diagnostics smoke creates temporary `LIZZIE_KATAGO_*` fixture paths so
the package can verify env-path status reporting without bundling KataGo or
requiring a real engine on the packaging machine, and reruns diagnostics with
missing, blank, and invalid KataGo env paths to verify the `ready`, `missing`,
and `invalid` readiness summaries. It also supplies a temporary
`LIZZIE_DIAGNOSTICS_SETTINGS_FILE` so saved engine setting path diagnostics and
stored analysis option diagnostics are checked without touching user settings.
The packaged target acceptance report smoke runs `--target-acceptance-report`
from the extracted archive with temporary `LIZZIE_KATAGO_*` fixture paths and
saved main/compare engine config fixtures. It checks that the Markdown report
includes ready KataGo env status, ready saved config status, dual-engine config
readiness, the PLAN acceptance status, target-specific verification status
fields, raw KataGo comparison section, and final acceptance section.
It also runs a mode-specific final blocker package smoke with only saved main
GTP config present, verifying that `finalAcceptanceBlocker` reports
`mainBatchAnalysisConfig`, `compareRealtimeGtpConfig`, and
`compareBatchAnalysisConfig` instead of coarse engine-config blockers.
The same packaged target acceptance report smoke also covers complete 4K
screenshot evidence blocker removal and complete checklist final blocker
removal by replaying explicit target acceptance records with a 4K screenshot
fixture and all checklist items set to pass.
The packaged diagnostics smoke output, and the raw diagnostics attached to a
completed target-machine report, include each KataGo env path value, nonblank
text status, absolute/canonical path, existence/readability/writability, and
file size/modification time, a KataGo env readiness summary, plus the packaged app executable path, app directory,
app-local Qt runtime artifact diagnostics, runtime Qt library paths, and path-status fields for the app executable,
app directory, Qt plugin path, runtime Qt library paths, QStandardPaths writable locations,
application font/text metrics, locale/UI-language diagnostics, runtime appearance style/palette diagnostics,
native file dialog/settings path selector diagnostics,
main-window UI structure diagnostics,
QSettings storage location,
stored and normalized appearance settings, saved engine setting path diagnostics,
stored engine path-readiness diagnostics including missing/invalid saved settings coverage,
stored analysis option diagnostics,
first-run completion diagnostics,
stored board display diagnostics, stored file behavior diagnostics,
stored shortcut diagnostics,
QSettings session/window restore keys, saved session SGF path-status,
saved window-geometry visibility checks,
and current platform plugin candidates, common Wayland platform plugin candidates, common Windows platform plugin candidates,
target-platform summary diagnostics,
target-platform blocker diagnostics,
display blocker diagnostics,
PLAN acceptance summary diagnostics under `plan.acceptance.*`,
plus available platform plugin listing so Qt plugin loading
can be checked against the extracted package layout. Filesystem path-status
fields include existence, type, readability, writability, executable status, size, and
modification time where applicable. It also records Qt Quick
scene graph, QML import paths, QML import environment, Qt Quick Controls configuration paths,
renderer-interface RHI/shader fields, OpenGL runtime context diagnostics for
context creation, format, vendor, renderer, and version strings, OS
product/kernel, Qt build ABI, build/current CPU architecture, Qt build/runtime version match,
Qt install paths, primary screen geometry, screen
physical size, refresh rate, and average/per-axis DPI,
screen orientation, device-pixel geometry, `screen.0.virtualSibling.count`, Qt
plugin, scale, style, logging, and extended graphics environment diagnostics plus common
KDE Wayland/NVIDIA/Vulkan/CUDA selection variables such as
`QT_PLUGIN_PATH`, `QT_QPA_PLATFORMTHEME`, `QT_SCREEN_SCALE_FACTORS`,
`QT_SCALE_FACTOR_ROUNDING_POLICY`, `QT_USE_PHYSICAL_DPI`,
`QT_DPI_ADJUSTMENT_POLICY`, `GBM_BACKEND`, `KWIN_DRM_DEVICES`,
`QSG_RHI_BACKEND`, `QT_RHI_BACKEND`,
`QSG_INFO`, `QSG_RENDER_LOOP`, `QSG_VISUALIZE`, `QSG_RENDERER_DEBUG`,
`QSG_RHI_DEBUG_LAYER`, `QSG_RHI_PREFER_SOFTWARE_RENDERER`,
`LIBGL_ALWAYS_SOFTWARE`, `LIBGL_DEBUG`, `EGL_PLATFORM`,
`__EGL_VENDOR_LIBRARY_FILENAMES`, `__NV_PRIME_RENDER_OFFLOAD`,
`__NV_PRIME_RENDER_OFFLOAD_PROVIDER`, `VK_INSTANCE_LAYERS`,
`VK_LOADER_DEBUG`, `VK_ICD_FILENAMES`, `VK_DRIVER_FILES`,
`NVIDIA_VISIBLE_DEVICES`, and `CUDA_DEVICE_ORDER`. Path-list environment
diagnostics expand `PATH`, `LD_LIBRARY_PATH`, `DYLD_LIBRARY_PATH`,
`QML_IMPORT_PATH`, `QML2_IMPORT_PATH`, `QT_QUICK_CONTROLS_STYLE_PATH`,
`QT_QPA_PLATFORM_PLUGIN_PATH`, `QT_PLUGIN_PATH`, `KWIN_DRM_DEVICES`,
`VK_ICD_FILENAMES`, `VK_DRIVER_FILES`, and
`__EGL_VENDOR_LIBRARY_FILENAMES` into entry counts and per-entry
path-status fields. Set-empty environment variables print `(empty)`, unset
variables print `(unset)`, and blank entries are reported as `hasText: false`
instead of being resolved as filesystem locations.

## Real KataGo

```bash
export LIZZIE_KATAGO_EXECUTABLE=/path/to/katago
export LIZZIE_KATAGO_MODEL=/path/to/model.bin.gz
export LIZZIE_KATAGO_ANALYSIS_CONFIG=/path/to/analysis.cfg
export LIZZIE_KATAGO_GTP_CONFIG=/path/to/gtp.cfg
QT_QPA_PLATFORM=offscreen ./install/bin/lizzieyzy_qt --diagnostics
ctest --test-dir build -R lizzie_katago_integration_tests --output-on-failure
```

Before running CTest, confirm the diagnostics output reports an executable
KataGo path plus existing model, analysis config, and GTP config paths through
the `katago.env.executable`, `katago.env.model`,
`katago.env.analysisConfig`, and `katago.env.gtpConfig` fields, including
canonical paths, readable status, nonnegative file sizes, and modification
times. Each path also reports `hasText`; unset or whitespace-only path values
are treated as missing. `katago.env.complete`, `katago.env.ready`, `katago.env.status`,
`katago.env.missing.count`, and `katago.env.invalid.count` provide the
KataGo env readiness summary that should be `ready` before real-engine runs.
The `plan.acceptance.realKataGoEnvReady` and
`plan.acceptance.realKataGoEnvStatus` diagnostics mirror this readiness in the
PLAN acceptance summary, while `plan.acceptance.realKataGoTargetRunCandidate`
should only become true on a supported target platform with a ready KataGo env.
`plan.acceptance.realKataGoManualVerificationCandidate` additionally requires
a same-screen 4K/high-DPI target display candidate.
The same summary also reports saved main/compare engine config readiness,
env-or-saved main config readiness, configured source diagnostics such as
`plan.acceptance.configuredMainConfigSource` and
`plan.acceptance.configuredCompareConfigSource`, and
`plan.acceptance.configuredStatus` so target-machine notes can distinguish
environment-variable setup from configuration saved through the UI.
`plan.acceptance.targetDisplayCandidate`
and `plan.acceptance.configuredManualVerificationCandidate` make the
configured-engine manual verification candidate explicit. The dual-engine
summary fields `plan.acceptance.configuredDualEngineConfigReady`,
`plan.acceptance.configuredDualEngineTargetRunCandidate`, and
`plan.acceptance.configuredDualEngineManualVerificationCandidate` show whether
main plus compare engine configuration is ready for dual KataGo comparison or
engine-vs-engine target-machine checks. The mode-specific fields
`plan.acceptance.configuredRealtimeGtpTargetRunCandidate`,
`plan.acceptance.configuredBatchAnalysisTargetRunCandidate`,
`plan.acceptance.configuredDualRealtimeGtpTargetRunCandidate`, and
`plan.acceptance.configuredDualBatchAnalysisTargetRunCandidate` split realtime
GTP and Analysis JSON batch readiness for main and dual-engine verification.
The matching `plan.acceptance.*ManualVerificationBlocker` lists name remaining
target-machine, KataGo config, or target-display blockers when a manual
verification candidate is still false.
`plan.acceptance.finalAcceptanceBlocker` combines mode-specific final acceptance
blocker diagnostics for the target platform, main realtime GTP config, main
batch Analysis JSON config, compare realtime GTP config, compare batch Analysis
JSON config, target-display, multi-display, external target verification,
raw-engine comparison, and manual UI inspection blockers that must be resolved
before final acceptance is recorded. The same blocker list is also emitted as
`plan.acceptance.finalAcceptanceOutstandingBlocker` in text diagnostics and the
final target-acceptance report section so manual evidence can reference one
stable outstanding-items field.
`plan.acceptance.externalTargetVerificationChecklist` lists the required
external target-machine checks that local offscreen diagnostics cannot prove.
The companion `plan.acceptance.linuxKdeWaylandNvidiaVerificationChecklist`,
`plan.acceptance.windowsNvidiaVerificationChecklist`, and
`plan.acceptance.physicalDisplayVerificationChecklist` fields split those
external checks into platform and display-specific evidence items for target
machine notes. Their matching `VerificationStatus` fields summarize whether the
current diagnostics have enough platform, GTP config, or display prerequisites
to start that target check, and `VerificationReadinessBlocker` fields name the
remaining prerequisites.
`plan.acceptance.manualUiInspectionChecklist` lists the high-DPI, 4K,
multi-display, and visible widget checkpoints that should be inspected during
manual target-machine UI verification.
`plan.acceptance.rawKataGoComparisonChecklist` lists the raw KataGo output and
UI behavior checkpoints that should be compared during manual real-engine
verification. The `rawKataGoComparisonLog` evidence must include the raw
KataGo protocol markers and the raw comparison checklist item names so the
target report can reject a log that contains engine output but omits UI
comparison notes.
The `manualUiInspectionLog` evidence must include the manual UI inspection checklist item names
so the same content gate can reject a UI note that omits the high-DPI, 4K,
multi-display, overlay, graph, and restore checks.
The `externalTargetVerificationLog` evidence must include the external target verification checklist item names
so KDE Wayland/NVIDIA, Windows/NVIDIA, physical 4K, and raw comparison sign-off
cannot be reduced to a bare pass/fail flag.
The regular CTest suite also runs `lizzie_katago_integration_preflight_tests`
without a real engine; invalid real-engine env paths should fail before process
launch with `absolutePath`, `canonicalPath`, `exists`, `file`, `readable`, and
`hasText`, plus executable status where applicable. Empty or whitespace-only
values should report `hasText=false` and `absolutePath=(blank)`. When all
`LIZZIE_KATAGO_*` values are unset, the skipped integration test output lists
each env path's `set`, `hasText`, `absolutePath`, `canonicalPath`, `exists`,
`file`, `readable`, and `usable` status before CTest marks the real-engine test
as skipped. The real integration
test should complete one 9x9 root handicap analysis JSON
request with `initialStones` and White to move while comparing the parsed
Analysis Engine response against its raw JSON `rootInfo`, `moveInfos`, PV,
PV visits, policy/prior, and ownership fields, one realtime GTP `kata-analyze`
stream after replaying a root handicap setup, one 9x13 realtime GTP analysis
after `rectangular_boardsize 9 13`, one selected-variation branch replay that
does not send sibling moves, one dual-process realtime compare smoke,
one Human-vs-AI style user move followed by a legal engine reply, one
engine-vs-engine two-process move exchange, and two GTP self-play moves. The realtime GTP paths also compare the accumulated
analysis snapshot against the same raw `kata-analyze` line for candidate order,
winrate, score mean, policy, PV, PV visits, root info, and full-board ownership
from standard top-level `ownership` output or compatible root-adjacent ownership
when KataGo emits it, including ownership-only realtime output lines, plus
per-candidate `movesOwnership` when ownership output is enabled. Confirm raw
lines that include extra edge-visit, utility,
no-result, stdev, and raw statistical fields still keep PV and PV Visits
columns aligned with the engine output. In the
application, first test a clean profile: verify the first-run dialog can enter
no-engine mode, and verify `Configure Engine` requires and saves the KataGo
executable, model, GTP config, and analysis config. Then verify
realtime analysis, ownership, PV preview, genmove, Human vs AI reply mode,
self-play, and engine-vs-engine mode. In Board Display settings, verify
ownership can be shown on the main board, the mini ownership board dock, and
both boards. Confirm the candidate table PV and PV Visits columns match KataGo
output, and that realtime SGF write-back preserves candidate `movesOwnership`
when KataGo emits it, while the compare table and graph display stable black-winrate and
black-score values and the graph computes loss bars against the player that just
moved. Open a handicap SGF with root
`HA`/`AB` setup stones and confirm realtime analysis starts from White to move
after the setup stones are replayed.
Open a supported rectangular SGF such as `SZ[9:13]` and confirm realtime sync
sends `rectangular_boardsize 9 13` before `kata-analyze`, without falling back
to a square `boardsize`.
Root `AB`/`AW`/`AE` setup should be canonicalized for realtime sync, batch
analysis JSON `initialStones`, and sidecar position keys. Mid-game setup stones
should still produce a realtime GTP sync diagnostic, while batch analysis should
send a snapshot `initialStones` request instead of skipping the node.

For invalid executable/model/config paths or a KataGo process that exits with a
nonzero status, the GTP console should show a diagnostic with `Command:`,
`Working directory:`, and `Stderr:` lines. Multi-line `stderr:` and `system:`
messages should keep their source prefix on every line. The separate Engine Log
dock should show stderr, capability-probe results, sync diagnostics, and process
diagnostics from the main engine, compare engine, and batch analysis process
with their source labels.

For batch analysis, verify both output modes. The default writes
`game.sgf.analysis.json`; successful nodes should contain analysis snapshots and
failed nodes should contain `analysisError` diagnostics. With sidecar writing
disabled in Settings, successful nodes should be written back into the current
SGF as `LZOP`/`LZ` plus `LZYPERSP` and a generated analysis comment, while
failed nodes should be written as `LZYERR` plus a generated
`LizzieYzy analysis error` comment.
Readable generated analysis comments should include candidate stdev, policy,
PV, and PV visit fields in addition to visits, winrate, and score.
Existing user comments and unrelated SGF properties should remain intact after
the write. For multi-game SGF collections, sidecars should include a matching
`collectionGameIndex`; old unindexed collection sidecars should be skipped with
a GTP Console diagnostic instead of applying analysis to the selected game.
Saving or batch-writing SGF output from a selected collection game should
preserve the unselected games, and session restore should reopen the same
collection game index rather than silently falling back to the first game.

## KDE Wayland + NVIDIA

Run the app from the installed or packaged output in a Wayland session:

```bash
echo "$XDG_SESSION_TYPE"
./install/bin/lizzieyzy_qt --diagnostics
./install/bin/lizzieyzy_qt
```

Verify that the main window opens, KataGo can start, realtime analysis resumes
after branch changes, and stderr reports CUDA/OpenCL/TensorRT information or
KataGo errors without crashing the UI. Keep the `--diagnostics` output with the
test notes; it should show the Qt platform, Qt Quick graphics API, scene graph
backend, renderer-interface RHI/shader fields, KDE/Wayland/display
environment, app directory, Qt library paths, primary and available screens,
OpenGL runtime context diagnostics,
virtual screen geometry, physical screen size, refresh rate, average/per-axis DPI values, scale
factors, Qt plugin and platform theme variables, and
common Wayland platform plugin candidates, common Windows platform plugin candidates,
the `plan.target.linuxKdeWayland.*` target-platform summary diagnostics
including `qtPlatformWayland`,
`plan.target.acceptance.linuxKdeWaylandNvidiaCandidate`,
`plan.target.acceptance.linuxKdeWaylandNvidiaBlocker`,
`plan.target.acceptance.firstReleaseNvidiaPlatformBlocker`,
`plan.acceptance.targetPlatformCandidate`,
`plan.acceptance.targetPlatformBlocker`,
`plan.acceptance.realKataGoTargetRunCandidate`,
`plan.acceptance.realKataGoManualVerificationCandidate`,
`plan.acceptance.realKataGoManualVerificationBlocker`,
`plan.acceptance.configuredManualVerificationCandidate`,
`plan.acceptance.configuredManualVerificationBlocker`,
`plan.acceptance.configuredDualEngineManualVerificationCandidate`,
`plan.acceptance.configuredDualEngineManualVerificationBlocker`,
`plan.acceptance.configuredRealtimeGtpManualVerificationCandidate`,
`plan.acceptance.configuredBatchAnalysisManualVerificationCandidate`,
`plan.acceptance.configuredDualRealtimeGtpManualVerificationCandidate`,
`plan.acceptance.configuredDualBatchAnalysisManualVerificationCandidate`,
`plan.acceptance.recordFile`,
`plan.acceptance.recordFile.canonicalPath`,
`plan.acceptance.recordFile.size`,
`plan.acceptance.recordFile.sha256`,
`plan.acceptance.recordFile.lastModifiedUtc`,
`plan.acceptance.recordFile.timestampStatus`,
`plan.acceptance.record.completedUtc`,
`plan.acceptance.record.completedUtcStatus`,
`plan.acceptance.record.appVersion`,
`plan.acceptance.record.appVersionStatus`,
`plan.acceptance.record.appExecutableSha256`,
`plan.acceptance.record.appExecutableSha256Status`,
`plan.acceptance.record.osPrettyName`,
`plan.acceptance.record.osPrettyNameStatus`,
`plan.acceptance.record.osKernelType`,
`plan.acceptance.record.osKernelTypeStatus`,
`plan.acceptance.record.osKernelVersion`,
`plan.acceptance.record.osKernelVersionStatus`,
`plan.acceptance.record.qtRuntimeVersion`,
`plan.acceptance.record.qtRuntimeVersionStatus`,
`plan.acceptance.record.qtBuildAbi`,
`plan.acceptance.record.qtBuildAbiStatus`,
`plan.acceptance.record.cpuArchitecture`,
`plan.acceptance.record.cpuArchitectureStatus`,
`plan.acceptance.record.reviewer`,
`plan.acceptance.record.targetMachine`,
`plan.acceptance.record.targetMachineStatus`,
`plan.acceptance.record.gpuDriver`,
`plan.acceptance.record.gpuDriverStatus`,
`plan.acceptance.record.kataGoVersion`,
`plan.acceptance.record.kataGoVersionStatus`,
`plan.acceptance.recordMetadata.ready`,
`plan.acceptance.recordMetadata.blocker`,
`plan.acceptance.recordHint.passValues`,
`plan.acceptance.recordHint.failValues`,
`plan.acceptance.recordHint.metadataKeysRequired`,
`plan.acceptance.recordHint.metadataExactMatchKeys`,
`plan.acceptance.recordHint.metadataValueCheckedKeys`,
`plan.acceptance.recordHint.completedUtcRequired`,
`plan.acceptance.recordHint.aggregateAcceptanceKeysRequired`,
`plan.acceptance.recordHint.checklistItemsRequired`,
`plan.acceptance.recordHint.evidencePathsRequired`,
`plan.acceptance.recordHint.evidenceSha256Required`,
`plan.acceptance.recordHint.evidenceContentMarkersRequired`,
`plan.acceptance.recordHint.recordAndEvidenceTimestampsMustNotAfterCompletedUtc`,
`plan.acceptance.evidence.diagnostics`,
`plan.acceptance.evidence.targetAcceptanceReport`,
`plan.acceptance.evidence.engineLog`,
`plan.acceptance.evidence.*.lastModifiedUtc`,
`plan.acceptance.evidence.*.timestampStatus`,
`plan.acceptance.evidence.diagnostics.contentStatus`,
`plan.acceptance.evidence.targetAcceptanceReport.contentStatus`,
`plan.acceptance.evidence.engineLog.contentStatus`,
`plan.acceptance.evidence.engineLog.gpuOrStderrContentStatus`,
`plan.acceptance.evidence.engineLog.missingGpuOrStderrContentMarker`,
`plan.acceptance.evidence.rawKataGoComparisonLog.contentStatus`,
`plan.acceptance.evidence.manualUiInspectionLog.contentStatus`,
`plan.acceptance.evidence.externalTargetVerificationLog.contentStatus`,
`plan.acceptance.evidence.*.missingContentMarker`,
`plan.acceptance.evidence.rawKataGoComparisonLog`,
`plan.acceptance.evidence.manualUiInspectionLog`,
`plan.acceptance.evidence.externalTargetVerificationLog`,
`plan.acceptance.evidence.screenshot`,
`plan.acceptance.evidence.screenshot.imagePixelsAtLeast4K`,
`plan.acceptance.evidence.screenshot.imageHasPixelVariation`,
`plan.acceptance.evidence.screenshot.readyFor4KAcceptance`,
`plan.acceptance.evidence.ready`,
`plan.acceptance.evidence.blocker`,
`plan.acceptance.evidence.*.expectedSha256`,
`plan.acceptance.evidence.*.sha256Status`,
`plan.acceptance.evidenceSha256.ready`,
`plan.acceptance.evidenceSha256.blocker`,
`plan.acceptance.recordTimestamp.ready`,
`plan.acceptance.recordTimestamp.blocker`,
`plan.acceptance.evidenceTimestamp.ready`,
`plan.acceptance.evidenceTimestamp.blocker`,
`plan.acceptance.linuxKdeWaylandNvidiaManualResult`,
`plan.acceptance.windowsNvidiaManualResult`,
`plan.acceptance.physicalDisplayManualResult`,
`plan.acceptance.externalTargetVerificationManualResult`,
`plan.acceptance.checklistPassedRecord`,
`plan.acceptance.checklistFailedRecord`,
`plan.acceptance.checklistInvalidRecord`,
`plan.acceptance.checklistMissingRecord`,
`plan.acceptance.manualFailedRecord`,
`plan.acceptance.manualInvalidRecord`,
`plan.acceptance.checklist.ready`,
`plan.acceptance.checklist.blocker`,
`plan.acceptance.externalTargetVerificationRequired`,
`plan.acceptance.externalTargetVerificationStatus`,
`plan.acceptance.externalTargetVerificationChecklist`,
`plan.acceptance.rawKataGoComparisonStatus`,
`plan.acceptance.manualUiInspectionStatus`,
`plan.acceptance.finalAcceptanceStatus`,
`plan.acceptance.finalAcceptanceBlocker`,
`plan.acceptance.finalAcceptanceOutstandingBlocker`,
`acceptanceChecklist`,
`acceptanceEvidenceSha256`,
`screenshotEvidence4K`,
`plan.acceptance.linuxKdeWaylandNvidiaVerificationChecklist`,
`plan.acceptance.linuxKdeWaylandNvidiaVerificationStatus`,
`plan.acceptance.linuxKdeWaylandNvidiaVerificationReadinessBlocker`,
`plan.acceptance.windowsNvidiaVerificationChecklist`,
`plan.acceptance.windowsNvidiaVerificationStatus`,
`plan.acceptance.windowsNvidiaVerificationReadinessBlocker`,
`plan.acceptance.physicalDisplayVerificationChecklist`,
`plan.acceptance.physicalDisplayVerificationStatus`,
`plan.acceptance.physicalDisplayVerificationReadinessBlocker`,
`plan.acceptance.manualUiInspectionChecklist`,
`plan.acceptance.rawKataGoComparisonChecklist`,
`plan.target.nvidiaEnvironmentHint.*` source entries from explicit GPU
variables and Vulkan/EGL/GBM path values,
plus graphics/NVIDIA/Vulkan/CUDA-related environment variables observed by the
packaged app.

## Windows + NVIDIA

Build or download the Windows CI package, extract it, and run
`lizzieyzy_qt.exe --diagnostics`, then `lizzieyzy_qt.exe`. Verify Settings path
selection, realtime KataGo analysis, package-local Qt plugins, and engine
diagnostics. Save the diagnostics output with the Windows package result so Qt
platform/plugin loading, app directory, app-local Qt runtime artifact diagnostics,
Qt library paths, Qt Quick graphics API,
OpenGL runtime context diagnostics,
path-status fields for the app executable, app directory, process working
directory, Qt plugin path, Qt library paths, current platform plugin candidates, available platform plugin
listing, common Wayland platform plugin candidates, common Windows platform plugin candidates,
the `plan.target.windows.*` target-platform summary diagnostics,
`plan.target.acceptance.windowsNvidiaCandidate`,
`plan.target.acceptance.windowsNvidiaBlocker`,
`plan.target.acceptance.firstReleaseNvidiaPlatformBlocker`,
`plan.acceptance.targetPlatformCandidate`,
`plan.acceptance.targetPlatformBlocker`,
`plan.acceptance.realKataGoTargetRunCandidate`,
`plan.acceptance.realKataGoManualVerificationCandidate`,
`plan.acceptance.realKataGoManualVerificationBlocker`,
`plan.acceptance.configuredManualVerificationCandidate`,
`plan.acceptance.configuredManualVerificationBlocker`,
`plan.acceptance.configuredDualEngineManualVerificationCandidate`,
`plan.acceptance.configuredDualEngineManualVerificationBlocker`,
`plan.acceptance.configuredRealtimeGtpManualVerificationCandidate`,
`plan.acceptance.configuredBatchAnalysisManualVerificationCandidate`,
`plan.acceptance.configuredDualRealtimeGtpManualVerificationCandidate`,
`plan.acceptance.configuredDualBatchAnalysisManualVerificationCandidate`,
`plan.acceptance.recordFile`,
`plan.acceptance.recordFile.canonicalPath`,
`plan.acceptance.recordFile.size`,
`plan.acceptance.recordFile.sha256`,
`plan.acceptance.recordFile.lastModifiedUtc`,
`plan.acceptance.recordFile.timestampStatus`,
`plan.acceptance.record.completedUtc`,
`plan.acceptance.record.completedUtcStatus`,
`plan.acceptance.record.appVersion`,
`plan.acceptance.record.appVersionStatus`,
`plan.acceptance.record.appExecutableSha256`,
`plan.acceptance.record.appExecutableSha256Status`,
`plan.acceptance.record.osPrettyName`,
`plan.acceptance.record.osPrettyNameStatus`,
`plan.acceptance.record.osKernelType`,
`plan.acceptance.record.osKernelTypeStatus`,
`plan.acceptance.record.osKernelVersion`,
`plan.acceptance.record.osKernelVersionStatus`,
`plan.acceptance.record.qtRuntimeVersion`,
`plan.acceptance.record.qtRuntimeVersionStatus`,
`plan.acceptance.record.qtBuildAbi`,
`plan.acceptance.record.qtBuildAbiStatus`,
`plan.acceptance.record.cpuArchitecture`,
`plan.acceptance.record.cpuArchitectureStatus`,
`plan.acceptance.record.reviewer`,
`plan.acceptance.record.targetMachine`,
`plan.acceptance.record.targetMachineStatus`,
`plan.acceptance.record.gpuDriver`,
`plan.acceptance.record.gpuDriverStatus`,
`plan.acceptance.record.kataGoVersion`,
`plan.acceptance.record.kataGoVersionStatus`,
`plan.acceptance.recordMetadata.ready`,
`plan.acceptance.recordMetadata.blocker`,
`plan.acceptance.recordHint.passValues`,
`plan.acceptance.recordHint.failValues`,
`plan.acceptance.recordHint.metadataKeysRequired`,
`plan.acceptance.recordHint.metadataExactMatchKeys`,
`plan.acceptance.recordHint.metadataValueCheckedKeys`,
`plan.acceptance.recordHint.completedUtcRequired`,
`plan.acceptance.recordHint.aggregateAcceptanceKeysRequired`,
`plan.acceptance.recordHint.checklistItemsRequired`,
`plan.acceptance.recordHint.evidencePathsRequired`,
`plan.acceptance.recordHint.evidenceSha256Required`,
`plan.acceptance.recordHint.evidenceContentMarkersRequired`,
`plan.acceptance.recordHint.recordAndEvidenceTimestampsMustNotAfterCompletedUtc`,
`plan.acceptance.evidence.diagnostics`,
`plan.acceptance.evidence.targetAcceptanceReport`,
`plan.acceptance.evidence.engineLog`,
`plan.acceptance.evidence.rawKataGoComparisonLog`,
`plan.acceptance.evidence.manualUiInspectionLog`,
`plan.acceptance.evidence.externalTargetVerificationLog`,
`plan.acceptance.evidence.screenshot`,
`plan.acceptance.evidence.*.lastModifiedUtc`,
`plan.acceptance.evidence.*.timestampStatus`,
`plan.acceptance.evidence.diagnostics.contentStatus`,
`plan.acceptance.evidence.targetAcceptanceReport.contentStatus`,
`plan.acceptance.evidence.engineLog.contentStatus`,
`plan.acceptance.evidence.rawKataGoComparisonLog.contentStatus`,
`plan.acceptance.evidence.manualUiInspectionLog.contentStatus`,
`plan.acceptance.evidence.externalTargetVerificationLog.contentStatus`,
`plan.acceptance.evidence.ready`,
`plan.acceptance.evidence.blocker`,
`plan.acceptance.recordTimestamp.ready`,
`plan.acceptance.recordTimestamp.blocker`,
`plan.acceptance.evidenceTimestamp.ready`,
`plan.acceptance.evidenceTimestamp.blocker`,
`plan.acceptance.linuxKdeWaylandNvidiaManualResult`,
`plan.acceptance.windowsNvidiaManualResult`,
`plan.acceptance.physicalDisplayManualResult`,
`plan.acceptance.externalTargetVerificationManualResult`,
`plan.acceptance.checklistPassedRecord`,
`plan.acceptance.checklistFailedRecord`,
`plan.acceptance.checklistInvalidRecord`,
`plan.acceptance.manualFailedRecord`,
`plan.acceptance.manualInvalidRecord`,
`plan.acceptance.externalTargetVerificationRequired`,
`plan.acceptance.externalTargetVerificationStatus`,
`plan.acceptance.externalTargetVerificationChecklist`,
`plan.acceptance.rawKataGoComparisonStatus`,
`plan.acceptance.manualUiInspectionStatus`,
`plan.acceptance.finalAcceptanceStatus`,
`plan.acceptance.linuxKdeWaylandNvidiaVerificationChecklist`,
`plan.acceptance.linuxKdeWaylandNvidiaVerificationStatus`,
`plan.acceptance.linuxKdeWaylandNvidiaVerificationReadinessBlocker`,
`plan.acceptance.windowsNvidiaVerificationChecklist`,
`plan.acceptance.windowsNvidiaVerificationStatus`,
`plan.acceptance.windowsNvidiaVerificationReadinessBlocker`,
`plan.acceptance.physicalDisplayVerificationChecklist`,
`plan.acceptance.physicalDisplayVerificationStatus`,
`plan.acceptance.physicalDisplayVerificationReadinessBlocker`,
`plan.acceptance.manualUiInspectionChecklist`,
`plan.acceptance.rawKataGoComparisonChecklist`,
`plan.target.nvidiaEnvironmentHint.*` source entries,
QStandardPaths writable locations for config/data/cache/runtime, application font/text metrics, locale/UI-language diagnostics,
runtime appearance style/palette diagnostics,
native file dialog/settings path selector diagnostics,
main-window UI structure diagnostics,
QSettings storage location, stored and normalized appearance settings, first-run completion diagnostics,
saved engine setting path diagnostics,
stored engine path-readiness diagnostics,
stored analysis option diagnostics, stored board display diagnostics,
stored file behavior diagnostics, stored shortcut diagnostics, QSettings session/window restore keys,
saved session SGF path-status, saved window-geometry visibility checks,
`HOME`, `USERPROFILE`, `APPDATA`, `LOCALAPPDATA`, XDG, temp directory, and
`XDG_RUNTIME_DIR` path status,
QML import paths, QML import environment, Qt Quick Controls configuration paths,
renderer-interface RHI/shader fields, primary screen
selection, screen geometry, physical screen size, refresh rate, average/per-axis DPI, screen orientation,
device-pixel geometry, virtual-sibling layout, Qt plugin/theme variables,
filesystem path size/modification time, and GPU-related environment can be
compared with the Linux target notes.

## High DPI And Multi-Display

Run the CTest UI suite at normal, 150%, and 200% scale:

```bash
ctest --test-dir build --parallel 2 -R "lizzie_ui_smoke_tests|lizzie_board_quick_item_tests|lizzie_analysis_graph_widget_tests" --output-on-failure
```

The automated UI suite includes main-window 4K/high-DPI layout geometry smoke
for toolbar action overlap, dock/tab sizing, candidate/compare table columns,
and nonblank top-level window rendering.

On a 4K display, open a 19x19 SGF with analysis data and verify that the board,
candidate labels, PV stones, PV visit counts, ownership overlays on both board
display modes, and winrate/score graph remain sharp and non-overlapping. Move
the window to a secondary display, close it, disconnect or rearrange displays,
then reopen the app; it should restore onto an available screen with enough
visible area to operate the window, not just a one-pixel edge.
Run `lizzieyzy_qt --diagnostics` before and after the display rearrangement so
the restored placement can be interpreted against the actual primary screen,
primary screen geometry, virtual screen geometries, available geometries, physical screen sizes, refresh
rates, color depth, average/per-axis DPI values, device-pixel ratios, device-pixel geometries, and saved
window-geometry visibility/recenter diagnostics, along with
`plan.target.display.*` target-platform summary diagnostics for primary-screen
and any-screen 4K/high-DPI detection, same-screen target-display candidates,
plus multi-display detection, and
`plan.target.acceptance.targetDisplayCandidate`,
`plan.target.acceptance.targetDisplayBlocker`,
`plan.acceptance.targetDisplayCandidate`,
`plan.acceptance.sameScreenTargetDisplayCandidate`,
`plan.acceptance.targetDisplayBlocker`,
`plan.acceptance.configuredManualVerificationCandidate`,
`plan.acceptance.configuredManualVerificationBlocker`,
`plan.acceptance.configuredDualEngineManualVerificationCandidate`,
`plan.acceptance.configuredDualEngineManualVerificationBlocker`,
`plan.acceptance.configuredRealtimeGtpManualVerificationCandidate`,
`plan.acceptance.configuredBatchAnalysisManualVerificationCandidate`,
`plan.acceptance.configuredDualRealtimeGtpManualVerificationCandidate`,
`plan.acceptance.configuredDualBatchAnalysisManualVerificationCandidate`,
`plan.acceptance.multiDisplayCandidate`,
`plan.acceptance.multiDisplayBlocker`,
`plan.acceptance.recordFile`,
`plan.acceptance.recordFile.canonicalPath`,
`plan.acceptance.recordFile.size`,
`plan.acceptance.recordFile.sha256`,
`plan.acceptance.recordFile.lastModifiedUtc`,
`plan.acceptance.recordFile.timestampStatus`,
`plan.acceptance.record.completedUtc`,
`plan.acceptance.record.completedUtcStatus`,
`plan.acceptance.record.appVersion`,
`plan.acceptance.record.appVersionStatus`,
`plan.acceptance.record.appExecutableSha256`,
`plan.acceptance.record.appExecutableSha256Status`,
`plan.acceptance.record.osPrettyName`,
`plan.acceptance.record.osPrettyNameStatus`,
`plan.acceptance.record.osKernelType`,
`plan.acceptance.record.osKernelTypeStatus`,
`plan.acceptance.record.osKernelVersion`,
`plan.acceptance.record.osKernelVersionStatus`,
`plan.acceptance.record.qtRuntimeVersion`,
`plan.acceptance.record.qtRuntimeVersionStatus`,
`plan.acceptance.record.qtBuildAbi`,
`plan.acceptance.record.qtBuildAbiStatus`,
`plan.acceptance.record.cpuArchitecture`,
`plan.acceptance.record.cpuArchitectureStatus`,
`plan.acceptance.record.reviewer`,
`plan.acceptance.record.targetMachine`,
`plan.acceptance.record.targetMachineStatus`,
`plan.acceptance.record.gpuDriver`,
`plan.acceptance.record.gpuDriverStatus`,
`plan.acceptance.record.kataGoVersion`,
`plan.acceptance.record.kataGoVersionStatus`,
`plan.acceptance.recordMetadata.ready`,
`plan.acceptance.recordMetadata.blocker`,
`plan.acceptance.recordHint.passValues`,
`plan.acceptance.recordHint.failValues`,
`plan.acceptance.recordHint.metadataKeysRequired`,
`plan.acceptance.recordHint.metadataExactMatchKeys`,
`plan.acceptance.recordHint.metadataValueCheckedKeys`,
`plan.acceptance.recordHint.completedUtcRequired`,
`plan.acceptance.recordHint.aggregateAcceptanceKeysRequired`,
`plan.acceptance.recordHint.checklistItemsRequired`,
`plan.acceptance.recordHint.evidencePathsRequired`,
`plan.acceptance.recordHint.evidenceSha256Required`,
`plan.acceptance.recordHint.evidenceContentMarkersRequired`,
`plan.acceptance.recordHint.recordAndEvidenceTimestampsMustNotAfterCompletedUtc`,
`plan.acceptance.evidence.diagnostics`,
`plan.acceptance.evidence.targetAcceptanceReport`,
`plan.acceptance.evidence.engineLog`,
`plan.acceptance.evidence.rawKataGoComparisonLog`,
`plan.acceptance.evidence.screenshot`,
`plan.acceptance.evidence.*.lastModifiedUtc`,
`plan.acceptance.evidence.*.timestampStatus`,
`plan.acceptance.evidence.diagnostics.contentStatus`,
`plan.acceptance.evidence.targetAcceptanceReport.contentStatus`,
`plan.acceptance.evidence.engineLog.contentStatus`,
`plan.acceptance.evidence.rawKataGoComparisonLog.contentStatus`,
`plan.acceptance.evidence.ready`,
`plan.acceptance.evidence.blocker`,
`plan.acceptance.recordTimestamp.ready`,
`plan.acceptance.recordTimestamp.blocker`,
`plan.acceptance.evidenceTimestamp.ready`,
`plan.acceptance.evidenceTimestamp.blocker`,
`plan.acceptance.linuxKdeWaylandNvidiaManualResult`,
`plan.acceptance.windowsNvidiaManualResult`,
`plan.acceptance.physicalDisplayManualResult`,
`plan.acceptance.externalTargetVerificationManualResult`,
`plan.acceptance.checklistPassedRecord`,
`plan.acceptance.checklistFailedRecord`,
`plan.acceptance.checklistInvalidRecord`,
`plan.acceptance.manualFailedRecord`,
`plan.acceptance.manualInvalidRecord`,
`plan.acceptance.externalTargetVerificationStatus`,
`plan.acceptance.rawKataGoComparisonStatus`,
`plan.acceptance.manualUiInspectionStatus`,
`plan.acceptance.finalAcceptanceStatus`,
`plan.acceptance.finalAcceptanceBlocker`,
`plan.acceptance.finalAcceptanceOutstandingBlocker`, and
`plan.acceptance.manualUiInspectionRequired`.

For boards wider than standard GTP coordinates can express, create a new
rectangular game such as 26x9 and confirm it saves as `SZ[26:9]`; also open an
existing wide SGF and verify that viewing/editing still works. Realtime KataGo
analysis and self-play should report a diagnostic instead of sending malformed
GTP moves, while batch analysis should send KataGo Analysis Engine coordinates
such as `(x,y)` and write a normal sidecar if the engine accepts the position.
