# Windows Test Hardening Design

**Status:** Approved in conversation on 2026-07-13; final user review approved on 2026-07-14.

## Goal

Make the known Windows Desktop test failures deterministic and meaningful under VS2022 v143 x64 and Qt 6.10.3 without masking real product defects or weakening the global test framework.

## Baseline

- Branch: `codex/windows-test-hardening`
- Base feature commit: `daae3a37b`
- Build fix included: `/Zc:preprocessor` for MSVC
- Clean Debug build: successful
- Targeted baseline: 11 suites, 8 failing suites, 11 assertion failures
- Individually passing suites: `HashCheckTest`, `SigningTest`, `RequestMetaDataTypeStateMachineTest`
- Windows token: medium integrity, non-administrator

## Repair Policy

1. Fix production code when a test exposes incorrect Windows behavior used by the application.
2. Fix test code when the assertion assumes a Unix-only environment, a specific locale, case-sensitive Windows paths, or unavailable host capabilities.
3. Probe optional operating-system capabilities at runtime. Use `QSKIP` only when the capability cannot be created or verified; when available, execute the full assertion path.
4. Do not add global warning suppression, broad logging allowlists, unconditional Windows skips, or weakened assertions.
5. Keep each independent root cause in a focused structured commit.

## Work Streams

### 1. Deterministic test assumptions

- `JsonHelperTest`: retain the failure-return assertion and verify language-independent evidence such as expected/actual file type values instead of untranslated English prose.
- `PlatformTest`: assert only environment variables that `Platform::initialize` is designed to set on the current platform. Preserve the headless unit-test assertion on Windows.
- `GeoTagControllerTest`: compare Windows paths with filesystem-appropriate semantics. Resolve the read-only temporary-file cleanup warning at its source so strict logging remains useful.

### 2. Windows local-path handling

`QGCFileHelper::toLocalPath` and `isLocalPath` currently allow `QUrl` to interpret `C:/...` as scheme `c`. Add explicit Windows drive-path and UNC-path recognition before generic URL parsing. Extend the focused file-helper tests first, verify RED, then implement the minimum production change. Re-run `QGCArchiveModelTest` as the downstream regression test.

### 3. Compression capabilities and Unicode

- Replace the assumption that `QFile::link` necessarily creates a directory symbolic link on Windows. The test helper must verify `QFileInfo::isSymLink`; if a real directory symlink cannot be created because Developer Mode/UAC capability is unavailable, skip with an explicit reason.
- Trace the Unicode extraction failure through Qt path conversion, libarchive pathname APIs, and extracted destination construction. Fix the first boundary that loses or misinterprets Unicode. Do not change test strings merely to make them ASCII-compatible.

### 4. Component information cache

Instrument or assert actual/expected cache paths and access state in the focused test to locate the first mismatch. Determine whether the cause is path normalization, stale state after a failed case, counter ordering, or filesystem behavior. Fix only the confirmed source and retain LRU semantics.

### 5. Mission strict-log failures

- Capture every uncategorized warning for `MissionCommandTreeEditorTest` and `MissionManagerTest`.
- Separate external environment warnings from QGC-generated warnings.
- Fix malformed QGC formatting/category usage at the source.
- For unavoidable third-party/Qt environment warnings, use the narrowest test-scoped expectation supported by the existing framework; do not install a global allowlist.

### 6. CTest runner and timeout behavior

- `HashCheckTest` passes individually but exceeds the current 120-second Integration timeout. Confirm whether its work is intentionally long and stable before changing the per-test timeout metadata.
- `SigningTest` and `RequestMetaDataTypeStateMachineTest` pass individually. Reproduce them through their exact CTest entries and inspect runner ordering/environment before changing code.
- A runner-only failure must be fixed in CTest metadata or test isolation, not in unrelated product logic.

## Verification Strategy

For each root cause:

1. Preserve or add a minimal focused test that demonstrates the defect.
2. Run it before implementation and record the expected RED failure.
3. Apply one minimal fix.
4. Incrementally rebuild `build/windows-debug`.
5. Run the focused test and directly related tests; require zero unexpected warnings.
6. Commit only the files belonging to that root cause.

After all focused repairs:

1. Run the 11-suite targeted set and archive JUnit XML and console logs.
2. Run full CTest once, with `--output-on-failure`, to measure the final Windows baseline.
3. Confirm the Debug application still starts with the configured Qt/GStreamer PATH.
4. Run `git diff --check` and review the complete branch diff against `origin/codex/joystick-aux-px4`.

## Git and Delivery

- Local and remote repair branch: `codex/windows-test-hardening`
- Push only this new branch; never force-push or update `codex/joystick-aux-px4`.
- Use structured commit messages such as `fix[filesystem]: recognize Windows drive paths` or `test[platform]: use platform-specific environment expectations`.
- Do not create a PR until explicitly requested.
- Keep `.tmp`, build directories, XML, and console logs untracked.

## Success Criteria

- Every changed behavior has focused RED/GREEN evidence.
- All capability-dependent skips state the exact missing capability.
- The 11 targeted suites have no unexplained failures.
- Full CTest results are recorded honestly; unrelated hardware/display failures are not hidden.
- No regression is introduced in Debug application startup or Release build configuration.
