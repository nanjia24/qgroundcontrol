# Windows Test Hardening Phase 1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Fix four confirmed Windows test root causes without weakening logging or masking production path defects.

**Architecture:** Keep locale/platform/path assertion corrections inside their focused tests. Fix Windows drive and UNC path recognition at the shared `QGCFileHelper` boundary so downstream archive consumers receive correct local-path classification.

**Tech Stack:** C++20, Qt 6.10.3 QtTest, CMake, Ninja, MSVC v143 x64.

## Global Constraints

- Work only in `E:\workspace\QGC\qgroundcontrol-worktrees\windows-test-hardening` on `codex/windows-test-hardening`.
- Use VS2022 Community v143 Hostx64/x64, Qt `E:\Qt\6.10.3\msvc2022_64`, and GStreamer `E:\PROGRA~1\GSTREA~1\1.0\MSVC_X~1`.
- Preserve `/Zc:preprocessor`; do not pass a temporary `CMAKE_CXX_FLAGS` override.
- Do not add global warning suppression, broad log allowlists, or unconditional Windows skips.
- Redirect build/test output to `.tmp/phase1/` and show fewer than 50 lines of summaries.
- Keep `.tmp/`, build products, JUnit XML, and console logs untracked.
- Use structured commits and push only `codex/windows-test-hardening`; no force-push and no PR creation.
- No GitHub issue number is currently known; do not fabricate a `Fixes` or `Resolves` footer.

## Common Test Command

For each QGC test suite, run `build/windows-debug/Debug/QGroundControl.exe` with this environment and arguments:

```powershell
$env:PATH = "E:\Qt\6.10.3\msvc2022_64\bin;E:\PROGRA~1\GSTREA~1\1.0\MSVC_X~1\bin;$env:PATH"
$env:GSTREAMER_1_0_ROOT_MSVC_X86_64 = "E:\PROGRA~1\GSTREA~1\1.0\MSVC_X~1"
$env:QT_QPA_PLATFORM = "offscreen"
$env:QGC_TEST_VERBOSE = "1"
$env:QT_LOGGING_RULES = "*.debug=false"

build/windows-debug/Debug/QGroundControl.exe `
  --unittest:<SuiteName> `
  --allow-multiple `
  --unittest-output:.tmp/phase1/<SuiteName>.xml `
  --log-output `
  --logging:*.debug=true `
  --no-windows-assert-ui
```

The runner rewrites the output name to `<SuiteName>-<QObjectName>.xml`; parse the actual generated file.

---

### Task 1: Make JSON file-type validation test locale-independent

**Files:**
- Modify: `test/Utilities/Parsing/Json/JsonHelperTest.cc`
- Test: `test/Utilities/Parsing/Json/JsonHelperTest.cc::_validateInternalWrongFileType_test`

**Interfaces:**
- Consumes: `JsonParsing::validateInternalQGCJsonFile(...)`
- Produces: a test that proves rejection and verifies expected/actual type evidence without depending on translated prose

- [ ] **Step 1: Record the existing RED result**

Run `JsonHelperTest` with the common command.

Expected JUnit failure:

```text
_validateInternalWrongFileType_test
'errorString.contains("Incorrect file type")' returned FALSE
```

- [ ] **Step 2: Replace the locale-sensitive assertion**

Keep the existing rejection assertion, and replace:

```cpp
QVERIFY(errorString.contains("Incorrect file type"));
```

with:

```cpp
QVERIFY2(errorString.contains(QStringLiteral("TestType")), qPrintable(errorString));
QVERIFY2(errorString.contains(QStringLiteral("WrongType")), qPrintable(errorString));
```

This retains semantic coverage: the function must reject the object and identify both expected and actual types.

- [ ] **Step 3: Incrementally rebuild**

Run:

```powershell
cmake --build build/windows-debug --parallel
```

Expected: exit code 0 and updated `Debug/QGroundControl.exe`.

- [ ] **Step 4: Verify GREEN**

Run `JsonHelperTest` again. Expected JUnit result: 19 tests, 0 failures, 0 errors.

- [ ] **Step 5: Commit**

```powershell
git add test/Utilities/Parsing/Json/JsonHelperTest.cc
git commit -m "test[json]: make file type validation locale-independent"
```

---

### Task 2: Make Platform initialization expectations platform-specific

**Files:**
- Modify: `test/Utilities/Platform/PlatformTest.cc`
- Test: `test/Utilities/Platform/PlatformTest.cc::_testInitializeSetsUnitTestEnvironment`

**Interfaces:**
- Consumes: `Platform::initialize(...)` platform contract
- Produces: Unix-only stderr assertions and a cross-platform headless unit-test assertion

- [ ] **Step 1: Record the existing RED result**

Run `PlatformTest` with the common command.

Expected Windows JUnit failure:

```text
Actual QT_ASSUME_STDERR_HAS_CONSOLE: ""
Expected: "1"
```

- [ ] **Step 2: Guard Unix-only assertions**

Replace the three unconditional comparisons after `Platform::initialize` with:

```cpp
#if defined(Q_OS_UNIX) && !defined(Q_OS_ANDROID)
    QCOMPARE(qgetenv("QT_ASSUME_STDERR_HAS_CONSOLE"), QByteArray("1"));
    QCOMPARE(qgetenv("QT_FORCE_STDERR_LOGGING"), QByteArray("1"));
#endif
    QCOMPARE(qgetenv("QT_QPA_PLATFORM"), QByteArray("offscreen"));
```

Do not assert that the Unix variables are absent on Windows because callers may legitimately provide them before initialization.

- [ ] **Step 3: Incrementally rebuild**

Run `cmake --build build/windows-debug --parallel`. Expected: exit code 0.

- [ ] **Step 4: Verify GREEN**

Run `PlatformTest`. Expected JUnit result: 5 tests, 0 failures, 0 errors.

- [ ] **Step 5: Commit**

```powershell
git add test/Utilities/Platform/PlatformTest.cc
git commit -m "test[platform]: use platform-specific environment expectations"
```

---

### Task 3: Make GeoTag test paths and copied resources Windows-safe

**Files:**
- Modify: `test/AnalyzeView/GeoTag/GeoTagControllerTest.cc`
- Test: `GeoTagControllerTest::_propertyAccessorsTest`
- Test: `GeoTagControllerTest::_calibrationMismatchTest`

**Interfaces:**
- Consumes: `GeoTagController` path accessors and Qt resource copying
- Produces: filesystem-appropriate path comparison and writable temporary image copies

- [ ] **Step 1: Record both existing RED failures**

Run `GeoTagControllerTest` with the common command.

Expected failures:

```text
_propertyAccessorsTest: drive-letter case differs (c:/ versus C:/)
_calibrationMismatchTest: QTemporaryDir cannot remove read-only copied files
```

- [ ] **Step 2: Add focused test helpers in the anonymous namespace**

Add:

```cpp
void compareLocalPaths(const QString &actual, const QString &expected)
{
    const QString cleanActual = QDir::cleanPath(actual);
    const QString cleanExpected = QDir::cleanPath(expected);
#ifdef Q_OS_WIN
    QCOMPARE(cleanActual.toCaseFolded(), cleanExpected.toCaseFolded());
#else
    QCOMPARE(cleanActual, cleanExpected);
#endif
}

bool copyWritableResource(QFile &source, const QString &destination)
{
    if (!source.copy(destination)) {
        return false;
    }

    const QFileDevice::Permissions permissions = QFileInfo(destination).permissions();
    return QFile::setPermissions(destination, permissions | QFileDevice::WriteOwner);
}
```

If not already included, add `#include <QtCore/QFileInfo>`.

- [ ] **Step 3: Use the helpers at the two affected boundaries**

Replace local-path `QCOMPARE(QDir::cleanPath(...), QDir::cleanPath(...))` assertions in `_propertyAccessorsTest` with `compareLocalPaths(...)`.

At lines currently copying `DSCN0010.jpg` in `_propertyAccessorsTest` and `_calibrationMismatchTest`, construct the destination once and call:

```cpp
const QString destination = imageDirPath + QStringLiteral("/geotag_temp_image_%1.jpg").arg(i);
QVERIFY(copyWritableResource(file, destination));
```

- [ ] **Step 4: Incrementally rebuild**

Run `cmake --build build/windows-debug --parallel`. Expected: exit code 0.

- [ ] **Step 5: Verify GREEN and strict cleanup**

Run `GeoTagControllerTest`. Expected JUnit result: 18 tests, 0 failures, 0 errors, and no `QTemporaryDir: Unable to remove` warning.

- [ ] **Step 6: Commit**

```powershell
git add test/AnalyzeView/GeoTag/GeoTagControllerTest.cc
git commit -m "test[geotag]: handle Windows path and file semantics"
```

---

### Task 4: Recognize Windows drive and UNC paths as local

**Files:**
- Modify: `test/Utilities/FileSystem/QGCFileHelperTest.cc`
- Modify: `src/Utilities/FileSystem/QGCFileHelper.cc`
- Test: `QGCFileHelperTest::_testToLocalPathPlainPaths`
- Test: `QGCFileHelperTest::_testIsLocalPath`
- Regression: `QGCArchiveModelTest::_testArchiveUrl`

**Interfaces:**
- Consumes: `QDir::isAbsolutePath`, `QGCFileHelper::toLocalPath`, `QGCFileHelper::isLocalPath`
- Produces: correct classification of Windows drive/UNC paths before generic `QUrl` scheme parsing

- [ ] **Step 1: Add Windows regression assertions**

Inside `_testToLocalPathPlainPaths`, add:

```cpp
#ifdef Q_OS_WIN
    QCOMPARE(QGCFileHelper::toLocalPath(QStringLiteral("C:/Users/Test/file.txt")),
             QStringLiteral("C:/Users/Test/file.txt"));
    QCOMPARE(QGCFileHelper::toLocalPath(QStringLiteral("C:\\Users\\Test\\file.txt")),
             QStringLiteral("C:\\Users\\Test\\file.txt"));
    QCOMPARE(QGCFileHelper::toLocalPath(QStringLiteral("\\\\server\\share\\file.txt")),
             QStringLiteral("\\\\server\\share\\file.txt"));
#endif
```

Inside `_testIsLocalPath`, add:

```cpp
#ifdef Q_OS_WIN
    QVERIFY(QGCFileHelper::isLocalPath(QStringLiteral("C:/Users/Test/file.txt")));
    QVERIFY(QGCFileHelper::isLocalPath(QStringLiteral("C:\\Users\\Test\\file.txt")));
    QVERIFY(QGCFileHelper::isLocalPath(QStringLiteral("\\\\server\\share\\file.txt")));
#endif
```

- [ ] **Step 2: Verify RED before production changes**

Incrementally build and run `QGCFileHelperTest`.

Expected: at least `isLocalPath("C:/Users/Test/file.txt")` fails because `QUrl` interprets `C` as a scheme. Confirm the failure is the new assertion, not a compile or fixture error.

- [ ] **Step 3: Add the minimum Windows absolute-path guard**

In `toLocalPath(const QString &urlOrPath)`, after the existing empty/resource checks and before constructing `QUrl`, add:

```cpp
#ifdef Q_OS_WIN
    if (QDir::isAbsolutePath(urlOrPath)) {
        return urlOrPath;
    }
#endif
```

In `isLocalPath(const QString &urlOrPath)`, after the empty/resource checks and before constructing `QUrl`, add:

```cpp
#ifdef Q_OS_WIN
    if (QDir::isAbsolutePath(urlOrPath)) {
        return true;
    }
#endif
```

Do not special-case arbitrary one-letter URL schemes; use Qt's platform path semantics.

- [ ] **Step 4: Verify helper GREEN**

Incrementally build and run `QGCFileHelperTest`. Expected: all helper tests pass with no unexpected warnings.

- [ ] **Step 5: Verify downstream archive regression GREEN**

Run `QGCArchiveModelTest`. Expected JUnit result: 26 tests, 0 failures, 0 errors; `_testArchiveUrl` retains the temporary file path instead of clearing it as unsupported.

- [ ] **Step 6: Commit**

```powershell
git add test/Utilities/FileSystem/QGCFileHelperTest.cc src/Utilities/FileSystem/QGCFileHelper.cc
git commit -m "fix[filesystem]: recognize Windows absolute paths"
```

---

### Task 5: Phase 1 regression gate and delivery

**Files:**
- Modify: `state/README.md`
- Modify: `state/TODO.md`
- Modify: `state/LOG.md`
- Artifacts: `.tmp/phase1/` (untracked)

**Interfaces:**
- Consumes: the four focused fixes
- Produces: evidence-backed Phase 1 result and a reviewable remote branch

- [ ] **Step 1: Run the Phase 1 focused set**

Run these suites individually with JUnit XML and console logs:

```text
JsonHelperTest
PlatformTest
GeoTagControllerTest
QGCFileHelperTest
QGCArchiveModelTest
```

Expected: all suites have 0 failures and 0 errors; capability skips are not expected in Phase 1.

- [ ] **Step 2: Check source and repository hygiene**

Run:

```powershell
git diff --check
git status --short
git log --oneline origin/codex/joystick-aux-px4..HEAD
```

Expected: only intentional state documentation changes remain; `.tmp/` stays untracked.

- [ ] **Step 3: Update and commit state**

Record exact suite counts, build result, resolved root causes, and remaining Phase 2 failures in the three state files.

```powershell
git add state/README.md state/TODO.md state/LOG.md
git commit -m "docs[tests]: record Windows hardening phase 1"
```

- [ ] **Step 4: Push only the new repair branch**

```powershell
git push -u origin codex/windows-test-hardening
```

Expected: the remote branch is created or fast-forwarded. Do not create a PR and do not push `codex/joystick-aux-px4`.
