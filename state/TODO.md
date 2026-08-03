# TODO

- [x] Clone target repository into current workspace parent.
- [x] Verify target branch codex/joystick-aux-px4 and create local working branch.
- [x] Detect Windows, Git, Python, CMake, Ninja, MSVC, Windows SDK, Qt, GStreamer, and NSIS.
- [x] Attempt official QGC Windows dependency script for GStreamer.
- [x] Attempt NSIS install via Chocolatey.
- [x] Write ENVIRONMENT_REPORT.md.
- [x] Install Qt 6.10.3 msvc2022_64 with required modules at the verified E: path.
- [x] Install GStreamer MSVC x86_64 at the verified E: path.
- [x] Install and verify NSIS 3.12.
- [x] Run Debug CMake configure/build.
- [x] Launch QGroundControl.exe and record result.
- [x] Run CTest and record result.
- [x] After Debug succeeds, configure/build Release.

- [x] Verify newly installed E:\Qt\6.10.3\msvc2022_64.
- [x] Retry GStreamer dependency script with 127.0.0.1:7897 proxy after network failure.
- [x] Establish Git Bash + MSVC CMake configure path.
- [x] Pre-download GStreamer installer via Python/httpx to CPM cache.
- [x] Complete GStreamer installation at the accepted E: path.
- [x] Rerun Debug configure/build after GStreamer install.



- [x] Verify E:\Program Files GStreamer SDK.
- [x] Configure app-only Debug with testing OFF.
- [x] Build app-only Debug QGroundControl.exe.
- [x] Launch app-only Debug QGroundControl.exe and observe 20 seconds.
- [x] Run CTest entry on test-enabled build.
- [x] Fix MSVC test compile failure by codifying `/Zc:preprocessor`.
- [x] Re-run full Debug build with `QGC_BUILD_TESTING=ON`.
- [x] Re-run CTest after full test build succeeds.
- [x] Run Release configure/build after Debug validation.



- [x] Verify VS2022 Community v143 x64 toolchain.
- [x] Re-run full Debug configure with tests ON.
- [x] Diagnose __VA_OPT__ / C2146 root cause.
- [x] Rebuild full Debug with /Zc:preprocessor.
- [x] Run full CTest.
- [x] Investigate and classify the 11 runtime test-suite failures from CTest.
- [x] Codify `/Zc:preprocessor` in project CMake for MSVC builds.


## 2026-07-13 continuation

- [x] Codify `/Zc:preprocessor` in `cmake/platform/Windows.cmake` for MSVC Windows builds.
- [x] Verify clean full Debug build with tests ON without `-DCMAKE_CXX_FLAGS=/Zc:preprocessor`.
- [x] Generate individual JUnit XML and console logs for the 11 previously failed tests.
- [x] Extract detailed failure functions/messages for `JsonHelperTest`, `SigningTest`, and `QGCCompressionTest`.
- [x] Build `origin/master` in a clean worktree with same VS2022/Qt/GStreamer environment.
- [x] Run the same 11 tests on `origin/master` and compare results.
- [x] Update `ENVIRONMENT_REPORT.md` with classification and recommendations.
- [x] Commit the CMake-only `/Zc:preprocessor` fix.
- [x] Start a separate Windows test-hardening branch for the existing failing tests.

## 2026-07-13 Release and full-permission validation

- [x] Configure and build clean Release with VS2022 v143 x64, Qt 6.10.3, Ninja, and tests disabled.
- [x] Observe the Release executable for 20 seconds and capture stdout/stderr.
- [x] Verify NSIS 3.12 is discoverable without running `cmake --install`.
- [x] Rerun only the 11 previous failed test classes with JUnit XML and console logs under Codex full access.
- [x] Compare full-access results with the prior run.
- [x] Fetch and fast-forward-check the integration branch before creating a separate test-hardening branch.
- [x] Keep Windows test fixes separate from joystick feature changes.

## Test-hardening worktree

- [x] Fetch and verify the feature branch is current with origin.
- [x] Create `codex/windows-test-hardening` in an isolated worktree.
- [x] Configure and build a clean Debug test baseline.
- [x] Reproduce only the 11 previously failing test suites.
- [x] Approve the Windows test-hardening design and fix policy.
- [x] Write and self-review the Windows test-hardening design specification.
- [x] Obtain final user review of the written specification.
- [x] Write the Phase 1 implementation plan before changing tests or production behavior.
- [x] Select subagent-driven execution and complete the approved Phase 1 plan.
- [x] Recover from the transient subagent execution-channel/network blockage.
- [x] Verify the Phase 1 focused set: 118 tests, 0 failures/errors/skips.
- [x] Investigate and design Phase 2 fixes for compression, cache, Mission logging, and CTest runner behavior.
- [x] Confirm and write the Phase 2A compression hardening design.
- [x] Obtain final user review of the written Phase 2A specification.
- [x] Write the Phase 2A implementation plan before changing code.
- [x] Execute Phase 2A Task 1: real Windows directory-symlink capability probe.
- [x] Execute Phase 2A Task 2: Unicode libarchive output entry paths.
- [x] Execute Phase 2A Task 3: focused regressions, evidence, and delivery.
- [x] Reproduce and identify the Phase 2B component-cache test lifecycle root cause.
- [x] Approve and write the Phase 2B test-only design.
- [x] Obtain final user review of the written Phase 2B specification.
- [x] Write the Phase 2B implementation plan before changing test code.
- [x] Execute Phase 2B Task 1: release cached file before fixture cleanup.
- [x] Execute Phase 2B Task 2: focused regressions, evidence, and delivery.
- [x] Reproduce and separate the Phase 2C Mission editor font-environment root cause.
- [x] Approve and write the Phase 2C font-environment design.
- [x] Obtain final user review of the written Phase 2C specification.
- [x] Write the Phase 2C implementation plan before changing CMake.
- [x] Approve the Phase 2C Windows-only 360-second editor-test timeout adjustment.
- [x] Execute Phase 2C Task 1: configure Windows offscreen fonts for CTest.
- [x] Execute Phase 2C Task 2: verification, evidence, and delivery.
- [x] Diagnose the Phase 2D `MissionManagerTest` Simplified Chinese placeholder failure.
- [x] Approve and write the Phase 2D translation-correction design.
- [x] Obtain final user review of the written Phase 2D specification.
- [x] Write the Phase 2D implementation plan before changing the translation.
- [x] Apply the Phase 2D Simplified Chinese placeholder correction and rebuild `QGroundControl`.
- [x] Verify `MissionManagerTest` in the generated CTest environment and capture authoritative JUnit/console evidence.
  - 2026-07-15: CTest 1/1 passed in 30.24 seconds; the build-relative CTest JUnit has one testcase and no failure/error node.
- [x] Reproduce the exact single-test CTest behavior of `HashCheckTest`, `SigningTest`, and `RequestMetaDataTypeStateMachineTest` before changing their code or timeouts.
  - 2026-07-15: all passed through CTest (124.14 s, 1.18 s, and 99.63 s respectively) with no JUnit failure/error node; no code or timeout change is needed.
- [x] Diagnose `MissionControllerTest` and `InitialConnectTest` full-CTest 120-second timeouts without changing shared timeout policy blindly.
- [x] Localize `InitialConnectTest` by function/data row and restore the temporary selector instrumentation.
  - 2026-07-15: 21/22 selectors passed; `_boardVendorProductId` crashed in `LinkManager::_linkDisconnected` / `MockLink` teardown with Windows access violation `0xc0000005`.
- [x] Diagnose the `_boardVendorProductId` teardown/lifecycle crash.
- [x] Write the dedicated board teardown fix design.
- [x] Obtain final user review of the board teardown fix specification.
- [x] Write the board teardown implementation plan before changing the test.
- [x] Apply the fixture cleanup change for diagnosis; it removed the teardown crash.
- [x] Diagnose the remaining `_boardVendorProductId` initial-connect wait failure.
- [x] Review the scoped `mediumMs()` to `longMs()` wait correction.
- [x] Write the board wait implementation plan.
- [x] Implement and verify the board wait correction.
  - 2026-07-15: direct JUnit passed 24/24 in 184.833 s; generated CTest passed 1/1 in 182.73 s with no crash or failure.
- [x] Create and verify the requested downstream branches.
  - `codex/joystick-aux-px4-development`
  - `change1_crocodile_2cda0d3cf`
- [x] Re-run full CTest to completion after resolving the timeout results.
- [x] Complete the first interruption-safe 186-test CTest run.
  - 2026-07-15: 182 passed, 4 failed in 1936.82 s.
- [x] Correct and focus-verify the two scoped Windows CTest timeout registrations.
- [x] Correct and focus-verify ParameterManager and VehicleLinkManager test wait bounds.
- [x] Re-run all 186 tests; final result 186/186, exit 0, 757.36 s.
- [x] Confirm and write the Windows controller/initial-connect timeout design.
- [x] Obtain final user review of the written timeout specification.
- [x] Write the timeout implementation and localization plan before changing source/CMake input.

## 2026-08-03 Mini Rover realtime tuning

- [x] Read the QGC task prompt and realtime tuning guide in full.
- [x] Verify the owner clone, remote, exact QGC baseline, target branch absence, and target worktree absence.
- [x] Create `codex/mini-rover-realtime-tuning` at `754135601a53d7650ddeb6562ca5a5cd2167880c` in the required worktree.
- [x] Capture focused baseline build/test evidence in a fresh task-local build directory.
  - 2026-08-03: configure and 2031-step `QGroundControl` build exited 0; `FactGroupTest` and `MAVLinkStreamConfigTest` passed 2/2 in 2.51 seconds.
- [x] Pin the MAVLink fork, exact commit, and `qgc_mini_rover` dialect; verify generated ID/LEN/CRC constants.
  - 2026-08-03: fetched HEAD and remote match exactly; generated constants are `60100/27/147`, `60101/23/85`, `60102/43/217`, and `60103/44/90`; `all.xml` remains included.
- [x] Implement and test the per-Vehicle `RoverTuningFactGroup` decoder and lifecycle state machine.
- [x] Implement and test four mutually exclusive Rover stream modes and restore/reconnect behavior.
- [x] Implement asynchronous Mini capability routing, source-timestamp chart sampling, four Rover pages, and metadata-driven parameter controls.
- [x] Run focused tests, affected regressions, and the Windows build/test gates available in this environment.
  - 2026-08-03: full Debug target and `QGCQmlQuickTests` build; authoritative focused gate passed 7/7 in 254.18 seconds.
  - 2026-08-03: compact layout now stacks chart/parameters at narrow widths and scrolls the parameter panel vertically.
  - 2026-08-03: post-review Debug rebuild passed; the expanded ACK/link/Fact/UI focused gate passed 8/8 in 288.69 seconds.
  - 2026-08-03: `QGroundControlControlsModule_qmllint` passed. Full `all_qmllint` remains blocked by the pre-existing generated `PowerComponent.qml` duplicate-id error.
  - 2026-08-03: all 188 CTest registrations completed in 1998.81 seconds; 187 passed and `HashCheckTest` alone exceeded its 180-second Windows budget by 0.04 seconds.
  - 2026-08-03: a diagnostic 300-second registration verified `HashCheckTest` through CTest in 176.38 seconds; the source timeout change was removed as unrelated to Rover. The final seven-suite gate passed 7/7 in 287.00 seconds, QML Quick Tests passed, and targeted changed-file qmllint exited 0.
  - 2026-08-04: final post-review gate passed 8/8 in 65.44 seconds after adding synchronous-callback, pinned-link disconnect, and source-snapshot FIFO regressions. Controls qmllint exited 0; QML Quick Tests reported 16/16. Final Debug artifact is 99,605,504 bytes with SHA-256 `BE7D31B6DBDAE09D8F7C6465D2FA869DEA510EEDA09FA0D71B158DB613A62703`.
- [x] Ask the user to connect the Mini Rover and execute all non-destructive USB checks possible with the current firmware.
  - 2026-08-03: firmware with Rover topics is flashed and the PX4 FMU V6U is connected on `COM16` over USB only.
  - 2026-08-03: fixed the nested Rover tuning page's missing size context after the first manual tab selection hung; targeted qmllint, Debug rebuild, and `QmlQuickTests` passed. Correct-build manual retest remains pending.
  - 2026-08-03: moved the terrain coordinator signal connection after object construction, eliminating the reported null-receiver warning. Final focused rerun passed `VehiclePIDTuningTelemetryTest` and `QmlQuickTests` 2/2 in 120.53 seconds.
  - In the correct QGC parameter page, set global `MAV_PROTO_VER=2`, restart the USB-powered flight controller, and verify that non-heartbeat MAVLink v1 traffic no longer occurs before evaluating Rover streams.
  - Verify the running process path is the task-local Debug executable before interpreting UI or COM16 evidence; installed and `quad-rover-design` QGC binaries were observed during attempted retests.
  - 2026-08-03: captured a 6.2 GB full hang dump and all thread stacks. The main UI thread repeatedly created QML delegates because metadata increment `0.01` over a `0..10000` range produced roughly one million major ticks plus two million minor ticks per slider.
  - 2026-08-03: bounded metadata-derived display ticks to about 50 major ticks without changing Fact decimal precision. New QML math regressions and existing QML tests pass 11/11; Controls qmllint and the Debug rebuild pass. Await explicit manual Rover-page confirmation.
  - 2026-08-04: correct-binary Rover Rate opens without hanging; USB-only status is accurately `Rate controller inactive`, and the user reports the UI is basically responsive.
  - 2026-08-04: the user also observed `Operation timeout aborting transfer`; this matches the captured Rally/standard-stream timeout path and is not evidence that any Rover stream was accepted.
  - 2026-08-04: firmware blocks remaining stream acceptance before Rover requests. Rally transfer fails first; standard message 281 then enters PX4's unbounded stream handoff and never ACKs within 60 seconds. The exact source/timing evidence is recorded in `state/README.md` and `state/LOG.md`.
- [ ] Finish four-page live-stream UI acceptance; static UI and the corrected Rate page were inspected, but active curves and stream switching require corrected firmware and powered Rover controllers.
- [ ] Obtain firmware with bounded/synchronized `configure_stream_threadsafe()` behavior, then repeat four-page stream and powered waveform acceptance.
- [ ] Review, commit, push, verify a clean worktree, and record final artifact hashes and residual risks.
