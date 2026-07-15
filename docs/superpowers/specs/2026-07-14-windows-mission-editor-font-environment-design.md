# Windows Mission Editor Test Font Environment Design

**Status:** Written specification approved by the user on 2026-07-14.

## Goal

Make Windows offscreen Qt tests discover the operating-system font directory through CTest environment configuration and give `MissionCommandTreeEditorTest` enough time to complete its verified normal runtime.

## Root Cause Evidence

- Branch: `codex/windows-test-hardening` at `fe791c7f5`.
- Fresh direct `MissionCommandTreeEditorTest`: 3 tests, 1 failure, 0 errors, 0 skips; XML time 198.845 seconds.
- The only uncategorized message was `QFontDatabase: Cannot find font directory E:/Qt/6.10.3/msvc2022_64/lib/fonts.` followed by Qt's notice that Qt no longer ships fonts.
- The other frequent FactMetaData and `QObject::connect` warnings are categorized and do not cause the UnitTest cleanup failure.
- Re-running the same executable with only `QT_QPA_FONTDIR=C:\Windows\Fonts` added produced 3 tests, 0 failures, 0 errors, 0 skips; XML time 197.590 seconds and no font warning.
- After the font environment fix, a fresh direct GREEN run took 209.797 seconds, while the registered non-sanitizer CTest timeout was 180 seconds. CTest correctly injected the font directory but terminated the test at 180.06 seconds.
- This proves the failure is an offscreen Windows test-environment boundary, not a Mission editor assertion or production behavior defect.

## Scope

- Limit implementation changes to `cmake/QGCTest.cmake` and `test/CMakeLists.txt`; design, plan, report, and state documentation are tracked separately.
- Add `QT_QPA_FONTDIR` to the Windows CTest environment for every test created by `add_qgc_test` when `%WINDIR%\Fonts` exists.
- Do not change test assertions, log-capture policy, Mission editor code, translations, or application runtime behavior.
- Do not add a warning whitelist, skip, retry, delay, or hard-coded `C:\Windows` path.
- Keep Phase 2D translation repair separate.
- Set a Windows-only 360-second timeout for `MissionCommandTreeEditorTest`; do not increase global or unrelated test timeouts.

## Considered Approaches

### Selected: CMake Windows test environment

Inside `add_qgc_test`, derive the font directory from the configure-time `WINDIR` environment variable, normalize it with `file(TO_CMAKE_PATH)`, and append `QT_QPA_FONTDIR=<normalized path>` to `_test_env`. The existing offscreen and logging environment remains unchanged.

If `WINDIR` is unavailable or its `Fonts` directory does not exist, emit a configure warning rather than silently inventing a path. The test will then expose the missing environment through its existing uncategorized warning.

### Rejected: Test-level warning whitelist

This would make the assertion pass while leaving the offscreen font environment broken.

### Rejected: Copying fonts into the Qt installation

This mutates a machine-wide SDK location and is not reproducible in a clean checkout or CI runner.

## Selected Design

In `cmake/QGCTest.cmake`, after initializing `_test_env`, add a `WIN32` block that checks `ENV{WINDIR}`, verifies its `Fonts` directory, normalizes the path, and appends the environment entry. No test source changes are required.

In `test/CMakeLists.txt`, retain `${QGC_TEST_TIMEOUT_EXTENDED}` on non-Windows platforms and use a dedicated 360-second value for `MissionCommandTreeEditorTest` on Windows. The value is based on observed successful runs of 197.590 and 209.797 seconds plus the previously used 360-second harness window. Other registered tests keep their existing timeout policy.

## Error Boundaries

- Expected configuration warning: `%WINDIR%` or `%WINDIR%\Fonts` is unavailable. CMake reports the missing environment; no fake path is injected.
- Test failure: a valid font directory is configured but Qt still emits the uncategorized missing-font warning.
- Test timeout failure: the Windows editor test exceeds its dedicated 360-second limit.
- Product failure: any Mission editor assertion or unrelated categorized warning changes. Do not suppress these.
- Out of scope: fixing the separate `MissionManagerTest` translation placeholder defect.

## Verification Strategy

1. Preserve the fresh direct RED XML and font-directory GREEN XML as evidence.
2. Apply only the CMake environment change.
3. Reconfigure the existing Ninja Debug tree and verify generated CTest properties contain `QT_QPA_FONTDIR` pointing to `%WINDIR%\Fonts`.
4. Verify the registered `MissionCommandTreeEditorTest` has `TIMEOUT=360` on Windows while unrelated test timeouts remain unchanged.
5. Run only the registered `MissionCommandTreeEditorTest` through CTest with `--output-on-failure`; require 1/1 CTest entry passed.
6. Reuse the fresh direct QGC GREEN evidence because the timeout-only follow-up does not affect direct execution; require 3 tests, 0 failures/errors/skips and no font warning.
7. Confirm `MissionManagerTest` remains a separate known RED test and no translation file changed.
8. Run `git diff --check`, review the two-file CMake diff, and obtain independent review before push.

## Git and Delivery

- Commit only to `codex/windows-test-hardening`.
- Use focused structured CMake commits for the environment and timeout boundaries.
- Push only after focused verification and independent review.
- Do not force-push or create a PR.
- No GitHub issue or PR number is known; do not fabricate a tracking footer.
- Keep `.tmp`, JUnit XML, console logs, and build products untracked.

## Success Criteria

- The registered `MissionCommandTreeEditorTest` passes 1/1 through CTest; the direct QGC JUnit invocation reports 3 tests, 0 failures, 0 errors, 0 skips.
- The Windows CTest property is `TIMEOUT=360`; unrelated tests retain their existing timeout values.
- No uncategorized QFontDatabase missing-font warning is captured.
- No Mission editor source, test assertions, translation, or production runtime code changes; implementation changes are limited to the two test-infrastructure CMake files.
- `MissionManagerTest` remains explicitly outside Phase 2C and is not claimed fixed.
