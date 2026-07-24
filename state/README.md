# State README

Project: QGroundControl Quad-Rover PX4 adaptation.
Primary repository: `E:\workspace\QGC\qgroundcontrol`.
Active worktree: `E:\workspace\QGC\qgroundcontrol-worktrees\quad-rover-design`.
Base integration branch: `codex/joystick-aux-px4-development` at `754135601`.
Active local branch: `codex/quad-rover-design`.

Current constraints: implementation is active under the approved SDD workflow.
Tasks 1-3 are complete: QGC pins and validates the released `qgc_hybrid` r2
contract, models type 200 as a permanent distinct Quad-Rover class, and owns
freshness/reboot epochs in `HybridVehicleState` through `93a5ad457`. Do not
force-push, update the base integration branch, create a PR without explicit
approval, store credentials, or stage the unrelated untracked `NUL` entry.
Preserve the historical Windows test-hardening record below.

## 2026-07-22 Quad-Rover adaptation

- PX4 source: `\\wsl.localhost\Ubuntu\home\crocodile\PX4-Autopilot-change1_v1.16.1`, branch `change1_v1.16.1`, protocol revision `82478dbdf2`.
- MAVLink release input: `https://github.com/QQgdiw/mavlink.git`, annotated tag `qgc-hybrid-change1-v1.16.1-r2`, tag object `80efde60a914f5b368eda7ec1b2d92100287ca32`, peeled commit `04ad1d63e9c11ed6767a35dae4e52adaca3538c5`, dialect `qgc_hybrid`, MAVLink 2.
- The user reports that fresh `generate_c_headers`, the root-header C++ probe, fresh Windows CPM configure, and a 2705-step Release QGC C++ build completed against r2. This agent independently confirmed the remote annotated/peeled references and the local QGC CPM variable plumbing; it did not rerun the reported full release build in this worktree.
- The old `hybrid-change1-v1.16.1` tag at `3b84efb97a7c0b4767868e8725bd6902c0d884e8` and r1 are rejected build inputs. `qgc_hybrid` retains APM support and excludes Storm32 because of message 60000 collision.
- The user selected the development branch as the implementation base to retain validated downstream work; no rebase to upstream QGC main is part of this task.
- The approved design is `docs/superpowers/specs/2026-07-22-quad-rover-adaptation-design.md`; the detailed implementation plan is `docs/superpowers/plans/2026-07-23-quad-rover-implementation.md`.
- Command 50000 creates one queue entry per UI request and uses the normal queue retry machinery until its first strictly matched ACK. `HybridTransitionController::requestTransform` is the only QML send path and rejects every further UI request before it reaches the queue; `IN_PROGRESS` then explicitly detaches the long lifecycle and validates the full ACK address tuple and exact transition sequence.
- QGC currently sends but does not consume `SYSTEM_TIME`. The design requires exact-autopilot, wrap-safe `time_boot_ms` rollback plus lower-HRT confirmation within a bounded local window; if PX4 lacks an RTC, QGC resends time and uses a constrained three-status fallback so a reboot cannot permanently fail-close hybrid controls.
- The CMake contract must use r2 `qgc_hybrid` with APM MAVLink/plugin enabled, validate the resolved CPM commit, and reject raw `all`, raw `hybrid_vehicle`, disabled APM options, or a changed release input before C++ compilation.
- Task 1 implemented that contract in commit `ec03aa473`; its seven-case CMake matrix passed, a fresh VS2022 x64 configure resolved MAVLink to `04ad1d63e9c11ed6767a35dae4e52adaca3538c5`, and independent review found no issues.
- Task 2 implemented the exclusive type-200 classification and `Vehicle::quadRover` property in `3825f50c2`; three relevant MSVC objects compiled and independent review found no issues.
- Task 3 implemented `HybridVehicleState` in `f3e799c02` and fixed delayed pre-wrap packets in `93a5ad457`. Its focused QtCore harness passed after a red/green regression cycle; independent re-review found no issues.

## 2026-07-13 status

- `/Zc:preprocessor` was codified in `cmake/platform/Windows.cmake` for MSVC Windows builds.
- Clean Debug build `build/windows-debug-vs2022-zc-fixed` succeeds without manually passing `CMAKE_CXX_FLAGS`.
- Failed-test details are stored in `.tmp/test-runs-zc-fixed`.
- `origin/master` comparison worktree is `E:\workspace\QGC\qgroundcontrol-origin-master`; same 11-test behavior was reproduced there.
- Current evidence indicates the 11 test failures are not introduced by `codex/joystick-aux-px4`.
- Release configure/build now succeeds in `build/windows-release`; the executable ran for a 20-second observation window with empty stdout/stderr.
- NSIS 3.12 is available, but install/package generation was intentionally not run.
- The 11 targeted classes were rerun under Codex full access with unchanged assertion patterns. The Windows token remained medium-integrity and non-administrator.

## 2026-07-13 test-hardening branch

- Worktree: `E:\workspace\QGC\qgroundcontrol-worktrees\windows-test-hardening`
- Branch: `codex/windows-test-hardening`
- Base feature commit: `daae3a37b`; `origin/codex/joystick-aux-px4` was fetched and had no newer commits.
- Clean Debug test build completed in `build/windows-debug` with VS2022 v143 x64.
- Targeted baseline reproduced 8 failing suites / 11 assertion failures; 3 suites pass individually.

## 2026-07-14 Phase 1 result

- Fixed locale-sensitive JSON validation assertions.
- Fixed Windows/Unix mismatch in Platform initialization expectations.
- Fixed GeoTag test drive-letter comparison and read-only temporary resource cleanup.
- Fixed production Windows drive/UNC path classification in `QGCFileHelper`; downstream archive URL handling now passes.
- Phase 1 regression: 5 suites, 118 tests, 0 failures, 0 errors, 0 skips.
- Remaining Phase 2 assertion baseline: 4 suites / 6 failures (`ComponentInformationCacheTest`, two Mission suites, and `QGCCompressionTest`).
- Remaining runner investigation: `HashCheckTest`, `SigningTest`, and `RequestMetaDataTypeStateMachineTest` pass individually but previously failed or timed out through CTest.

## 2026-07-14 Phase 2A result

- Replaced the unreliable Windows `QFile::link` directory-symlink probe with a real nonthrowing `std::filesystem::create_directory_symlink` capability check.
- A medium-integrity token without symlink privilege now produces one evidence-bearing error 1314 skip and does not continue into a false `isSymLink()` failure.
- Fixed Windows Unicode extraction output paths at the libarchive entry boundary with `archive_entry_copy_pathname_w`; archive input handling was not changed.
- Focused regression: 5 suites, 169 tests, 0 failures, 0 errors, 1 expected capability skip.
- Remaining Phase 2 assertion baseline: 3 suites / 4 failures (`ComponentInformationCacheTest` and two Mission suites).

## 2026-07-14 Phase 2B result

- Fixed the Windows-only test contamination caused by `_basic_test` recursively deleting its fixture while a cached-data `QFile` remained open.
- The correction is an inner RAII scope in `ComponentInformationCacheTest`; production cache behavior was not changed.
- Focused regression: 2 suites, 14 tests, 0 failures, 0 errors, 0 skips.
- The component test left zero PID-specific cache/source fixture directories.
- Remaining Phase 2 assertion baseline: 2 suites / 2 failures (the two Mission suites).
- `RequestMetaDataTypeStateMachineTest` passed independently in 100.437 seconds; its CTest runner/timeout investigation remains open.

## 2026-07-15 Phase 2C result

- Configured Windows `add_qgc_test` entries with normalized `%WINDIR%\Fonts` through `QT_QPA_FONTDIR`; no warning whitelist or SDK mutation was used.
- Assigned a 360-second timeout only to Windows `MissionCommandTreeEditorTest`; global, non-Windows, and unrelated test timeouts were unchanged.
- CTest result: 1/1 passed in 186.68 seconds. Direct JUnit evidence: 3 tests, 0 failures, 0 errors, 0 skips in 209.797 seconds.
- Mission source/tests/translations were unchanged.
- Remaining Phase 2 assertion baseline: 1 suite / 1 failure (`MissionManagerTest`, Simplified Chinese placeholder defect).
- Remaining runner investigations: `HashCheckTest`, `SigningTest`, and `RequestMetaDataTypeStateMachineTest`.

## 2026-07-15 Phase 2D design

- Approved scope: correct the Simplified Chinese `Frame: %1` translation so it retains the `%1` placeholder, then verify the existing `MissionManagerTest` in the established Windows Debug environment.
- Out of scope: production Mission code, test expectations/whitelists, English-locale forcing, and the unrelated CTest runner investigations.

## 2026-07-15 Phase 2D Task 1 result

- Corrected the Simplified Chinese `Frame: %1` catalogue entry to `框架：%1`.
- Rebuilt `QGroundControl` successfully under VS2022/Qt with `--parallel 2` after an initial parallel C1060 heap exhaustion.
- The generated CTest registration passed 1/1 in 30.24 seconds with the configured offscreen, logging, and font environment; its build-relative JUnit has one testcase and no failure/error node.
- Earlier direct-run artifacts were concurrent/non-equivalent evidence and are superseded by the CTest result.
- The three remaining runner checks also passed individually through CTest: `HashCheckTest` in 124.14 seconds, `SigningTest` in 1.18 seconds, and `RequestMetaDataTypeStateMachineTest` in 99.63 seconds, all with no JUnit failure/error node.
- No known focused Windows test failure remains. Full CTest has not been rerun after Phase 2D and remains the broader regression check.

## 2026-07-15 Full CTest status

- A fresh VS2022 x64 Debug rebuild passed after loading `VsDevCmd`; a shell with only `cl.exe` on `PATH` is insufficient because it lacks the MSVC `INCLUDE` environment.
- The attempted full 186-test CTest run is not a passing baseline: `MissionControllerTest` and `InitialConnectTest` each timed out at 120.06 seconds before the outer execution limit interrupted the run during test 182.
- Do not branch downstream from this hardening branch as a verified development baseline until the two timeout failures are diagnosed and a full CTest run completes.

## 2026-07-15 InitialConnect localization result

- The temporary selector was removed after running all 22 functions/data rows and rebuilding the uninstrumented runner.
- 21 selectors passed. `_boardVendorProductId` alone failed after 13.60 seconds with Windows access violation `0xc0000005` during `LinkManager::_linkDisconnected` / `MockLink` teardown.
- No individual selector timed out; the class-level 360-second timeout is therefore not an acceptable fix. The teardown/lifecycle crash needs its own design and focused correction before full CTest.

## 2026-07-15 Board teardown fix design

- Approved direction: assign the custom board-test MockLink to the existing `_mockLink` fixture member and use `_disconnectMockLink()` for the established vehicle-removal and event-loop cleanup.
- Production `LinkManager` changes, arbitrary delays, timeout inflation, warning whitelists, and skipped rows are out of scope.

## 2026-07-15 Board wait follow-up

- The fixture-owned cleanup and bounded wait correction are implemented and focused verification is complete.
- Direct QGC JUnit passed all 24 cases with 0 failures/errors in 184.833 seconds and process exit code 0.
- The generated CTest entry passed 1/1 in 182.73 seconds with its Windows-only 360-second timeout; no access violation or teardown failure was present.
- A completion-capable full 186-test CTest run remains the final gate before downstream development branches are created.

## 2026-07-15 Completion-capable full CTest

- The background-controlled run completed all 186 registrations in 1936.82 seconds: 182 passed and 4 failed.
- `VehicleCameraControlTest` and `MissionControllerTreeTest` timed out at 120 seconds, then passed direct JUnit with 15/15 in 161.519 seconds and 11/11 in 123.426 seconds respectively.
- `ParameterManagerTest` reproduced 3 progress-signal wait failures at the 1-second bound; `VehicleLinkManagerTest` reproduced 3 initial-connect wait failures at the 5-second bound.
- Downstream development branches remain blocked until scoped fixes pass focused CTest and a new full run is 186/186.

## 2026-07-15 Final Windows Debug gate

- The scoped corrections passed their focused direct JUnit and generated CTest entries.
- A fresh interruption-safe run completed all 186 CTest registrations with exit code 0: 186 passed, 0 failed, 0 skipped in 757.36 seconds.
- The wrapper ran from 18:43:37 to 18:56:15 Asia/Shanghai. Stderr was empty and the actionable failure/crash/timeout scan had zero matches.
- The Windows Debug environment is ready for downstream development branch delivery after final diff review and record push.

## 2026-07-15 Development branch delivery

- Final whole-phase review passed with no remaining Critical, Important, or Minor findings.
- `codex/joystick-aux-px4-development` was created and pushed from the verified hardening head.
- Delivery-time `git describe --tags --always` returned `2cda0d3cf`; `change1_crocodile_2cda0d3cf` was created from the development branch and pushed.
- `codex/joystick-aux-px4` remained unchanged at `daae3a37bbdd186ac42958be06567c342e4a0a5b`; no PR or force-push was used.

## 2026-07-15 Timeout diagnosis design

- A reversible CTest metadata probe proved `MissionControllerTest` is slow rather than stalled: it passed in 133.82 seconds with a 360-second probe limit.
- `InitialConnectTest` still timed out at 360.04 seconds, so increasing its timeout without function/data-row localization is prohibited.
- Approved direction: Windows-only 180 seconds for `MissionControllerTest`; temporary uncommitted QTest function/data-row selection for Initial Connect localization; evidence-driven durable correction; full CTest before downstream branching.
