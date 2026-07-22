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
- Phase 2C fresh evidence separated `MissionCommandTreeEditorTest` from the Mission manager translation failure: the editor suite failed only on Qt's uncategorized missing-font warning, then passed 3/0/0/0 when only `QT_QPA_FONTDIR=C:\Windows\Fonts` was supplied. The approved boundary is a Windows CTest environment entry, not a warning whitelist.
- Phase 2C CTest verification proved the registered non-sanitizer timeout was also too short: CTest injected the correct font directory but stopped at 180.06 seconds, while fresh direct GREEN runs took 197.590 and 209.797 seconds. The user approved a Windows-only 360-second timeout for this test without changing global timeouts.
- Phase 2C committed the font environment as `589306c81` and the scoped timeout as `a7f5b7a08`. Final configure properties showed `QT_QPA_FONTDIR=C:/WINDOWS/Fonts` and `TIMEOUT=360.0`; CTest passed 1/1 in 186.68 seconds and direct JUnit evidence was 3/0/0/0 with zero font/default-category matches.
- Phase 2C did not touch Mission source/tests/translations. `MissionManagerTest` remains the sole assertion-failing suite and is reserved for Phase 2D because `Frame: %1` is mistranslated as `框架1` in Simplified Chinese.
- Phase 2D root cause: `PlanManager.cc` calls `tr("Frame: %1").arg(item->frame())`, but the matching `qgc_source_zh_CN.ts` translation is `框架1` and discards `%1`. In a zh-CN process Qt logs `QString::arg: Argument missing` and `MissionManagerTest::_testErrorAckFailureStrings` fails. The approved correction is the catalogue value `框架：%1`; no locale forcing or source-code workaround is allowed.
- Phase 2D execution-path correction: CTest registers `MissionManagerTest` as `build\\windows-debug\\Debug\\QGroundControl.exe --unittest:MissionManagerTest --allow-multiple`; each test is not a standalone executable. The valid existing `build\\windows-debug` cache uses VS2022 v143 x64, Qt 6.10.3, Ninja, and both test options ON. The historical `windows-debug-vs2022-zc-fixed\\MissionManagerTest.exe` plan path was therefore invalid; it was corrected before rerunning RED, and no translation edit occurred during the failed launch.
- Phase 2D Task 1: the approved executable invocation generated zero-byte RED/GREEN logs and JUnit files in this shell, with no observable exit output. The translation correction was applied and the target rebuilt successfully with VSDevCmd and `--parallel 2`; unrestricted parallelism first hit MSVC C1060 heap exhaustion.
- Phase 2D evidence reconciliation: the current direct `MissionManagerTest` XML reports 7 tests / 5 uncategorized-log failures / 0 errors / 0 skips and no `QString::arg: Argument missing`. It was produced while multiple subagents wrote the same `.tmp\\phase2d` directory, alongside zero-byte artifacts; it cannot certify the fix. Use a single executor and the generated CTest environment for the next authoritative run. Do not mark Phase 2D verification complete until that run is clean or the five log sources are separately classified.
- Phase 2D authoritative result: CTest's generated `MissionManagerTest` registration ran with its declared offscreen, logging, and font environment and passed 1/1 in 30.24 seconds. The CTest JUnit output is relative to the build directory, not the source worktree; it contains one testcase with no failure/error node. This supersedes the competing direct-run artifacts and closes the placeholder-failure verification.
- Exact runner rechecks closed the remaining focused investigations without source or timeout changes: `HashCheckTest` passed in 124.14 seconds despite exceeding the historical 120-second observation, `SigningTest` passed in 1.18 seconds, and `RequestMetaDataTypeStateMachineTest` passed in 99.63 seconds. Each emitted CTest JUnit without failure/error nodes. Full CTest remains unrun after Phase 2D.
- Full CTest attempt after Phase 2D: the fresh Debug rebuild required loading VS2022 `VsDevCmd` because a compiler-only `PATH` omitted MSVC standard-library include paths. With the correct environment, CTest recorded `MissionControllerTest` and `InitialConnectTest` timing out at 120.06 seconds each. The outer 30-minute execution limit later interrupted the still-running CTest process during test 182, so no final result exists for the tail of the suite. Treat the two timeouts as unresolved until individually reproduced and diagnosed; do not raise global timeouts to hide them.
- InitialConnect selector evidence: a temporary `QGC_TEST_FUNCTION` runner selector isolated all 22 functions/data rows. Twenty-one passed; `_boardVendorProductId` crashed after 13.60 seconds with access violation `0xc0000005` in the `LinkManager::_linkDisconnected` / `MockLink` destruction path. The selector code was removed and the uninstrumented QGroundControl target rebuilt with exit 0. This is a teardown/lifecycle defect candidate, not evidence for a larger timeout.
- Timeout probe evidence: after temporarily changing only the two generated build-tree CTest properties from 120 to 360 seconds, `MissionControllerTest` passed in 133.82 seconds with clean CTest JUnit, while `InitialConnectTest` timed out again at 360.04 seconds with a timeout failure node. The generated file was restored to 120 seconds and Git remained clean. Current `CI`/`GITHUB_ACTIONS` are unset, so internal long waits are 30 seconds. Use function/data-row localization before any durable Initial Connect timeout or source correction.
- Board teardown design: `_boardVendorProductId` directly calls `LinkManager::disconnectAll()` after creating a custom MockConfiguration and never tracks the MockLink in the `VehicleTest` fixture. The approved fix boundary is to assign `_mockLink` and call `_disconnectMockLink()` so vehicle removal and event-loop settling complete before local configuration release.
- Board wait follow-up: after the cleanup change removed the access violation, direct JUnit reported one `_boardVendorProductId` failure because `initialConnectComplete` used `TestTimeout::mediumMs()` (5000 ms) while 9950 ms was needed. The bounded, test-only follow-up is `TestTimeout::longMs()` for that wait; the source remains uncommitted until review.
- Board wait verification: after assigning the custom link to `_mockLink`, using `_disconnectMockLink()`, and changing only the initial-connect wait to `TestTimeout::longMs()`, direct QGC JUnit exited 0 with 24 tests / 0 failures / 0 errors in 184.833 seconds. Generated CTest then exited 0 and passed 1/1 in 182.73 seconds under the registered Windows-only 360-second limit. Neither log contained `0xc0000005`, `FAIL!`, segmentation-fault, or assertion evidence.
- Completion-capable full CTest exited 8 after all 186 registrations in 1936.82 seconds: 182 passed and 4 failed. `VehicleCameraControlTest` and `MissionControllerTreeTest` timed out at 120.05 seconds; direct JUnit later passed 15/15 in 161.519 seconds and 11/11 in 123.426 seconds. `ParameterManagerTest` directly reproduced 3/14 failures waiting one second for `loadProgressChanged`. `VehicleLinkManagerTest` directly reproduced 3/7 failures waiting five seconds for initial connect; Qt reported 6.1, 9.95, and 10.05 seconds would have sufficed. These four test sources match `origin/master`.
- Final Windows Debug gate: after scoped Windows registration limits and established positive-wait bounds were applied, the focused suites passed and the background-controlled CTest completed 186/186 with exit 0 in 757.36 seconds (wrapper 757.445 seconds). Stderr was empty. The only raw `Timeout` scan matches were the passing `TimeoutTransitionTest` name; after excluding that name, the actionable `Timeout|FAIL!|Segmentation fault|ASSERT|0xc0000005|Errors while running CTest` scan contained zero matches.
- Delivery: final review passed after adding the validated implementation head to the environment report. Normal pushes created `codex/joystick-aux-px4-development` and `change1_crocodile_2cda0d3cf`; no force-push or PR was used. The protected integration ref remained `daae3a37bbdd186ac42958be06567c342e4a0a5b`.

2026-07-22:
- The user selected `codex/joystick-aux-px4-development` at `754135601` as the Quad-Rover adaptation base; it intentionally retains prior downstream development and is not to be rebased onto upstream QGC main during this task.
- Verified the immutable MAVLink release contract with `git ls-remote`: tag `hybrid-change1-v1.16.1` peels to `3b84efb97a7c0b4767868e8725bd6902c0d884e8`, and maintenance branch `change1_v1.16.1-hybrid` resolves to the same commit. Release builds must use the tag, not the branch.
- Created clean worktree `E:\workspace\QGC\qgroundcontrol-worktrees\quad-rover-design` on `codex/quad-rover-design`; it was clean before the approved design specification was added.
- The previous agent guide incorrectly required bypassing QGC's command queue. Source inspection proved `MavCommandQueue` retains `MAV_RESULT_IN_PROGRESS` and supplies complete ACKs to progress handlers; the new controller must add sequence/state semantics without duplicating the queue.
- PX4 `decideTransition` proves that false actuator-health/online diagnostic flags are not independent command gates. QGC must display them without synthesizing a fault; PX4 ACKs and transformation fault state remain authoritative.
- PX4 increments `transition_sequence` only for a new started transition. Same-target stable requests return direct `ACCEPTED` without a new sequence; exact equality, not numeric sequence ordering, is required for a controller transaction.
- `MavCommandQueue` cannot own the long-running hybrid ACK lifecycle unchanged: after `IN_PROGRESS` it sets one retry window of about 1200 ms, while PX4's default `HYBRID_TRANS_T` is 6 s and PX4 does not emit periodic progress ACKs. The approved design therefore uses a generalized detach-on-progress policy for initial reliable dispatch, then routes detached terminal ACKs and status handling to the Hybrid controller.
- Design self-review added the missing ACK-routing boundary: the generic queue matches a pending entry by command and sending component only. `Vehicle` supplies system-id filtering, while the hybrid matcher validates the original autopilot component, QGC ACK address, and exact optional `uint32` transition sequence. Direct terminal ACKs before progress must use that same matcher.
- Post-commit critical review corrected the release and state contracts: a raw `hybrid_vehicle` dialect would drop QGC's default APM dialect/plugin compatibility, so QGC requires a new `qgc_hybrid` composite dialect that includes `all.xml` plus `hybrid_vehicle.xml`. The currently protected tag does not contain it; PX4/MAVLink must publish a new immutable release and update the guide before QGC configuration.
- Airframe `22001_quad_rover` sets `HYBRID_TRANS_T=2.0`, whereas the module fallback is 6.0. QGC must not encode either value. Its status fallback needs exact sequence, stable requested current shape, `MAV_RESULT_ACCEPTED`, monotonic status ordering, and `command_timestamp` association because PX4's command-result field can be overwritten by another command.
- `VehicleClassQuadRover` remains distinct, so PX4's existing `other` flight-mode path cannot be reused. A mode-only effective shape profile selects existing multicopter modes in stable Quad, a narrow custom-mode allowlist in stable Rover, and no shape-dependent mode controls during unavailable/superseded states.
