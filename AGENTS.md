【人类编写区域】

# AGENTS.md

Instructions for AI coding agents (Codex, Claude Code, etc.) working on QGroundControl.

## Quick References

- [CODING_STYLE.md](CODING_STYLE.md) — Naming, formatting, C++20 features, QML style, logging
- [.github/CONTRIBUTING.md](.github/CONTRIBUTING.md) — Architecture patterns (Fact System, Multi-Vehicle, FirmwarePlugin)
- [tools/README.md](tools/README.md) — Development scripts and tooling
- [test/TESTING.md](test/TESTING.md) — Test framework, base classes, CTest labels, MultiSignalSpy, coverage
- [.pre-commit-config.yaml](.pre-commit-config.yaml) — All enforced linters (clang-format, clang-tidy, ruff, pyright, shellcheck, actionlint, zizmor, qmllint, clazy, vehicle-null-check, check-no-qassert)

## Review Process

Your output will be reviewed by another AI agent before being accepted. Write code and commit messages that are easy to machine-review: keep changes focused and minimal, use clear naming, and leave explanatory commit messages. Avoid unrelated changes, commented-out code, or ambiguous TODOs.

## Critical Files (Read First!)

1. `src/FactSystem/Fact.h` - Parameter system foundation
2. `src/Vehicle/Vehicle.h` - Core vehicle model
3. `src/FirmwarePlugin/FirmwarePlugin.h` - Firmware abstraction

## Build & Test Commands

See [tools/README.md](tools/README.md) for the full tooling reference and [test/TESTING.md](test/TESTING.md) for the complete testing guide.

```bash
# Configure (Release)
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build build --config Release --parallel

# Run unit tests
cd build && ctest --output-on-failure -L Unit --parallel $(nproc)

# Lint (pre-commit hooks)
pre-commit run --all-files

# Format C++
clang-format -i path/to/changed/files.cc

# CI Python script tests
cd .github/scripts && PYTHONPATH=. python3 -m pytest tests/ -q

# Tools Python tests
cd tools && uv run --extra scripts --extra test pytest tests/ -q
```

## Golden Rules

1. **Fact System**: ALL vehicle parameters use Facts. Never create custom parameter storage.
2. **Multi-Vehicle**: ALWAYS null-check `MultiVehicleManager::instance()->activeVehicle()`
3. **Firmware Plugin**: Use `vehicle->firmwarePlugin()` for firmware-specific behavior.
4. **QML Sizing**: Use `ScreenTools.defaultFontPixelHeight/Width`, never hardcoded values.
5. **QML Colors**: Use `QGCPalette`, never hardcoded colors.
6. **Match existing style**: Follow conventions of surrounding code. See CODING_STYLE.md.

## Code Structure

```
src/
├── Vehicle/          # Vehicle state/comms
├── FactSystem/       # Parameter management
├── FirmwarePlugin/   # PX4/ArduPilot abstraction
├── AutoPilotPlugins/ # Vehicle setup UI
├── MissionManager/   # Mission planning
├── MAVLink/          # Protocol handling
├── QmlControls/      # Reusable QML components
└── Settings/         # Persistent settings
```

## CI Structure

Platform workflows (`linux.yml`, `macos.yml`, `windows.yml`, `android.yml`, `ios.yml`) share logic via composite actions and reusable workflows.

```
.github/
├── workflows/
│   ├── linux.yml, macos.yml, windows.yml  # Platform build + test
│   ├── android.yml, ios.yml               # Mobile builds
│   ├── _detect-changes.yml                # Reusable: skip builds on unrelated PRs
│   ├── build-results.yml                  # Aggregate PR comment (workflow_run trigger)
│   ├── build-gstreamer.yml                # GStreamer SDK builds
│   ├── custom-build.yml                   # Custom build validation
│   ├── docker.yml                         # Docker image builds
│   ├── pre-commit.yml                     # Linting and formatting checks
│   ├── check-links.yml                    # Markdown link validation
│   ├── ci-scripts.yml                     # CI Python script tests
│   ├── analysis.yml                       # Static analysis
│   ├── performance.yml                    # Performance benchmarks
│   ├── pr-checks.yml                      # PR validation checks
│   ├── release.yml                        # Release automation
│   ├── docs_deploy.yml, doxygen_deploy.yml  # Documentation deployment
│   ├── cache-cleanup.yml, cache-cleanup-pr.yml  # Cache maintenance
│   ├── crowdin.yml, lupdate.yml           # Translation workflows
│   ├── dependency-review.yml              # Dependency security review
│   ├── scorecard.yml                      # OpenSSF Scorecard
│   ├── flatpak.yml                        # Flatpak builds
│   ├── px4-metadata.yml                   # PX4 metadata sync
│   ├── stale.yml                          # Stale issue/PR management
│   └── welcome.yml                        # New contributor welcome
├── actions/
│   ├── cmake-configure/                   # CMake configure with consistent options
│   ├── cmake-build/                       # Build with timing, reviewdog, ccache
│   ├── run-unit-tests/                    # CTest runner with JUnit output
│   ├── detect-changes/                    # Path-based change detection per platform
│   ├── attest-and-upload/                 # SBOM attestation + artifact upload
│   ├── attest-sbom/                       # SBOM generation and attestation
│   ├── deploy-docs/                       # Deploy built docs to external repo
│   ├── gstreamer/                         # Build GStreamer from source
│   ├── setup-python/                      # Python + uv + dependency installation
│   ├── qt-install/                        # Qt SDK installation with caching
│   ├── qt-android/, qt-ios/               # Mobile Qt setup
│   ├── coverage/                          # Code coverage reports
│   ├── test-report/                       # Test result publishing
│   ├── test-duration-report/              # Test timing analysis
│   ├── benchmark-runner/                  # Performance benchmark runner
│   ├── size-analysis/                     # Binary size tracking
│   ├── collect-artifact-sizes/            # Artifact size collection
│   ├── download-all-artifacts/            # Cross-workflow artifact download
│   ├── build-config/                      # Read build-config.json values
│   ├── build-setup/                       # Common build environment setup
│   ├── build-action/                      # Unified build action
│   ├── cache/                             # Caching helpers
│   ├── custom-build/                      # Custom build support
│   ├── docker/                            # Docker build helpers
│   ├── install-dependencies/              # Platform dependency installation
│   ├── aws-upload/                        # AWS S3 upload
│   ├── playstore/                         # Google Play Store upload
│   ├── upload/                            # Generic artifact upload
│   ├── verify-executable/                 # Post-build executable verification
│   └── common/                            # Shared action utilities
├── scripts/                               # Python scripts for CI jobs
│   ├── templates/                         # Jinja2 templates (build_results.md.j2)
│   └── tests/                             # Tests for CI scripts (pytest)
└── build-config.json                      # Centralized version numbers
```

### CI Conventions

- **Dependencies**: CI Python scripts use `httpx` for GitHub API access and `jinja2` for templating. Deps managed in `tools/pyproject.toml` under `[project.optional-dependencies] scripts`.
- **Shared helpers**: `gh_actions.py` provides GitHub API pagination (httpx) with `gh` CLI fallback. Import as `from common.gh_actions import ...`.
- **Bootstrap scripts** (`install_dependencies.py`, `ccache_helper.py`): Use stdlib only — they run before dependencies are installed.
- **Config**: Version numbers and build settings live in `.github/build-config.json`. Read via `common.build_config.get_build_config_value()`.
- **Outputs**: Use `common.gh_actions.write_github_output()` for `$GITHUB_OUTPUT` writes.

## Quick Patterns

```cpp
// Always null-check vehicle
Vehicle* vehicle = MultiVehicleManager::instance()->activeVehicle();
if (!vehicle) return;

// Access parameters via Fact System
Fact* param = vehicle->parameterManager()->getParameter(-1, "PARAM_NAME");
if (param) param->setCookedValue(newValue);
```

```qml
// QML vehicle access
property var vehicle: QGroundControl.multiVehicleManager.activeVehicle
enabled: vehicle && vehicle.armed
```

---

**Key Principle**: Match the style of code you're editing. See CODING_STYLE.md for conventions.

【客观事实区域】

- Release 构建中，位于 `E:\Program Files` 的 GStreamer 必须使用已验证短路径 `E:\PROGRA~1\GSTREA~1\1.0\MSVC_X~1`；长路径可能被 CMake imported target 截断为无效的 `E:/Program`。
- Codex CLI full 工作区权限不等于 Windows UAC 提权；判断符号链接或受保护资源测试前，必须验证进程完整性级别和管理员角色。

- 2026-07-13 Windows Desktop 验证环境：VS2022 Community v143 x64 (`D:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207\bin\Hostx64\x64\cl.exe`), Qt `E:\Qt\6.10.3\msvc2022_64`, GStreamer `E:\Program Files\gstreamer\1.0\msvc_x86_64`, NSIS `C:\Program Files (x86)\NSIS\makensis.exe`。
- MSVC/QGC 全量 Debug 测试构建需要启用 `/Zc:preprocessor`；否则 VS2022 v143 下也会因 `__VA_OPT__` 触发 `warning C5109` 并在 warnings-as-errors 下导致 `C2146` 等编译失败。
- `build/windows-debug-vs2022-zc-fixed` 已验证：不手动传 `-DCMAKE_CXX_FLAGS=/Zc:preprocessor` 也能完成全量 Debug 测试构建，原因是该选项已固化到 `cmake/platform/Windows.cmake`。
- 11 个 Windows 单测失败在同环境 `origin/master` 对照 worktree 中复现；当前证据不支持将这些失败归因于 `codex/joystick-aux-px4` 的 joystick 改动。
- 中文本地化 MSVC 的 `/showIncludes` 输出可能未被 Ninja 解析，已观察到 `QGCCompressionTest.cc.obj` 为 `#deps 0`；头文件改动后普通增量构建可能错误报告 `no work to do`。
- Windows 上打开的 `QFile` 会阻止 `QDir::removeRecursively()` 删除对应文件；`ComponentInformationCacheTest` 曾因此只删除 `.meta` 而遗留 `.cache`，污染后续 LRU 测试。
- Windows offscreen Qt tests require `QT_QPA_FONTDIR` pointing to the existing `%WINDIR%\Fonts` directory; otherwise `MissionCommandTreeEditorTest` captures an uncategorized `QFontDatabase` missing-font warning.
- `MissionCommandTreeEditorTest` has verified Windows offscreen runtimes above 180 seconds (197.590 and 209.797 seconds); its registered Windows CTest timeout must be 360 seconds while unrelated tests retain their existing timeouts.

【项目规范区域】

- Windows/MSVC 桌面构建优先使用 VS2022 v143 x64 + Ninja + CMake + Qt `msvc2022_64`，不要用 MinGW 或 qmake 替代。
- QGC Windows 单测采集详细结果时，使用 `--unittest-output:<xml> --log-output --logging:<rules>` 和 `QGC_TEST_VERBOSE=1`；不要传 `-- -v2`，当前 QGC 命令行解析会拒绝该 positional argument。
- QGC unittest XML 会把 `Test.xml` 改写成 `Test-<QObjectName>.xml`，检查结果文件时必须匹配实际生成文件名。
- 修改测试头文件后必须从构建日志确认受影响对象实际重编；若 Ninja 记录 `#deps 0`，先验证对象绝对路径位于目标 build 根目录，再仅删除该陈旧对象并重新构建。
- Windows 测试在递归删除 fixture 目录前，必须通过 RAII 结束所有 `QFile`/流对象生命周期；不要依赖删除打开文件的 Unix 语义。
- Windows CTest environment paths must derive from `%WINDIR%`, normalize with CMake, and never hard-code `C:\Windows` or whitelist the resulting font warning.
- Long-test timeout corrections must be scoped to the proven test/platform pair; do not raise shared timeout variables to mask one Windows offscreen runtime.
