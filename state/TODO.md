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

## 2026-07-22 Quad-Rover PX4 adaptation

- [x] Verify the QGC development baseline, PX4 protocol branch, and published MAVLink release tag.
- [x] Obtain approval for the first-class Quad-Rover integration design.
- [x] Create the isolated `codex/quad-rover-design` worktree from `codex/joystick-aux-px4-development`.
- [x] Write the Quad-Rover adaptation design specification.
- [x] Self-review and commit the design specification.
  - 2026-07-22: reconciled the document with PX4's one-progress/one-terminal ACK behavior and QGC's 1.2-second post-progress queue timeout; reviewed placeholders, scope, state initialization, negative configuration tests, ACK address matching, and exact sequence rules.
- [x] Revise and commit the written specification for the post-commit critical review.
  - Resolve the combined dialect contract, remove the fixed transition-time assumption, require strict ACK matching from the first ACK, tighten status fallback semantics, and define the effective Quad/Rover mode profile.
- [x] Submit the revised specification to the PX4 agent for protocol/release-contract review.
- [x] Receive and evaluate the follow-up review: reboot epoch detection, initial queue retry semantics, and executable MAVLink release validation.
- [x] Revise the written specification for the follow-up review.
- [x] Submit the follow-up specification to the PX4 agent for final protocol/release-contract review.
- [x] Receive and evaluate the final review: bounded reboot recovery, controller-owned detached-request gate, and executable release evidence.
- [x] Apply the final review corrections to the written specification.
- [x] Submit the corrected specification for closing PX4 protocol/release-contract review.
- [x] Receive closing PX4 review: no Critical, Important, or Minor findings; the then-unreleased `qgc_hybrid` tag remained the only executable release-gate blocker.
- [x] Record the published r2 release handoff and independently verify its annotated/peeled remote references.
- [x] Obtain user authorization to proceed from the written specification into planning.
- [x] Write and self-review the implementation plan before changing QGC behavior.
- [x] Obtain plan review and select the implementation workflow.
  - 2026-07-23: selected subagent-driven development with a fresh implementer and independent reviewer per task.
- [x] Execute the approved implementation plan with TDD, focused reviews, and release evidence.
  - [x] Task 1: pin and prove the composite MAVLink r2 dependency contract (`ec03aa473`, review clean).
  - [x] Task 2: register type 200 as a permanent Quad-Rover class (`3825f50c2`, review clean).
  - [x] Task 3: implement authoritative hybrid state, freshness, and reboot epochs (`f3e799c02`, `93a5ad457`, review clean).
  - [x] Task 4: route MAVLink 2 status and SYSTEM_TIME through Vehicle safely (`3dd2a2989`, review clean).
  - [x] Task 5: generalize command queue strict matching and detach-on-progress (`1aec8c936`, `c95ded41c`, `5b38cb1e3`; review clean; Windows runtime regressions passed on 2026-07-25).
  - [x] Task 6: implement the transition-controller lifecycle (`8b57c600b`, `ef0e50f1a`, `8d3555549`; review clean; five focused Windows tests passed).
  - [x] Task 7: apply the effective stable shape profile to PX4 modes and command gates (`3357342c6`; review clean; five Windows regressions passed).
  - [x] Task 8: preserve and validate hybrid transition mission items (`f7a389382`; review clean; four Mission regressions passed).
  - [x] Task 9: add compact state-driven toolbar controls (`d64ec186b`, `856d7483a`; review clean after one fix round).
  - [x] Task 10: run final integration gates and prepare handoff (`08588c3a2` registered the omitted ingress test; Release/Debug builds passed; focused 6/6 and broad 180/180 passed; `3f366f56c` closed the final mission-category coverage finding and passed independent review).
- [ ] Complete physical PX4/QGC acceptance on the target aircraft and attach the source commit, CMake cache, resolved MAVLink commit, desktop binary, automated logs, MAVLink 2 capture, and evidence for Quad, Rover, transition, landed rejection, fault, and a mission item.

## 2026-08-03 QGC-only hardening

- [x] Harden HRT/SYSTEM_TIME reboot detection, status validity, and selective mode-policy notifications without changing PX4.
- [x] Reserve command 50000 across the full controller transaction and cancel queue entries by exact token.
- [x] Preserve first-ACK retry behavior, restart every retry ACK window, and keep COMMAND_LONG confirmation at 0/1/2.
- [x] Require post-request status corroboration before a baseline-sequence terminal failure ACK can end a new transaction.
- [x] Scope Windows HashCheck and MissionCommandTreeEditor timeout corrections to the proven test/platform pair.
- [x] Complete VS2022 Debug rebuild, changed-line formatting, dependency-pin checks, and static consistency gates.
- [x] Pass the final focused State/Ingress/Controller/mode/queue/mission/link regression set.
- [x] Pass the final serial `Unit|Vehicle|MissionManager|MAVLink` broad regression.
- [x] Record final evidence and remaining protocol/hardware risks in README/LOG.

## 2026-08-04 Windows desktop UX and functionality

- [x] Push an immutable remote backup of the completed reliability-hardening head.
- [x] Limit evaluation and validation to Windows desktop QGC.
- [x] Assess native and Quad-Rover usability/function gaps and review MicoAir, CUAV, Mission Planner, UgCS, Auterion, and DJI evidence.
- [x] Preserve airframe 22001 identity over stale firmware metadata caches, including same-group imagery.
- [x] Add a healthy last-confirmed manual-control profile and align virtual/USB joystick behavior.
- [x] Make transform and Guided Action confirmations vehicle-safe across active-vehicle changes.
- [x] Add retained-shape ground/flight Guided Actions and a Hybrid preflight checklist.
- [x] Complete Windows Debug build, focused Hybrid/airframe/native-path tests, QML lint, XML/dependency checks, and isolated startup smoke.
- [x] Close final review hazards for left-handed Quad throttle restoration and mutable multi-vehicle confirmation targets.
- [x] Add QML logic tests for all handedness/auto-center/Quad-Rover combinations and multi-vehicle selection drift.
- [x] Add a production-component QML integration gate and close the confirmation lifecycle, selection shadowing, and cross-component wrapper findings it exposed.
- [ ] Complete physical target-aircraft acceptance and attach MAVLink 2, shape, transition, fault, joystick, mission, and checklist evidence.
- [ ] Execute the assessment's remaining P1/P2 backlog in separately reviewed slices.

## 2026-09-02 Hybrid Quad-Rover realtime tuning merge

- [x] Create `codex/quad-rover-realtime-tuning` from validated Windows UX head `c9064aaa0` in an isolated worktree.
- [x] Audit the Mini Rover source commit without cherry-picking it and identify overlapping Hybrid files for manual merge.
- [x] Read-only verify PX4 `feature/testc4-rover-tuning` and its combined MAVLink wire schema.
- [x] Verify the existing protected `qgc-hybrid-rover-tuning-v1.16.1-r1` tag resolves to `21922689c6fb113884df0f66582d8e602286fdc1`, generates fresh C headers, and passes a C++ root-header probe.
- [x] Pin QGC to the combined immutable MAVLink release and extend its contract tests.
- [x] Merge Rover tuning Facts, per-vehicle telemetry, link-pinned stream lifecycle, and command-511 serialization into the current Hybrid architecture.
- [x] Add a dedicated Quad-Rover tuning container with Multirotor and four Rover pages; keep type 22 on the ordinary VTOL path.
- [x] Add routing, protocol, stream, FactGroup, Vehicle, link, QML math, and Hybrid regression coverage.
- [x] Complete a fresh Windows MSVC Debug configure/build, focused tests, dependency/cache checks, targeted qmllint, and isolated startup smoke. The full PX4 qmllint target still reports the unrelated generated `PowerComponent.qml` duplicate-id baseline defect.
- [x] Prepare non-actuating USB acceptance instructions; powered controller waveform acceptance remains user-coordinated.
- [x] Complete independent review and update state evidence; both Important findings were closed with no new Critical/Important result.
- [x] Commit and push the QGC feature branch, then verify the remote head.
