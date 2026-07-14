# Target Acceptance Report

Use this report for the PLAN.md acceptance checks that must run on real target
machines with a real KataGo setup. Keep the raw `--diagnostics` output and any
real-engine command logs with the completed report.
Run `lizzieyzy_qt --target-acceptance-report` on the target machine to generate
a prefilled Markdown summary of the acceptance status fields, readiness
blockers, and checklist names before recording manual pass/fail results.
Run `lizzieyzy_qt --target-acceptance-record-template` to generate a complete
INI skeleton for those manual results. To fold completed manual results back
into the generated status fields, save that INI file and run with
`--target-acceptance-record <ini>`; scripted runs can also set
`LIZZIE_TARGET_ACCEPTANCE_RECORD_FILE` to the same path.
The generated outputs include the acceptance record file canonical path, size,
SHA-256 digest, and modification time so reviewers can identify the exact INI
file used for the report.
The record-template command runs before Qt platform initialization, so it is
available even when display or platform-plugin setup is broken.
It also includes `[evidenceContentMarkers]` and `[recordHints]` guidance
sections. The former lists the strings each evidence log should contain, and the
generated report enforces the same markers through `contentStatus`. The latter
repeats the accepted pass/fail values, required metadata keys, metadata matching
rules, and the evidence, SHA-256, content-marker, and timestamp requirements. The
acceptance parser ignores both guidance sections.
Generated reports and diagnostics expose the same guidance as
`plan.acceptance.recordHint.*` fields for grep-friendly review logs.
Accepted pass values are `pass`, `passed`, `accepted`, `true`, and `yes`;
accepted fail values are `fail`, `failed`, `rejected`, `false`, and `no`.
Evidence paths can be absolute or relative; relative evidence paths are resolved
against the directory containing the target acceptance record file. Each evidence
path must point at a readable, non-empty file.
If the record path starts with `-`, use
`--target-acceptance-record -- <ini>` so the path is not parsed as an option.
The seven evidence paths must resolve to distinct canonical files; duplicate
evidence paths keep an `acceptanceEvidence` blocker even when their SHA-256 pins
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
`TensorRT`, `GPU`, `backend`, or `stderr:`; the generated
`plan.acceptance.evidence.engineLog.gpuOrStderrContentStatus` and
`plan.acceptance.evidence.engineLog.missingGpuOrStderrContentMarker` fields
report that target-machine evidence gate. Raw
KataGo comparison log evidence must contain `KataGo`, `kata-analyze`,
`rootInfo`, `moveInfos`, `winrate`, `scoreMean`, `scoreStdev`, `visits`,
`policy`, `pv`, `pvVisits`, and `ownership`, and, when the record
`kataGoVersion` matches the current app's KataGo requirement, the recorded
version string. Missing markers keep an
`acceptanceEvidence` blocker and are reported through `contentStatus`.
The generated report includes each evidence file size and SHA-256 digest so the
attached artifacts can be matched to the acceptance record later.
Record and evidence timestamp status compares each file modification time
against `completedUtc`; final acceptance keeps an `acceptanceRecordTimestamp`
blocker if the record file was modified after the recorded completion time plus
the built-in clock-skew tolerance, and keeps an `acceptanceEvidenceTimestamp`
blocker if any readable evidence file was modified after that threshold.
The `[evidenceSha256]` fields pin the expected digest for each evidence file.
Final acceptance keeps an `acceptanceEvidenceSha256` blocker until every
expected hash is present, valid, and matches the current evidence file.
Missing expected hashes are reported as `missing-expected`.
Screenshot evidence additionally reports image readability, detected image
format, pixel width/height, whether the captured image reaches a 4K pixel
envelope, and whether the image has pixel variation. The final acceptance gate
keeps a `screenshotEvidence4K` blocker until the screenshot evidence is
readable, reaches that 4K pixel envelope, and is not a uniform image.
The final acceptance gate also requires the key record metadata fields
`completedUtc`, `appVersion`, `appExecutableSha256`, `osPrettyName`,
`osKernelType`, `osKernelVersion`, `qtRuntimeVersion`, `qtBuildAbi`,
`cpuArchitecture`, `reviewer`, `targetMachine`, `gpuDriver`, and `kataGoVersion`;
`completedUtc` must be an ISO UTC timestamp ending in `Z` and must not be in
the future. `plan.acceptance.record.completedUtcStatus` reports `ready`,
`missing`, `invalid-format`, or `future`. `appVersion` must match the generated
report `app.version`, and `plan.acceptance.record.appVersionStatus` reports
`match`, `missing`, or `mismatch`. `appExecutableSha256` must match the SHA-256
digest of the current `app.executable`; `plan.acceptance.record.appExecutableSha256Status`
reports `match`, `missing`, `invalid`, `current-unavailable`, or `mismatch`.
The OS, Qt runtime, CPU, and target-machine metadata must match the current
`os.*`, `qt.*`, `cpu.*`, and machine host-name values; their status fields
report `match`, `missing`, `current-unavailable`, or `mismatch`.
`gpuDriver` must identify NVIDIA; `plan.acceptance.record.gpuDriverStatus`
reports `match`, `missing`, or `mismatch`.
`kataGoVersion` must identify KataGo; `plan.acceptance.record.kataGoVersionStatus`
reports `match`, `missing`, or `mismatch`.
Every target acceptance checklist item must also be recorded as pass. Aggregate
`[acceptance]` pass values are useful for summaries, but the final gate keeps an
`acceptanceChecklist` blocker until the checklist details prove each required
item. `plan.acceptance.checklistMissingRecord` lists checklist items that still
need an explicit record, separate from failed or invalid item records.
`plan.acceptance.manualFailedRecord` and
`plan.acceptance.manualInvalidRecord` list failed or invalid aggregate manual
acceptance results, including target components, raw KataGo comparison, manual
UI inspection, and the derived or explicit external target verification result.
`plan.acceptance.finalAcceptanceStatus` reports invalid or failed manual
records, including target component results and checklist item records, before
generic readiness gaps so reviewers can spot bad target evidence even when the
report was rerun from a non-target or incomplete environment.

```ini
[metadata]
completedUtc=2026-07-02T12:34:56Z
appVersion=0.1.0
appExecutableSha256=0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef
osPrettyName=Debian GNU/Linux 12 (bookworm)
osKernelType=linux
osKernelVersion=6.1.0
qtRuntimeVersion=6.8.3
qtBuildAbi=x86_64-little_endian-lp64
cpuArchitecture=x86_64
reviewer=target tester
targetMachine=target-hostname
gpuDriver=NVIDIA 555.55
kataGoVersion=KataGo 1.15.3
notes=raw diagnostics, engine logs, screenshots, raw KataGo comparison logs, manual UI inspection logs, and external target verification logs attached

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

[acceptance]
linuxKdeWaylandNvidia=pass
windowsNvidia=pass
physicalDisplay=pass
externalTargetVerification=pass
rawKataGoComparison=pass
manualUiInspection=pass

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

## Package

- Package file:
- Package platform: Linux / Windows
- App version:
- Target machine:
- GPU and driver:
- KataGo version:
- Model:
- GTP config:
- Analysis config:
- `app.version`:
- `generatedUtc`:
- `app.executable`:
- `app.dir`:
- `process.currentWorkingDirectory`:
- `qt.version`:
- `qt.platform`:
- `os.prettyName`:
- `os.kernelType`:
- `os.kernelVersion`:
- `plan.target.osFamily`:

## Configured Paths

- `katago.env.executable`:
- `katago.env.model`:
- `katago.env.gtpConfig`:
- `katago.env.analysisConfig`:
- `qt.settings.diagnosticsOverrideFile`:
- `qt.settings.fileName`:
- `qt.settings.engine.executable.value`:
- `qt.settings.engine.model.value`:
- `qt.settings.engine.gtpConfig.value`:
- `qt.settings.engine.analysisConfig.value`:
- `qt.settings.engine.workingDirectory.value`:
- `qt.settings.engine.extraArgs.value`:
- `qt.settings.engine.extraArgs.parsed.count`:
- `qt.settings.engine.extraArgs.parsed.*`:
- `qt.settings.compare.executable.value`:
- `qt.settings.compare.model.value`:
- `qt.settings.compare.gtpConfig.value`:
- `qt.settings.compare.analysisConfig.value`:
- `qt.settings.compare.workingDirectory.value`:
- `qt.settings.compare.extraArgs.value`:
- `qt.settings.compare.extraArgs.parsed.count`:
- `qt.settings.compare.extraArgs.parsed.*`:

## Diagnostics

Paste or attach the installed app diagnostics output:

```text
lizzieyzy_qt --diagnostics
lizzieyzy_qt --target-acceptance-report
lizzieyzy_qt --target-acceptance-report --target-acceptance-record target-acceptance-record.ini
```

Record these summary fields:

- `plan.acceptance.status`:
- `plan.acceptance.configuredStatus`:
- `plan.acceptance.recordFile`:
- `plan.acceptance.recordFile.set`:
- `plan.acceptance.recordFile.exists`:
- `plan.acceptance.recordFile.readable`:
- `plan.acceptance.recordFile.canonicalPath`:
- `plan.acceptance.recordFile.size`:
- `plan.acceptance.recordFile.sha256`:
- `plan.acceptance.recordFile.lastModifiedUtc`:
- `plan.acceptance.recordFile.timestampStatus`:
- `plan.acceptance.record.completedUtc`:
- `plan.acceptance.record.completedUtcStatus`:
- `plan.acceptance.record.appVersion`:
- `plan.acceptance.record.appVersionStatus`:
- `plan.acceptance.record.appExecutableSha256`:
- `plan.acceptance.record.appExecutableSha256Status`:
- `plan.acceptance.record.osPrettyName`:
- `plan.acceptance.record.osPrettyNameStatus`:
- `plan.acceptance.record.osKernelType`:
- `plan.acceptance.record.osKernelTypeStatus`:
- `plan.acceptance.record.osKernelVersion`:
- `plan.acceptance.record.osKernelVersionStatus`:
- `plan.acceptance.record.qtRuntimeVersion`:
- `plan.acceptance.record.qtRuntimeVersionStatus`:
- `plan.acceptance.record.qtBuildAbi`:
- `plan.acceptance.record.qtBuildAbiStatus`:
- `plan.acceptance.record.cpuArchitecture`:
- `plan.acceptance.record.cpuArchitectureStatus`:
- `plan.acceptance.record.reviewer`:
- `plan.acceptance.record.targetMachine`:
- `plan.acceptance.record.targetMachineStatus`:
- `plan.acceptance.record.gpuDriver`:
- `plan.acceptance.record.gpuDriverStatus`:
- `plan.acceptance.record.kataGoVersion`:
- `plan.acceptance.record.kataGoVersionStatus`:
- `plan.acceptance.record.notes`:
- `plan.acceptance.recordMetadata.ready`:
- `plan.acceptance.recordMetadata.blocker.*`:
- `plan.acceptance.recordHint.passValues`:
- `plan.acceptance.recordHint.failValues`:
- `plan.acceptance.recordHint.metadataKeysRequired`:
- `plan.acceptance.recordHint.metadataExactMatchKeys`:
- `plan.acceptance.recordHint.metadataValueCheckedKeys`:
- `plan.acceptance.recordHint.completedUtcRequired`:
- `plan.acceptance.recordHint.aggregateAcceptanceKeysRequired`:
- `plan.acceptance.recordHint.checklistItemsRequired`:
- `plan.acceptance.recordHint.evidencePathsRequired`:
- `plan.acceptance.recordHint.evidenceSha256Required`:
- `plan.acceptance.recordHint.evidenceContentMarkersRequired`:
- `plan.acceptance.recordHint.recordAndEvidenceTimestampsMustNotAfterCompletedUtc`:
- `plan.acceptance.evidence.diagnostics.*`:
- `plan.acceptance.evidence.targetAcceptanceReport.*`:
- `plan.acceptance.evidence.engineLog.*`:
- `plan.acceptance.evidence.rawKataGoComparisonLog.*`:
- `plan.acceptance.evidence.manualUiInspectionLog.*`:
- `plan.acceptance.evidence.externalTargetVerificationLog.*`:
- `plan.acceptance.evidence.screenshot.*`:
- `plan.acceptance.evidence.*.lastModifiedUtc`:
- `plan.acceptance.evidence.*.timestampStatus`:
- `plan.acceptance.evidence.diagnostics.contentStatus`:
- `plan.acceptance.evidence.targetAcceptanceReport.contentStatus`:
- `plan.acceptance.evidence.engineLog.contentStatus`:
- `plan.acceptance.evidence.engineLog.gpuOrStderrContentStatus`:
- `plan.acceptance.evidence.engineLog.missingGpuOrStderrContentMarker.*`:
- `plan.acceptance.evidence.rawKataGoComparisonLog.contentStatus`:
- `plan.acceptance.evidence.rawKataGoComparisonLog.missingContentMarker.*`:
  includes missing raw KataGo protocol markers and missing raw comparison
  checklist item names.
- `plan.acceptance.evidence.manualUiInspectionLog.contentStatus`:
- `plan.acceptance.evidence.manualUiInspectionLog.missingContentMarker.*`:
  includes missing manual UI inspection checklist item names.
- `plan.acceptance.evidence.externalTargetVerificationLog.contentStatus`:
- `plan.acceptance.evidence.externalTargetVerificationLog.missingContentMarker.*`:
  includes missing external target verification checklist item names.
- `plan.acceptance.evidence.*.missingContentMarker.*`:
- `plan.acceptance.evidence.screenshot.imageReadable`:
- `plan.acceptance.evidence.screenshot.imageFormat`:
- `plan.acceptance.evidence.screenshot.imageWidth`:
- `plan.acceptance.evidence.screenshot.imageHeight`:
- `plan.acceptance.evidence.screenshot.imagePixelsAtLeast4K`:
- `plan.acceptance.evidence.screenshot.imageHasPixelVariation`:
- `plan.acceptance.evidence.screenshot.readyFor4KAcceptance`:
- `plan.acceptance.evidence.ready`:
- `plan.acceptance.evidence.blocker.*`:
- `plan.acceptance.evidence.*.expectedSha256`:
- `plan.acceptance.evidence.*.sha256Status`:
- `plan.acceptance.evidenceSha256.ready`:
- `plan.acceptance.evidenceSha256.blocker.*`:
- `plan.acceptance.recordTimestamp.ready`:
- `plan.acceptance.recordTimestamp.blocker.*`:
- `plan.acceptance.evidenceTimestamp.ready`:
- `plan.acceptance.evidenceTimestamp.blocker.*`:
- `plan.acceptance.linuxKdeWaylandNvidiaManualResult`:
- `plan.acceptance.windowsNvidiaManualResult`:
- `plan.acceptance.physicalDisplayManualResult`:
- `plan.acceptance.externalTargetVerificationManualResult`:
- `plan.acceptance.checklistPassedRecord.*`:
- `plan.acceptance.checklistFailedRecord.*`:
- `plan.acceptance.checklistInvalidRecord.*`:
- `plan.acceptance.checklistMissingRecord.*`:
- `plan.acceptance.manualFailedRecord.*`:
- `plan.acceptance.manualInvalidRecord.*`:
- `plan.acceptance.checklist.ready`:
- `plan.acceptance.checklist.blocker.*`:
- `plan.target.inFirstReleaseScope`:
- `plan.acceptance.targetPlatformCandidate`:
- `plan.acceptance.realKataGoEnvReady`:
- `plan.acceptance.realKataGoEnvStatus`:
- `plan.acceptance.realKataGoTargetRunCandidate`:
- `plan.acceptance.realKataGoManualVerificationCandidate`:
- `plan.acceptance.savedMainConfigReady`:
- `plan.acceptance.savedMainGtpReady`:
- `plan.acceptance.savedMainAnalysisReady`:
- `plan.acceptance.savedCompareConfigReady`:
- `plan.acceptance.savedCompareGtpReady`:
- `plan.acceptance.savedCompareAnalysisReady`:
- `plan.acceptance.envOrSavedMainConfigReady`:
- `plan.acceptance.envOrSavedMainGtpReady`:
- `plan.acceptance.envOrSavedMainAnalysisReady`:
- `plan.acceptance.configuredMainConfigSource`:
- `plan.acceptance.configuredMainGtpSource`:
- `plan.acceptance.configuredMainAnalysisSource`:
- `plan.acceptance.configuredCompareConfigSource`:
- `plan.acceptance.configuredCompareGtpSource`:
- `plan.acceptance.configuredCompareAnalysisSource`:
- `plan.acceptance.configuredKataGoTargetRunCandidate`:
- `plan.acceptance.configuredManualVerificationCandidate`:
- `plan.acceptance.configuredRealtimeGtpTargetRunCandidate`:
- `plan.acceptance.configuredBatchAnalysisTargetRunCandidate`:
- `plan.acceptance.configuredRealtimeGtpManualVerificationCandidate`:
- `plan.acceptance.configuredBatchAnalysisManualVerificationCandidate`:
- `plan.acceptance.configuredDualEngineConfigReady`:
- `plan.acceptance.configuredDualEngineTargetRunCandidate`:
- `plan.acceptance.configuredDualEngineManualVerificationCandidate`:
- `plan.acceptance.configuredDualEngineGtpReady`:
- `plan.acceptance.configuredDualEngineAnalysisReady`:
- `plan.acceptance.configuredDualRealtimeGtpTargetRunCandidate`:
- `plan.acceptance.configuredDualBatchAnalysisTargetRunCandidate`:
- `plan.acceptance.configuredDualRealtimeGtpManualVerificationCandidate`:
- `plan.acceptance.configuredDualBatchAnalysisManualVerificationCandidate`:
- `plan.target.acceptance.linuxKdeWaylandNvidiaCandidate`:
- `plan.target.acceptance.windowsNvidiaCandidate`:
- `plan.target.acceptance.firstReleaseNvidiaPlatformCandidate`:
- `plan.target.acceptance.display4KCandidate`:
- `plan.target.acceptance.highDpiCandidate`:
- `plan.target.acceptance.targetDisplayCandidate`:
- `plan.target.display.primaryDevicePixelsAtLeast4K`:
- `plan.target.display.primaryScaleAtLeast1_5`:
- `plan.target.display.primaryTargetScreenCandidate`:
- `plan.acceptance.display4KCandidate`:
- `plan.acceptance.highDpiCandidate`:
- `plan.acceptance.targetDisplayCandidate`:
- `plan.acceptance.sameScreenTargetDisplayCandidate`:
- `plan.acceptance.multiDisplayCandidate`:
- `plan.acceptance.externalTargetVerificationRequired`:
- `plan.acceptance.externalTargetVerificationStatus`:
- `plan.acceptance.manualRawEngineComparisonRequired`:
- `plan.acceptance.rawKataGoComparisonStatus`:
- `plan.acceptance.manualUiInspectionRequired`:
- `plan.acceptance.manualUiInspectionStatus`:
- `plan.acceptance.finalAcceptanceStatus`:
- `plan.acceptance.finalAcceptanceBlocker.*`:
- `plan.acceptance.finalAcceptanceOutstandingBlocker.*`:
- `acceptanceChecklist` blocker resolved: yes / no
- `acceptanceEvidenceSha256` blocker resolved: yes / no
- `acceptanceRecordTimestamp` blocker resolved: yes / no
- `acceptanceEvidenceTimestamp` blocker resolved: yes / no
- `screenshotEvidence4K` blocker resolved: yes / no
- `plan.acceptance.linuxKdeWaylandNvidiaVerificationStatus`:
- `plan.acceptance.windowsNvidiaVerificationStatus`:
- `plan.acceptance.physicalDisplayVerificationStatus`:

Record any readiness blockers:

- `plan.acceptance.targetPlatformBlocker.*`:
- `plan.target.acceptance.firstReleaseNvidiaPlatformBlocker.*`:
- `plan.target.acceptance.linuxKdeWaylandNvidiaBlocker.*`:
- `plan.target.acceptance.windowsNvidiaBlocker.*`:
- `plan.target.acceptance.targetDisplayBlocker.*`:
- `plan.target.acceptance.multiDisplayBlocker.*`:
- `plan.acceptance.targetDisplayBlocker.*`:
- `plan.acceptance.multiDisplayBlocker.*`:
- `plan.acceptance.realKataGoManualVerificationBlocker.*`:
- `plan.acceptance.configuredManualVerificationBlocker.*`:
- `plan.acceptance.configuredDualEngineManualVerificationBlocker.*`:
- `plan.acceptance.linuxKdeWaylandNvidiaVerificationReadinessBlocker.*`:
- `plan.acceptance.windowsNvidiaVerificationReadinessBlocker.*`:
- `plan.acceptance.physicalDisplayVerificationReadinessBlocker.*`:

## Linux KDE Wayland + NVIDIA

- `plan.acceptance.linuxKdeWaylandNvidiaVerificationChecklist` reviewed: yes / no
- `plan.target.linuxKdeWayland.detected`:
- KDE session detected:
- Wayland session detected:
- Qt `qwayland` platform plugin loaded:
- NVIDIA environment hint present:
- Package starts from extracted directory:
- KataGo executable, model, GTP config, and analysis config configured:
- Realtime analysis starts on the current position:
- Branch navigation resyncs analysis:
- CUDA/OpenCL/TensorRT or GPU stderr is visible in engine diagnostics:
- Result: pass / fail
- Notes:

## Windows + NVIDIA

- `plan.acceptance.windowsNvidiaVerificationChecklist` reviewed: yes / no
- `plan.target.windows.detected`:
- Windows package extracts cleanly:
- App-local Qt runtime and `qwindows` platform plugin load:
- NVIDIA environment hint present:
- Settings path dialogs use native Windows dialogs:
- KataGo executable, model, GTP config, and analysis config configured:
- Realtime analysis starts on the current position:
- Engine diagnostics capture startup, stderr, and failures:
- Result: pass / fail
- Notes:

## High DPI And Multi-Display

- `plan.acceptance.physicalDisplayVerificationChecklist` reviewed: yes / no
- Physical 4K display present:
- 150% or 200% scale tested:
- Multi-display layout tested:
- Board lines, coordinates, star points, stones, and candidate labels are sharp:
- Candidate labels, PV preview, ownership overlays, and graph text do not overlap:
- Saved window geometry restores visibly on the current screens:
- Result: pass / fail
- Notes:

## Raw KataGo Comparison

- `plan.acceptance.rawKataGoComparisonChecklist` reviewed: yes / no
- Analysis JSON raw response matches parsed root info, moveInfos, PV, policy, and ownership:
- Realtime GTP raw line matches candidate table, PV preview, winrate, score, and ownership:
- Genmove, Human vs AI, self-play, and engine-vs-engine moves are legal and synced:
- Result: pass / fail
- Notes:

## External Target Verification

- `plan.acceptance.externalTargetVerificationChecklist` reviewed: yes / no
- Linux KDE Wayland + NVIDIA realtime KataGo run completed:
- Windows + NVIDIA realtime KataGo run completed:
- Physical 4K/high-DPI/multi-display UI inspection completed:
- Raw KataGo UI comparison completed:
- Result: pass / fail
- Notes:

## Manual UI Inspection

- `plan.acceptance.manualUiInspectionChecklist` reviewed: yes / no
- Main-window 4K/high-DPI layout reviewed:
- Board, coordinates, stones, candidate labels, and PV preview reviewed:
- Ownership overlays, mini board, and winrate/score graph reviewed:
- Toolbar, dock, menu, bilingual text, saved-window restore, and multi-display placement reviewed:
- Result: pass / fail
- Notes:

## Final Acceptance

- KDE Wayland + NVIDIA acceptance: pass / fail
- Windows + NVIDIA acceptance: pass / fail
- 4K/high-DPI/multi-display UI acceptance: pass / fail
- Raw KataGo UI comparison acceptance: pass / fail
- `plan.acceptance.finalAcceptanceBlocker` resolved: yes / no
- Mode-specific config blockers resolved (`mainRealtimeGtpConfig`,
  `mainBatchAnalysisConfig`, `compareRealtimeGtpConfig`,
  `compareBatchAnalysisConfig`): yes / no
- `plan.acceptance.finalAcceptanceOutstandingBlocker.*`:
