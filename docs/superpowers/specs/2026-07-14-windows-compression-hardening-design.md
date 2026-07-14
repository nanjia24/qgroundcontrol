# Windows Compression Hardening Design

**Status:** Approved by the user on 2026-07-14.

## Goal

Make `QGCCompressionTest` correctly exercise Windows directory symlink capability and preserve Unicode extraction output paths without weakening archive safety checks or reducing test data to ASCII.

## Baseline

- Branch: `codex/windows-test-hardening`
- Toolchain: VS2022 v143 Hostx64/x64, Qt 6.10.3 `msvc2022_64`
- `QGCCompressionTest`: 51 tests, 2 failures
- `_testExtractArchiveToSymlinkedOutputPath`: `QFile::link` reports success but `QFileInfo::isSymLink()` is false
- `_testUnicodePaths`: extraction returns success but `manifest.json` is absent from the Unicode output directory
- Current Windows process is medium-integrity and non-administrator

## Root Causes

### Directory symlink capability

`TempDirectoryTest::createSymlinkOrSkip` uses `QFile::link`. On Windows this API can create a shell shortcut instead of a real directory symbolic link, so its boolean return value is not a valid capability test. A real directory symlink must be created through a Windows-capable filesystem API and then verified.

### Unicode output pathname

`QGClibarchive::extractArchiveEntries` constructs the correct output `QString`, then calls `archive_entry_set_pathname(entry, outputPath.toUtf8().constData())`. On Windows, the narrow pathname passed to `archive_write_disk` is converted through platform locale semantics and can lose characters. Libarchive provides `archive_entry_copy_pathname_w` for a copied wide pathname.

## Selected Design

### Windows directory symlink probe

- Under `Q_OS_WIN`, create directory links with `std::filesystem::create_directory_symlink` using `std::filesystem::path` values constructed from `QString::toStdWString()`.
- Use the non-throwing `std::error_code` overload. Symlink denial is an expected capability failure, not a fatal exception.
- If creation returns an error, call `QSKIP` with the caller-provided reason plus the system error message/code.
- After reported success, require `QFileInfo(linkPath).isSymLink()` before running extraction assertions.
- If verification fails, remove any created artifact and skip with an explicit verification reason.
- Return a boolean result from the helper so callers immediately return after the helper records `QTest::qSkip` or `QTest::qFail`; a `QSKIP` macro inside a `void` helper does not stop the caller test slot.
- On non-Windows platforms, keep the existing `QFile::link` path and verification behavior.
- Do not enable Developer Mode, request UAC elevation, or silently replace a symlink with a junction/shortcut.

### Unicode archive output

- Preserve all existing archive traversal, symlink-target, parent-directory, and extraction-limit checks.
- Under `Q_OS_WIN`, pass the final output path to `archive_entry_copy_pathname_w` using a stable wide string for the duration of the call.
- On other platforms, retain `archive_entry_set_pathname(...toUtf8...)`.
- Keep the existing Japanese, Chinese, Greek, accented Latin, and Cyrillic test directories unchanged.
- Limit this phase to output entry paths. Unicode archive input filenames are a separate boundary and must not be changed without a focused failing test.

## Error Boundaries

- Expected environment failure: Windows refuses real directory symlink creation. Result: explicit `QSKIP` with capability evidence.
- Expected environment failure: creation reports success but Qt cannot verify a real symlink. Result: remove the artifact, then explicitly `QSKIP` with verification evidence.
- Test failure: cleanup cannot remove an artifact created by the capability probe.
- Product failure: Unicode output extraction reports success but files are not created at the requested `QString` path.
- Fatal/implementation failure: compilation error, libarchive wide-path API unavailable, or archive safety regression. Do not bypass these failures.

## Verification Strategy

1. Record the existing two-failure `QGCCompressionTest` JUnit result as RED.
2. Implement and verify the symlink probe separately. On the current token, the expected result is one explicit skip; on a capable Windows environment, the full symlink extraction assertions must run and pass.
3. Preserve `_testUnicodePaths` as the RED test for the wide pathname change.
4. Incrementally rebuild with VS2022 amd64/amd64 developer environment. Verify that the affected test object is actually recompiled; localized MSVC `/showIncludes` output can leave Ninja with `#deps 0`, requiring safe removal of that single stale object before rebuilding.
5. Run `QGCCompressionTest` with JUnit XML and console logs. Require zero failures/errors; at most one symlink capability skip; Unicode extraction must not skip.
6. Run directly related archive/file-helper suites to detect path-handling regressions.
7. Run `git diff --check`, review the focused diff, and commit each independent root cause separately.

## Git and Delivery

- Commit only to `codex/windows-test-hardening`.
- Use focused structured commits, one for symlink test capability and one for Unicode production handling.
- Push only after focused verification and task review.
- Do not force-push, update `codex/joystick-aux-px4`, or create a PR without explicit approval.
- Keep `.tmp`, build products, XML, and logs untracked.

## Success Criteria

- Symlink behavior is either fully exercised or explicitly skipped because a real directory symlink cannot be created.
- Unicode output paths create and expose `manifest.json` in every existing Unicode directory case.
- `QGCCompressionTest` has zero failures and zero errors.
- No archive traversal, link-target, resource-limit, or cleanup behavior is weakened.
- Related archive/path suites remain green.
