# PLAN.md Requirement Audit

This audit turns the repository-root `PLAN.md` into explicit verification
items. It complements `PlanCoverage.md`: that file lists the broad automated
coverage, while this file states what evidence proves each requirement and what
still requires a real target machine.

## Evidence Legend

- Automated: covered by CTest, package verification, static checks, or installed
  CLI smoke tests.
- Optional real KataGo: covered when `LIZZIE_KATAGO_*` points at a real KataGo
  executable, model, GTP config, and analysis config.
- External acceptance: cannot be proven by the local offscreen fixture suite and
  must be recorded with `--target-acceptance-record`.

## Architecture

| PLAN item | Evidence |
| --- | --- |
| Qt 6, C++20, root CMake project | Automated: `CMakeLists.txt`, CI workflow tests, package verification. |
| Replace the legacy Java app while retaining migration compatibility | Automated: legacy Java sources and build files are absent from the working tree; Java UI/runtime artifacts are rejected from Qt packages, while config-import tests cover migration from existing installations. |
| Linux KDE Wayland NVIDIA and Windows NVIDIA target scope | Automated diagnostics and package checks name the target scope; external acceptance records prove actual hardware runs. |
| No macOS first-release support | Automated: CMake rejects Darwin and package verification rejects macOS artifacts. |
| Core, engine, app, and UI layering | Automated: library split plus `LayerBoundaryCheck.cmake`, `NoDirectUiGtpCommands.cmake`, and `NoJavaUiDeps.cmake`; executable-entry separation from the app library, non-engine `QProcess`/`QThread` API leaks, app/UI engine-private process header leaks, and engine public `QThread` API leaks are rejected. |
| No recreated Java global UI state | Automated: app and UI tests exercise state through model/settings/session helpers; legacy `Lizzie.*` global-state markers are rejected. |

## Core Model

| PLAN item | Evidence |
| --- | --- |
| 9x9, 13x13, 19x19, and rectangular board support | Automated: core, position-sync, batch, UI, and real-KataGo preflight tests cover square, rectangular, and wide boards. |
| Point, color, move, board, node, game-info, and candidate data | Automated: core model and analysis tests cover these structures and serialization. |
| SGF read/write for GM/FF/SZ/KM/HA/PB/PW/BR/WR/RE/DT/C/B/W/AB/AW/AE/LB/CR/SQ/TR/MA/MN | Automated: SGF and UI smoke tests cover metadata, setup stones, moves, markup, comments, branches, and write-back. |
| Preserve unknown SGF properties | Automated: core tests preserve unknown properties and filter invalid in-memory property names. |
| Legacy Lizzie/KataGo analysis comments | Automated: batch and SGF tests cover legacy/generated comment import and write-back. |
| Local legality, captures, suicide, ko, undo, branch switching, and try play | Automated: core and UI smoke tests cover legal moves, captures, ko, editing, navigation, and try/play modes. |

## KataGo Engine

| PLAN item | Evidence |
| --- | --- |
| User-provided KataGo executable, model, GTP config, analysis config, work dir, and extra args | Automated: settings, diagnostics, legacy import, and package smoke tests cover env and saved path readiness. |
| Do not guess GPU backend; display stderr | Automated: process diagnostics and target evidence gates check CUDA/OpenCL/TensorRT/stderr markers. |
| Startup probes for name, version, list_commands, kata-analyze, kata-set-rules, kata-raw-nn, genmove | Automated: engine fixture, worker, UI, and integration preflight tests. |
| Position sync through clear_board, board size or rectangular_boardsize, komi, and full play replay | Automated: position-sync and UI smoke tests, including branch resync and wide-board rejection. |
| Realtime kata-analyze with ownership, turn, visits, playouts, and time options | Automated: realtime parser, settings, UI, and optional real KataGo tests. |
| Batch analysis JSON with unique ids, moves, initialStones, rules, komi, limits, rootInfo, moveInfos, and ownership | Automated: analysis JSON, batch analysis, UI progress, and optional real KataGo tests. |
| Startup, parse, and exit diagnostics without automatic restart | Automated: fixture, process, worker, and UI smoke tests cover startup failure, raw-line preservation, clean/failing exits, and manual restart. |

## UI/UX

| PLAN item | Evidence |
| --- | --- |
| Main board, candidate/PV dock, graph, game tree, toolbar, menu, and docks | Automated: UI smoke, board item, graph widget, and target diagnostics check structure and layout. |
| Toolbar limited to common actions | Automated: UI smoke tests verify named toolbar actions and icons. |
| 4K/high-DPI vector board, stones, labels, candidates, and ownership overlays | Automated: offscreen 4K and scale 1.5/2 tests; external acceptance proves physical display sharpness. |
| Winrate, score, loss, and visits graph with hover/click navigation | Automated: graph widget and UI smoke tests. |
| Engine, analysis, board display, shortcut, theme, language, and file behavior settings | Automated: settings dialog, settings model, diagnostics, localization, and UI smoke tests. |
| First-run Configure Engine and No Engine Mode | Automated: first-run and UI smoke tests cover both paths. |

## Feature Scope

| PLAN item | Evidence |
| --- | --- |
| SGF open, save, save as, branch editing, comments, marks, and navigation | Automated: core and UI smoke tests. |
| KataGo realtime candidates, PV, ownership, scoreMean, and winrate | Automated fixtures plus optional real KataGo raw-output comparison. |
| Batch analysis progress, cancel, UI recovery, SGF/sidecar write-back, and failure diagnostics | Automated: batch, UI, performance, and file-write tests. |
| Human-vs-AI, self-play, dual-engine comparison, and engine-vs-engine play | Automated: UI smoke, engine automation, and optional real KataGo tests. |
| GTP console, engine log, and error diagnostics | Automated: console widget, process, worker, diagnostics, and UI smoke tests. |
| Chinese/English UI, light/dark theme, 4K/high DPI | Automated: localization, settings, UI smoke, and offscreen scale tests; external acceptance proves physical target displays. |
| Deferred features stay out of scope | Automated: `FirstReleaseScopeCheck.cmake` rejects first-release out-of-scope implementation markers, and the package verifier rejects remote SSH, online board, screen sync, distributed training, non-KataGo engine, macOS, Java, and bundled runtime artifacts. |
| One-time Java config import | Automated: legacy config import and UI smoke tests cover main/compare engine paths and skipped SSH entries. |

## Acceptance Criteria

| Criterion | Required proof |
| --- | --- |
| KDE Wayland NVIDIA startup, KataGo configuration, and realtime analysis | External acceptance record with Linux checklist pass values, diagnostics evidence, engine log evidence, and target acceptance report evidence. |
| Windows NVIDIA startup, extraction, Qt plugin loading, native path dialogs, KataGo configuration, and realtime analysis | External acceptance record with Windows checklist pass values and the same evidence bundle. |
| 19x19 4K board, text, candidates, graph, ownership, and multi-display restoration are sharp and non-overlapping | External acceptance record with physical-display and manual-UI checklist pass values plus screenshot evidence that reaches the 4K pixel envelope. |
| SGF read/write and branch navigation do not lose mainline or common properties | Automated: core/UI tests plus package verification coverage. |
| KataGo candidates, PV, winrate, scoreMean, and ownership match raw KataGo output | Optional real KataGo tests plus external raw comparison evidence and checklist pass values. |
| Batch analysis can cancel, recover UI, and report failed nodes | Automated: batch, UI, and performance tests. |
| No Java Swing/AWT dependency in the first Qt release | Automated: static source checks and package artifact rejection. |

## Completion Gate

The project is locally release-candidate complete only when all automated tests
and package verification pass. The full `PLAN.md` goal is complete only when the
external acceptance record has `finalAcceptanceStatus` ready and no final
acceptance blockers after running on the required KDE Wayland NVIDIA and Windows
NVIDIA target machines.
