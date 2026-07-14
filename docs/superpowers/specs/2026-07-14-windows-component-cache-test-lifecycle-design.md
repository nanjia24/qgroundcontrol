# Windows Component Cache Test Lifecycle Design

**Status:** Approved in conversation on 2026-07-14; written specification pending final user review.

## Goal

Make `ComponentInformationCacheTest` release its cached-file handle before recursive fixture cleanup so Windows starts every test with a genuinely empty cache directory.

## Scope

- Modify only `test/Vehicle/ComponentInformation/ComponentInformationCacheTest.cc`.
- Keep all existing cache content, LRU, persistence, and multi-instance assertions.
- Do not modify `ComponentInformationCache` production code.
- Do not add orphan-cache recovery behavior in this phase.
- Do not run full CTest.

## Baseline Evidence

- Branch: `codex/windows-test-hardening` at `a0a8af6c7`.
- Toolchain: VS2022 v143 Hostx64/x64, Qt 6.10.3 `msvc2022_64`.
- Fresh focused result: 5 tests, 2 failures, 0 errors, 0 skips.
- Failing methods: `_lru_test` and `_multi_test`.
- Both failures occur when `cache.access(...)` cannot find a valid paired cache entry.

A Windows `FileSystemWatcher` trace captured the first invalid transition:

1. `_basic_test` created `_tag_00000000_xy.cache` and its `.meta` file.
2. `_basic_test` opened the cached data file for content verification.
3. `_cleanup()` ran while that `QFile` was still alive and open.
4. Windows deleted the `.meta` file but retained the open `.cache` file.
5. `_lru_test` reused the same fixture directory. Inserting tag 0 saw the orphan `.cache` file as an existing entry, so no new `.meta` was written.
6. `access(tag0)` therefore returned a cache miss. `_multi_test` then observed cascading state from the same incomplete cleanup.

The initiating defect is test resource lifetime, not the production cache path or LRU algorithm.

## Considered Approaches

### Selected: RAII inner scope

Place the `QFile` content-verification block in an inner C++ scope. The file closes through its destructor before `_cleanup()` executes. This remains correct if a future assertion returns early from the inner block.

### Rejected: explicit `QFile::close()`

This is smaller textually but is easier to break if assertions or early-return paths are inserted before the explicit close.

### Rejected: fixture or production-cache refactor

Converting the fixture to a broader temporary-directory abstraction or adding orphan-entry recovery would expand the behavioral surface beyond the observed test-lifetime defect.

## Selected Design

In `_basic_test`, retain the existing `QFile`, open, `QTextStream`, and content assertions, but enclose them in a local block. Call `_cleanup()` only after that block ends.

No test expectations change. No skip, retry, platform conditional, delay, or path normalization is introduced.

## Error Boundaries

- Test failure: the cached file cannot be opened or its content differs.
- Test failure: `_lru_test` or `_multi_test` still observes an invalid cache entry after the handle lifetime is fixed.
- Out of scope: production recovery from a process crash or externally created orphan `.cache/.meta` files.
- Fatal implementation failure: the modified test object is not rebuilt or the test executable fails to link.

## Verification Strategy

1. Preserve the fresh 5-test/2-failure JUnit result as RED evidence.
2. Apply only the RAII scope change.
3. Build through VS2022 v143 x64 and confirm `ComponentInformationCacheTest.cc.obj` is compiled before linking QGroundControl.
4. Run `ComponentInformationCacheTest` with JUnit XML, console logs, `QGC_TEST_VERBOSE=1`, `--log-output`, and `--logging`; require 5 tests with zero failures/errors/skips.
5. Run `RequestMetaDataTypeStateMachineTest` as the directly related shared-cache regression; require zero failures/errors.
6. Confirm the fresh test run leaves no directory for its unique `QGCCacheTest_<pid>_<uuid>` suffix.
7. Run `git diff --check` and review the one-file production-free diff.

## Git and Delivery

- Commit only to `codex/windows-test-hardening`.
- Use a focused structured test commit.
- Push only after focused verification and independent review.
- Do not force-push or create a PR.
- No GitHub issue or PR number is known; do not fabricate a tracking footer.
- Keep `.tmp`, JUnit XML, console logs, and build products untracked.

## Success Criteria

- `ComponentInformationCacheTest`: 5 tests, 0 failures, 0 errors, 0 skips.
- `RequestMetaDataTypeStateMachineTest`: zero failures and zero errors.
- The current run's uniquely named cache and source fixture directories are removed.
- No production cache implementation or behavior changes.
- Broader Phase 2 Mission and CTest-runner investigations remain open.
