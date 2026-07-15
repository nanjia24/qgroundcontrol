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
- [ ] Investigate and design Phase 2 fixes for compression, cache, Mission logging, and CTest runner behavior.
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
- [ ] Verify `MissionManagerTest` in the generated CTest environment and capture authoritative JUnit/console evidence.
  - The latest direct artifact has 7 tests / 5 uncategorized-log failures, though it no longer contains `QString::arg: Argument missing`; concurrent agents also created zero-byte artifacts, so this is not an acceptance result.
- [ ] Reproduce the exact single-test CTest behavior of `HashCheckTest`, `SigningTest`, and `RequestMetaDataTypeStateMachineTest` before changing their code or timeouts.
