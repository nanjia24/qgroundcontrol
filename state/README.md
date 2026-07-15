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
- The current direct JUnit artifact contains 7 tests, 5 uncategorized-log failures, 0 errors, and 0 skips; it no longer contains `QString::arg: Argument missing`.
- This is not yet an acceptance result: concurrent agents wrote competing artifacts, including zero-byte XML/log files, and the direct invocation did not reliably reproduce the generated CTest environment.
- Open work: re-run `MissionManagerTest` through its generated CTest registration with fresh JUnit/console capture, then separately verify the `HashCheckTest`, `SigningTest`, and `RequestMetaDataTypeStateMachineTest` runner/timeout reports.
