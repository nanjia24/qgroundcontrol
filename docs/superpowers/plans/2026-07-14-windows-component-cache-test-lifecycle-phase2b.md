# Windows Component Cache Test Lifecycle Phase 2B Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Release the component-cache test's open data file before Windows fixture cleanup so the cache, LRU, and multi-instance tests start from deterministic empty directories.

**Architecture:** Fix only the test resource lifetime with an inner C++ scope around the cached-file content check. Preserve production cache behavior and all existing assertions, then verify the direct component cache suite and its state-machine consumer.

**Tech Stack:** C++20, Qt 6.10.3 QtTest, CMake, Ninja, MSVC 2022 v143 Hostx64/x64.

## Global Constraints

- Work only in `E:\workspace\QGC\qgroundcontrol-worktrees\windows-test-hardening` on `codex/windows-test-hardening`.
- Use VS2022 Community v143 Hostx64/x64, Qt `E:\Qt\6.10.3\msvc2022_64`, and GStreamer `E:\PROGRA~1\GSTREA~1\1.0\MSVC_X~1`.
- Modify only `test/Vehicle/ComponentInformation/ComponentInformationCacheTest.cc` for the implementation.
- Do not modify `ComponentInformationCache` production code or add orphan-cache recovery.
- Preserve every existing cache content, LRU, persistence, and multi-instance assertion.
- Use RAII scope lifetime; do not use a delay, retry, skip, platform conditional, path normalization, or explicit `QFile::close()` workaround.
- Do not run full CTest and do not use `-- -v2`.
- Use `--unittest-output`, `--log-output`, `--logging`, and `QGC_TEST_VERBOSE=1`.
- Confirm the affected object actually recompiles; if Ninja reports no work, verify and remove only the stale object inside `build/windows-debug` before rebuilding.
- Redirect build/test output to `.tmp/phase2b/`; keep build products, XML, logs, and `.tmp/` untracked.
- Subagents must treat `state/README.md`, `state/TODO.md`, and `state/LOG.md` as read-only.
- Use structured commits and push only `codex/windows-test-hardening`; no force-push and no PR creation.
- No GitHub issue or PR number is known; do not fabricate a `Fixes` or `Resolves` footer.

## Common Build Command

Create `.tmp/phase2b/build.cmd` with this exact content:

```cmd
@call "D:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64
@set "PATH=E:\Qt\6.10.3\msvc2022_64\bin;E:\PROGRA~1\GSTREA~1\1.0\MSVC_X~1\bin;%PATH%"
@cmake --build build\windows-debug --target QGroundControl --parallel
```

Run it with all output redirected:

```powershell
cmd.exe /d /c .tmp\phase2b\build.cmd *> .tmp\phase2b\build.log
if ($LASTEXITCODE -ne 0) { Get-Content .tmp\phase2b\build.log -Tail 40; exit $LASTEXITCODE }
```

## Common Test Helper

Define this PowerShell helper in the current session:

```powershell
function Invoke-QgcSuite([string] $SuiteName, [string] $EvidenceName) {
    $env:PATH = "E:\Qt\6.10.3\msvc2022_64\bin;E:\PROGRA~1\GSTREA~1\1.0\MSVC_X~1\bin;$env:PATH"
    $env:GSTREAMER_1_0_ROOT_MSVC_X86_64 = 'E:\PROGRA~1\GSTREA~1\1.0\MSVC_X~1'
    $env:QT_PLUGIN_PATH = 'E:\Qt\6.10.3\msvc2022_64\plugins'
    $env:QT_QPA_PLATFORM = 'offscreen'
    $env:QGC_TEST_VERBOSE = '1'
    $env:QT_LOGGING_RULES = '*.debug=false'
    $arguments = @(
        "--unittest:$SuiteName",
        '--allow-multiple',
        "--unittest-output:.tmp/phase2b/$EvidenceName.xml",
        '--log-output',
        '--logging:*.debug=true',
        '--no-windows-assert-ui'
    )
    $process = Start-Process -FilePath 'build\windows-debug\Debug\QGroundControl.exe' `
        -ArgumentList $arguments `
        -RedirectStandardOutput ".tmp\phase2b\$EvidenceName.stdout.log" `
        -RedirectStandardError ".tmp\phase2b\$EvidenceName.console.log" `
        -PassThru -Wait -WindowStyle Hidden
    $process.ExitCode | Set-Content ".tmp\phase2b\$EvidenceName.exit.txt"
    return $process
}
```

The runner appends the Qt object name to the requested XML basename. Parse the generated XML filename rather than assuming the requested name exists unchanged.

---

### Task 1: Release the cached file before fixture cleanup

**Files:**
- Modify: `test/Vehicle/ComponentInformation/ComponentInformationCacheTest.cc:61-66`
- Test: `test/Vehicle/ComponentInformation/ComponentInformationCacheTest.cc:50-136`

**Interfaces:**
- Consumes: the cached path returned by `ComponentInformationCache::insert`.
- Produces: no new interface; the existing local `QFile` and `QTextStream` are destroyed before `_cleanup()`.

- [x] **Step 1: Confirm the preserved RED evidence**

Parse `.tmp/phase2b/baseline-ComponentInformationCacheTest.xml`.

Expected: 5 tests, 2 failures, 0 errors, 0 skips; `_lru_test` and `_multi_test` fail after `_basic_test` leaves an open-file orphan.

- [x] **Step 2: Apply the minimal RAII lifetime change**

Replace the cached-file verification block in `_basic_test` with:

```cpp
    {
        QFile f(_tmpFiles[0].cachedPath);
        QVERIFY(f.open(QFile::ReadOnly | QFile::Text));
        QTextStream in(&f);
        QVERIFY(in.readAll() == _tmpFiles[0].content);
    }
    _cleanup();
```

Do not change any assertion or any other test method.

- [x] **Step 3: Build and prove the test object recompiled**

Run the Common Build Command. Require the log to contain compilation of:

```text
CMakeFiles/QGroundControl.dir/test/Vehicle/ComponentInformation/ComponentInformationCacheTest.cc.obj
```

If it does not, resolve and verify this exact object is inside the resolved `build/windows-debug` root before deleting only it:

```powershell
$objectPath = Resolve-Path 'build/windows-debug/CMakeFiles/QGroundControl.dir/test/Vehicle/ComponentInformation/ComponentInformationCacheTest.cc.obj'
$buildRoot = (Resolve-Path 'build/windows-debug').Path
if (-not $objectPath.Path.StartsWith($buildRoot, [System.StringComparison]::OrdinalIgnoreCase)) {
    throw "Refusing to remove object outside build root: $objectPath"
}
Remove-Item -LiteralPath $objectPath.Path
```

Then rerun the Common Build Command and confirm the object compilation and `Debug/QGroundControl.exe` link.

- [x] **Step 4: Run the focused GREEN test and verify fixture removal**

Run:

```powershell
$process = Invoke-QgcSuite -SuiteName ComponentInformationCacheTest -EvidenceName after-raii
$cacheDirs = Get-ChildItem -LiteralPath ([System.IO.Path]::GetTempPath()) -Directory -ErrorAction SilentlyContinue |
    Where-Object Name -Like "QGCCacheTest_$($process.Id)_*"
$sourceDirs = Get-ChildItem -LiteralPath ([System.IO.Path]::GetTempPath()) -Directory -ErrorAction SilentlyContinue |
    Where-Object Name -Like "QGCTestFiles_$($process.Id)_*"
if ($cacheDirs -or $sourceDirs) {
    throw "Fixture directories remain for test process $($process.Id)"
}
```

Expected: process exit 0; generated JUnit contains 5 tests, 0 failures, 0 errors, 0 skips; neither unique fixture directory remains.

- [x] **Step 5: Review and commit the one-file test fix**

Run:

```powershell
git diff --check
git diff -- test/Vehicle/ComponentInformation/ComponentInformationCacheTest.cc
git add test/Vehicle/ComponentInformation/ComponentInformationCacheTest.cc
git commit -m "test[cache]: release file before fixture cleanup"
```

Expected: one focused test commit; `.tmp/` remains untracked and no production file is staged.

---

### Task 2: Verify the shared cache consumer and record Phase 2B

**Files:**
- Modify: `ENVIRONMENT_REPORT.md`
- Modify: `state/README.md`
- Modify: `state/TODO.md`
- Modify: `state/LOG.md`
- Modify: `docs/superpowers/plans/2026-07-14-windows-component-cache-test-lifecycle-phase2b.md`

**Interfaces:**
- Consumes: the reviewed Task 1 test commit and its JUnit/build evidence.
- Produces: focused regression evidence and an updated remaining Phase 2 baseline without closing Mission or CTest-runner work.

- [x] **Step 1: Run the two-suite final focused regression**

Run:

```powershell
Invoke-QgcSuite -SuiteName ComponentInformationCacheTest -EvidenceName focused.ComponentInformationCacheTest
Invoke-QgcSuite -SuiteName RequestMetaDataTypeStateMachineTest -EvidenceName focused.RequestMetaDataTypeStateMachineTest
```

Expected: both processes exit 0 and both JUnit files contain zero failures/errors/skips.

- [x] **Step 2: Parse JUnit and scan console evidence**

Run:

```powershell
@'
from pathlib import Path
import xml.etree.ElementTree as ET

for path in sorted(Path('.tmp/phase2b').glob('focused.*-*.xml')):
    root = ET.parse(path).getroot()
    suites = [root] if root.tag == 'testsuite' else list(root.findall('.//testsuite'))
    totals = {
        key: sum(int(suite.get(key, '0')) for suite in suites)
        for key in ('tests', 'failures', 'errors', 'skipped')
    }
    print(path.name, ' '.join(f'{key}={value}' for key, value in totals.items()))
'@ | python - > .tmp/phase2b/focused-summary.txt

rg -n -i "cache miss|failed to create dir|fatal|assert|crash|missing.*dll|plugin.*failed" `
    .tmp/phase2b/focused.*.console.log > .tmp/phase2b/focused-error-scan.txt
Get-Content .tmp/phase2b/focused-summary.txt -TotalCount 20
Get-Content .tmp/phase2b/focused-error-scan.txt -TotalCount 20
```

Expected: zero failures/errors/skips and no failure-pattern matches. The known QML debugging notice may appear separately.

- [x] **Step 3: Update report and state with measured evidence**

Append a Phase 2B section to `ENVIRONMENT_REPORT.md` containing the exact test commit, toolchain, build command/exit code, object compilation evidence, per-suite JUnit totals, process exit codes, fixture-directory result, and console scan. State explicitly that production cache code and full CTest were not changed or run.

Update `state/README.md`, `state/TODO.md`, and `state/LOG.md` with the measured result. Keep broader Phase 2 open and reduce the remaining assertion baseline only if fresh evidence supports it.

- [x] **Step 4: Complete independent review and final verification**

Run `superpowers:verification-before-completion`, then request a fresh task review and a whole-Phase-2B review. Resolve every Critical or Important finding and rerun only affected focused tests.

- [x] **Step 5: Commit documentation and push the approved branch**

Run:

```powershell
git diff --check
git add ENVIRONMENT_REPORT.md state/README.md state/TODO.md state/LOG.md `
    docs/superpowers/plans/2026-07-14-windows-component-cache-test-lifecycle-phase2b.md
git commit -m "docs[cache]: record Phase 2B verification"
git push origin codex/windows-test-hardening
```

Expected: local and remote `codex/windows-test-hardening` point to the same commit; `codex/joystick-aux-px4` remains unchanged; only `.tmp/` is untracked.
