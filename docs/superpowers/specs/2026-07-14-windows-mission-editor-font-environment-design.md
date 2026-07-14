# Windows Mission Editor Test Font Environment Design

**Status:** Approved in conversation on 2026-07-14; written specification pending final user review.

## Goal

Make Windows offscreen Qt tests discover the operating-system font directory through CTest environment configuration so `MissionCommandTreeEditorTest` does not fail on an uncategorized Qt font warning.

## Root Cause Evidence

- Branch: `codex/windows-test-hardening` at `fe791c7f5`.
- Fresh direct `MissionCommandTreeEditorTest`: 3 tests, 1 failure, 0 errors, 0 skips; XML time 198.845 seconds.
- The only uncategorized message was `QFontDatabase: Cannot find font directory E:/Qt/6.10.3/msvc2022_64/lib/fonts.` followed by Qt's notice that Qt no longer ships fonts.
- The other frequent FactMetaData and `QObject::connect` warnings are categorized and do not cause the UnitTest cleanup failure.
- Re-running the same executable with only `QT_QPA_FONTDIR=C:\Windows\Fonts` added produced 3 tests, 0 failures, 0 errors, 0 skips; XML time 197.590 seconds and no font warning.
- This proves the failure is an offscreen Windows test-environment boundary, not a Mission editor assertion or production behavior defect.

## Scope

- Modify only `cmake/QGCTest.cmake`.
- Add `QT_QPA_FONTDIR` to the Windows CTest environment for every test created by `add_qgc_test` when `%WINDIR%\Fonts` exists.
- Do not change test assertions, log-capture policy, Mission editor code, translations, or application runtime behavior.
- Do not add a warning whitelist, skip, retry, delay, or hard-coded `C:\Windows` path.
- Keep Phase 2D translation repair separate.

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

## Error Boundaries

- Expected configuration warning: `%WINDIR%` or `%WINDIR%\Fonts` is unavailable. CMake reports the missing environment; no fake path is injected.
- Test failure: a valid font directory is configured but Qt still emits the uncategorized missing-font warning.
- Product failure: any Mission editor assertion or unrelated categorized warning changes. Do not suppress these.
- Out of scope: fixing the separate `MissionManagerTest` translation placeholder defect.

## Verification Strategy

1. Preserve the fresh direct RED XML and font-directory GREEN XML as evidence.
2. Apply only the CMake environment change.
3. Reconfigure the existing Ninja Debug tree and verify generated CTest properties contain `QT_QPA_FONTDIR` pointing to `%WINDIR%\Fonts`.
4. Run only the registered `MissionCommandTreeEditorTest` through CTest with `--output-on-failure`; require 1/1 CTest entry passed.
5. Run the direct QGC test entry with the same environment and JUnit/logging flags; require the same result and no font warning.
6. Confirm `MissionManagerTest` remains a separate known RED test and no translation file changed.
7. Run `git diff --check`, review the one-file CMake diff, and obtain independent review before push.

## Git and Delivery

- Commit only to `codex/windows-test-hardening`.
- Use a focused structured CMake commit.
- Push only after focused verification and independent review.
- Do not force-push or create a PR.
- No GitHub issue or PR number is known; do not fabricate a tracking footer.
- Keep `.tmp`, JUnit XML, console logs, and build products untracked.

## Success Criteria

- The registered `MissionCommandTreeEditorTest` passes 1/1 through CTest; the direct QGC JUnit invocation reports 3 tests, 0 failures, 0 errors, 0 skips.
- No uncategorized QFontDatabase missing-font warning is captured.
- No Mission editor source, test assertions, translation, or production runtime code changes.
- `MissionManagerTest` remains explicitly outside Phase 2C and is not claimed fixed.
