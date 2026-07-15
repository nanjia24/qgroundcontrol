# QGroundControl Windows Desktop Environment Report

Date: 2026-07-06
Host: Microsoft Windows NT 10.0.26200.0, x64, machine CROCODILE
Repository parent: E:\workspace\QGC
Repository path: E:\workspace\QGC\qgroundcontrol
Target integration branch: codex/joystick-aux-px4
Local working branch: codex/windows-desktop-env-setup

## Scope

This run was limited to Windows Desktop. No Android SDK/NDK actions were performed.
No business source code was modified, no commits were created, no pushes were performed, and no PR was opened.
Existing HTTP_PROXY and HTTPS_PROXY were detected but not overwritten; values are intentionally not recorded.

## Git verification

Commands executed:

~~~powershell
git clone https://github.com/nanjia24/qgroundcontrol.git qgroundcontrol
git remote -v
git fetch origin --prune
git switch codex/joystick-aux-px4
git pull --ff-only origin codex/joystick-aux-px4
git log --oneline master..HEAD -5
git submodule status
git switch -c codex/windows-desktop-env-setup
~~~

Observed:

- Origin: https://github.com/nanjia24/qgroundcontrol.git
- Target branch checkout/pull: succeeded; already up to date.
- Target HEAD: daae3a37b Show joystick inputs on FlyView virtual sticks
- Recent master..HEAD sample:
  - daae3a37b Show joystick inputs on FlyView virtual sticks
  - b0cecda19 Support duplicate vehicle IDs by source
  - 1c62b2fb0 Add quad rover hybrid airframe icon
  - 4b48dc013 Add joystick axis action mapping
- Local working branch created: codex/windows-desktop-env-setup
- .gitmodules: not present.
- git submodule status: failed because this Git installation could not find bundled Unix helper commands basename, sed, and git-sh-setup. Since .gitmodules is absent, no recursive submodule operation was run.

## Tool detection

| Tool | Result |
| --- | --- |
| PowerShell | 5.1.26100.8655 |
| Git | C:\Program Files\Git\cmd\git.exe, version 2.51.0.windows.1 |
| Python launcher | C:\Windows\py.exe |
| Python | C:\Python314\python.exe, Python 3.14.4 |
| CMake | C:\Program Files\CMake\bin\cmake.exe, version 3.31.0 |
| Ninja | D:\vscode\environment\ninja.exe, version 1.12.1 |
| Visual Studio / MSVC | Visual Studio Build Tools 18.5.2 at C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools |
| MSVC compiler after VsDevCmd | cl.exe found at ...\MSVC\14.50.35717\bin\Hostx64\x64\cl.exe; compiler reports 19.50.35730 |
| Windows SDK | 10.0.26100.0 detected |
| Qt required path | C:\Qt\6.10.3\msvc2022_64 not found |
| Qt existing path | E:\Qt\6.9.0\msvc2022_64 found, but it is below the requested version and was not used |
| Qt MaintenanceTool | E:\Qt\MaintenanceTool.exe found |
| GStreamer required path | C:\gstreamer\1.0\msvc_x86_64\bin not found |
| GStreamer env | GSTREAMER_1_0_ROOT_MSVC_X86_64 not set in current session |
| NSIS | makensis.exe not found |
| Proxies | HTTP_PROXY and HTTPS_PROXY were set; values intentionally not recorded |

## MSVC environment

Current shell initially did not have cl on PATH.
VsDevCmd successfully loaded the x64 toolchain and found cl.
Compiler output summary: Microsoft C/C++ optimizing compiler 19.50.35730 for x64.

## Qt 6.10.3 status

Required Qt kit was not installed:

- Expected: C:\Qt\6.10.3\msvc2022_64
- Found only: E:\Qt\6.9.0\msvc2022_64

The existing Qt MaintenanceTool supports headless commands, but searching for qt.qt6.6103.win64_msvc2022_64 failed with:

- cache lock creation access denied under the user Qt installer cache;
- network connection failure;
- no valid license / not logged in.

No Qt account password or token was requested, stored, or written.

Required manual Qt action:

1. Run the official Qt Online Installer or Qt MaintenanceTool interactively.
2. Use the user's Qt account only in the GUI; do not share or store credentials.
3. Install Qt 6.10.3 with kit msvc2022_64.
4. Preferred target path for the requested build commands: C:\Qt\6.10.3\msvc2022_64.
5. Include at least these Qt modules: qtgraphs, qtlocation, qtpositioning, qtspeech, qtmultimedia, qtserialport, qtimageformats, qtshadertools, qtconnectivity, qtquick3d, qtsensors, qtscxml, qtwebsockets, qthttpserver.

Equivalent component naming observed from installed Qt 6.9.0 suggests the 6.10.3 add-on component pattern is likely qt.qt6.6103.addons.<module> plus qt.qt6.6103.win64_msvc2022_64, but availability must be confirmed in the installer UI.

## GStreamer dependency status

Commands executed:

~~~powershell
py .\tools\setup\install_dependencies.py --platform windows
py -m pip install --user httpx
C:\Python314\python.exe .\tools\setup\install_dependencies.py --platform windows
~~~

Results:

- First attempts with py under the elevated shell resolved to a different Python runtime that lacked httpx and then failed certificate validation through urllib.
- Installed missing user Python package httpx using py -m pip install --user httpx.
- Re-running with explicit C:\Python314\python.exe reached the project dependency mirror but failed with HTTP 403 Forbidden from https://qgroundcontrol.s3.us-west-2.amazonaws.com/dependencies/gstreamer/windows/1.28.1/.
- Official GStreamer 1.28.1 directory was reachable via Python httpx, but it exposes .exe installers, while the current QGC script expects runtime/devel .msi installers.

No GStreamer install completed. Required post-install checks remain:

- C:\gstreamer\1.0\msvc_x86_64\bin exists.
- GSTREAMER_1_0_ROOT_MSVC_X86_64=C:\gstreamer\1.0\msvc_x86_64.
- Current build session PATH includes C:\gstreamer\1.0\msvc_x86_64\bin.

Manual GStreamer options:

1. Prefer fixing access to the QGC S3 dependency mirror and rerun:
   C:\Python314\python.exe .\tools\setup\install_dependencies.py --platform windows
2. If using official GStreamer installer manually, install MSVC x86_64 GStreamer 1.28.1 to C:\gstreamer and verify runtime, headers, and libraries exist under C:\gstreamer\1.0\msvc_x86_64. Then set the environment variable and PATH above.

## NSIS status

Command attempted:

~~~powershell
choco install nsis -y --no-progress
~~~

Result: failed. Chocolatey was not running in an elevated administrator shell and could not access/create required paths under C:\ProgramData\chocolatey. makensis.exe is still not found.

Manual NSIS action:

1. Open PowerShell as Administrator.
2. Ensure no other Chocolatey process is running.
3. Run:
   choco install nsis -y --no-progress
   where makensis
4. If Chocolatey reports a stale lock, remove only the specific stale lock file after verifying no active Chocolatey/NuGet process owns it.

## Configure/build status

Debug CMake configure was not run because the required Qt root does not exist:

~~~powershell
C:\Qt\6.10.3\msvc2022_64\bin\qt-cmake.bat -S . -B build\windows-debug -G Ninja -DCMAKE_BUILD_TYPE=Debug
~~~

Reason: continuing with E:\Qt\6.9.0\msvc2022_64 would violate the requested Qt version/kit requirement.

Therefore these steps were not run:

~~~powershell
cmake --build build\windows-debug --parallel
ctest --test-dir build\windows-debug -C Debug --output-on-failure
C:\Qt\6.10.3\msvc2022_64\bin\qt-cmake.bat -S . -B build\windows-release -G Ninja -DCMAKE_BUILD_TYPE=Release -DQGC_BUILD_TESTING=OFF
cmake --build build\windows-release --parallel
~~~

No cmake --install was run.

## Expected commands after manual blockers are resolved

Use a fresh PowerShell session, preserve any existing proxy variables, then load MSVC and set paths for the current process:

~~~powershell
cmd /c ""C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\Common7\Tools\VsDevCmd.bat" -arch=amd64 -host_arch=amd64 && powershell"
$QtRoot = 'C:\Qt\6.10.3\msvc2022_64'
$env:GSTREAMER_1_0_ROOT_MSVC_X86_64 = 'C:\gstreamer\1.0\msvc_x86_64'
$env:PATH = "$QtRoot\bin;C:\gstreamer\1.0\msvc_x86_64\bin;$env:PATH"
& "$QtRoot\bin\qt-cmake.bat" -S . -B build\windows-debug -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build\windows-debug --parallel
ctest --test-dir build\windows-debug -C Debug --output-on-failure
~~~

Only if Debug succeeds:

~~~powershell
& "$QtRoot\bin\qt-cmake.bat" -S . -B build\windows-release -G Ninja -DCMAKE_BUILD_TYPE=Release -DQGC_BUILD_TESTING=OFF
cmake --build build\windows-release --parallel
~~~

Installer generation command for later:

~~~powershell
cmake --install build\windows-release --config Release
~~~

Notes for installer generation:

- makensis.exe must be findable on PATH.
- Install output first stages into build\windows-release\staging.
- The installer is generated in the Release build directory.



## Update 2026-07-06 after user-provided Qt install

- Qt 6.10.3 msvc2022_64 was verified at E:\Qt\6.10.3\msvc2022_64.
- Required Qt CMake modules verified present: Qt6Graphs, Qt6Location, Qt6Positioning, Qt6TextToSpeech, Qt6Multimedia, Qt6SerialPort, Qt6ShaderTools, Qt6Bluetooth, Qt6Quick3D, Qt6Sensors, Qt6Scxml, Qt6WebSockets, Qt6HttpServer.
- Re-ran QGC dependency script from parent path:
  C:\Python314\python.exe .\qgroundcontrol\tools\setup\install_dependencies.py --platform windows
- Dependency script still failed on QGC S3 with HTTP 403. Retried with current-process HTTP_PROXY/HTTPS_PROXY/ALL_PROXY=http://127.0.0.1:7897; result remained HTTP 403.
- Direct CMake Debug configure initially selected MSYS/GNU due environment pollution. Fixed by running CMake from Git Bash after loading VsDevCmd, manually adding MSVC/Windows SDK/Qt/CMake/Ninja to Bash PATH.
- The Git Bash/MSVC configure path fixed the Git for Windows submodule helper issue and progressed through CPM dependencies.
- CMake then failed while downloading GStreamer via CMake file(DOWNLOAD): SSL connect error, even with 7897 proxy variables.
- Workaround attempted: pre-downloaded official GStreamer 1.28.1 MSVC x86_64 installer with Python/httpx to .cache\CPM\gstreamer-win-x86_64-1.28.1\gstreamer-x86_64.exe. Size: 518,115,454 bytes.
- CMake accepted the cached installer and attempted silent SDK install, but installer returned exit code 1 and did not create installer log or SDK directory.
- Manual Start-Process with Inno-style silent args returned exit code 1. NSIS-style /S attempt hung for over 5 minutes and was terminated; no SDK directory was created.
- A visible GStreamer installer window was launched for manual completion. After 60 seconds, C:\gstreamer\1.0\msvc_x86_64\bin\pkg-config.exe was still not detected.

Current blocker: complete GStreamer installation manually so that C:\gstreamer\1.0\msvc_x86_64 contains bin\pkg-config.exe, lib\gstreamer-1.0, and lib\pkgconfig\gstreamer-1.0.pc. Then rerun Debug configure/build using .tmp\configure-debug-msvc.sh and .tmp\build-debug-msvc.sh.



## Update 2026-07-06 final run

User-provided GStreamer SDK path verified:

- E:\Program Files\gstreamer\1.0\msvc_x86_64
- gst-launch-1.0 version 1.28.1
- Required files present: bin\pkg-config.exe, lib\gstreamer-1.0, lib\pkgconfig\gstreamer-1.0.pc

Important environment fix:

- Because the GStreamer path contains a space, CMake/pkg-config split E:/Program Files into E:/Program and failed generation.
- Workaround used for this session: Windows 8.3 short path E:\PROGRA~1\GSTREA~1\1.0\MSVC_X~1, passed as GStreamer_ROOT_DIR=E:/PROGRA~1/GSTREA~1/1.0/MSVC_X~1.

Debug configure/build results:

- Full Debug configure with tests ON generated build files, but CMake process returned exit code 1 due repeated Qt/QML input file warnings for build\windows-debug\qml\QGroundControl\... files not existing during configure.
- Full Debug build failed in test sources:
  - test/MissionManager/QGCMapPolygonTest.cc
  - test/MissionManager/MissionControllerTest.cc
  - MSVC errors include C2220 and C2146 around __VA_OPT__ macro usage.
- QGC app-only Debug configure was run separately with -DQGC_BUILD_TESTING=OFF and -DQGC_USE_CACHE=OFF.
- App-only Debug build succeeded:
  - build\windows-debug-app\Debug\QGroundControl.exe
  - size: 81,943,552 bytes
- Launch validation succeeded for app-only Debug:
  - QGroundControl process stayed running for 20 seconds.
  - It was then stopped intentionally.
  - stderr contained only: "QML debugging is enabled. Only use this in a safe environment."

CTest result:

- Command executed: ctest --test-dir build\windows-debug -C Debug --output-on-failure
- Exit code: 8
- Most tests were Not Run because full Debug/test build did not complete.
- QmlQuickTests failed with exit code 0xc0000135.

Release status:

- Release configure/build was not executed because the requested full Debug build and CTest validation did not pass.



## Update 2026-07-07 VS2022 v143 full Debug revalidation

User-confirmed toolchain was verified:

- VS DevCmd: D:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat
- cl resolved in build shell to: /d/Program Files/Microsoft Visual Studio/2022/Community/VC/Tools/MSVC/14.44.35207/bin/Hostx64/x64/cl
- cl version: MSVC 19.44.35228 for x64
- Qt: E:\Qt\6.10.3\msvc2022_64
- GStreamer: E:\Program Files\gstreamer\1.0\msvc_x86_64, exposed to CMake/pkg-config via short path E:\PROGRA~1\GSTREA~1\1.0\MSVC_X~1
- NSIS: C:\Program Files (x86)\NSIS\makensis.exe, version v3.12

Revalidation steps:

1. Configured a fresh full Debug build directory:
   build\windows-debug-vs2022
   with QGC_BUILD_TESTING=ON and QGC_USE_CACHE=OFF.
2. Build with VS2022 v143 still failed at MissionControllerTest.cc on __VA_OPT__.
3. The compiler warning identified the actual required flag:
   warning C5109: using __VA_OPT__ in macro requires /Zc:preprocessor.
4. Configured a second fresh full Debug build directory:
   build\windows-debug-vs2022-zc2
   with:
   - MSVC 19.44.35228
   - CMAKE_CXX_FLAGS=/Zc:preprocessor
   - QGC_BUILD_TESTING=ON
   - QGC_USE_CACHE=OFF
5. Full Debug build succeeded.
   Output executable:
   build\windows-debug-vs2022-zc2\Debug\QGroundControl.exe
   Size: 91,630,592 bytes.

CTest result for build\windows-debug-vs2022-zc2:

- Command:
  ctest --test-dir build/windows-debug-vs2022-zc2 -C Debug --output-on-failure
- Total: 186 tests
- Passed: 175
- Failed: 11
- Result: 94% tests passed; CTest exit code 1.

Failed tests:

- GeoTagControllerTest
- HashCheckTest
- SigningTest
- MissionCommandTreeEditorTest
- MissionManagerTest
- QGCArchiveModelTest
- QGCCompressionTest
- JsonHelperTest
- PlatformTest
- ComponentInformationCacheTest
- RequestMetaDataTypeStateMachineTest

Additional observations:

- QmlQuickTests passed in the full CTest run.
- Rerunning selected failed tests directly returned nonzero status and only emitted the standard "QML debugging is enabled" line; LastTest.log did not include detailed assertion output for these failures.
- Attempting to pass QtTest verbose arguments using "-- -v2" was rejected by the current QGC command-line parser as unexpected positional arguments.
- No business source code was modified.

Conclusion:

- The previous __VA_OPT__/C2146 compile failure is not caused by VS2026 specifically; it also occurs with VS2022 v143 unless the conforming preprocessor flag /Zc:preprocessor is enabled.
- With /Zc:preprocessor, full Debug compilation succeeds.
- Remaining issue is test runtime failures: 11 of 186 tests fail under this Windows environment.


## Update 2026-07-13 MSVC preprocessor fix and failed-test triage

Scope of this update:

- No business source code was changed.
- No commit, push, or PR was created.
- Only CMake platform configuration, environment report/state documentation, and local diagnostic artifacts were changed.
- Full CTest was not repeated. Only the previously failed 11 tests were run individually.

### CMake change

File changed:

- `cmake/platform/Windows.cmake`

Change:

- Added `/Zc:preprocessor` for MSVC Windows builds via:
  - `add_compile_options(/Zc:preprocessor)`
  - `target_compile_options(${CMAKE_PROJECT_NAME} PRIVATE /Zc:preprocessor)`

Reason:

- Under VS2022 v143 x64, the previous test build failure emitted `warning C5109: using __VA_OPT__ in macro requires /Zc:preprocessor`, followed by warnings-as-errors and `C2146` parser failures.
- This is a project/compiler configuration requirement, not a VS2026-only issue.

### Clean Debug build verification without manual CMAKE_CXX_FLAGS

Fresh build directory:

- `build/windows-debug-vs2022-zc-fixed`

Configure command equivalent:

```powershell
cmd /c ""D:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64 && "C:\Program Files\Git\bin\bash.exe" .tmp/configure-debug-vs2022-zc-fixed.sh > configure-debug-vs2022-zc-fixed.log 2>&1"
```

Important configure options:

- `-G Ninja`
- `-DCMAKE_BUILD_TYPE=Debug`
- `-DCMAKE_PREFIX_PATH=E:/Qt/6.10.3/msvc2022_64`
- `-DCMAKE_C_COMPILER=cl.exe`
- `-DCMAKE_CXX_COMPILER=cl.exe`
- `-DGStreamer_ROOT_DIR=E:/PROGRA~1/GSTREA~1/1.0/MSVC_X~1`
- `-DQGC_USE_CACHE=OFF`
- `-DQGC_BUILD_TESTING=ON`
- No `-DCMAKE_CXX_FLAGS=/Zc:preprocessor` was passed.

Build command equivalent:

```powershell
cmd /c ""D:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64 && "C:\Program Files\Git\bin\bash.exe" .tmp/build-debug-vs2022-zc-fixed.sh > build-debug-vs2022-zc-fixed.log 2>&1"
```

Result:

- Configure exit code: 0.
- Build completed and produced:
  - `build/windows-debug-vs2022-zc-fixed/Debug/QGroundControl.exe`
  - Size: 98,421,248 bytes.
- `build/windows-debug-vs2022-zc-fixed/build.ninja` contains `/Zc:preprocessor` in QGroundControl compile flags.
- `build-debug-vs2022-zc-fixed.log` did not contain `__VA_OPT__`, `C2146`, `warning C5109`, `FAILED:`, or `ninja: build stopped` fatal patterns.

### Individual failed-test collection

Current branch test output directory:

- `.tmp/test-runs-zc-fixed`

Command pattern used for each failed test:

```powershell
QGroundControl.exe --unittest:<TestName> --allow-multiple --unittest-output:<xml-base> --log-output --logging:*.debug=true --no-windows-assert-ui
```

Environment used:

- `QT_QPA_PLATFORM=offscreen`
- `QGC_TEST_VERBOSE=1`
- `QT_LOGGING_RULES=*.debug=false`
- PATH prepended with:
  - `E:\Qt\6.10.3\msvc2022_64\bin`
  - `E:\Program Files\gstreamer\1.0\msvc_x86_64\bin`

Note:

- QGC's unittest framework rewrites `--unittest-output:Test.xml` to `Test-<QObjectName>.xml`. The actual XML files therefore have names such as `JsonHelperTest-JsonHelperTest.xml`.
- `MissionCommandTreeEditorTest` first timed out at 180s and produced a zero-length XML. It was rerun with a 360s harness timeout and produced `MissionCommandTreeEditorTest-rerun360-MissionCommandTreeEditorTest.xml` in 93.1s.

### origin/master comparison

Clean comparison worktree:

- `E:\workspace\QGC\qgroundcontrol-origin-master`
- Commit: `898aee795 PlanView: improve Expected Vehicle Speeds section in Defaults`

Comparison build:

- `build/windows-debug-vs2022-master-zc`
- Built with the same VS2022/Qt/GStreamer environment.
- Because `origin/master` does not contain the CMake `/Zc:preprocessor` change, the comparison configure used `-DCMAKE_CXX_FLAGS=/Zc:preprocessor` only to make the master test binary buildable under the same compiler.

Comparison result:

- The same test suites, same failing functions, and same failure messages reproduce on `origin/master` for all XML-visible failures.
- `HashCheckTest`, `SigningTest`, and `RequestMetaDataTypeStateMachineTest` generated XML with zero failures when run individually on both current branch and `origin/master`.

### Failed-test classification

| Test | Current individual XML result | origin/master comparison | Classification | Next step |
| --- | --- | --- | --- | --- |
| `GeoTagControllerTest` | 2 failures: `_propertyAccessorsTest`, `_calibrationMismatchTest` | Same | Existing Windows/test issue, not introduced by joystick branch | Normalize Windows path comparison case; address/expect QTemporaryDir read-only cleanup warning or test fixture cleanup. |
| `HashCheckTest` | XML: 14 tests, 0 failures; duration 170.8s | XML: 14 tests, 0 failures; duration 171.4s | CTest timeout/runner issue, not assertion failure | Integration timeout is 120s; either optimize test or mark/raise timeout with evidence. |
| `SigningTest` | XML: 8 tests, 0 failures | XML: 8 tests, 0 failures | Not reproducible individually; likely earlier CTest runner/environment artifact | Re-run via exact CTest single test before changing code. |
| `MissionCommandTreeEditorTest` | Rerun XML: 3 tests, 1 failure in `testEditors` due uncategorized warnings | Same | Existing strict-log/test-environment issue; also near/over CTest integration timeout | Address log expectations/categories and Qt font path warning; consider timeout only after log issue is understood. |
| `MissionManagerTest` | 1 failure in `_testErrorAckFailureStrings` due uncategorized warnings | Same | Existing strict-log/test issue | Investigate `QString::arg: Argument missing` and warning capture in this test. |
| `QGCArchiveModelTest` | 1 failure in `_testArchiveUrl`: `archivePath()` empty; file URL reported unsupported | Same | Existing behavior/test mismatch | Inspect `QGCArchiveModel::setArchiveUrl` supported URL schemes versus test expectation for local file URLs. |
| `QGCCompressionTest` | 2 failures: `_testExtractArchiveToSymlinkedOutputPath`, `_testUnicodePaths` | Same | Existing Windows filesystem/encoding issue | Verify symlink privileges/Developer Mode and libarchive Unicode path extraction on Windows. |
| `JsonHelperTest` | 1 failure: `_validateInternalWrongFileType_test` expects English substring `Incorrect file type` | Same | Existing localization/message-string test issue | Compare actual localized error string; avoid asserting English UI text under zh_CN locale or force test locale. |
| `PlatformTest` | 1 failure: `QT_ASSUME_STDERR_HAS_CONSOLE` actual empty, expected `1` | Same | Existing test harness/environment initialization issue | Determine whether `Platform::initialize()` should set env before test or test should set expected Windows console env. |
| `ComponentInformationCacheTest` | 2 failures: `_lru_test`, `_multi_test`, `cache.access(...)` false | Same | Existing cache path/fixture issue | Inspect cache file creation/access semantics on Windows temp paths. |
| `RequestMetaDataTypeStateMachineTest` | XML: 9 tests, 0 failures; duration 94.7s | XML: 9 tests, 0 failures; duration 19.5s | Not reproducible individually; likely earlier runner/timing artifact | Re-run exact CTest single test before changing code. |

### Focus-test details requested

`JsonHelperTest`:

- Failing function: `_validateInternalWrongFileType_test`
- Failure: `errorString.contains("Incorrect file type")` returned false.
- Likely cause category: locale-sensitive assertion or changed error message text.

`SigningTest`:

- XML result: 8 tests, 0 failures.
- Same on `origin/master`.
- No assertion failure was available to extract from XML.

`QGCCompressionTest`:

- Failing function: `_testExtractArchiveToSymlinkedOutputPath`
  - Failure: `symlinkInfo.isSymLink()` returned false.
- Failing function: `_testUnicodePaths`
  - Failure: `QFile::exists(unicodeDir + "/manifest.json")` returned false.
- Likely cause category: Windows symlink privilege/Developer Mode and Unicode path handling in archive extraction.

### Current conclusion

- The MSVC compile blocker is resolved by codifying `/Zc:preprocessor` in Windows CMake configuration.
- Full Debug build with tests now compiles in a clean build directory without manually passing `CMAKE_CXX_FLAGS`.
- The 11 previous CTest failures are not attributable to the joystick branch based on same-environment `origin/master` comparison.
- Several failures are Windows test-environment/test-assumption problems; fixing them should be handled as a separate test-hardening task, not as part of joystick branch functional development.

### Remaining manual/engineering actions

1. Decide whether to keep the CMake `/Zc:preprocessor` fix in this branch and commit it later.
2. If test hardening is desired, open a separate branch/task for Windows test fixes.
3. Before changing tests, run exact single-test CTest commands for `SigningTest` and `RequestMetaDataTypeStateMachineTest` to confirm whether the earlier full CTest failures still reproduce in the fixed build directory.
4. Release build remains intentionally not re-run in this step because the current task focused on Debug/test diagnostics.

## Update 2026-07-13 Release validation and full-permission targeted rerun

### Release configure/build

- Build directory: `build/windows-release`
- Generator/build type: Ninja / Release
- Testing: `QGC_BUILD_TESTING=OFF`
- Compiler: VS2022 v143 `Hostx64/x64/cl.exe` 14.44.35207
- Qt kit: `E:\Qt\6.10.3\msvc2022_64`
- Configure result: exit code 0
- Build result: exit code 0; final link created `Release\QGroundControl.exe`
- Executable: `build/windows-release/Release/QGroundControl.exe`, 46,882,816 bytes
- Build log scan: zero `FAILED:`, `ninja: build stopped`, fatal MSVC error, or `error Cxxxx` markers

The first generation attempt failed because the long GStreamer install path was truncated to the invalid include path `E:/Program`. The successful clean configuration used `E:\PROGRA~1\GSTREA~1\1.0\MSVC_X~1`. The build subprocess also removed inherited `E:\msys64\mingw64\bin` so dependency checks could not select MinGW/GNU tools. Existing proxy variables were preserved.

Logs are under `.tmp/release-validation/`. The Release application remained running for the full 20-second observation window and was then terminated by the validation script. stdout and stderr were both empty, with no missing DLL, Qt plugin, or GStreamer error. `makensis.exe /VERSION` returned `v3.12` with exit code 0. `cmake --install` was not executed.

The future installer command is:

```powershell
cmake --install build\windows-release --config Release
```

This requires `makensis.exe` to be discoverable. Files are staged in `build/windows-release/staging`, and the installer is generated in the Release build directory.

### Full-permission rerun of the 11 previous failures

Only the 11 previously failing test classes were run; full CTest was not repeated. Artifacts are in `.tmp/test-runs-full-permission/`.

| Test class | Result | Comparison with prior run |
|---|---:|---|
| GeoTagControllerTest | 2 failures | Same functions/pattern |
| HashCheckTest | 0 failures | Same; 132.585 s still exceeds 120 s Integration timeout |
| SigningTest | 0 failures | Same |
| MissionCommandTreeEditorTest | 1 failure | Same strict-log warning pattern |
| MissionManagerTest | 1 failure | Same strict-log warning pattern |
| QGCArchiveModelTest | 1 failure | Same archive URL/path assertion |
| QGCCompressionTest | 2 failures | Same symlink and Unicode extraction assertions |
| JsonHelperTest | 1 failure | Same error-string assertion |
| PlatformTest | 1 failure | Same environment-variable assertion |
| ComponentInformationCacheTest | 2 failures | Same cache path assertions |
| RequestMetaDataTypeStateMachineTest | 0 failures | Same |

The process identity was `CROCODILE\86178`, Windows Medium Mandatory Level (`S-1-16-8192`), with `is_admin_role=False`. Codex CLI full access removed workspace sandbox restrictions but did not grant a UAC-elevated token or Developer Mode symlink capability. The unchanged results do not support filesystem sandbox permissions as the cause of the original failures.

## Phase 2A Windows compression hardening (2026-07-14)

- Branch/worktree: `codex/windows-test-hardening` at `E:\workspace\QGC\qgroundcontrol-worktrees\windows-test-hardening`
- Directory-symlink commit: `791125bb12a6ce90a863ee1374b97691c488f83d`
- Unicode output-path commit: `5699430948c0ed78facd5e0807f175cfa07c75a1`
- Compiler: VS2022 Community v143 Hostx64/x64, `cl.exe` 14.44.35207
- Qt kit: `E:\Qt\6.10.3\msvc2022_64`
- GStreamer root: `E:\PROGRA~1\GSTREA~1\1.0\MSVC_X~1`
- Build directory: `build\windows-debug`

The evidence-bearing build imported `D:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat` with `-arch=amd64 -host_arch=amd64`, prepended the confirmed Qt and GStreamer `bin` directories, and ran:

```powershell
cmake --build build/windows-debug --parallel
```

The first incremental invocation reported `ninja: no work to do` because localized MSVC `/showIncludes` output left stale dependency information. After verifying the exact object paths were inside `build/windows-debug`, only the affected objects were removed and rebuilt. The authoritative Unicode build exited `0` and logged compilation of `QGClibarchive.cc.obj`, linking of `Debug/lib/QGCCompression.lib`, and linking of `Debug/QGroundControl.exe`.

| Focused suite | Tests | Failures | Errors | Skips | Process exit |
|---|---:|---:|---:|---:|---:|
| QGCCompressionTest | 51 | 0 | 0 | 1 | 0 |
| QGCArchiveModelTest | 26 | 0 | 0 | 0 | 0 |
| QGCStreamingDecompressionTest | 21 | 0 | 0 | 0 | 0 |
| QGCFileHelperTest | 50 | 0 | 0 | 0 | 0 |
| QGCArchiveWatcherTest | 21 | 0 | 0 | 0 | 0 |
| **Total** | **169** | **0** | **0** | **1** | — |

The sole skip is `_testExtractArchiveToSymlinkedOutputPath`: `Directory symlinks are not supported in this environment: A required privilege is not held by the client. (error 1314)`. It is now a skip-only result; the caller no longer continues to a false `isSymLink()` assertion. On a token capable of creating a real directory symlink, the existing extraction assertions still execute.

`_testUnicodePaths` passed for the existing Japanese, Chinese, Greek, accented Latin, and Cyrillic output directories. Windows now copies the final libarchive output entry pathname through `archive_entry_copy_pathname_w`; non-Windows retains the narrow pathname call. No `archive_read_open_filename_w` usage was introduced, so Unicode archive input behavior remains outside this phase.

All five console logs contained only the expected `QML debugging is enabled. Only use this in a safe environment.` notice. The scan found no archive-header write failure, missing DLL/plugin failure, fatal assertion, or crash marker. Full CTest was intentionally not rerun; remaining cache, Mission logging, and CTest-runner investigations are outside Phase 2A. Evidence is stored under `.tmp\phase2a\` and remains untracked.

## Phase 2B Windows component-cache test lifecycle (2026-07-14)

- Branch/worktree: `codex/windows-test-hardening` at `E:\workspace\QGC\qgroundcontrol-worktrees\windows-test-hardening`
- Test fix commit: `22d923c592d3dac1f29aaa0b20cc63dff6a05156`
- Compiler: VS2022 Community v143 Hostx64/x64, `cl.exe` 14.44.35207
- Qt kit: `E:\Qt\6.10.3\msvc2022_64`
- GStreamer root: `E:\PROGRA~1\GSTREA~1\1.0\MSVC_X~1`
- Build directory: `build\windows-debug`

Root-cause evidence came from a fresh 5-test/2-failure baseline plus a Windows filesystem event trace. `_basic_test` called `_cleanup()` while its cached-data `QFile` was still open. Windows deleted the `.meta` file but retained the open `.cache` file; `_lru_test` and `_multi_test` then reused that orphan entry. The fix is test-only: an inner RAII scope destroys `QTextStream` and `QFile` before `_cleanup()`. No `ComponentInformationCache` production file changed.

The confirmed VS2022 x64 build ran:

```powershell
cmake --build build\windows-debug --target QGroundControl --parallel
```

Build exit code was `0`. The log explicitly contains compilation of `ComponentInformationCacheTest.cc.obj` and linking of `Debug\QGroundControl.exe`.

| Focused suite | Tests | Failures | Errors | Skips | XML time | Process exit |
|---|---:|---:|---:|---:|---:|---:|
| ComponentInformationCacheTest | 5 | 0 | 0 | 0 | 0.381 s | 0 |
| RequestMetaDataTypeStateMachineTest | 9 | 0 | 0 | 0 | 100.437 s | 0 |
| **Total** | **14** | **0** | **0** | **0** | N/A | N/A |

The component test process was PID 18980. Immediately after exit, both `QGCCacheTest_18980_*` and `QGCTestFiles_18980_*` directory counts were zero. Console scans found no cache miss, directory-creation failure, missing DLL/plugin, fatal assertion, or crash marker; each console contained only the expected QML debugging notice.

Full CTest was intentionally not rerun. `RequestMetaDataTypeStateMachineTest` passed independently but took 100.437 seconds, so its existing CTest runner/timeout investigation remains open. Evidence is under `.tmp\phase2b\` and remains untracked.

## Phase 2C Windows Mission editor font environment (2026-07-15)

- Branch/worktree: `codex/windows-test-hardening` at `E:\workspace\QGC\qgroundcontrol-worktrees\windows-test-hardening`
- Font environment commit: `589306c81dcc0d745d9962d6d1fbca312c4e2435`
- Windows timeout commit: `a7f5b7a08558e43480d11c4d7cc3e89dd17f9677`
- Compiler: VS2022 Community v143 Hostx64/x64, `cl.exe` 14.44.35207
- Qt kit: `E:\Qt\6.10.3\msvc2022_64`
- GStreamer root: `E:\PROGRA~1\GSTREA~1\1.0\MSVC_X~1`
- Build directory: `build\windows-debug`

Fresh RED evidence showed `MissionCommandTreeEditorTest` at 3 tests, 1 failure, 0 errors, and 0 skips. The only uncategorized message was Qt's missing-font warning for the absent Qt SDK `lib/fonts` directory. Supplying only `QT_QPA_FONTDIR=%WINDIR%\Fonts` produced a direct 3/0/0/0 run with no font/default-category warning, proving the failure was the Windows offscreen test environment.

`cmake/QGCTest.cmake` now resolves `%WINDIR%\Fonts` once, normalizes it, warns once if unavailable, and appends the validated path to Windows CTest environments. `test/CMakeLists.txt` retains existing non-Windows/global timeout policies and gives only the Windows `MissionCommandTreeEditorTest` a 360-second timeout. Fresh successful direct runs took 197.590 and 209.797 seconds, while the previous registered 180-second timeout stopped CTest at 180.06 seconds.

The final VS2022 x64/Qt configure exited `0`. Generated CTest properties were:

```text
QT_QPA_PLATFORM=offscreen
QT_LOGGING_RULES=*.debug=false
QT_QPA_FONTDIR=C:/WINDOWS/Fonts
TIMEOUT=360.0
```

| Entry | Result | Duration/Time | Exit |
|---|---|---:|---:|
| CTest `MissionCommandTreeEditorTest` | 1/1 passed | 186.68 s | 0 |
| Direct QGC JUnit | 3 tests, 0 failures/errors/skips | 209.797 s | 0 |

The final scans found zero `QFontDatabase`, `Cannot find font directory`, default-category, missing DLL/plugin, fatal assertion, or crash matches. Mission source, Mission tests, and translations were unchanged. Full CTest was intentionally not run. `MissionManagerTest` remains RED at 7 tests / 1 failure because the Simplified Chinese translation `Frame: %1` is currently `框架1`; that independent defect is reserved for Phase 2D. Evidence is under `.tmp\phase2c\` and remains untracked.

## 2026-07-15 Phase 2D pause

The approved catalogue-only correction was not applied during the first Task 1
attempt. The initial plan incorrectly expected a standalone
`MissionManagerTest.exe` in `build\windows-debug-vs2022-zc-fixed`; this linked
worktree instead has a valid VS2022 x64 test-enabled `build\windows-debug`
configuration. Generated CTest metadata proves that `MissionManagerTest` is
linked into `build\windows-debug\Debug\QGroundControl.exe` and must be run as
`--unittest:MissionManagerTest --allow-multiple`. No translation change,
JUnit XML, GREEN run, correction commit, or correction push was produced by
the failed command. The plan was corrected before restarting the mandatory
pre-change RED test.

## 2026-07-15 Phase 2D authoritative verification

The Simplified Chinese catalogue correction `框架：%1` was verified through the
generated CTest registration rather than an ad hoc executable invocation.
With Qt and GStreamer on `PATH`, `QGC_TEST_VERBOSE=1`, and CTest-provided
`QT_QPA_PLATFORM=offscreen`, `QT_LOGGING_RULES=*.debug=false`, and
`QT_QPA_FONTDIR=C:/WINDOWS/Fonts`, the command below exited `0`:

```powershell
ctest --test-dir build\windows-debug -R '^MissionManagerTest$' --output-on-failure --output-junit .tmp\phase2d\ctest-authoritative\MissionManagerTest.xml
```

CTest reported `1/1` passed in 30.24 seconds (32.38 seconds real time). Its
JUnit file is relative to the CTest build directory at
`build\windows-debug\.tmp\phase2d\ctest-authoritative\MissionManagerTest.xml`
and contains one `MissionManagerTest` testcase with no failure or error node.
The CTest console scan found no `QString::arg: Argument missing`,
uncategorized-log, missing DLL/plugin, fatal, or failed-test match. Earlier
direct-run artifacts were generated concurrently and are not acceptance
evidence. Full CTest remains intentionally unrun; the separate `HashCheckTest`,
`SigningTest`, and `RequestMetaDataTypeStateMachineTest` runner investigations
remain open.

## 2026-07-15 Remaining runner verification

The three runner investigations were then repeated one at a time through the
generated CTest registrations in the same VS2022 x64/Qt/GStreamer environment.
All commands used `--output-on-failure` and `--output-junit`; each JUnit file
is relative to `build\windows-debug` and has no failure or error node.

| Test | CTest result | Duration | JUnit failure/error nodes |
|---|---|---:|---|
| `HashCheckTest` | 1/1 passed | 124.14 s | none |
| `SigningTest` | 1/1 passed | 1.18 s | none |
| `RequestMetaDataTypeStateMachineTest` | 1/1 passed | 99.63 s | none |

`HashCheckTest` exceeded the historical 120-second observation but did not
timeout in its current registered CTest configuration. No code or timeout
change is justified by these runs. The known focused Windows test failures are
therefore closed; full CTest remains intentionally unrun after Phase 2D and is
the remaining broader regression check.

## 2026-07-15 Full CTest baseline attempt

After a fresh Debug build completed with VS2022 x64 `VsDevCmd` loaded and
`--parallel 2`, a full 186-test CTest run was started with JUnit and console
output redirected under `.tmp\final-validation`. The shell used only the
compiler path at first and failed to find the MSVC standard header
`type_traits`; loading `VsDevCmd.bat -arch=x64 -host_arch=x64` corrected that
environment issue and the rebuild then exited `0`.

The full CTest run is not a passing baseline. Before the outer 30-minute
execution limit interrupted the run during test 182, CTest completed 181 test
result lines and recorded these failures:

| Test | Result | Duration |
|---|---|---:|
| `MissionControllerTest` | timeout | 120.06 s |
| `InitialConnectTest` | timeout | 120.06 s |

`MissionManagerTest`, `MissionCommandTreeEditorTest`, and
`ComponentInformationCacheTest` passed within the same full run. The remaining
tests from `RequestMetaDataTypeStateMachineTest` onward have no final result
from this attempt. Do not use this run to claim a complete Windows test
baseline or create a downstream development branch; first reproduce the two
timeouts individually, determine whether their registered timeouts are
incorrect or the tests are stalled, then run a completion-capable full CTest
session.

## 2026-07-15 InitialConnect selector localization

The temporary, uncommitted `QGC_TEST_FUNCTION` selector was built only into
the Debug test runner, used to execute every Initial Connect function/data row,
and then removed. The restored runner rebuilt successfully and both temporary
test files have no Git diff.

| Selector group | Count | Result |
|---|---:|---|
| `_performTestCases` data rows | 3 | 3 passed (10.51-16.54 s) |
| Independent functions | 6 | 5 passed; `_boardVendorProductId` crashed |
| `_stateTimeoutFallsThrough` data rows | 5 | 5 passed (3.12-16.21 s) |
| `_stateRunMatrix` data rows | 8 | 8 passed (2.63-11.91 s) |

The only failure was `_boardVendorProductId`: CTest exited with code 8 after
13.60 seconds and classified it as `SEGFAULT`. The Windows exception code was
`0xc0000005`; the captured stack runs through
`LinkManager::_linkDisconnected`, `MockLink::~MockLink`, and
`LinkManager::~LinkManager` during teardown. No selector timed out, so the
class-level 360-second timeout is caused by aggregate/order-dependent behavior
or teardown state, not a single slow row. Evidence is under
`.tmp\phase3-plan\InitialConnect-selector-results.csv` and the corresponding
selector logs/JUnit files; those generated files remain untracked.

## 2026-07-15 Board wait follow-up

The cleanup change was applied in the working tree for diagnosis but is not yet
committed. It removed the previous access violation; the direct QGC JUnit then
reported 24 cases with one failure in `_boardVendorProductId`. The exact
failure was a 5,000 ms `TestTimeout::mediumMs()` wait for
`initialConnectComplete`, while Qt reported that 9,950 ms would have been
sufficient. The next approved design is limited to using the existing bounded
`TestTimeout::longMs()` for this one wait; no code commit or full CTest run is
valid until that follow-up is reviewed and verified.

## 2026-07-15 InitialConnect final focused verification

The approved test-only correction now tracks the custom board MockLink through
the existing `VehicleTest` fixture cleanup path and gives only the
`initialConnectComplete` wait the existing `TestTimeout::longMs()` bound. The
CTest registration uses 360 seconds on Windows while retaining the integration
default on other platforms.

| Verification | Result | Runtime |
|---|---|---:|
| Direct QGC JUnit | exit 0; 24 tests, 0 failures, 0 errors, 0 skipped | 184.833 s |
| Generated CTest entry | exit 0; 1/1 passed, 0 failed | 182.73 s |

The direct stderr contains only the expected QML debugging notice. Scans of
the direct and CTest logs found no `0xc0000005`, `FAIL!`, segmentation-fault,
assertion, or `LinkManager::_linkDisconnected` teardown evidence. Artifacts are
under `.tmp\phase4\initial-connect-final`. A fresh full 186-test CTest run is
still required before downstream branches can be delivered.
