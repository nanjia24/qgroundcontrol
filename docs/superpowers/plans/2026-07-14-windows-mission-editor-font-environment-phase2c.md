# Windows Mission Editor Font Environment Phase 2C Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Configure all Windows `add_qgc_test` CTest entries with the operating-system font directory so offscreen Qt does not emit an uncategorized missing-font warning.

**Architecture:** Resolve `%WINDIR%\Fonts` once when `QGCTest.cmake` is loaded, normalize it to a CMake path, and append it to each Windows test's existing environment. Keep Mission source, test assertions, translation data, and application runtime untouched.

**Tech Stack:** CMake, CTest, Qt 6.10.3, Ninja, MSVC 2022 v143 Hostx64/x64.

## Global Constraints

- Work only in `E:\workspace\QGC\qgroundcontrol-worktrees\windows-test-hardening` on `codex/windows-test-hardening`.
- Use VS2022 Community v143 Hostx64/x64, Qt `E:\Qt\6.10.3\msvc2022_64`, and GStreamer `E:\PROGRA~1\GSTREA~1\1.0\MSVC_X~1`.
- Modify only `cmake/QGCTest.cmake` for the implementation.
- Derive the font directory from configure-time `ENV{WINDIR}` and normalize it with `file(TO_CMAKE_PATH)`.
- Resolve and validate the Windows font directory once at module scope; do not emit one warning per registered test.
- Do not hard-code `C:\Windows`, modify Mission source/tests/translations, whitelist the font warning, skip, retry, delay, or suppress unrelated warnings.
- Preserve the existing `QT_QPA_PLATFORM=offscreen` and `QT_LOGGING_RULES=*.debug=false` entries.
- Do not run full CTest and do not use `-- -v2`.
- Use `--unittest-output`, `--log-output`, `--logging`, and `QGC_TEST_VERBOSE=1` for the direct JUnit run.
- Redirect configure/test output to `.tmp/phase2c/`; keep build products, XML, logs, and `.tmp/` untracked.
- Subagents must treat `state/README.md`, `state/TODO.md`, and `state/LOG.md` as read-only.
- Use structured commits and push only `codex/windows-test-hardening`; no force-push and no PR creation.
- No GitHub issue or PR number is known; do not fabricate a `Fixes` or `Resolves` footer.

## Common Configure Command

Create `.tmp/phase2c/configure.cmd` with:

```cmd
@call "D:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64
@set "PATH=E:\Qt\6.10.3\msvc2022_64\bin;E:\PROGRA~1\GSTREA~1\1.0\MSVC_X~1\bin;%PATH%"
@set "GSTREAMER_1_0_ROOT_MSVC_X86_64=E:\PROGRA~1\GSTREA~1\1.0\MSVC_X~1"
@call "E:\Qt\6.10.3\msvc2022_64\bin\qt-cmake.bat" -S . -B build\windows-debug -G Ninja -DCMAKE_BUILD_TYPE=Debug
```

Run:

```powershell
cmd.exe /d /c .tmp\phase2c\configure.cmd *> .tmp\phase2c\configure.log
if ($LASTEXITCODE -ne 0) { Get-Content .tmp\phase2c\configure.log -Tail 40; exit $LASTEXITCODE }
```

## Direct Test Environment

```powershell
$env:PATH = "E:\Qt\6.10.3\msvc2022_64\bin;E:\PROGRA~1\GSTREA~1\1.0\MSVC_X~1\bin;$env:PATH"
$env:GSTREAMER_1_0_ROOT_MSVC_X86_64 = 'E:\PROGRA~1\GSTREA~1\1.0\MSVC_X~1'
$env:QT_PLUGIN_PATH = 'E:\Qt\6.10.3\msvc2022_64\plugins'
$env:QT_QPA_PLATFORM = 'offscreen'
$env:QT_QPA_FONTDIR = "$env:WINDIR\Fonts"
$env:QGC_TEST_VERBOSE = '1'
$env:QT_LOGGING_RULES = '*.debug=false'
```

---

### Task 1: Configure Windows offscreen fonts for CTest

**Files:**
- Modify: `cmake/QGCTest.cmake:20-28,136-145`
- Test: registered CTest entry `MissionCommandTreeEditorTest`

**Interfaces:**
- Consumes: configure-time `ENV{WINDIR}` and the existing `_test_env` list.
- Produces: module-scope `_qgc_test_windows_font_dir` and a Windows-only `QT_QPA_FONTDIR=<normalized path>` CTest environment entry.

- [ ] **Step 1: Confirm preserved RED and hypothesis evidence**

Parse the existing files:

- `.tmp/phase2c/editor-baseline-MissionCommandTreeEditorTest.xml`: 3 tests, 1 failure, missing-font warning present.
- `.tmp/phase2c/editor-fontdir-MissionCommandTreeEditorTest.xml`: 3 tests, 0 failures/errors/skips, missing-font warning absent.

- [ ] **Step 2: Resolve the Windows font directory once**

After the timeout configuration in `cmake/QGCTest.cmake`, add:

```cmake
set(_qgc_test_windows_font_dir "")
if(WIN32)
    if(DEFINED ENV{WINDIR})
        file(TO_CMAKE_PATH "$ENV{WINDIR}/Fonts" _qgc_test_windows_font_dir)
    endif()

    if(NOT IS_DIRECTORY "${_qgc_test_windows_font_dir}")
        message(WARNING "Windows test font directory was not found through WINDIR; offscreen Qt tests may emit QFontDatabase warnings")
    endif()
endif()
```

- [ ] **Step 3: Append the validated directory to each Windows test environment**

Immediately after the existing `_test_env` initialization inside `add_qgc_test`, add:

```cmake
    if(WIN32 AND IS_DIRECTORY "${_qgc_test_windows_font_dir}")
        list(APPEND _test_env "QT_QPA_FONTDIR=${_qgc_test_windows_font_dir}")
    endif()
```

Do not change the existing environment entries.

- [ ] **Step 4: Reconfigure and inspect the generated CTest property**

Run the Common Configure Command, then:

```powershell
ctest --test-dir build/windows-debug -C Debug --show-only=json-v1 > .tmp/phase2c/ctest-show-only.json
@'
import json
from pathlib import Path

data = json.loads(Path('.tmp/phase2c/ctest-show-only.json').read_text(encoding='utf-8-sig'))
test = next(item for item in data['tests'] if item['name'] == 'MissionCommandTreeEditorTest')
environment = next(prop['value'] for prop in test['properties'] if prop['name'] == 'ENVIRONMENT')
print(environment)
if not any(value.replace('\\', '/').lower().endswith('/windows/fonts') and value.startswith('QT_QPA_FONTDIR=') for value in environment):
    raise SystemExit('QT_QPA_FONTDIR is missing from MissionCommandTreeEditorTest')
'@ | python - > .tmp/phase2c/ctest-environment-check.txt
```

Expected: configure exit 0 and the test environment contains normalized `QT_QPA_FONTDIR` pointing to the current `%WINDIR%\Fonts` directory.

- [ ] **Step 5: Run the registered CTest entry**

Run with output redirected because the suite takes about 200 seconds:

```powershell
ctest --test-dir build/windows-debug -C Debug -R '^MissionCommandTreeEditorTest$' --output-on-failure `
    *> .tmp/phase2c/ctest-editor.log
$LASTEXITCODE | Set-Content .tmp/phase2c/ctest-editor.exit.txt
```

Expected: exit 0 and `100% tests passed, 0 tests failed out of 1`.

- [ ] **Step 6: Run the direct JUnit entry with the same font environment**

After applying the Direct Test Environment, run:

```powershell
$arguments = @(
    '--unittest:MissionCommandTreeEditorTest',
    '--allow-multiple',
    '--unittest-output:.tmp/phase2c/editor-after-cmake.xml',
    '--log-output',
    '--logging:*.debug=true',
    '--no-windows-assert-ui'
)
$process = Start-Process -FilePath 'build\windows-debug\Debug\QGroundControl.exe' `
    -ArgumentList $arguments `
    -RedirectStandardOutput '.tmp\phase2c\editor-after-cmake.stdout.log' `
    -RedirectStandardError '.tmp\phase2c\editor-after-cmake.console.log' `
    -PassThru -Wait -WindowStyle Hidden
$process.ExitCode | Set-Content '.tmp\phase2c\editor-after-cmake.exit.txt'
```

Expected: process exit 0; actual JUnit file reports 3 tests, 0 failures/errors/skips; no QFontDatabase missing-font warning or uncategorized default-category message.

- [ ] **Step 7: Review and commit the one-file CMake change**

Run:

```powershell
git diff --check
git diff -- cmake/QGCTest.cmake
git add cmake/QGCTest.cmake
git commit -m "test[cmake]: configure Windows offscreen fonts"
```

Expected: one focused CMake test-infrastructure commit; no Mission, test-source, or translation file is staged.

---

### Task 2: Record Phase 2C verification and preserve Phase 2D

**Files:**
- Modify: `ENVIRONMENT_REPORT.md`
- Modify: `state/README.md`
- Modify: `state/TODO.md`
- Modify: `state/LOG.md`
- Modify: `docs/superpowers/plans/2026-07-14-windows-mission-editor-font-environment-phase2c.md`

**Interfaces:**
- Consumes: the reviewed Task 1 CMake commit and CTest/direct JUnit evidence.
- Produces: Phase 2C environment documentation while leaving the Chinese translation placeholder defect open for Phase 2D.

- [ ] **Step 1: Parse and scan final evidence**

Use Python `xml.etree.ElementTree` to parse the direct JUnit XML. Scan CTest/direct logs for `QFontDatabase`, `Cannot find font directory`, uncategorized default-category warnings, missing DLL/plugin errors, fatal assertions, and crashes. Store summaries below `.tmp/phase2c/` and display at most 30 lines.

Expected: CTest 1/1 passed; direct JUnit 3/0/0/0; no font/default-category failure message.

- [ ] **Step 2: Update report and state**

Append a Phase 2C section to `ENVIRONMENT_REPORT.md` with the exact CMake commit, normalized font directory, configure result, CTest result/duration, direct JUnit totals/duration, and log scan. State explicitly that Mission source/tests/translations were unchanged and `MissionManagerTest` remains RED for Phase 2D.

Update the three state files with measured evidence. Keep broader Phase 2 and the separate runner investigations open.

- [ ] **Step 3: Complete verification and independent review**

Use `superpowers:verification-before-completion`, obtain task-scoped and whole-Phase-2C reviews, and resolve every Critical or Important finding. Do not rerun the 200-second suite for documentation-only changes.

- [ ] **Step 4: Commit documentation and push**

Run:

```powershell
git diff --check
git add ENVIRONMENT_REPORT.md state/README.md state/TODO.md state/LOG.md `
    docs/superpowers/plans/2026-07-14-windows-mission-editor-font-environment-phase2c.md
git commit -m "docs[mission-tests]: record Phase 2C verification"
git push origin codex/windows-test-hardening
```

Expected: local and remote test-hardening branches match; `codex/joystick-aux-px4` is unchanged; only `.tmp/` remains untracked.
