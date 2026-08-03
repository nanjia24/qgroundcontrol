# State README

Project: QGroundControl Mini Rover realtime tuning.
Primary repository: `E:\workspace\QGC\qgroundcontrol-source`.
Active worktree: `E:\workspace\QGC\qgroundcontrol-worktrees\mini-rover-realtime-tuning`.
Base commit: `754135601a53d7650ddeb6562ca5a5cd2167880c` from `codex/joystick-aux-px4-development`.
Active branch: `codex/mini-rover-realtime-tuning`.

Current constraints: write only in this worktree; do not modify the PX4 firmware worktree, force-push, create a PR, or commit generated build/debug artifacts and UI screenshots. No GitHub Issue or PR exists, so do not invent a closing footer.


## 2026-07-13 status

- `/Zc:preprocessor` was codified in `cmake/platform/Windows.cmake` for MSVC Windows builds.
- Clean Debug build `build/windows-debug-vs2022-zc-fixed` succeeds without manually passing `CMAKE_CXX_FLAGS`.
- Failed-test details are stored in `build/mini-rover-debug/delivery-artifacts/diagnostics/test-runs-zc-fixed`.
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

## 2026-08-03 Mini Rover realtime tuning task

- Development worktree: `E:\workspace\QGC\qgroundcontrol-worktrees\mini-rover-realtime-tuning`.
- Feature branch: `codex/mini-rover-realtime-tuning`, created from exact QGC baseline `754135601a53d7650ddeb6562ca5a5cd2167880c`.
- MAVLink contract: `https://github.com/QQgdiw/mavlink.git` at `07c6964a8fcc364c49d394f0bf0275b9fc05857d`, dialect `qgc_mini_rover`.
- Scope is Differential Mini Rover phase 1: Rate, Attitude, Velocity, and Position/Path realtime tuning with standard Fact-based parameter writes.
- Capability is restricted to `MAV_TYPE_VTOL_FIXEDROTOR` with `HYBR_QUAD_ROV == 1`; existing Multirotor/VTOL and joystick/AUX behavior must remain intact.
- The user confirmed that USB hardware is available and will connect it when explicitly requested. No hardware operation or firmware flashing occurs before that request.
- No GitHub Issue or PR number exists for this task; commit messages must not fabricate a `Fixes` or `Resolves` footer.
- Fresh stock-MAVLink baseline configure and `QGroundControl` build completed successfully in `build\mini-rover-baseline` (2031 build steps, exit 0).
- Baseline `FactGroupTest` and `MAVLinkStreamConfigTest` passed 2/2 in 2.51 seconds after supplying the Qt and GStreamer runtime DLL paths.
- Fresh `build\mini-rover-debug` configuration fetched `QQgdiw/mavlink` at exact HEAD `07c6964a8fcc364c49d394f0bf0275b9fc05857d`; its generated `qgc_mini_rover` dialect includes both `all.xml` and `mini_rover.xml`.
- Generated Rover message `ID/LEN/CRC` values match the fixed contract: `60100/27/147`, `60101/23/85`, `60102/43/217`, and `60103/44/90`.
- The four Rover pages, per-Vehicle decoder, source-timestamp sampling, Mini capability routing, and metadata-backed parameter controls are implemented.
- Stream transitions serialize `SET_MESSAGE_INTERVAL`, keep failed restore IDs as retry debt, reject MAVLink1 Rover requests, and isolate stale command generations.
- Manual primary-link migration restores the old link before configuring the new link; the focused dual-link regression passes.
- The authoritative focused gate passed 7/7 in 254.18 seconds after the final lifecycle review fixes.
- The parameter panel is vertically scrollable and moves below the chart when the available width cannot hold both panes.
- `SET_MESSAGE_INTERVAL` is now single-flight per vehicle/component across all callers, and pinned interval commands only accept ACKs from their originating link.
- Failed restore debt from an old primary link is retained without blocking stream configuration on the new primary; delayed/wrong-link ACK and timeout regressions are covered.
- Invalid source timestamps clear charts immediately, invalid Path diagnostics have an explicit availability state, unsupported drive types cannot edit Differential parameters, and short wide windows no longer force an oversized chart.
- The post-review Debug target build passed, followed by 8/8 focused suites in 288.69 seconds.
- The complete Windows Debug CTest run finished all 188 registrations in 1998.81 seconds: 187 passed and only `HashCheckTest` hit its 180-second limit. A diagnostic 300-second registration passed the same test through CTest in 176.38 seconds; that unrelated source timeout change is not part of this Rover branch.
- After the final queue and UI review, the authoritative seven-suite gate passed 7/7 in 287.00 seconds; `QmlQuickTests` and targeted qmllint for all changed PX4/Controls QML also exited 0.
- Final Debug artifact before USB acceptance: `build\mini-rover-debug\Debug\QGroundControl.exe`, 99,453,440 bytes, SHA-256 `239C5220918AA5AEE002F0F53E2DD044564CAE2A6D35877002CE4A4A18596408`.
- The user flashed the Rover-topic firmware and connected the PX4 FMU V6U on `COM16` over USB only; no battery is connected, so acceptance remains non-arming and non-actuating.
- The first manual Rover-tab attempt exposed missing `availableWidth`/`availableHeight` context in the nested `PX4TuningComponentRoverAll`; the child now bridges both dimensions from its parent Loader. Targeted qmllint, the Debug incremental build, and `QmlQuickTests` passed; hardware UI retest is still pending.
- The reported `QObject::connect(Vehicle, Unknown)` warning came from connecting `_terrainQueryCoordinator` before construction; the connection now follows object creation. This warning was not the Rover-tab hang.
- PX4 uses the global `MAV_PROTO_VER` parameter: hardware acceptance requires value `2` (forced MAVLink 2) followed by a flight-controller restart. Reading the parser's transient inbound-version flag is not a reliable QGC-side substitute because v1 startup heartbeats are explicitly allowed.
- Current post-fix Debug artifact: `build\mini-rover-debug\Debug\QGroundControl.exe`, 99,453,440 bytes, SHA-256 `78CE0FE49D7B5E265CB291F588E3A6475AF92157BAC27B3B5D8FFF21DB527739`.
- A full dump of the repeat Rover-tab hang proved the UI thread was continuously creating QML delegate objects. PX4 metadata for parameters such as `RO_YAW_RATE_LIM` is `0..10000` with increment `0.01`; treating that increment as a major slider tick requested about one million major and two million minor delegates for one slider.
- Metadata-backed PID sliders now derive a display tick step that preserves Fact precision while limiting the range to approximately 50 major ticks. A pure QML math regression covers the `0..10000 / 0.01` case; the Controls qmllint target and all 11 QML Quick Test cases pass. Manual confirmation of the rebuilt Rover page is pending.

## 2026-08-04 USB and performance result

- The correct task-local Debug executable opens Rover Rate without hanging. With USB only, no battery and no arming, the page correctly reports `Rate controller inactive`; the user reported that the interface was basically responsive.
- The same USB session displayed `Operation timeout aborting transfer`. This is consistent with the captured Rally/standard message-281 timeout sequence; it does not prove that a Rover message interval request was sent or accepted.
- Source-timestamp charts snapshot every distinct PX4 frame into a FIFO, flush the complete FIFO at a 20 ms rendering cadence, skip transient NaN values without deleting history, batch three-minute history removal, and always initialize non-degenerate axes. Legacy Multirotor sampling remains on its 10 ms path.
- The Simplified Chinese PID stop-mode translation now preserves `%1`, removing the reproducible `QString::arg: Argument missing` warnings.
- USB startup proved that `COMMAND_ACK` for command 511 may be delayed beyond 40 seconds and cannot identify `param1`. The queue now serializes command 511 per `(component, link)`, quarantines a timed-out key, consumes its late ACK as a tombstone, and clears quarantine on ACK or link disconnect/destruction. A pinned-link disconnect immediately fails that link's active and queued 511 entries so a backup primary is not blocked by the USB timeout. The 60-second timeout is limited to PX4 direct USB.
- Direct runtime logging requires `QT_FORCE_STDERR_LOGGING=1`; otherwise this Windows GUI build writes only the QML-debugging notice to redirected stderr.
- Hardware acceptance is blocked by the current PX4 firmware before any Rover stream request. At process time 23.839-34.379 seconds, Rally synchronization exhausted retries; the first command 511 was only sent later at 34.543 seconds for standard message 281 (`GIMBAL_MANAGER_STATUS`). Request-message ACKs then arrived in a batch at 74.353-74.451 seconds, while the 281 interval command still had no ACK at its 94.627-second timeout.
- PX4 source at `d656c31dd794e3cfec992c63af789deb012dce83` uses an unbounded, unsynchronized `_subscribe_to_stream` handoff in `Mavlink::configure_stream_threadsafe()`. That file is unchanged from the task's firmware anchor. The QGC task explicitly forbids editing PX4 or fabricating ACK/data, so four-page live-stream and powered waveform acceptance remain blocked pending a firmware-side correction.
- Final automated gate: `MAVLinkStreamConfigTest`, `RequestMessageTest`, `RoverTuningFactGroupTest`, both `SendMavCommand` suites, `VehicleLinkManagerTest`, `VehiclePIDTuningTelemetryTest`, and `QmlQuickTests` passed 8/8 in 65.44 seconds. QML Quick Tests explicitly reported 16/16, including the source-snapshot FIFO regression. The final Debug executable is 99,605,504 bytes with SHA-256 `BE7D31B6DBDAE09D8F7C6465D2FA869DEA510EEDA09FA0D71B158DB613A62703`.
- Feature implementation commit: `2696ed4085b48faeeb111cd7bab6f64efcaca66f` (`feat[px4-rover]: add mini rover realtime tuning`).
- Untracked diagnostics and screenshots were preserved under `build/mini-rover-debug/delivery-artifacts/`, an ignored build-artifact path, so they remain available without entering the delivery commit.
