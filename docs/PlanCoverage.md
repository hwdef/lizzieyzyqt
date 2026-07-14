# PLAN.md Coverage

This document maps the first Qt release scope in the repository-root `PLAN.md`
to code, tests, and the remaining checks that need real target machines or a
real KataGo setup. `PlanRequirementAudit.md` provides the same scope as a
requirement-by-requirement completion audit.

## Automated Evidence

- Project and packaging: `CMakeLists.txt`, `.github/workflows/qt-cmake.yml`,
  `cmake/VerifyPackage.cmake`, `lizzie_ci_workflow_tests`, and
  `lizzie_verify_package_script_tests` cover Qt 6/C++20, Linux/Windows CI,
  PLAN.md-triggered CI for the shipped repository-root plan,
  platform-specific package filename verification,
  Linux/Windows packaging, installed executable smoke tests, installed diagnostics smoke tests,
  installed target acceptance report smoke tests,
  installed target acceptance record template smoke tests with an invalid QPA platform,
  target acceptance report CLI smoke tests, packaged target acceptance record template smoke tests
  through `PACKAGE_RUN_TARGET_ACCEPTANCE_RECORD_TEMPLATE_SMOKE`,
  using Linux `offscreen` QPA and Windows native `windows` QPA,
  OS product/kernel, Qt build ABI, build/current CPU architecture,
  Qt build/runtime version match, and Qt install-path diagnostics,
  for ready/missing/invalid KataGo env readiness, Windows Qt
  deployment checks, package artifact upload, installed documentation presence
  with README-relative `docs/` layout plus the shipped root `PLAN.md`, first-run
  analysis-config documentation smoke checks, and
  onboarding/migration/verification content including command-argument
  migration notes, installed strict CLI and target acceptance record argument validation smoke tests, target-machine acceptance report full evidence/SHA-256/checklist template coverage with evidence-content-marker guidance, record-hint metadata requirement manifest, and record-hint guidance, CI packaged `--diagnostics`
  `--target-acceptance-report`, `--target-acceptance-record-template`, and
  explicit `--target-acceptance-record <ini>` record ingestion Qt
  platform/app, app/Qt filesystem path size/mtime, app-local Qt runtime artifact diagnostics,
  QPA-independent target acceptance record template generation,
  Qt library path statuses,
  QStandardPaths writable locations, QSettings storage location,
  QSettings session/window restore keys, saved session SGF path-status,
  saved window-geometry visibility checks, saved window-state restore checks, and current platform plugin candidates
  plus always-reported common Wayland `qwayland` plugin detection,
  always-reported common Windows `qwindows` plugin detection, available
  platform plugin listing, Qt Quick graphics API, scene graph, QML import paths,
  renderer-interface RHI/shader fields, and screen smoke tests
  with KDE/Wayland/Windows/high-DPI/GPU environment fields, screen geometry and average/per-axis DPI,
  physical-size, refresh-rate, device-pixel geometry, orientation, and virtual-sibling fields,
  OpenGL runtime context diagnostics for context creation, format, vendor,
  renderer, and version strings,
  temporary `LIZZIE_KATAGO_*` path preflight fixtures,
  target acceptance report configured-path and argument summaries plus ready-env and saved-config smoke output,
  target acceptance report manual-record, record-file path/size/mtime/SHA-256 diagnostics, checklist-item, checklist missing-record diagnostics, checklist readiness/final `acceptanceChecklist` blocker enforcement, UTC metadata-readiness with `completedUtcStatus` future-timestamp rejection, record-file timestamp diagnostics/final `acceptanceRecordTimestamp` blocker enforcement, app-version/app-executable SHA-256/OS/Qt-runtime/CPU/GPU-driver/KataGo-version metadata match diagnostics/final `acceptanceMetadata` blocker enforcement, non-empty evidence-path ingestion with distinct-path rejection, diagnostics/report/engine-log/raw-log/manual-UI-log/external-target-log content-marker checks and missing-marker diagnostics including diagnostics app-version/executable-path/record-path/record-sha/final-blocker binding, target-report app-version/executable-path/record-path/record-sha/final-blocker binding, engine-log and raw-log recorded KataGo-version binding, engine-log GPU/stderr marker evidence gates, raw comparison checklist-item evidence markers, manual UI inspection log evidence markers, external target verification log evidence markers, and raw `kata-analyze`/`rootInfo`/`moveInfos`/`winrate`/`scoreMean`/`scoreStdev`/`visits`/`policy`/`pv`/`pvVisits`/`ownership` evidence markers with targeted winrate/scoreMean/scoreStdev/policy/visits/pv/pvVisits/ownership missing-marker diagnostics, and SHA-256 evidence digests, required evidence SHA-256 pin missing/mismatch diagnostics/final `acceptanceEvidenceSha256` blocker enforcement, evidence timestamp diagnostics/final `acceptanceEvidenceTimestamp` blocker enforcement, screenshot evidence image format/dimension/4K-pixel/variation diagnostics, complete 4K screenshot evidence blocker removal, complete checklist final blocker removal, diagnostics complete checklist final blocker removal, installed diagnostics complete checklist final blocker smoke, a final `screenshotEvidence4K` blocker for weak screenshot evidence, target-component/checklist failed/invalid manual final status priority with aggregate manual failed/invalid record diagnostics, and satisfied manual blocker removal,
  target acceptance report mode-specific final blocker smoke output, and rejection of
  bundled KataGo, KataGo config across common config file extensions,
  KataGo/common neural-network model artifacts across package paths,
  non-KataGo engine artifacts, Java runtime artifacts,
  Windows runtime artifacts in Linux packages,
  Linux runtime artifacts in Windows packages, and out-of-scope artifacts
  for remote SSH engines, online board integrations, screen board synchronization,
  distributed training, and macOS artifacts.
- No Java UI dependencies and UI/app/engine boundaries:
  `cmake/NoJavaUiDeps.cmake`, `cmake/NoDirectUiGtpCommands.cmake`,
  `cmake/LayerBoundaryCheck.cmake`,
  `cmake/FirstReleaseScopeCheck.cmake`,
  `cmake/LocalizationTextCheck.cmake`, `lizzie_no_java_ui_dependencies`,
  `lizzie_no_direct_ui_gtp_commands`, `lizzie_layer_boundary_check`,
  `lizzie_first_release_scope_check`, and
  `lizzie_localization_text_check`
  scan the Qt source for Swing/AWT markers, legacy `Lizzie.*` global-state
  markers, core/engine/app layer boundary violations, executable-entry separation from the app library,
  non-engine QProcess/QThread dependency leaks,
  app/UI engine-private process header leaks, engine public QThread API leaks,
  first-release out-of-scope implementation markers for remote SSH, online board sync, screen board sync, distributed
  training, and non-KataGo engines, and direct presentation-side GTP command
  construction/sending, plus mixed
  Chinese/English analysis-config and GTP-config diagnostics.
- Core model: `lizzie_core_tests`, `lizzie_batch_analysis_tests`, and
  `lizzie_position_sync_tests` cover board sizes up to 52x52, rectangular SGF,
  legal move checks, captures, ko, suicide, pass/resign, branches, setup stones,
  invalid setup-position diagnostics across core, GTP sync, and batch analysis,
  SGF metadata, trimmed SGF numeric metadata parsing with positive `HA` and nonnegative `MN` filtering,
  finite SGF/Analysis/GTP komi handling, rule alias canonicalization, unknown-property preservation
  with invalid in-memory SGF property-name filtering,
  labels, marks, core/editor duplicate/conflicting setup, comment, and markup canonicalization, and legacy
  analysis import/export including root and child generated-comment imports with readable stdev, policy, PV-visit fields, strict legacy/generated PV validation, strict legacy/generated PV-visit validation, strict sidecar/Analysis JSON PV validation, strict sidecar/Analysis JSON PV-visit validation, PV/PV-visit length consistency, normalized readable analysis rate export, KataGo extension-field PV boundaries, finite legacy/generated analysis numeric parsing, legacy/generated visit-count overflow rejection, legacy candidate ownership alias import, strict legacy candidate ownership validation, generated-comment root/candidate ownership import/write-back, strict generated-comment ownership validation, sidecar precedence over legacy SGF analysis, visits-sorted sidecar candidate reload/export, sidecar scoreLead/prior alias import, sidecar invalid root field fallback, sidecar success/error exclusivity, sidecar top-level nodes diagnostics, sidecar malformed node-entry diagnostics, strict sidecar nodeId diagnostics, strict sidecar visit-count parsing, finite sidecar numeric parsing, strict sidecar ownership array validation, trimmed numeric-string sidecar scalar/ownership parsing, finite sidecar/SGF/readable analysis export, nonnegative sidecar/SGF visit export, visits-sorted legacy analysis import with candidate-only root fallback, persisted analysis import ownership filtering, sidecar/SGF ownership write-back size filtering, and generated analysis-error comments kept from becoming success snapshots.
- KataGo protocol and process handling: `lizzie_engine_protocol_tests`,
  `lizzie_realtime_analysis_tests`, `lizzie_analysis_json_tests`,
  `lizzie_katago_process_fixture_tests`, `lizzie_analysis_process_fixture_tests`,
  and `lizzie_engine_worker_tests` cover GTP parsing, `kata-analyze` output,
  strict realtime GTP visit-count parsing, strict realtime GTP PV/PV-visit validation and length consistency, finite realtime GTP numeric parsing,
  startup `name`/`version`/`list_commands` probes, detected `kata-analyze`,
  `kata-set-rules`, `kata-raw-nn`, and `genmove` capabilities, unique Analysis
  Engine request ids, JSON requests/responses with numeric-string Analysis JSON parsing and visits-sorted Analysis JSON candidates,
  newline-safe GTP command serialization, positive GTP command id serialization/reservation,
  threaded GTP command-id alignment,
  strict GTP response id parsing, numeric GTP response payload preservation,
  invalid GTP board-size coordinate rejection,
  per-field Analysis JSON root fallback,
  zero Analysis JSON root visit fallback/rejection,
  complete Analysis JSON ownership filtering, strict Analysis JSON ownership array validation,
  per-field realtime rootInfo fallback, invalid-board realtime success-update rejection,
  strict Analysis JSON response id parsing, strict Analysis JSON visit-count parsing, isolated Analysis JSON error payload parsing, empty Analysis JSON success response rejection,
  unknown/duplicate Analysis JSON response ignoring, invalid-board Analysis JSON success-response rejection,
  Analysis JSON request point filtering, Analysis JSON request board-size fallback, search limits including visits,
  playouts, time, PDA, and WRN, bounded realtime `kata-analyze` interval serialization,
  finite Analysis JSON request option serialization,
  finite realtime search-parameter serialization, positive Analysis JSON/realtime search-limit serialization,
  realtime candidate ownership size filtering, finite realtime direct-update ownership filtering,
  root and candidate ownership sized to the requested board including
  rootInfo ownership fallback, finite applied-response ownership filtering,
  finite applied-response scalar/ownership filtering, visits-sorted applied-response candidates
  with root fallback, and incomplete ownership filtering, PV visits, process diagnostics,
  standard top-level `kata-analyze` ownership blocks as well as legacy-adjacent
  root ownership, top-level realtime ownership precedence, movesOwnership realtime candidate precedence, ownership-only realtime output forwarding,
  documented `kata-analyze` extension fields as PV boundaries,
  worker threads, engine-level outbound GTP command-name validation, burst realtime output forwarding, UI event-loop
  responsiveness during threaded output, CUDA/OpenCL/TensorRT stderr fixture
  lines, working directories, argument-vector construction for paths with spaces
  and settings persistence plus legacy Java import round trips for quoted, empty,
  and Windows-style extra arguments, finite legacy Java analysis option import,
  bounded legacy Java analysis option import, strict legacy Java boolean option import,
  startup failure diagnostics with
  command/working-directory/stderr summaries, clean and failing process exits,
  and preservation of final probe responses before exit.
- UI workflows: `lizzie_ui_smoke_tests`, `lizzie_board_quick_item_tests`,
  `lizzie_analysis_graph_widget_tests`, `lizzie_settings_dialog_tests`, and
  `lizzie_gtp_console_widget_tests` cover main-window actions, the named main
  toolbar limited to PLAN.md common actions with icons, first-run Configure
  Engine including GTP and analysis path browse buttons, Import Java Config with main and
  compare GTP/analysis command migration, and No Engine Mode, SGF open/save,
  dock placement, game info including analysis-preserving metadata edits,
  analysis-invalidating rules/komi edits, and save/reload protection against
  stale sidecar analysis, Save As copying of loaded sidecar analysis, and
  interactive Save warnings when loaded sidecar synchronization fails plus
  structural-edit sidecar cleanup, new
  square/rectangular games, branch editing, setup stones, comments, markup,
  realtime analysis, candidate tables, PV
  previews, stone textures with generated fallback stones, ownership display
  modes, graph winrate/score/loss/visits rendering plus plot-column hover/click, batch
  analysis including main/compare, wide-board sidecar ownership/PV-visit preservation, and simultaneous dual-realtime isolation plus navigation while a batch run is active, estimate mode including main/compare and simultaneous dual-realtime isolation while showing compare-table policy and PV visits, plus navigation while an estimate request is running, human-vs-AI, self-play, engine-vs-engine, manual AI
  Move, human-vs-AI, and self-play including realtime Analyze stop before genmove plus AI Move/human-vs-AI/self-play Compare resync after generated/user moves, Engine Game startup stopping realtime/compare analysis before genmove, realtime and compare analysis, terminal pass/resign handling for
  automated play including stopping Compare analysis on engine resigns, unparsable
  genmove diagnostics and no-SGF-write protection for manual AI Move,
  localized wide-board GTP sync diagnostics for realtime and Compare analysis
  plus manual AI Move, human-vs-AI replies, self-play, and Engine Game
  main/compare turns, and wide-board manual AI Move, human-vs-AI,
  self-play, and Engine Game main/compare turns,
  human-vs-AI, self-play, and engine-vs-engine paths including cold and already-running
  main/compare missing-genmove capability shutdown before stale Engine Game requests,
  main startup-failure shutdown before compare starts, compare startup-failure
  shutdown after preserving the main Engine Game move, manual AI Move,
  human-vs-AI, and self-play clean exits after generated moves with no stale
  follow-up or auto-restart, pending-reply manual moves and Pass, Resign confirmations with terminal Analyze/Compare shutdown, graph navigation, Open, Save/Save As, Import Java Config, New Game, non-play board edit modes, Play/Try/Pass, Delete Branch, Game Info,
  branch selection, Pass, Undo, Promote Variation, and Delete Branch resync while Analyze/Compare are active,
  Settings including runtime pure-UI updates that do not resync running realtime
  or compare analysis, complete Batch/Estimate runs over pending human-vs-AI/self-play/engine-game
  genmove requests and incomplete Batch/Estimate configuration prompts clearing, close-event cleanup for pending automated
  replies and running batch analysis, manual move, engine-generated moves, Pass, Resign, setup, and branch edits, New Game/Game
  Info/Settings modal dialogs plus Open/Save As/Import Java Config path
  selectors cancelling running batch analysis before stale sidecar output, New Game standard handicap option gating
  for square-board `HA`/`AB` generation without invalid `HA[1]` or rectangular handicap metadata, UI-level collection sidecar
  metadata, label/mark non-play edit modes
  clearing pending automated replies, batch and estimate
  cancellation without stale sidecar output, raw
  GTP console input including quoted, empty, and spaced arguments plus single-token command-name validation, balanced-quote validation, invalid raw GTP console diagnostics, and localized system/stderr prefixes, engine log
  output, theme/language switching including live refresh of loaded candidate/PV and game-tree move labels, localized
  ownership/sidecar load warnings/legacy SGF import warnings/analysis-error display/process diagnostic/raw-response labels, UI-level unknown/duplicate Analysis JSON response diagnostics, engine-log source prefixes/config/local move legality/GTP sync/batch planning
  diagnostics, SGF write-back of edited setup/label/mark properties plus `LZOP`/`LZ`/`LZYPERSP` and `LZYERR` with user comments preserved, stale structured SGF properties removed, and generated analysis comments replaced cleanly, SGF parse, missing-session, invalid SGF collection-index restore diagnostics, and collection restore diagnostics, compare engine
  console/status prefixes, localized main/compare clean-exit diagnostics that
  stop analysis without auto-restart, manual Restart recovery for main and
  compare engines, and standard dialog
  buttons, Settings path selector routing for KataGo/model/GTP config/main and
  compare analysis config/work-dir/stone texture rows including cancellation,
  zero-value unlimited/off semantics for Settings search limits, PDA, and WRN,
  Settings file-behavior toggles for legacy SGF analysis import and analysis
  sidecar load/write behavior,
  startup and runtime configurable shortcuts for common navigation/settings flows,
  realtime raw `kata-analyze`
  candidate/rootInfo/ownership propagation into the candidate table, PV preview,
  root ownership display including standalone ownership-line UI/SGF write-back,
  candidate ownership SGF write-back, malformed realtime `kata-analyze` raw-line
  preservation without corrupting later candidates, 100-node batch-analysis UI
  progress/event-loop responsiveness with action recovery and sidecar output,
  and CLI target-machine diagnostics for Qt platform, app
  executable path status, app directory status, process working directory status,
  runtime Qt library path status,
  Qt plugin path status, current platform plugin candidate paths and status,
  app/Qt filesystem path size/mtime,
  QStandardPaths writable locations for config/data/cache/runtime directories,
  application font/text metrics for high-DPI and bilingual UI inspection,
  locale/UI-language diagnostics, runtime appearance style/palette diagnostics,
  native file dialog/settings path selector diagnostics,
  main-window UI structure diagnostics for the Quick board, toolbar, docks, tables, tree, and menus,
  main-window 4K/high-DPI layout geometry smoke for toolbar action overlap, dock/tab sizing, and nonblank window rendering,
  and stored and normalized appearance settings,
  finite stored/saved UI settings normalization,
  strict stored boolean settings parsing,
  first-run completion diagnostics,
  saved engine setting path diagnostics for main/compare executable, model,
  GTP config, analysis config, working directory, and extra args,
  stored extra-args parsed diagnostics,
  stored engine minimum-readiness diagnostics and stored engine path-readiness diagnostics
  with missing/invalid saved settings coverage,
  stored analysis option diagnostics for interval, ownership, visits, playouts,
  time, PDA, and wide-root noise,
  stored board display diagnostics for ownership mode, opacity, and stone texture paths,
  stored file behavior diagnostics for legacy SGF import and sidecar load/write toggles,
  stored shortcut diagnostics for configured and disabled keyboard shortcuts,
  QSettings storage location and file-backed/native backend marker,
  QSettings session/window restore keys for saved SGF, current node, collection index, geometry, and dock state,
  saved session SGF path-status,
  saved window-geometry restore/visibility/recenter checks against current available screens,
  saved window-state restore checks,
  always-reported common Wayland `qwayland` plugin detection with candidate path statuses,
  always-reported common Windows `qwindows` plugin detection with candidate path statuses,
  available platform plugin listing,
  path-list environment diagnostics for runtime search paths, QML import paths,
  Qt Quick Controls style paths, Qt plugin, KWin DRM, Vulkan ICD, Vulkan driver,
  and `__EGL_VENDOR_LIBRARY_FILENAMES` lists, including
  set-empty versus unset environment values and blank path-list entry handling,
  Qt Quick graphics API, scene graph backend, QML import paths,
  QML import environment, Qt Quick Controls configuration paths,
  renderer-interface RHI/shader fields,
  KDE/Wayland/Windows graphics backend, user/profile/temp path environment,
  `XDG_RUNTIME_DIR`, and high-DPI scale environment,
  target-platform summary diagnostics for first-release OS scope, KDE Wayland,
  Windows platform, NVIDIA environment hints from explicit GPU variables plus
  Vulkan/EGL/GBM path values, target-platform blocker diagnostics,
  Linux KDE Wayland + NVIDIA and Windows + NVIDIA acceptance candidate flags, primary/any-screen 4K detection,
  PLAN acceptance summary diagnostics for real KataGo env readiness, saved
  main/compare engine config readiness, env-or-saved main config readiness,
  configured source diagnostics, configured acceptance status, target platform/display candidates, manual
  verification candidate flags, manual verification blocker diagnostics, realtime GTP and batch-analysis acceptance
  candidate flags, dual-engine acceptance candidate flags, mode-specific final acceptance blocker diagnostics, text diagnostics and final-section outstanding blocker diagnostics, manual UI inspection checklist diagnostics, raw KataGo comparison checklist diagnostics, an external target verification checklist, target-specific verification checklist diagnostics, target-specific verification status diagnostics, explicit external/manual verification gates, and final/manual acceptance status diagnostics,
  primary/any-screen high-DPI scale, same-screen target-display candidate diagnostics, display blocker diagnostics, and
  multi-display detection,
  extended graphics environment diagnostics for Qt scale rounding,
  Qt Quick render-loop/debug settings, OpenGL/EGL vendor selection, and
  NVIDIA/CUDA/Vulkan selection, primary screen geometry,
  primary available geometry, virtual screen geometry, screens, average/per-axis DPI, device-pixel ratio, device-pixel
  geometry, physical size, refresh rate, screen orientation,
  virtual-sibling layout, screen identity fields, and `LIZZIE_KATAGO_*`
  path value/nonblank-text/absolute/canonical/readable/writable/size/mtime preflight plus a
  KataGo env readiness summary with missing, blank, and invalid path coverage,
  high-DPI scale factors, 4K offscreen rendering, and window
  restore from offscreen or barely visible stale multi-display geometry, with shared window placement helper coverage for
  minimum visible-area thresholds and primary-screen recentering.
- Performance-sensitive paths: `lizzie_performance_tests` and
  `lizzie_ui_smoke_tests` cover large SGF round trips, branch-heavy SGF round
  trips, long PVs, high-frequency realtime updates, 100-node batch planning,
  and 100-node batch UI progress/event-loop responsiveness through sidecar
  completion.
- Optional real KataGo: `lizzie_katago_integration_preflight_tests` runs
  without a real engine and verifies all-unset KataGo integration skip path-status diagnostics plus invalid `LIZZIE_KATAGO_*` path diagnostics
  before process launch, including empty and whitespace-only env values reported
  with `hasText=false` and `absolutePath=(blank)`; `lizzie_katago_integration_tests` is part of CTest and
  runs when the same environment variables point at a real KataGo executable,
  model, analysis config, and/or GTP config. It covers analysis
  JSON raw-response consistency for root info, moveInfos, PV visits, policy,
  and ownership, realtime GTP, rectangular realtime GTP, selected-variation branch
  replay, dual-process realtime compare,
  Human-vs-AI style user move plus legal engine reply, engine-vs-engine
  two-process genmove/play exchange,
  realtime snapshot consistency against the raw `kata-analyze` line for
  candidates, PV visits, root info, and ownership, plus short self-play smoke
  paths.

## External Acceptance Still Required

The following `PLAN.md` acceptance items cannot be fully proven from the local
offscreen fixture suite alone:

- KDE Wayland + NVIDIA startup, realtime KataGo analysis, branch resync, and
  CUDA/OpenCL/TensorRT stderr behavior.
- Windows + NVIDIA package extraction, Qt plugin loading, native Settings path
  dialog behavior, and realtime KataGo analysis.
- Physical 4K and multi-display inspection for sharp board text, candidates,
  PV stones, ownership overlays, graph rendering, and restored window placement.
- Manual UI-level Real KataGo comparison against raw engine output for candidate
  table rendering, winrate/score perspective, ownership display, genmove,
  human-vs-AI, self-play, and engine-vs-engine behavior.

Use `Verification.md` for the exact manual and real-engine commands.
