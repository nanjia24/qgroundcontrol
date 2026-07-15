# Windows Controller and Initial Connect Timeout Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make the proven-slow Windows Mission controller test reliable and produce function/data-row evidence that identifies why Initial Connect does not finish within 360 seconds.

**Architecture:** Apply one durable Windows-only CTest timeout correction for `MissionControllerTest`. Use a temporary environment-driven QTest function selector to isolate every `InitialConnectTest` function/data row, then remove the selector before committing evidence; the actual Initial Connect correction receives a separate evidence-backed spec and plan.

**Tech Stack:** CMake 3.31, Ninja, VS2022 v143 x64, Qt 6.10.3 `msvc2022_64`, Qt Test, CTest JUnit, PowerShell.

## Global Constraints

- Work only in `E:\workspace\QGC\qgroundcontrol-worktrees\windows-test-hardening` on `codex/windows-test-hardening`.
- Use VS2022 Community v143 with x64 host/target, Qt `E:\Qt\6.10.3\msvc2022_64`, and GStreamer `E:\PROGRA~1\GSTREA~1\1.0\MSVC_X~1`.
- Preserve `/Zc:preprocessor`; do not pass manual `CMAKE_CXX_FLAGS`, use MinGW, or use qmake.
- Do not raise shared Unit, Integration, Slow, or default timeout values.
- Do not modify production Initial Connect behavior, skip tests/data rows, whitelist warnings, or introduce unbounded waits.
- The QTest function selector is temporary and uncommitted; restore it and rebuild before recording localization evidence.
- Do not run full CTest or create downstream branches in this plan. Those require the later Initial Connect correction and a complete zero-failure full run.
- Preserve the existing untracked `.tmp/` tree and all proxy environment variables.

---

### Task 1: Give MissionControllerTest a Windows-only extended timeout

**Files:**
- Modify: `test/CMakeLists.txt:184-195`
- Test: `build/windows-debug/test/CTestTestfile.cmake`
- Test: `build/windows-debug/Debug/QGroundControl.exe --unittest:MissionControllerTest --allow-multiple`

**Interfaces:**
- Consumes: `QGC_TEST_TIMEOUT_INTEGRATION` (120 seconds) and `QGC_TEST_TIMEOUT_EXTENDED` (180 seconds for this non-sanitizer Debug build).
- Produces: `_mission_controller_timeout`, passed explicitly to `add_qgc_test(MissionControllerTest ...)`; only Windows selects the extended value.

- [ ] **Step 1: Reconfirm the current RED timeout**

```powershell
Set-Location 'E:\workspace\QGC\qgroundcontrol-worktrees\windows-test-hardening'
$env:Path = 'E:\Qt\6.10.3\msvc2022_64\bin;E:\PROGRA~1\GSTREA~1\1.0\MSVC_X~1\bin;' + $env:Path
$env:QGC_TEST_VERBOSE = '1'
New-Item -ItemType Directory -Force '.tmp\phase3-plan' | Out-Null
ctest --test-dir build\windows-debug -R '^MissionControllerTest$' `
  --output-on-failure `
  --output-junit "$PWD\.tmp\phase3-plan\MissionControllerTest-red.xml" `
  *> '.tmp\phase3-plan\MissionControllerTest-red.log'
$exitCode = $LASTEXITCODE
if ($exitCode -eq 0) { throw 'Expected the registered 120-second test to time out before the fix.' }
Get-Content '.tmp\phase3-plan\MissionControllerTest-red.log' -Tail 16
```

Expected: CTest reports `MissionControllerTest (Timeout)` at approximately
120 seconds and returns non-zero.

- [ ] **Step 2: Apply the scoped CMake correction**

Replace the direct registration:

```cmake
add_qgc_test(MissionControllerTest LABELS Integration MissionManager RESOURCE_LOCK MockLink)
```

with:

```cmake
set(_mission_controller_timeout ${QGC_TEST_TIMEOUT_INTEGRATION})
if(WIN32)
    set(_mission_controller_timeout ${QGC_TEST_TIMEOUT_EXTENDED})
endif()
add_qgc_test(MissionControllerTest
    LABELS Integration MissionManager
    RESOURCE_LOCK MockLink
    TIMEOUT ${_mission_controller_timeout}
)
```

Do not change the previously scoped `MissionCommandTreeEditorTest` timeout or
any shared timeout variable.

- [ ] **Step 3: Regenerate and build with the complete MSVC environment**

```powershell
$vsDevCmd = 'D:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat'
$vsEnvironment = & cmd.exe /d /s /c "call `"$vsDevCmd`" -arch=x64 -host_arch=x64 >nul && set"
foreach ($entry in $vsEnvironment) {
    if ($entry -match '^([^=]+)=(.*)$') {
        [Environment]::SetEnvironmentVariable($matches[1], $matches[2], 'Process')
    }
}
if ($env:VSCMD_ARG_TGT_ARCH -ne 'x64') { throw 'VS2022 target is not x64.' }
$env:Path = 'E:\Qt\6.10.3\msvc2022_64\bin;E:\PROGRA~1\GSTREA~1\1.0\MSVC_X~1\bin;' + $env:Path
& 'E:\Qt\6.10.3\msvc2022_64\bin\qt-cmake.bat' -S . -B build\windows-debug -G Ninja -DCMAKE_BUILD_TYPE=Debug `
  *> '.tmp\phase3-plan\MissionControllerTest-configure.log'
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
cmake --build build\windows-debug --parallel 2 *> '.tmp\phase3-plan\MissionControllerTest-build.log'
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
ctest --test-dir build\windows-debug -N -V -R '^MissionControllerTest$' `
  *> '.tmp\phase3-plan\MissionControllerTest-properties.log'
rg -n 'MissionControllerTest|TIMEOUT' build\windows-debug\test\CTestTestfile.cmake
```

Expected: configure/build exit zero and the generated registration contains
`MissionControllerTest` with `TIMEOUT "180"` in this non-sanitizer build.

- [ ] **Step 4: Verify GREEN through CTest**

```powershell
$env:QGC_TEST_VERBOSE = '1'
ctest --test-dir build\windows-debug -R '^MissionControllerTest$' `
  --output-on-failure `
  --output-junit "$PWD\.tmp\phase3-plan\MissionControllerTest-green.xml" `
  *> '.tmp\phase3-plan\MissionControllerTest-green.log'
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
$bad = Select-String '.tmp\phase3-plan\MissionControllerTest-green.log' `
  -Pattern 'Timeout|FAILED|FAIL!|Segmentation fault|ASSERT|missing DLL|plugin'
if ($bad) { $bad; throw 'Unexpected MissionControllerTest diagnostic.' }
Get-Content '.tmp\phase3-plan\MissionControllerTest-green.log' -Tail 14
```

Expected: CTest 1/1 passes below 180 seconds; its JUnit contains no failure or
error node.

- [ ] **Step 5: Commit the durable timeout correction**

```powershell
git diff --check
git add -- test/CMakeLists.txt
git commit -m 'test[mission]: extend Windows controller timeout'
```

Expected: one focused CMake/test-registration commit; no generated build file
or `.tmp` artifact is tracked.

---

### Task 2: Localize InitialConnectTest by function and data row

**Files:**
- Temporarily modify, then restore: `test/UnitTestList.cc:16-45`
- Temporarily modify, then restore: `test/UnitTestFramework/UnitTest.cc:550-575`
- Modify after restoration: `ENVIRONMENT_REPORT.md`
- Modify after restoration: `state/README.md`
- Modify after restoration: `state/TODO.md`
- Modify after restoration: `state/LOG.md`
- Test: `build/windows-debug/Debug/QGroundControl.exe --unittest:InitialConnectTest --allow-multiple`

**Interfaces:**
- Consumes: optional process environment variable `QGC_TEST_FUNCTION`, containing a Qt Test selector such as `_stateRunMatrix:HL_0_LR_0_Fly_0`.
- Produces temporarily: exactly one selected QTest function/data row when exactly one registered test class is requested; ambiguous multi-class use returns a diagnostic error.
- Produces durably: selector-by-selector timing and outcome evidence with no test-runner source remaining in the committed diff.

- [ ] **Step 1: Add the temporary single-suite guard**

In `QGCUnitTest::runTests`, after `testsToRun.isEmpty()` has been handled and
before iterations begin, add:

```cpp
    const QString functionSelector = qEnvironmentVariable("QGC_TEST_FUNCTION").trimmed();
    if (!functionSelector.isEmpty() && testsToRun.size() != 1) {
        qCWarning(UnitTestListLog) << "QGC_TEST_FUNCTION requires exactly one registered test class";
        return -1;
    }
```

This change is diagnostic and must not be committed.

- [ ] **Step 2: Add the temporary QTest selector argument**

In `UnitTest::run`, replace:

```cpp
        QStringList args;
        args << "*" << "-maxwarnings" << QString::number(maxWarnings);
```

with:

```cpp
        QStringList args;
        args << "*";
        const QString functionSelector = qEnvironmentVariable("QGC_TEST_FUNCTION").trimmed();
        if (!functionSelector.isEmpty()) {
            args << functionSelector;
        }
        args << "-maxwarnings" << QString::number(maxWarnings);
```

The first `*` remains QTest's program-name argument. This change is diagnostic
and must not be committed.

- [ ] **Step 3: Build the instrumented test runner**

```powershell
$vsDevCmd = 'D:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat'
$vsEnvironment = & cmd.exe /d /s /c "call `"$vsDevCmd`" -arch=x64 -host_arch=x64 >nul && set"
foreach ($entry in $vsEnvironment) {
    if ($entry -match '^([^=]+)=(.*)$') {
        [Environment]::SetEnvironmentVariable($matches[1], $matches[2], 'Process')
    }
}
$env:Path = 'E:\Qt\6.10.3\msvc2022_64\bin;E:\PROGRA~1\GSTREA~1\1.0\MSVC_X~1\bin;' + $env:Path
cmake --build build\windows-debug --target QGroundControl --parallel 2 `
  *> '.tmp\phase3-plan\InitialConnect-selector-build.log'
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
```

Expected: the affected test-runner objects compile and `QGroundControl.exe`
links successfully.

- [ ] **Step 4: Verify ambiguous selector use is rejected**

```powershell
$env:QGC_TEST_FUNCTION = '_boardVendorProductId'
Push-Location build\windows-debug
& '.\Debug\QGroundControl.exe' '--unittest' '--allow-multiple' `
  *> '..\..\.tmp\phase3-plan\InitialConnect-selector-guard.log'
$guardExit = $LASTEXITCODE
Pop-Location
Remove-Item Env:QGC_TEST_FUNCTION
if ($guardExit -eq 0) { throw 'Expected ambiguous QGC_TEST_FUNCTION use to fail.' }
rg -n 'requires exactly one registered test class' '.tmp\phase3-plan\InitialConnect-selector-guard.log'
```

Expected: non-zero exit with the exact guard diagnostic and no test suite run.

- [ ] **Step 5: Run all 22 selectors through CTest**

```powershell
$selectors = @(
    '_performTestCases:case_0',
    '_performTestCases:case_1',
    '_performTestCases:case_2',
    '_boardVendorProductId',
    '_progressTracking',
    '_highLatencySkipsPlanRequests',
    '_genericAutopilotVersionFailureSkipsUnsupportedPlanTypes',
    '_multipleReconnects',
    '_rallyTimeoutPathDoesNotLeakCompletionHandler',
    '_stateTimeoutFallsThrough:StandardModes',
    '_stateTimeoutFallsThrough:CompInfo',
    '_stateTimeoutFallsThrough:Parameters',
    '_stateTimeoutFallsThrough:Mission',
    '_stateTimeoutFallsThrough:GeoFence',
    '_stateRunMatrix:HL_0_LR_0_Fly_0',
    '_stateRunMatrix:HL_1_LR_0_Fly_0',
    '_stateRunMatrix:HL_0_LR_1_Fly_0',
    '_stateRunMatrix:HL_1_LR_1_Fly_0',
    '_stateRunMatrix:HL_0_LR_0_Fly_1',
    '_stateRunMatrix:HL_1_LR_0_Fly_1',
    '_stateRunMatrix:HL_0_LR_1_Fly_1',
    '_stateRunMatrix:HL_1_LR_1_Fly_1'
)
$results = foreach ($selector in $selectors) {
    $safeName = $selector -replace '[^A-Za-z0-9_-]', '_'
    $env:QGC_TEST_FUNCTION = $selector
    $timer = [System.Diagnostics.Stopwatch]::StartNew()
    ctest --test-dir build\windows-debug -R '^InitialConnectTest$' `
      --output-on-failure `
      --output-junit "$PWD\.tmp\phase3-plan\$safeName.xml" `
      *> ".tmp\phase3-plan\$safeName.log"
    $exitCode = $LASTEXITCODE
    $timer.Stop()
    [pscustomobject]@{
        Selector = $selector
        ExitCode = $exitCode
        ElapsedSeconds = [Math]::Round($timer.Elapsed.TotalSeconds, 2)
        Log = ".tmp\phase3-plan\$safeName.log"
        JUnit = ".tmp\phase3-plan\$safeName.xml"
    }
}
Remove-Item Env:QGC_TEST_FUNCTION
$results | Export-Csv '.tmp\phase3-plan\InitialConnect-selector-results.csv' -NoTypeInformation
$results | Format-Table -AutoSize
```

Expected: exactly 22 result rows. Do not reinterpret a non-zero row as a
reason to skip it; retain its log/JUnit and identify the exact selector.

- [ ] **Step 6: Restore the temporary instrumentation and rebuild**

Use `apply_patch` to remove the exact guard added in Step 1 and restore the
original two-line QTest argument construction from Step 2:

```cpp
        QStringList args;
        args << "*" << "-maxwarnings" << QString::number(maxWarnings);
```

Then run:

```powershell
cmake --build build\windows-debug --target QGroundControl --parallel 2 `
  *> '.tmp\phase3-plan\InitialConnect-selector-restore-build.log'
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
$unexpected = git diff -- test/UnitTestList.cc test/UnitTestFramework/UnitTest.cc
if ($unexpected) { $unexpected; throw 'Temporary selector instrumentation was not fully restored.' }
```

Expected: rebuild exits zero and both test-runner files have no Git diff.

- [ ] **Step 7: Classify the evidence and commit only durable records**

Use `InitialConnect-selector-results.csv` and the corresponding JUnit/log files
to record:

- every selector, exit code, and elapsed time;
- the exact selector(s) with timeout/failure, if any;
- whether all selectors pass and their cumulative runtime explains the class
  timeout;
- whether failure depends on function/data-row order;
- which next design boundary applies: lifecycle/wait correction, leaked-state
  correction, or test-suite split.

Update `ENVIRONMENT_REPORT.md`, `state/README.md`, `state/TODO.md`, and
`state/LOG.md` with those measured results. Do not mark Initial Connect fixed,
run full CTest, or create downstream branches in this task.

```powershell
git diff --check
git status --short
git add -- ENVIRONMENT_REPORT.md state/README.md state/TODO.md state/LOG.md
git commit -m 'test[vehicle]: localize Initial Connect timeout'
git push origin codex/windows-test-hardening
```

Expected: the Mission timeout commit and evidence-record commit are pushed;
no temporary selector code, generated CTest file, JUnit, log, CSV, or `.tmp`
artifact is tracked.
