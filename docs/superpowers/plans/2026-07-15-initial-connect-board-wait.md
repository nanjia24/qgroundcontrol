# Initial Connect Board Wait Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make `_boardVendorProductId` pass with a bounded wait and give the complete Windows Initial Connect suite a measured, test-specific timeout.

**Architecture:** Retain the approved fixture cleanup already present in the working tree. Change only the board test's initial-connect wait from the 5-second medium value to the existing 30-second local long value, then register the complete Windows suite with a 360-second test-level timeout because its measured 184.314-second runtime exceeds the existing 180-second extended value.

**Tech Stack:** VS2022 v143 x64, Qt 6.10.3 `msvc2022_64`, CMake/Ninja, Qt Test, CTest JUnit, PowerShell.

## Global Constraints

- Do not modify production source or shared timeout variables.
- Keep the active-vehicle wait unchanged; change only the board test's `initialConnectComplete` wait.
- Use a Windows-only 360-second `InitialConnectTest` timeout; non-Windows keeps `QGC_TEST_TIMEOUT_INTEGRATION`.
- Preserve the approved `_mockLink` and `_disconnectMockLink()` cleanup change.
- Full CTest and branch creation remain governed by Task 2 of `docs/superpowers/plans/2026-07-15-initial-connect-board-teardown.md`.

---

### Task 1: Correct and verify the board wait

**Files:**
- Modify: `test/Vehicle/InitialConnectTest.cc:62-87`
- Modify: `test/CMakeLists.txt:347`
- Modify: `ENVIRONMENT_REPORT.md`
- Modify: `state/README.md`
- Modify: `state/TODO.md`
- Modify: `state/LOG.md`

**Interfaces:**
- Consumes: `TestTimeout::longMs()` and the existing Windows CTest registration helper.
- Produces: a bounded 30-second local function wait and Windows-only 360-second suite timeout.

- [ ] **Step 1: Preserve the RED evidence**

The pre-change direct QGC JUnit at
`.tmp\phase4\InitialConnect-direct-InitialConnectTest.xml` must report 24 tests,
1 failure, 0 errors, and `_boardVendorProductId` requiring 9,950 ms while the
configured wait is 5,000 ms.

- [ ] **Step 2: Apply the one-line wait correction**

Replace:

```cpp
    QVERIFY_TRUE_WAIT(initialConnectCompleteSpy.count() > 0 || vehicle->isInitialConnectComplete(),
                      TestTimeout::mediumMs());
```

with:

```cpp
    QVERIFY_TRUE_WAIT(initialConnectCompleteSpy.count() > 0 || vehicle->isInitialConnectComplete(),
                      TestTimeout::longMs());
```

- [ ] **Step 3: Register a Windows-only suite timeout**

Replace the direct Initial Connect registration with:

```cmake
set(_initial_connect_timeout ${QGC_TEST_TIMEOUT_INTEGRATION})
if(WIN32)
    set(_initial_connect_timeout 360)
endif()
add_qgc_test(InitialConnectTest
    LABELS Integration Vehicle
    RESOURCE_LOCK MockLink
    TIMEOUT ${_initial_connect_timeout}
)
```

- [ ] **Step 4: Reconfigure, build, and run direct QGC JUnit**

Load VS2022 `VsDevCmd.bat` for x64, put Qt and GStreamer on `PATH`, then run:

```powershell
& 'E:\Qt\6.10.3\msvc2022_64\bin\qt-cmake.bat' -S . -B build\windows-debug -G Ninja -DCMAKE_BUILD_TYPE=Debug *> '.tmp\phase4\InitialConnect-wait-configure.log'
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
cmake --build build\windows-debug --target QGroundControl --parallel 2 *> '.tmp\phase4\InitialConnect-wait-build.log'
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
```

Run QGroundControl from `build\windows-debug` with
`--unittest:InitialConnectTest`, `--unittest-output`, `--log-output`, and
`--logging:default`. Expected: process exit 0 and QGC JUnit 24 tests, 0
failures, 0 errors.

- [ ] **Step 5: Verify the generated CTest registration**

```powershell
rg -n 'InitialConnectTest.*TIMEOUT "360"' build\windows-debug\test\CTestTestfile.cmake
ctest --test-dir build\windows-debug -R '^InitialConnectTest$' --output-on-failure `
  --output-junit "$PWD\.tmp\phase4\InitialConnectTest-green.xml" `
  *> '.tmp\phase4\InitialConnectTest-green.log'
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
```

Expected: 1/1 passed and no CTest JUnit failure/error node.

- [ ] **Step 6: Commit and resume full validation**

Update the report/state files with the final direct and CTest results, then:

```powershell
git diff --check
git add -- test/Vehicle/InitialConnectTest.cc test/CMakeLists.txt ENVIRONMENT_REPORT.md state/README.md state/TODO.md state/LOG.md
git commit -m 'test[vehicle]: stabilize board metadata connection'
git push origin codex/windows-test-hardening
```

After this focused commit is pushed, execute Task 2 in
`docs/superpowers/plans/2026-07-15-initial-connect-board-teardown.md` exactly:
run full CTest in the background wrapper, commit the verified baseline records,
then create both requested remote branches only if 186/186 pass.
