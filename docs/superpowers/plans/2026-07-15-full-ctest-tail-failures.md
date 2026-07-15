# Full CTest Tail Failures Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make the Windows VS2022 x64 Debug suite pass all 186 CTest registrations without changing production behavior or weakening assertions.

**Architecture:** Correct only the proven timeout boundary for each failing test. CTest runtime limits stay platform/test-specific, while internal waits retain their existing conditions and change only to established `TestTimeout` classes. Each correction receives focused RED/GREEN evidence before the final full-suite gate.

**Tech Stack:** CMake, Ninja, CTest, Qt 6.10.3 QTest, MSVC v143 x64, PowerShell 7, QGroundControl test runner.

## Global Constraints

- Use VS2022 Community v143 Hostx64/x64, Qt `E:\Qt\6.10.3\msvc2022_64`, and GStreamer `E:\Program Files\gstreamer\1.0\msvc_x86_64`.
- Do not modify production source, shared timeout constants, non-Windows behavior, or unrelated registrations.
- Do not skip tests, add retries, whitelist warnings, suppress failures, use MinGW/qmake, or install Android tooling.
- Keep build/test output redirected to `.tmp\phase4`; print only summaries under 50 lines.
- Do not create downstream branches until a fresh full CTest reports 186/186 and exit code 0.
- Do not force-push, modify `codex/joystick-aux-px4`, or create a pull request.

## File Map

- `test/CMakeLists.txt`: platform-scoped CTest runtime limits.
- `test/FactSystem/ParameterManagerTest.cc`: positive parameter progress-signal wait bounds.
- `test/Vehicle/VehicleLinkManagerTest.cc`: initial-connect condition wait bounds.
- `ENVIRONMENT_REPORT.md`: authoritative Windows build/test result.
- `state/README.md`: current verified project state.
- `state/TODO.md`: execution checklist and branch gate.
- `state/LOG.md`: failure mechanism and durable troubleshooting evidence.
- `AGENTS.md`: stable Windows test timing/project rules only when new evidence warrants it.

---

### Task 1: Scope The Two CTest Runtime Limits

**Files:**
- Modify: `test/CMakeLists.txt:95`
- Modify: `test/CMakeLists.txt:200`

**Interfaces:**
- Consumes: `QGC_TEST_TIMEOUT_INTEGRATION`, `QGC_TEST_TIMEOUT_EXTENDED`, and `add_qgc_test(... TIMEOUT ...)`.
- Produces: Windows timeout properties of 360 seconds for `VehicleCameraControlTest` and 180 seconds for `MissionControllerTreeTest`.

- [ ] **Step 1: Preserve the existing RED evidence**

Record from `.tmp\phase4\full-ctest\full-ctest.xml`:

```text
VehicleCameraControlTest: Timeout at 120.05 s
MissionControllerTreeTest: Timeout at 120.048 s
```

Record from `.tmp\phase4\full-failures`:

```text
VehicleCameraControlTest: 15/15 passed in 161.519 s
MissionControllerTreeTest: 11/11 passed in 123.426 s
```

- [ ] **Step 2: Add test/platform-specific timeout variables**

Replace the two one-line registrations with:

```cmake
set(_vehicle_camera_control_timeout ${QGC_TEST_TIMEOUT_INTEGRATION})
if(WIN32)
    set(_vehicle_camera_control_timeout 360)
endif()
add_qgc_test(VehicleCameraControlTest
    LABELS Integration Vehicle
    RESOURCE_LOCK MockLink
    TIMEOUT ${_vehicle_camera_control_timeout}
)
```

```cmake
set(_mission_controller_tree_timeout ${QGC_TEST_TIMEOUT_INTEGRATION})
if(WIN32)
    set(_mission_controller_tree_timeout ${QGC_TEST_TIMEOUT_EXTENDED})
endif()
add_qgc_test(MissionControllerTreeTest
    LABELS Integration MissionManager
    RESOURCE_LOCK MockLink
    TIMEOUT ${_mission_controller_tree_timeout}
)
```

- [ ] **Step 3: Reconfigure and verify generated properties**

Run from the repository root with the explicit VS2022 x64 environment:

```powershell
$vsDevCmd = 'D:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat'
$qtCMake = 'E:\Qt\6.10.3\msvc2022_64\bin\qt-cmake.bat'
cmd /d /s /c "`"$vsDevCmd`" -arch=x64 -host_arch=x64 && set `"PATH=E:\Qt\6.10.3\msvc2022_64\bin;E:\Program Files\gstreamer\1.0\msvc_x86_64\bin;%PATH%`" && `"$qtCMake`" -S . -B build\windows-debug -G Ninja -DCMAKE_BUILD_TYPE=Debug" *> .tmp\phase4\tail-timeouts-configure.log
```

Expected: exit 0. `build\windows-debug\test\CTestTestfile.cmake` contains `TIMEOUT "360"` for the camera test and `TIMEOUT "180"` for the mission tree test. Check one unrelated integration test still has 120 seconds.

- [ ] **Step 4: Run focused CTest GREEN verification**

```powershell
ctest --test-dir build\windows-debug -R '^(VehicleCameraControlTest|MissionControllerTreeTest)$' --output-on-failure --output-junit .tmp\phase4\tail-timeouts-ctest.xml *> .tmp\phase4\tail-timeouts-ctest.log
```

Expected: exit 0, 2/2 passed, JUnit failures=0.

- [ ] **Step 5: Commit the scoped registrations**

```powershell
git add test/CMakeLists.txt
git commit -m "test[windows]: extend focused CTest timeouts"
```

### Task 2: Restore Parameter Progress Wait Margin

**Files:**
- Modify: `test/FactSystem/ParameterManagerTest.cc:48`
- Modify: `test/FactSystem/ParameterManagerTest.cc:118`

**Interfaces:**
- Consumes: `QVERIFY_SIGNAL_WAIT` and `TestTimeout::mediumMs()`.
- Produces: unchanged progress-signal/value assertions with a five-second positive wait bound.

- [ ] **Step 1: Preserve the focused RED result**

Use `.tmp\phase4\full-failures\ParameterManagerTest-ParameterManagerTest.xml` as the RED artifact. Expected existing result: 14 tests, 3 failures in `_noFailure`, `_requestListMissingParamSuccess`, and `_requestListMissingParamFail`, each timing out after 1,000 ms waiting for `spyProgress`.

- [ ] **Step 2: Change only positive progress waits**

In `_noFailureWorker` and `_requestListMissingParamFail`, replace:

```cpp
QVERIFY_SIGNAL_WAIT(spyProgress, TestTimeout::shortMs());
```

with:

```cpp
QVERIFY_SIGNAL_WAIT(spyProgress, TestTimeout::mediumMs());
```

The worker covers `_noFailure` and `_requestListMissingParamSuccess`, so there are exactly two source replacements covering three failing cases. Leave `QVERIFY_NO_SIGNAL_WAIT(spyProgress, TestTimeout::shortMs())` unchanged.

- [ ] **Step 3: Rebuild with the VS2022 x64 developer environment**

```powershell
$vsDevCmd = 'D:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat'
cmd /d /s /c "`"$vsDevCmd`" -arch=x64 -host_arch=x64 && cmake --build build\windows-debug --parallel 2" *> .tmp\phase4\parameter-wait-build.log
```

Expected: exit 0; the log shows `ParameterManagerTest.cc.obj` compiled and `QGroundControl.exe` relinked.

- [ ] **Step 4: Run direct JUnit GREEN verification**

```powershell
$dir = New-Item -ItemType Directory -Force .tmp\phase4\parameter-wait
$env:PATH = 'E:\Qt\6.10.3\msvc2022_64\bin;E:\Program Files\gstreamer\1.0\msvc_x86_64\bin;' + $env:PATH
$env:QT_QPA_PLATFORM = 'offscreen'
$env:QT_LOGGING_RULES = '*.debug=false'
$env:QT_QPA_FONTDIR = "$env:WINDIR\Fonts"
$env:QGC_TEST_VERBOSE = '1'
$exe = Resolve-Path build\windows-debug\Debug\QGroundControl.exe
$xml = Join-Path $dir.FullName 'ParameterManagerTest.xml'
$stdout = Join-Path $dir.FullName 'ParameterManagerTest.stdout.log'
$stderr = Join-Path $dir.FullName 'ParameterManagerTest.stderr.log'
$process = Start-Process $exe -ArgumentList @('--unittest:ParameterManagerTest', "--unittest-output:$xml", '--log-output', '--logging:default', '--allow-multiple') -WorkingDirectory (Resolve-Path build\windows-debug) -RedirectStandardOutput $stdout -RedirectStandardError $stderr -PassThru -Wait
if ($process.ExitCode -ne 0) { throw "ParameterManagerTest exit $($process.ExitCode)" }
```

Expected: process exit 0; generated QTest XML contains 14 tests, 0 failures, 0 errors.

- [ ] **Step 5: Run generated CTest GREEN verification**

```powershell
ctest --test-dir build\windows-debug -R '^ParameterManagerTest$' --output-on-failure --output-junit .tmp\phase4\parameter-wait-ctest.xml *> .tmp\phase4\parameter-wait-ctest.log
```

Expected: exit 0, 1/1 passed.

- [ ] **Step 6: Commit the parameter test correction**

```powershell
git add test/FactSystem/ParameterManagerTest.cc
git commit -m "test[parameters]: restore progress wait margin"
```

### Task 3: Extend Equivalent Vehicle Initial-Connect Waits

**Files:**
- Modify: `test/Vehicle/VehicleLinkManagerTest.cc:32`
- Modify: `test/Vehicle/VehicleLinkManagerTest.cc:63`
- Modify: `test/Vehicle/VehicleLinkManagerTest.cc:99`
- Modify: `test/Vehicle/VehicleLinkManagerTest.cc:174`

**Interfaces:**
- Consumes: existing initial-connect condition and `TestTimeout::longMs()`.
- Produces: four equivalent condition waits with a 30-second upper bound and unchanged link cleanup behavior.

- [ ] **Step 1: Preserve the focused RED result**

Use `.tmp\phase4\full-failures\VehicleLinkManagerTest-VehicleLinkManagerTest.xml`. Expected existing result: 7 tests, 3 failures; Qt reports 6,100, 9,950, and 10,050 ms would have satisfied the 5,000 ms checks.

- [ ] **Step 2: Change all equivalent initial-connect bounds**

For each of the four occurrences of:

```cpp
QVERIFY_TRUE_WAIT(spyVehicleInitialConnectComplete.count() > 0 || vehicle->isInitialConnectComplete(),
                  TestTimeout::mediumMs());
```

use:

```cpp
QVERIFY_TRUE_WAIT(spyVehicleInitialConnectComplete.count() > 0 || vehicle->isInitialConnectComplete(),
                  TestTimeout::longMs());
```

Do not alter signal creation, boolean conditions, event-loop settling, disconnect calls, or reference-count assertions.

- [ ] **Step 3: Rebuild with the VS2022 x64 developer environment**

```powershell
$vsDevCmd = 'D:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat'
cmd /d /s /c "`"$vsDevCmd`" -arch=x64 -host_arch=x64 && cmake --build build\windows-debug --parallel 2" *> .tmp\phase4\vehicle-link-wait-build.log
```

Expected: exit 0; the log shows `VehicleLinkManagerTest.cc.obj` compiled and `QGroundControl.exe` relinked.

- [ ] **Step 4: Run direct JUnit GREEN verification**

```powershell
$dir = New-Item -ItemType Directory -Force .tmp\phase4\vehicle-link-wait
$env:PATH = 'E:\Qt\6.10.3\msvc2022_64\bin;E:\Program Files\gstreamer\1.0\msvc_x86_64\bin;' + $env:PATH
$env:QT_QPA_PLATFORM = 'offscreen'
$env:QT_LOGGING_RULES = '*.debug=false'
$env:QT_QPA_FONTDIR = "$env:WINDIR\Fonts"
$env:QGC_TEST_VERBOSE = '1'
$exe = Resolve-Path build\windows-debug\Debug\QGroundControl.exe
$xml = Join-Path $dir.FullName 'VehicleLinkManagerTest.xml'
$stdout = Join-Path $dir.FullName 'VehicleLinkManagerTest.stdout.log'
$stderr = Join-Path $dir.FullName 'VehicleLinkManagerTest.stderr.log'
$process = Start-Process $exe -ArgumentList @('--unittest:VehicleLinkManagerTest', "--unittest-output:$xml", '--log-output', '--logging:default', '--allow-multiple') -WorkingDirectory (Resolve-Path build\windows-debug) -RedirectStandardOutput $stdout -RedirectStandardError $stderr -PassThru -Wait
if ($process.ExitCode -ne 0) { throw "VehicleLinkManagerTest exit $($process.ExitCode)" }
```

Expected: process exit 0; generated QTest XML contains 7 tests, 0 failures, 0 errors.

- [ ] **Step 5: Run generated CTest GREEN verification**

```powershell
ctest --test-dir build\windows-debug -R '^VehicleLinkManagerTest$' --output-on-failure --output-junit .tmp\phase4\vehicle-link-wait-ctest.xml *> .tmp\phase4\vehicle-link-wait-ctest.log
```

Expected: exit 0, 1/1 passed.

- [ ] **Step 6: Commit the vehicle test correction**

```powershell
git add test/Vehicle/VehicleLinkManagerTest.cc
git commit -m "test[vehicle]: extend initial connect waits"
```

### Task 4: Run The Full Gate And Reconcile Records

**Files:**
- Modify: `ENVIRONMENT_REPORT.md`
- Modify: `state/README.md`
- Modify: `state/TODO.md`
- Modify: `state/LOG.md`
- Modify if warranted by new durable facts: `AGENTS.md`

**Interfaces:**
- Consumes: the rebuilt Debug runner and all three focused correction commits.
- Produces: authoritative 186-test CTest JUnit, exit code, timing, and reconciled project state.

- [ ] **Step 1: Verify the worktree and executable before the expensive run**

```powershell
git diff --check
git status --short --branch
Get-Item build\windows-debug\Debug\QGroundControl.exe
ctest --test-dir build\windows-debug -N *> .tmp\phase4\final-ctest-list.log
```

Expected: only `.tmp/` is untracked, the executable exists, and the list reports 186 tests.

- [ ] **Step 2: Run CTest with an interruption-safe wrapper**

Launch CTest in a hidden background PowerShell process, inheriting Qt and GStreamer paths, and write:

```text
.tmp\phase4\final-full-ctest\full-ctest.stdout.log
.tmp\phase4\final-full-ctest\full-ctest.stderr.log
.tmp\phase4\final-full-ctest\full-ctest.xml
.tmp\phase4\final-full-ctest\full-ctest.exit.txt
.tmp\phase4\final-full-ctest\full-ctest.started.txt
.tmp\phase4\final-full-ctest\full-ctest.finished.txt
```

The underlying command is:

```powershell
ctest --test-dir build\windows-debug --output-on-failure --output-junit <absolute-junit-path>
```

Expected: wrapper exits naturally; exit file is `0`.

- [ ] **Step 3: Parse the authoritative result**

Parse the JUnit root and require:

```text
tests=186
failures=0
disabled=0
skipped=0
```

Scan stdout/stderr for `Timeout`, `FAIL!`, `Segmentation fault`, `ASSERT`, `0xc0000005`, and `Errors while running CTest`. Any match requires diagnosis; do not proceed to delivery.

- [ ] **Step 4: Update state and environment records**

Record exact start/end time, real duration, exit code, 186/186 result, focused results, build evidence, branch head, and artifact paths. Mark the full-suite and downstream-branch TODO items complete only after their corresponding evidence exists.

- [ ] **Step 5: Commit and push the verified records**

```powershell
git add AGENTS.md ENVIRONMENT_REPORT.md state/README.md state/TODO.md state/LOG.md
git commit -m "docs[tests]: record complete Windows validation"
git push origin codex/windows-test-hardening
```

Expected: local and remote `codex/windows-test-hardening` point to the same commit; only `.tmp/` remains untracked.

### Task 5: Create The Two Delivery Branches

**Files:**
- No tracked file changes.

**Interfaces:**
- Consumes: verified/pushed `codex/windows-test-hardening` head with a 186/186 result.
- Produces: `origin/codex/joystick-aux-px4-development` and `origin/change1_crocodile_<version>` at the same verified commit, with the worktree switched to the final development branch.

- [ ] **Step 1: Refresh and verify protected refs**

```powershell
git fetch origin --prune
git status --short --branch
git rev-parse codex/windows-test-hardening
git rev-parse origin/codex/windows-test-hardening
git rev-parse origin/codex/joystick-aux-px4
```

Expected: hardening local/remote hashes match; only `.tmp/` is untracked. Record the original integration hash and do not change it.

- [ ] **Step 2: Create and push the development base without force**

```powershell
git branch codex/joystick-aux-px4-development codex/windows-test-hardening
git push --set-upstream origin codex/joystick-aux-px4-development
```

If the branch already exists, stop and compare hashes instead of overwriting it.

- [ ] **Step 3: Derive and validate the final branch name**

```powershell
$version = (git describe --tags --always).Trim() -replace '[^0-9A-Za-z._-]', '_'
$finalBranch = "change1_crocodile_$version"
$finalBranch
```

Expected: a valid Git ref beginning with `change1_crocodile_` and containing the delivery-time QGC description.

- [ ] **Step 4: Create, switch, and push the final branch without force**

```powershell
git switch -c $finalBranch codex/joystick-aux-px4-development
git push --set-upstream origin $finalBranch
```

If the ref already exists locally or remotely, stop and compare hashes instead of overwriting it.

- [ ] **Step 5: Verify all delivery refs and protected integration state**

```powershell
git ls-remote --heads origin codex/joystick-aux-px4 codex/joystick-aux-px4-development $finalBranch
git status --short --branch
git log -1 --oneline --decorate
```

Expected: both new remote refs resolve to the verified hardening commit, the original integration ref retains its recorded hash, the worktree tracks the final branch, and no PR exists or is created by this workflow.
