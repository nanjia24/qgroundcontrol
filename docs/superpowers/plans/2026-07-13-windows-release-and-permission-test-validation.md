# Windows Release and Permission Test Validation Plan

> **For agentic workers:** Execute inline in the current session; do not commit, push, or create a PR.

**Goal:** Validate the Windows Release desktop build and determine whether full filesystem/process permissions change the 11 previously failing Debug tests.

**Architecture:** Use the confirmed VS2022 v143 x64 toolchain through `VsDevCmd.bat`, Qt 6.10.3 `msvc2022_64`, Ninja, and the installed GStreamer SDK. Keep Release validation isolated in `build/windows-release`; reuse the already verified full Debug binary only for the targeted test reruns.

**Tech Stack:** CMake, Ninja, MSVC v143 x64, Qt 6.10.3, GStreamer, CTest/QGC unittest runner, NSIS.

## Global Constraints

- Do not modify business source code.
- Do not run the complete Debug build or complete CTest suite again.
- Redirect build and test output to files and summarize fewer than 50 lines.
- Do not commit, push, or create a pull request.
- Preserve existing proxy environment variables.

---

### Task 1: Release build validation

- [x] Load VS2022 x64 developer environment and verify compiler identity.
- [x] Configure `build/windows-release` with Ninja, Release, and `QGC_BUILD_TESTING=OFF`.
- [x] Build Release and verify the executable exists.
- [x] Start the executable with Qt and GStreamer paths active; capture exit behavior and logs.
- [x] Confirm `makensis.exe` is discoverable without running installation.

### Task 2: Full-permission targeted test comparison

- [x] Reuse `build/windows-debug-vs2022-zc-fixed/Debug/QGroundControl.exe`.
- [x] Run only the 11 previously failing tests with `QGC_TEST_VERBOSE=1`, JUnit XML, and console logs.
- [x] Compare pass/fail results and failure messages with prior artifacts.

### Task 3: Evidence and state update

- [x] Update `ENVIRONMENT_REPORT.md` with Release and permission-test evidence.
- [x] Update `state/README.md`, `state/TODO.md`, and `state/LOG.md`.
- [x] Run final Git/diff and artifact checks.
