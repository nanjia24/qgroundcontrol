# LOG

2026-07-06:
- Current workspace E:\workspace\QGC was not a Git repository and was empty; cloned https://github.com/nanjia24/qgroundcontrol.git into qgroundcontrol.
- Target branch codex/joystick-aux-px4 switched and pulled successfully; created local branch codex/windows-desktop-env-setup for report/state work.
- git submodule status failed because Git helper Unix tools basename/sed/git-sh-setup were not found; .gitmodules is absent, so no recursive submodule work is needed.
- Current shell did not have cl on PATH; VS BuildTools 18 at C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools can load x64 MSVC via VsDevCmd.
- Required Qt C:\Qt\6.10.3\msvc2022_64 is missing. Existing E:\Qt\6.9.0\msvc2022_64 was not used because it is below the requested version.
- Qt MaintenanceTool headless search hit cache permission/network/license/login blockers. No Qt credentials were requested or stored.
- QGC GStreamer dependency script failed through QGC S3 mirror with HTTP 403 after using explicit C:\Python314\python.exe.
- Installed Python httpx in user environment to satisfy the dependency script download path.
- Chocolatey NSIS install failed because the command was not in an elevated Administrator shell and could not access C:\ProgramData\chocolatey paths.



2026-07-06 continuation:
- Verified Qt 6.10.3 at E:\Qt\6.10.3\msvc2022_64 with required modules.
- Retried QGC GStreamer dependency script with and without current-process 127.0.0.1:7897 proxy; QGC S3 still returned HTTP 403.
- Found robust configure environment: load VS DevCmd, then run Git Bash script with explicit MSVC/SDK/Qt/CMake/Ninja PATH. This fixes Git submodule helper failures.
- CMake progressed to GStreamer and failed on CMake downloader SSL connect error.
- Python/httpx successfully downloaded GStreamer official installer to CPM cache.
- GStreamer installer failed silent Inno args with exit 1, /S hung without producing SDK. Visible installer was launched for manual completion.



2026-07-06 final:
- Verified GStreamer 1.28.1 installed at E:\Program Files\gstreamer\1.0\msvc_x86_64.
- Used 8.3 short path E:\PROGRA~1\GSTREA~1\1.0\MSVC_X~1 to avoid pkg-config/CMake path splitting on spaces.
- Disabled ccache with -DQGC_USE_CACHE=OFF due ccache permission error at C:\Users\86178\AppData\Local\ccache/tmp.
- Full Debug build with tests ON failed compiling MissionManager tests due MSVC __VA_OPT__ syntax errors.
- App-only Debug build with -DQGC_BUILD_TESTING=OFF succeeded and QGroundControl.exe launched/stayed alive for 20 seconds.
- CTest entry ran but failed because tests were not built; QmlQuickTests exit code 0xc0000135.



2026-07-07:
- Verified VS2022 Community v143 x64 toolchain path and cl 19.44.35228.
- Reproduced __VA_OPT__ failure with VS2022 when /Zc:preprocessor was absent.
- Added /Zc:preprocessor as a CMake command-line flag, not a source edit.
- Full Debug build with tests ON succeeded in build\windows-debug-vs2022-zc2.
- CTest completed: 175/186 passed, 11 failed.
- No source changes, commits, pushes, or PRs.


2026-07-13:
- `apply_patch` failed in this Windows sandbox with `orchestrator_helper_launch_canceled`; documentation/CMake edits in this continuation were applied with constrained PowerShell writes and verified with `git diff`.
- Fixed MSVC test compile blocker properly by adding `/Zc:preprocessor` to `cmake/platform/Windows.cmake` instead of relying on command-line `CMAKE_CXX_FLAGS`.
- Verified clean Debug test build in `build\windows-debug-vs2022-zc-fixed`; `build.ninja` contains `/Zc:preprocessor` and build produced `Debug\QGroundControl.exe`.
- QGC unittest XML output rewrites `Test.xml` to `Test-<QObjectName>.xml`; initial direct existence checks for the base XML path were misleading.
- `MissionCommandTreeEditorTest` needed a longer standalone harness timeout to produce XML; rerun with 360s completed in 93.1s and showed a strict-log failure.
- Same 11-test failure pattern reproduces on `origin/master` under the same environment. Current evidence does not attribute these failures to the joystick branch.
- Release generation truncates the long GStreamer root under `E:\Program Files` to `E:/Program`; use `E:\PROGRA~1\GSTREA~1\1.0\MSVC_X~1` in this environment.
- Remove inherited `E:\msys64\mingw64\bin` only from the build subprocess PATH to prevent GNU/MinGW tool detection during the MSVC configuration.
- Codex CLI full access does not imply Windows UAC elevation. The verified token was medium-integrity/non-administrator, and the symlink test remained failed.
- The isolated `codex/windows-test-hardening` baseline reproduced the same 8 failing suites and 11 assertion failures after a clean Debug build.
- `QGCFileHelper::isLocalPath` treats a Windows drive path such as `C:/...` as URL scheme `c`; this explains the empty archive path in `QGCArchiveModelTest` and is a production portability issue, not only a test assertion issue.
- `PlatformTest` expects `QT_ASSUME_STDERR_HAS_CONSOLE` and `QT_FORCE_STDERR_LOGGING` on Windows although `Platform::initialize` sets them only under `Q_OS_UNIX`.
- `JsonHelperTest` asserts untranslated English text while the production error is translated, making the test locale-dependent.

2026-07-14:
- SDD Task 1 was dispatched twice to fresh subagents. Both remained in `running` state without starting QGroundControl/Ninja/CMake, changing files, writing the required report, or responding to a status request; both were interrupted after bounded waits.
- This is an agent execution-channel blocker, not evidence of a JsonHelper implementation or build failure. No Task 1 source change or commit was produced.
- A third fresh dispatch succeeded after the suspected network fluctuation; subsequent SDD tasks completed with implementer and independent review gates.
- Phase 1 fixed four confirmed root causes in focused commits: locale-dependent JSON text, Unix-only Platform assertions on Windows, GeoTag path/file semantics, and Windows absolute path recognition in `QGCFileHelper`.
- Final Phase 1 focused regression parsed 118 tests across 5 suites with 0 failures, 0 errors, and 0 skips. Console logs contained only the expected QML debugging notice.
- Phase 2 still has 4 failing suites / 6 assertions; Phase 1 did not suppress or skip them.
- Final branch review found no code Critical/Important issues. State documentation was reconciled to distinguish the 4 assertion-failing suites from 3 CTest runner investigations and to remove obsolete branch/permission guidance.
- Phase 2A root cause: `QFile::link` is not a reliable Windows directory-symlink capability probe because it can create a shortcut rather than a real symlink.
- Phase 2A root cause: extraction constructs a valid Unicode `QString` output path but passes it to Windows `archive_write_disk` through narrow UTF-8 `archive_entry_set_pathname`; libarchive exposes a wide pathname API for this boundary.
- Phase 2A spec review clarified the symlink verification boundary: inability to verify a real link is an evidence-bearing environment skip after successful cleanup; inability to clean up the probe artifact remains a test failure.
- Phase 2A RED evidence showed that `QSKIP` inside a `void` helper records a skip but only returns from the helper; the caller then continued and failed `isSymLink()`. The user approved a boolean helper result plus immediate caller return on 2026-07-14.
- This Chinese-localized MSVC environment emitted localized `/showIncludes` text that Ninja did not parse for `QGCCompressionTest.cc.obj`, leaving `#deps 0`. Header-only test changes must verify actual object recompilation and may require safe removal of the single stale object inside the build root.
- Phase 2A committed a real Windows directory-symlink probe (`791125bb1`) and a wide libarchive output-entry pathname fix (`569943094`). The authoritative forced builds exited 0 and explicitly compiled the affected objects before linking QGroundControl.
- Final Phase 2A focused regression parsed 169 tests across 5 suites with 0 failures, 0 errors, and 1 expected error 1314 capability skip. `_testUnicodePaths` passed all existing scripts; console scans found only the expected QML debugging notice.
- Phase 2B fresh baseline reproduced `ComponentInformationCacheTest` with 5 tests, 2 failures, 0 errors, and 0 skips. A Windows filesystem event trace proved `_basic_test` called `_cleanup()` while its cached `QFile` remained open: the `.meta` was deleted but the open `.cache` remained, causing `_lru_test` and `_multi_test` to reuse an orphan entry.
- The approved Phase 2B boundary is test-only RAII lifetime correction. Production orphan-cache recovery is explicitly out of scope.
- Phase 2B committed the one-file RAII correction as `22d923c59`. The VS2022 x64 build exited 0 and explicitly compiled `ComponentInformationCacheTest.cc.obj` before relinking QGroundControl.
- Final Phase 2B focused regression parsed 14 tests across `ComponentInformationCacheTest` and `RequestMetaDataTypeStateMachineTest` with 0 failures, 0 errors, and 0 skips. The cache test left no PID-specific fixture directory; the state-machine suite took 100.437 seconds, so its CTest runner investigation remains open.
