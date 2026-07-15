# Initial Connect Board Teardown Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Fix the proven `_boardVendorProductId` MockLink teardown crash, complete the Initial Connect suite, and establish a full-test baseline before creating downstream development branches.

**Architecture:** Keep the correction in `InitialConnectTest`: track its custom MockLink in the existing fixture and reuse `_disconnectMockLink()` for ordered vehicle removal and event-loop settling. Measure the aggregate suite with a reversible build-tree timeout probe before choosing any scoped Initial Connect timeout; do not change production LinkManager behavior.

**Tech Stack:** VS2022 v143 x64, Qt 6.10.3 `msvc2022_64`, CMake/Ninja, Qt Test, CTest JUnit, PowerShell.

## Global Constraints

- Work only in `E:\workspace\QGC\qgroundcontrol-worktrees\windows-test-hardening` on `codex/windows-test-hardening`.
- Use the complete VS2022 x64 `VsDevCmd.bat` environment, Qt `E:\Qt\6.10.3\msvc2022_64`, and GStreamer `E:\PROGRA~1\GSTREA~1\1.0\MSVC_X~1`.
- Preserve `/Zc:preprocessor`; do not pass manual `CMAKE_CXX_FLAGS`, use MinGW, or use qmake.
- Change only test fixture cleanup and, if justified by measured runtime, the Windows-specific CTest timeout for `InitialConnectTest`.
- Do not modify production LinkManager code, globally raise Integration timeouts, skip rows, whitelist crashes/warnings, or create a PR.
- Full CTest must run to completion with zero failures before downstream branch creation.
- Preserve existing proxy variables and untracked `.tmp/` evidence.

---

### Task 1: Repair and verify the custom board-test teardown

**Files:**
- Modify: `test/Vehicle/InitialConnectTest.cc:62-87`
- Optionally modify after measurement: `test/CMakeLists.txt:347`
- Test: `build/windows-debug/Debug/QGroundControl.exe --unittest:InitialConnectTest --allow-multiple`

**Interfaces:**
- Consumes: `SharedLinkConfigurationPtr linkConfig`, fixture member `_mockLink`, and `VehicleTest::_disconnectMockLink()`.
- Produces: ordered custom-MockLink cleanup through the existing fixture helper.

- [ ] **Step 1: Preserve the approved RED evidence**

The pre-change regression evidence already exists at
`.tmp\phase3-plan\_boardVendorProductId.*`: CTest code 8 after 13.60 seconds,
Windows exception `0xc0000005`, and a stack through
`LinkManager::_linkDisconnected`, `MockLink::~MockLink`, and
`LinkManager::~LinkManager`. Confirm those files still exist before editing.

- [ ] **Step 2: Apply the minimal fixture cleanup change**

Replace the unchecked create call with a checked call and fixture assignment:

```cpp
    QVERIFY(LinkManager::instance()->createConnectedLink(linkConfig));
    _mockLink = qobject_cast<MockLink*>(linkConfig->link());
    QVERIFY(_mockLink);
```

Keep the board vendor/product assertions unchanged. Replace:

```cpp
    LinkManager::instance()->disconnectAll();
    QSignalSpy vehicleRemovedSpy{mvm, &MultiVehicleManager::activeVehicleChanged};
    QVERIFY_SIGNAL_WAIT(vehicleRemovedSpy, TestTimeout::mediumMs());
```

with:

```cpp
    _disconnectMockLink();
```

- [ ] **Step 3: Build the focused target with VS2022 x64**

```powershell
Set-Location 'E:\workspace\QGC\qgroundcontrol-worktrees\windows-test-hardening'
New-Item -ItemType Directory -Force '.tmp\phase4' | Out-Null
$vsDevCmd = 'D:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat'
$vsEnvironment = & cmd.exe /d /s /c "call `"$vsDevCmd`" -arch=x64 -host_arch=x64 >nul && set"
foreach ($entry in $vsEnvironment) {
    if ($entry -match '^([^=]+)=(.*)$') {
        [Environment]::SetEnvironmentVariable($matches[1], $matches[2], 'Process')
    }
}
$env:Path = 'E:\Qt\6.10.3\msvc2022_64\bin;E:\PROGRA~1\GSTREA~1\1.0\MSVC_X~1\bin;' + $env:Path
cmake --build build\windows-debug --target QGroundControl --parallel 2 *> '.tmp\phase4\InitialConnect-fix-build.log'
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
rg -n 'InitialConnectTest\.cc\.obj|Linking CXX executable.*QGroundControl' '.tmp\phase4\InitialConnect-fix-build.log'
```

Expected: exit 0 and `InitialConnectTest.cc.obj` is rebuilt before linking.

- [ ] **Step 4: Probe the full suite with a reversible 360-second generated timeout**

```powershell
$testFile = 'build\windows-debug\test\CTestTestfile.cmake'
$backup = '.tmp\phase4\CTestTestfile.cmake.before-initial-probe'
Copy-Item -LiteralPath $testFile -Destination $backup -Force
$changed = 0
$updated = foreach ($line in Get-Content -LiteralPath $testFile) {
    if ($line -match 'set_tests_properties\(\[=\[InitialConnectTest\]=\].*TIMEOUT "120"') {
        $changed++
        $line -replace 'TIMEOUT "120"', 'TIMEOUT "360"'
    } else {
        $line
    }
}
if ($changed -ne 1) { throw "Expected one InitialConnectTest timeout, changed $changed." }
Set-Content -LiteralPath $testFile -Value $updated
ctest --test-dir build\windows-debug -R '^InitialConnectTest$' --output-on-failure `
  --output-junit "$PWD\.tmp\phase4\InitialConnectTest-360.xml" `
  *> '.tmp\phase4\InitialConnectTest-360.log'
$probeExit = $LASTEXITCODE
Copy-Item -LiteralPath $backup -Destination $testFile -Force
"INITIAL_PROBE_EXIT=$probeExit"
Get-Content '.tmp\phase4\InitialConnectTest-360.log' -Tail 18
```

Expected: no crash and CTest exit 0. If the suite still times out at 360
seconds, stop and write a new root-cause design; do not increase it again.

- [ ] **Step 5: Select and verify the smallest justified Windows timeout**

If the probe passes within the current 120-second default, keep the existing
registration. If it passes above 120 seconds, use the smallest existing scoped
timeout that exceeds the measured duration; prefer `QGC_TEST_TIMEOUT_EXTENDED`
when sufficient. If no existing value provides margin, stop for design review
instead of inventing a value during execution.

Regenerate CTest and run:

```powershell
& 'E:\Qt\6.10.3\msvc2022_64\bin\qt-cmake.bat' -S . -B build\windows-debug -G Ninja -DCMAKE_BUILD_TYPE=Debug *> '.tmp\phase4\InitialConnect-configure.log'
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
ctest --test-dir build\windows-debug -R '^InitialConnectTest$' --output-on-failure `
  --output-junit "$PWD\.tmp\phase4\InitialConnectTest-green.xml" `
  *> '.tmp\phase4\InitialConnectTest-green.log'
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
```

Expected: 1/1 passed and no JUnit failure/error node.

- [ ] **Step 6: Commit the focused fix and evidence**

Update `ENVIRONMENT_REPORT.md` and all three `state/` files with the build
exit, probe duration, selected timeout decision, and focused CTest result.

```powershell
git diff --check
git add -- test/Vehicle/InitialConnectTest.cc test/CMakeLists.txt ENVIRONMENT_REPORT.md state/README.md state/TODO.md state/LOG.md
git commit -m 'test[vehicle]: stabilize board metadata teardown'
git push origin codex/windows-test-hardening
```

Expected: no generated CTest file, temporary selector code, or `.tmp` artifact
is tracked.

---

### Task 2: Complete full CTest and create downstream branches

**Files:**
- Modify: `ENVIRONMENT_REPORT.md`
- Modify: `state/README.md`
- Modify: `state/TODO.md`
- Modify: `state/LOG.md`
- Create temporarily: `.tmp\phase4\run-full-ctest.ps1`

**Interfaces:**
- Consumes: the focused green build and CTest registration from Task 1.
- Produces: one verified baseline commit and two remote branches at that commit.

- [ ] **Step 1: Run full CTest through a background wrapper**

Create `.tmp\phase4\run-full-ctest.ps1` with `apply_patch` containing:

```powershell
$ErrorActionPreference = 'Stop'
Set-Location 'E:\workspace\QGC\qgroundcontrol-worktrees\windows-test-hardening'
$env:Path = 'E:\Qt\6.10.3\msvc2022_64\bin;E:\PROGRA~1\GSTREA~1\1.0\MSVC_X~1\bin;' + $env:Path
$env:QGC_TEST_VERBOSE = '1'
ctest --test-dir build\windows-debug -C Debug --output-on-failure `
  --output-junit "$PWD\.tmp\phase4\ctest-full.xml" `
  *> '.tmp\phase4\ctest-full.log'
$LASTEXITCODE | Set-Content '.tmp\phase4\ctest-full.exit'
exit $LASTEXITCODE
```

Start it with `Start-Process pwsh -ArgumentList '-NoProfile','-File',...` and
poll the process and exit file until it ends. Do not terminate it at the
controller's 30-minute command limit.

Expected: 186/186 passed, exit file `0`, and no JUnit failure/error node. If
any test fails, stop before records/branch creation and report that failure.

- [ ] **Step 2: Commit the verified baseline records**

Update the report and state files with the complete CTest count, elapsed time,
JUnit path, console scan, and verified HEAD. Remove the temporary wrapper.

```powershell
git diff --check
git add -- ENVIRONMENT_REPORT.md state/README.md state/TODO.md state/LOG.md
git commit -m 'docs[windows]: record verified development baseline'
git push origin codex/windows-test-hardening
```

Expected: the hardening branch is clean except `.tmp/`, and its remote points
to the verified records commit.

- [ ] **Step 3: Create and push both downstream branches**

```powershell
git fetch origin --prune
$verifiedCommit = git rev-parse HEAD
git switch -c codex/joystick-aux-px4-development
git push -u origin codex/joystick-aux-px4-development
$version = (git describe --tags --always).Trim() -replace '[^0-9A-Za-z._-]', '_'
$finalBranch = "change1_crocodile_$version"
git switch -c $finalBranch
git push -u origin $finalBranch
git ls-remote --heads origin codex/joystick-aux-px4 codex/joystick-aux-px4-development $finalBranch
"VERIFIED_COMMIT=$verifiedCommit"
"FINAL_BRANCH=$finalBranch"
```

Expected: both new refs point to `$verifiedCommit`; the original
`codex/joystick-aux-px4` remains unchanged. No force push and no PR.
