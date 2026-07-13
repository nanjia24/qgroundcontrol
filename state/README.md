# State README

Project: QGroundControl Windows desktop environment setup.
Repository: E:\workspace\QGC\qgroundcontrol.
Target integration branch: codex/joystick-aux-px4.
Local working branch for environment/report files: codex/windows-desktop-env-setup.

Key constraint: do not modify business source code, do not commit, do not push, and do not store credentials.


## 2026-07-13 status

- `/Zc:preprocessor` was codified in `cmake/platform/Windows.cmake` for MSVC Windows builds.
- Clean Debug build `build/windows-debug-vs2022-zc-fixed` succeeds without manually passing `CMAKE_CXX_FLAGS`.
- Failed-test details are stored in `.tmp/test-runs-zc-fixed`.
- `origin/master` comparison worktree is `E:\workspace\QGC\qgroundcontrol-origin-master`; same 11-test behavior was reproduced there.
- Current evidence indicates the 11 test failures are not introduced by `codex/joystick-aux-px4`.
- Release configure/build now succeeds in `build/windows-release`; the executable ran for a 20-second observation window with empty stdout/stderr.
- NSIS 3.12 is available, but install/package generation was intentionally not run.
- The 11 targeted classes were rerun under Codex full access with unchanged assertion patterns. The Windows token remained medium-integrity and non-administrator.
