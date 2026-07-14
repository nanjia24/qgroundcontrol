# Windows Compression Hardening Phase 2A Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make `QGCCompressionTest` correctly distinguish Windows directory-symlink capability from failure and preserve all existing Unicode extraction output paths.

**Architecture:** Keep the real-directory-symlink capability probe in the shared test fixture, using the Windows wide filesystem API only under `Q_OS_WIN`. Fix Unicode at the final libarchive output-entry boundary with `archive_entry_copy_pathname_w`; retain the existing narrow API on non-Windows platforms and leave archive input handling unchanged.

**Tech Stack:** C++20, Qt 6.10.3 QtTest, libarchive, `std::filesystem`, CMake, Ninja, MSVC 2022 v143 Hostx64/x64.

## Global Constraints

- Work only in `E:\workspace\QGC\qgroundcontrol-worktrees\windows-test-hardening` on `codex/windows-test-hardening`.
- Use VS2022 Community v143 Hostx64/x64, Qt `E:\Qt\6.10.3\msvc2022_64`, and GStreamer `E:\PROGRA~1\GSTREA~1\1.0\MSVC_X~1`.
- Keep `/Zc:preprocessor` supplied by `cmake/platform/Windows.cmake`; do not pass `CMAKE_CXX_FLAGS` manually.
- Do not enable Developer Mode, request UAC elevation, or replace a real directory symlink with a shortcut or junction.
- Preserve all archive traversal, symlink-target, parent-directory, and extraction-limit checks.
- Change only Unicode output entry paths; do not use `archive_read_open_filename_w` or otherwise alter Unicode archive input handling.
- Keep the Japanese, Chinese, Greek, accented Latin, and Cyrillic path cases unchanged.
- Do not run full CTest. Run only `QGCCompressionTest` and the directly related suites named in Task 3.
- Do not use `-- -v2`; use `--unittest-output`, `--log-output`, `--logging`, and `QGC_TEST_VERBOSE=1`.
- Redirect build/test output to `.tmp/phase2a/`; keep build products, XML, logs, and `.tmp/` untracked.
- Use structured commits and push only `codex/windows-test-hardening`; no force-push and no PR creation.
- No GitHub issue number is known; do not fabricate a `Fixes` or `Resolves` footer.

## Common Commands

Create the evidence directory once:

```powershell
New-Item -ItemType Directory -Force .tmp/phase2a | Out-Null
```

Incrementally build the test-enabled executable through the confirmed toolchain:

```powershell
cmd.exe /d /c 'call "D:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64 >nul && set "PATH=E:\Qt\6.10.3\msvc2022_64\bin;E:\PROGRA~1\GSTREA~1\1.0\MSVC_X~1\bin;%PATH%" && cmake --build build\windows-debug --target QGroundControl --parallel' *> .tmp/phase2a/build.log
if ($LASTEXITCODE -ne 0) { Get-Content .tmp/phase2a/build.log -Tail 40; exit $LASTEXITCODE }
```

Define this session-local helper, then call it with the exact suite and evidence names shown in each task:

```powershell
function Invoke-QgcSuite([string] $SuiteName, [string] $EvidenceName) {
$env:PATH = "E:\Qt\6.10.3\msvc2022_64\bin;E:\PROGRA~1\GSTREA~1\1.0\MSVC_X~1\bin;$env:PATH"
$env:GSTREAMER_1_0_ROOT_MSVC_X86_64 = 'E:\PROGRA~1\GSTREA~1\1.0\MSVC_X~1'
$env:QT_QPA_PLATFORM = 'offscreen'
$env:QGC_TEST_VERBOSE = '1'
$env:QT_LOGGING_RULES = '*.debug=false'
& build/windows-debug/Debug/QGroundControl.exe `
  "--unittest:$SuiteName" `
  --allow-multiple `
  "--unittest-output:.tmp/phase2a/$EvidenceName.xml" `
  --log-output `
  '--logging:*.debug=true' `
  --no-windows-assert-ui *> ".tmp/phase2a/$EvidenceName.console.log"
return $LASTEXITCODE
}
```

The runner appends the Qt object name to the requested XML basename; inspect the generated filename, not the requested basename.

---

### Task 1: Use a real Windows directory-symlink capability probe

**Files:**
- Modify: `test/UnitTestFramework/BaseClasses/TempDirectoryTest.h:3-7,70-87`
- Test: `test/Utilities/Compression/QGCCompressionTest.cc:1225-1239`

**Interfaces:**
- Consumes: `QString targetPath`, `QString linkPath`, and caller-provided `QString skipReason`.
- Produces: unchanged `void TempDirectoryTest::createSymlinkOrSkip(...)`; on Windows it creates a genuine directory symlink or records an evidence-bearing QtTest skip.

- [ ] **Step 1: Record the existing RED result**

Run:

```powershell
Invoke-QgcSuite -SuiteName QGCCompressionTest -EvidenceName QGCCompressionTest.before-symlink
```

Expected: `_testExtractArchiveToSymlinkedOutputPath` fails at `QVERIFY(symlinkInfo.isSymLink())`; `_testUnicodePaths` remains the other failure.

- [ ] **Step 2: Add the Windows-only standard-library includes**

Add these after the Qt includes in `TempDirectoryTest.h`:

```cpp
#ifdef Q_OS_WIN
#include <filesystem>
#include <system_error>
#endif
```

Also add the direct Qt dependency used for verification:

```cpp
#include <QtCore/QFileInfo>
```

- [ ] **Step 3: Replace the Windows branch of `createSymlinkOrSkip`**

Keep the signature unchanged and replace its body with:

```cpp
    {
        (void) QFile::remove(linkPath);
#ifdef Q_OS_WIN
        const std::filesystem::path target(targetPath.toStdWString());
        const std::filesystem::path link(linkPath.toStdWString());
        std::error_code linkError;
        std::filesystem::create_directory_symlink(target, link, linkError);
        if (linkError) {
            const QByteArray message = QStringLiteral("%1: %2 (error %3)")
                                           .arg(skipReason, QString::fromLocal8Bit(linkError.message()))
                                           .arg(linkError.value())
                                           .toLocal8Bit();
            QSKIP(message.constData());
        }

        if (!QFileInfo(linkPath).isSymLink()) {
            std::error_code cleanupError;
            const bool removed = std::filesystem::remove(link, cleanupError);
            if (cleanupError || !removed) {
                const QString cleanupDetail = cleanupError ? QString::fromLocal8Bit(cleanupError.message())
                                                           : QStringLiteral("path was not removed");
                const QByteArray message = QStringLiteral("Directory symlink verification failed and cleanup failed: %1 (error %2)")
                                               .arg(cleanupDetail)
                                               .arg(cleanupError.value())
                                               .toLocal8Bit();
                QFAIL(message.constData());
            }
            const QByteArray message = QStringLiteral("%1: created path was not verified as a real directory symlink")
                                           .arg(skipReason)
                                           .toLocal8Bit();
            QSKIP(message.constData());
        }
#else
        if (!QFile::link(targetPath, linkPath)) {
            QSKIP(qPrintable(skipReason));
        }
#endif
    }
```

- [ ] **Step 4: Build and verify the capability result**

Run the Common Build Command, then:

```powershell
Invoke-QgcSuite -SuiteName QGCCompressionTest -EvidenceName QGCCompressionTest.after-symlink
```

Expected on the current medium-integrity token: `_testExtractArchiveToSymlinkedOutputPath` is an explicit skip containing the Windows system error. If Windows permits symlink creation, the method passes all extraction assertions. It must not fail `isSymLink()` and must not create a `.lnk` or junction. `_testUnicodePaths` remains RED.

- [ ] **Step 5: Review and commit the focused test-infrastructure change**

Run:

```powershell
git diff --check
git diff -- test/UnitTestFramework/BaseClasses/TempDirectoryTest.h
git add test/UnitTestFramework/BaseClasses/TempDirectoryTest.h
git commit -m "test[compression]: probe Windows directory symlinks"
```

Expected: one focused commit; `.tmp/` remains untracked.

---

### Task 2: Preserve Unicode libarchive output entry paths on Windows

**Files:**
- Modify: `src/Utilities/Compression/QGClibarchive.cc:3-11,470`
- Test: `test/Utilities/Compression/QGCCompressionTest.cc:1264-1289` (existing RED coverage; do not alter its path cases)

**Interfaces:**
- Consumes: the already validated final `QString outputPath` inside `extractArchiveEntries(...)`.
- Produces: a copied Windows wide pathname in the current `archive_entry`; non-Windows behavior is unchanged.

- [ ] **Step 1: Confirm the remaining RED boundary**

Parse `QGCCompressionTest.after-symlink-*.xml` and confirm `_testUnicodePaths` fails because `manifest.json` is missing after extraction reports success. Do not modify archive input opening.

- [ ] **Step 2: Add the explicit standard string include**

Add after the libarchive includes in `QGClibarchive.cc`:

```cpp
#include <string>
```

- [ ] **Step 3: Use the libarchive wide output-path API only on Windows**

Replace the narrow pathname assignment with:

```cpp
#ifdef Q_OS_WIN
        const std::wstring wideOutputPath = outputPath.toStdWString();
        archive_entry_copy_pathname_w(entry, wideOutputPath.c_str());
#else
        archive_entry_set_pathname(entry, outputPath.toUtf8().constData());
#endif
```

Do not change `currentFile`, output canonicalization, traversal checks, symlink checks, permission masking, or reader-opening APIs.

- [ ] **Step 4: Build and verify GREEN compression behavior**

Run the Common Build Command, then:

```powershell
Invoke-QgcSuite -SuiteName QGCCompressionTest -EvidenceName QGCCompressionTest.after-unicode
```

Expected: 51 tests, zero failures, zero errors; `_testUnicodePaths` passes for Japanese, Chinese, Greek, accented Latin, and Cyrillic directories. The symlink test either passes or is the single evidence-bearing capability skip.

- [ ] **Step 5: Review and commit the production boundary fix**

Run:

```powershell
git diff --check
git diff -- src/Utilities/Compression/QGClibarchive.cc
rg -n "archive_read_open_filename_w" src/Utilities/Compression/QGClibarchive.cc
git add src/Utilities/Compression/QGClibarchive.cc
git commit -m "fix[compression]: preserve Unicode output paths on Windows"
```

Expected: `rg` finds no `archive_read_open_filename_w`; the commit contains only the output-path boundary change.

---

### Task 3: Run focused regressions, document evidence, and deliver

**Files:**
- Modify: `ENVIRONMENT_REPORT.md`
- Modify: `state/TODO.md`
- Modify: `state/LOG.md`
- Read-only verification: `test/CMakeLists.txt:238-254`

**Interfaces:**
- Consumes: the two reviewed implementation commits and `.tmp/phase2a` JUnit/console/build evidence.
- Produces: a focused regression record and an updated Phase 2 status without claiming unrelated failing suites are fixed.

- [ ] **Step 1: Run only directly related suites**

Run:

```powershell
Invoke-QgcSuite -SuiteName QGCCompressionTest -EvidenceName focused.QGCCompressionTest
Invoke-QgcSuite -SuiteName QGCArchiveModelTest -EvidenceName focused.QGCArchiveModelTest
Invoke-QgcSuite -SuiteName QGCStreamingDecompressionTest -EvidenceName focused.QGCStreamingDecompressionTest
Invoke-QgcSuite -SuiteName QGCFileHelperTest -EvidenceName focused.QGCFileHelperTest
Invoke-QgcSuite -SuiteName QGCArchiveWatcherTest -EvidenceName focused.QGCArchiveWatcherTest
```

Expected: all five suites have zero failures and zero errors. Only `QGCCompressionTest` may contain one directory-symlink capability skip.

- [ ] **Step 2: Parse JUnit evidence and scan console errors**

Run:

```powershell
@'
from pathlib import Path
import xml.etree.ElementTree as ET

for path in sorted(Path('.tmp/phase2a').glob('focused.*-*.xml')):
    root = ET.parse(path).getroot()
    suites = [root] if root.tag == 'testsuite' else list(root.findall('.//testsuite'))
    totals = {
        key: sum(int(suite.get(key, '0')) for suite in suites)
        for key in ('tests', 'failures', 'errors', 'skipped')
    }
    print(path.name, ' '.join(f'{key}={value}' for key, value in totals.items()))
'@ | python - > .tmp/phase2a/focused-summary.txt

rg -n -i "Failed to write header|missing.*dll|plugin.*failed|fatal|assert|crash" `
  .tmp/phase2a/focused.*.console.log > .tmp/phase2a/focused-error-scan.txt
Get-Content .tmp/phase2a/focused-summary.txt -TotalCount 40
Get-Content .tmp/phase2a/focused-error-scan.txt -TotalCount 40
```

Expected: no failure/error totals and no Unicode output-write diagnostics. Any unexpected result stops delivery and returns to systematic debugging.

- [ ] **Step 3: Update the environment report with measured results**

Append a `Phase 2A Windows compression hardening` section to `ENVIRONMENT_REPORT.md` containing:

- Branch and exact commit hashes for the symlink probe and Unicode output fix.
- Confirmed compiler, Qt kit, GStreamer root, and build directory.
- The incremental build command and exit code.
- The five-suite table with measured tests/failures/errors/skips.
- The exact symlink outcome: either real directory symlink assertions passed, or the exact system error/code from the JUnit skip.
- Confirmation that every existing Unicode case passed and `archive_read_open_filename_w` was not introduced.
- Explicit statement that full CTest was not rerun by design and unrelated Phase 2 failures remain out of scope.

- [ ] **Step 4: Reconcile state files**

Mark the Phase 2A plan and implementation items complete in `state/TODO.md`. Add the measured build/test result, symlink capability outcome, and Unicode boundary conclusion to `state/LOG.md`. Do not mark broader Phase 2 complete.

- [ ] **Step 5: Perform final verification before delivery**

Run:

```powershell
git diff --check
git status --short
git log --oneline -5
git diff HEAD~2 -- test/UnitTestFramework/BaseClasses/TempDirectoryTest.h src/Utilities/Compression/QGClibarchive.cc
```

Use `superpowers:verification-before-completion`, then request a fresh code review. Resolve any Critical or Important finding and rerun the affected focused tests.

- [ ] **Step 6: Commit documentation and push the approved branch**

Run:

```powershell
git add ENVIRONMENT_REPORT.md state/TODO.md state/LOG.md
git commit -m "docs[compression]: record Phase 2A verification"
git push origin codex/windows-test-hardening
```

Expected: only `codex/windows-test-hardening` advances; `.tmp/` remains untracked; no PR is created.
