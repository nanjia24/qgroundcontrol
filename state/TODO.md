# TODO

- [x] Clone target repository into current workspace parent.
- [x] Verify target branch codex/joystick-aux-px4 and create local working branch.
- [x] Detect Windows, Git, Python, CMake, Ninja, MSVC, Windows SDK, Qt, GStreamer, and NSIS.
- [x] Attempt official QGC Windows dependency script for GStreamer.
- [x] Attempt NSIS install via Chocolatey.
- [x] Write ENVIRONMENT_REPORT.md.
- [ ] Manually install Qt 6.10.3 msvc2022_64 with required modules.
- [ ] Resolve/install GStreamer 1.28.1 MSVC x86_64 to C:\gstreamer.
- [ ] Install NSIS from elevated Administrator PowerShell.
- [ ] Run Debug CMake configure/build.
- [ ] Launch QGroundControl.exe and record result.
- [ ] Run CTest and record result.
- [ ] Only after Debug succeeds, configure/build Release.

- [x] Verify newly installed E:\Qt\6.10.3\msvc2022_64.
- [x] Retry GStreamer dependency script with 127.0.0.1:7897 proxy after network failure.
- [x] Establish Git Bash + MSVC CMake configure path.
- [x] Pre-download GStreamer installer via Python/httpx to CPM cache.
- [ ] Manually complete visible GStreamer installer to C:\gstreamer.
- [ ] Rerun Debug configure/build after GStreamer install.



- [x] Verify E:\Program Files GStreamer SDK.
- [x] Configure app-only Debug with testing OFF.
- [x] Build app-only Debug QGroundControl.exe.
- [x] Launch app-only Debug QGroundControl.exe and observe 20 seconds.
- [x] Run CTest entry on test-enabled build.
- [ ] Fix or decide handling for MSVC test compile failure in MissionManager tests (__VA_OPT__).
- [ ] Re-run full Debug build with QGC_BUILD_TESTING=ON after test compile issue is resolved.
- [ ] Re-run CTest after full test build succeeds.
- [ ] Only then run Release configure/build.



- [x] Verify VS2022 Community v143 x64 toolchain.
- [x] Re-run full Debug configure with tests ON.
- [x] Diagnose __VA_OPT__ / C2146 root cause.
- [x] Rebuild full Debug with /Zc:preprocessor.
- [x] Run full CTest.
- [ ] Investigate 11 runtime test failures from CTest.
- [ ] Decide whether /Zc:preprocessor should be codified in project CMake for MSVC builds.


## 2026-07-13 continuation

- [x] Codify `/Zc:preprocessor` in `cmake/platform/Windows.cmake` for MSVC Windows builds.
- [x] Verify clean full Debug build with tests ON without `-DCMAKE_CXX_FLAGS=/Zc:preprocessor`.
- [x] Generate individual JUnit XML and console logs for the 11 previously failed tests.
- [x] Extract detailed failure functions/messages for `JsonHelperTest`, `SigningTest`, and `QGCCompressionTest`.
- [x] Build `origin/master` in a clean worktree with same VS2022/Qt/GStreamer environment.
- [x] Run the same 11 tests on `origin/master` and compare results.
- [x] Update `ENVIRONMENT_REPORT.md` with classification and recommendations.
- [ ] If authorized later, commit the CMake-only `/Zc:preprocessor` fix.
- [ ] If authorized later, start a separate Windows test-hardening branch for the existing failing tests.

## 2026-07-13 Release and full-permission validation

- [x] Configure and build clean Release with VS2022 v143 x64, Qt 6.10.3, Ninja, and tests disabled.
- [x] Observe the Release executable for 20 seconds and capture stdout/stderr.
- [x] Verify NSIS 3.12 is discoverable without running `cmake --install`.
- [x] Rerun only the 11 previous failed test classes with JUnit XML and console logs under Codex full access.
- [x] Compare full-access results with the prior run.
- [ ] Fetch and fast-forward-check the integration branch before creating a separate test-hardening branch.
- [ ] Keep Windows test fixes separate from joystick feature changes.

## Test-hardening worktree

- [x] Fetch and verify the feature branch is current with origin.
- [x] Create `codex/windows-test-hardening` in an isolated worktree.
- [x] Configure and build a clean Debug test baseline.
- [x] Reproduce only the 11 previously failing test suites.
- [x] Approve the Windows test-hardening design and fix policy.
- [x] Write and self-review the Windows test-hardening design specification.
- [ ] Obtain final user review of the written specification.
- [ ] Write an implementation plan before changing tests or production behavior.
