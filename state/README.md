# State README

Project: QGroundControl Windows Desktop test hardening.
Primary repository: `E:\workspace\QGC\qgroundcontrol`.
Active worktree: `E:\workspace\QGC\qgroundcontrol-worktrees\windows-test-hardening`.
Base integration branch: `codex/joystick-aux-px4` at `daae3a37b`.
Active local/remote branch: `codex/windows-test-hardening`.

Current constraints: production and test fixes, structured commits, and pushes are authorized only on `codex/windows-test-hardening`; do not force-push, update the integration branch, create a PR without explicit approval, or store credentials.


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
