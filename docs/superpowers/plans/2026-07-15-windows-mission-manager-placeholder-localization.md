# Windows Mission Manager Placeholder Localization Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Restore the missing Qt placeholder in the Simplified Chinese frame diagnostic and prove `MissionManagerTest` passes in the Windows Debug environment.

**Architecture:** The defect is catalogue data, not Mission Manager logic. The implementation changes one existing translation value; the existing test exercises the unchanged `PlanManager::tr("Frame: %1").arg(...)` call after CMake regenerates the embedded translation resource.

**Tech Stack:** Qt 6.10.3 `msvc2022_64`, CMake, Ninja, MSVC v143 x64, Qt Test JUnit output.

## Global Constraints

- Work only in `E:\workspace\QGC\qgroundcontrol-worktrees\windows-test-hardening` on `codex/windows-test-hardening`.
- Use the VS2022 v143 x64 environment, Qt `E:\Qt\6.10.3\msvc2022_64`, and the validated short GStreamer path `E:\PROGRA~1\GSTREA~1\1.0\MSVC_X~1`.
- Do not pass manual `-DCMAKE_CXX_FLAGS`; `/Zc:preprocessor` is already configured in `cmake/platform/Windows.cmake`.
- Do not alter Mission production code, tests, test whitelists, CTest timeouts, or locale variables.
- Do not run full CTest. Capture the focused test with `QGC_TEST_VERBOSE=1`, `--unittest-output`, `--log-output`, and `--logging`; do not use `-- -v2`.
- Preserve the pre-existing untracked `.tmp/` directory and commit only Phase 2D changes.

---

### Task 1: Correct the Simplified Chinese frame placeholder and verify the existing regression

**Files:**
- Modify: `translations/qgc_source_zh_CN.ts:12560-12562`
- Modify: `ENVIRONMENT_REPORT.md`
- Modify: `state/README.md`
- Modify: `state/TODO.md`
- Modify: `state/LOG.md`
- Test: `build/windows-debug/Debug/QGroundControl.exe --unittest:MissionManagerTest --allow-multiple`

**Interfaces:**
- Consumes: `PlanManager.cc` source key `Frame: %1` and its existing `.arg(item->frame())` argument contract.
- Produces: the `zh_CN` translation `框架：%1`, which Qt substitutes with the numeric MAVLink frame value.

- [ ] **Step 1: Reproduce the existing failure and retain focused evidence**

Run from the VS2022 x64 developer environment:

```powershell
$env:Path = 'E:\Qt\6.10.3\msvc2022_64\bin;E:\PROGRA~1\GSTREA~1\1.0\MSVC_X~1\bin;' + $env:Path
$env:QGC_TEST_VERBOSE = '1'
New-Item -ItemType Directory -Force .tmp\phase2d | Out-Null
& .\build\windows-debug\Debug\QGroundControl.exe `
  --unittest:MissionManagerTest --allow-multiple `
  --unittest-output:.tmp\phase2d\MissionManagerTest-red.xml `
  --log-output --logging:default *> .tmp\phase2d\MissionManagerTest-red.log
$exitCode = $LASTEXITCODE
if ($exitCode -eq 0) { throw 'Expected the pre-fix MissionManagerTest to fail.' }
rg -n 'Argument missing|_testErrorAckFailureStrings|FAIL' .tmp\phase2d\MissionManagerTest-red.log
exit 0
```

Expected: a non-zero test exit before the final `exit 0`, a JUnit failure in
`MissionManagerTest-red-*.xml`, and a log entry containing
`QString::arg: Argument missing` from `_testErrorAckFailureStrings`.

- [ ] **Step 2: Apply the smallest catalogue-only correction**

In the message whose source is `Frame: %1`, replace exactly:

```xml
<translation>框架1</translation>
```

with:

```xml
<translation>框架：%1</translation>
```

Do not change the source key, neighbouring messages, or any C++/test file.

- [ ] **Step 3: Rebuild the affected Debug target and confirm the translation resource is regenerated**

```powershell
$env:Path = 'E:\Qt\6.10.3\msvc2022_64\bin;E:\PROGRA~1\GSTREA~1\1.0\MSVC_X~1\bin;' + $env:Path
cmake --build .\build\windows-debug --target QGroundControl --parallel *> .tmp\phase2d\MissionManagerTest-build.log
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
rg -n 'qgc_source_zh_CN|lrelease|MissionManagerTest|QGroundControl' .tmp\phase2d\MissionManagerTest-build.log
```

Expected: build exits 0 and its log shows the relevant resource/target work;
if the log does not establish it, inspect the generated target dependency and
perform a normal CMake regeneration rather than deleting unrelated artifacts.

- [ ] **Step 4: Run the focused GREEN regression with JUnit and console evidence**

```powershell
$env:Path = 'E:\Qt\6.10.3\msvc2022_64\bin;E:\PROGRA~1\GSTREA~1\1.0\MSVC_X~1\bin;' + $env:Path
$env:QGC_TEST_VERBOSE = '1'
& .\build\windows-debug\Debug\QGroundControl.exe `
  --unittest:MissionManagerTest --allow-multiple `
  --unittest-output:.tmp\phase2d\MissionManagerTest-green.xml `
  --log-output --logging:default *> .tmp\phase2d\MissionManagerTest-green.log
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
rg -n 'Argument missing|FAIL|QWARN|QFATAL' .tmp\phase2d\MissionManagerTest-green.log
if ($LASTEXITCODE -eq 0) { throw 'Unexpected diagnostic match in GREEN log.' }
Get-ChildItem .tmp\phase2d\MissionManagerTest-green-*.xml | Select-Object -ExpandProperty FullName
```

Expected: test exits 0; generated JUnit XML reports 7 tests, 0 failures, 0
errors, and 0 skips; console scan finds no missing-argument or test-failure
diagnostic.

- [ ] **Step 5: Record evidence and commit the focused correction**

Update `ENVIRONMENT_REPORT.md` and the three `state/` files with the exact
build exit code, JUnit counts, elapsed time, console scan result, and the
fact that the three CTest-runner investigations remain open. Confirm the diff
contains only the one translation entry and delivery records, then commit:

```powershell
git diff --check
git add -- translations/qgc_source_zh_CN.ts ENVIRONMENT_REPORT.md state/README.md state/TODO.md state/LOG.md
git commit -m 'fix[translations]: preserve Mission frame placeholder'
git push origin codex/windows-test-hardening
```

Expected: committed and pushed focused fix; no forced push, no PR creation,
and no changes to `codex/joystick-aux-px4`.
