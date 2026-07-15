# Windows Mission Editor Font Environment Phase 2C Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Configure Windows CTest entries with the operating-system font directory and give `MissionCommandTreeEditorTest` a Windows-specific timeout that covers its verified runtime.

**Architecture:** Resolve `%WINDIR%\Fonts` once when `QGCTest.cmake` is loaded, normalize it to a CMake path, and append it to each Windows test's existing environment. Keep Mission source, test assertions, translation data, and application runtime untouched.

**Tech Stack:** CMake, CTest, Qt 6.10.3, Ninja, MSVC 2022 v143 Hostx64/x64.

## Global Constraints

- Work only in `E:\workspace\QGC\qgroundcontrol-worktrees\windows-test-hardening` on `codex/windows-test-hardening`.
- Use VS2022 Community v143 Hostx64/x64, Qt `E:\Qt\6.10.3\msvc2022_64`, and GStreamer `E:\PROGRA~1\GSTREA~1\1.0\MSVC_X~1`.
- Modify only `cmake/QGCTest.cmake` and `test/CMakeLists.txt` for the implementation.
- Derive the font directory from configure-time `ENV{WINDIR}` and normalize it with `file(TO_CMAKE_PATH)`.
- Resolve and validate the Windows font directory once at module scope; do not emit one warning per registered test.
- Set only the Windows `MissionCommandTreeEditorTest` timeout to 360 seconds; do not increase global or unrelated test timeouts.
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
- Modify: `test/CMakeLists.txt:184`
- Test: registered CTest entry `MissionCommandTreeEditorTest`

**Interfaces:**
- Consumes: configure-time `ENV{WINDIR}` and the existing `_test_env` list.
- Produces: module-scope `_qgc_test_windows_font_dir`, a Windows-only `QT_QPA_FONTDIR=<normalized path>` CTest environment entry, and a Windows-only 360-second editor-test timeout.

- [x] **Step 1: Confirm preserved RED and hypothesis evidence**

Parse the existing files:

- `.tmp/phase2c/editor-baseline-MissionCommandTreeEditorTest.xml`: 3 tests, 1 failure, missing-font warning present.
- `.tmp/phase2c/editor-fontdir-MissionCommandTreeEditorTest.xml`: 3 tests, 0 failures/errors/skips, missing-font warning absent.

- [x] **Step 2: Resolve the Windows font directory once**

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

- [x] **Step 3: Append the validated directory to each Windows test environment**

Immediately after the existing `_test_env` initialization inside `add_qgc_test`, add:

```cmake
    if(WIN32 AND IS_DIRECTORY "${_qgc_test_windows_font_dir}")
        list(APPEND _test_env "QT_QPA_FONTDIR=${_qgc_test_windows_font_dir}")
    endif()
```

Do not change the existing environment entries.

- [x] **Step 4: Give only the Windows editor test a 360-second timeout**

Immediately before the existing `MissionCommandTreeEditorTest` registration in `test/CMakeLists.txt`, add:

```cmake
set(_mission_command_tree_editor_timeout ${QGC_TEST_TIMEOUT_EXTENDED})
if(WIN32)
    set(_mission_command_tree_editor_timeout 360)
endif()
```

Change only that registration to use `TIMEOUT ${_mission_command_tree_editor_timeout}`. Do not modify global timeout variables or other test registrations.

- [x] **Step 5: Reconfigure and inspect the generated CTest properties**

Run the Common Configure Command, then:

```powershell
ctest --test-dir build/windows-debug -C Debug --show-only=json-v1 > .tmp/phase2c/ctest-show-only.json
@'
import json
from pathlib import Path

data = json.loads(Path('.tmp/phase2c/ctest-show-only.json').read_text(encoding='utf-8-sig'))
test = next(item for item in data['tests'] if item['name'] == 'MissionCommandTreeEditorTest')
environment = next(prop['value'] for prop in test['properties'] if prop['name'] == 'ENVIRONMENT')
timeout = next(prop['value'] for prop in test['properties'] if prop['name'] == 'TIMEOUT')
print(environment)
print(f'TIMEOUT={timeout}')
if not any(value.replace('\\', '/').lower().endswith('/windows/fonts') and value.startswith('QT_QPA_FONTDIR=') for value in environment):
    raise SystemExit('QT_QPA_FONTDIR is missing from MissionCommandTreeEditorTest')
if float(timeout) != 360:
    raise SystemExit(f'Unexpected MissionCommandTreeEditorTest timeout: {timeout}')
'@ | python - > .tmp/phase2c/ctest-environment-check.txt
```

Expected: configure exit 0; the environment contains normalized `QT_QPA_FONTDIR`; the editor test timeout is 360 seconds.

- [x] **Step 6: Run the registered CTest entry**

Run with output redirected because the suite takes about 200 seconds:

```powershell
ctest --test-dir build/windows-debug -C Debug -R '^MissionCommandTreeEditorTest$' --output-on-failure `
    *> .tmp/phase2c/ctest-editor.log
$LASTEXITCODE | Set-Content .tmp/phase2c/ctest-editor.exit.txt
```

Expected: exit 0 and `100% tests passed, 0 tests failed out of 1`.

- [x] **Step 7: Reuse and verify the fresh direct JUnit GREEN evidence**

Do not repeat the 200-second direct run after the timeout-only change. Parse the already generated files:

```text
.tmp/phase2c/editor-after-cmake-MissionCommandTreeEditorTest.xml
.tmp/phase2c/editor-after-cmake.exit.txt
.tmp/phase2c/editor-after-cmake-check.txt
```

Expected: process exit 0; actual JUnit file reports 3 tests, 0 failures/errors/skips; no QFontDatabase missing-font warning or uncategorized default-category message.

- [x] **Step 8: Review and commit the timeout change**

Run:

```powershell
git diff --check
git diff -- test/CMakeLists.txt
git add test/CMakeLists.txt
git commit -m "test[mission]: extend Windows editor timeout"
```

Expected: the existing font-environment commit remains focused and the new timeout commit contains only `test/CMakeLists.txt`; no Mission source, test-source, or translation file is staged.

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

- [x] **Step 1: Parse and scan final evidence**

Use Python `xml.etree.ElementTree` to parse the direct JUnit XML. Scan CTest/direct logs for `QFontDatabase`, `Cannot find font directory`, uncategorized default-category warnings, missing DLL/plugin errors, fatal assertions, and crashes. Store summaries below `.tmp/phase2c/` and display at most 30 lines.

Expected: CTest 1/1 passed; direct JUnit 3/0/0/0; no font/default-category failure message.

- [x] **Step 2: Update report and state**

Append a Phase 2C section to `ENVIRONMENT_REPORT.md` with the exact CMake commit, normalized font directory, configure result, CTest result/duration, direct JUnit totals/duration, and log scan. State explicitly that Mission source/tests/translations were unchanged and `MissionManagerTest` remains RED for Phase 2D.

Update the three state files with measured evidence. Keep broader Phase 2 and the separate runner investigations open.

- [x] **Step 3: Complete verification and independent review**

Use `superpowers:verification-before-completion`, obtain task-scoped and whole-Phase-2C reviews, and resolve every Critical or Important finding. Do not rerun the 200-second suite for documentation-only changes.

- [x] **Step 4: Commit documentation and push**

Run:

```powershell
git diff --check
git add ENVIRONMENT_REPORT.md state/README.md state/TODO.md state/LOG.md `
    docs/superpowers/plans/2026-07-14-windows-mission-editor-font-environment-phase2c.md
git commit -m "docs[mission-tests]: record Phase 2C verification"
git push origin codex/windows-test-hardening
```

Expected: local and remote test-hardening branches match; `codex/joystick-aux-px4` is unchanged; only `.tmp/` remains untracked.
