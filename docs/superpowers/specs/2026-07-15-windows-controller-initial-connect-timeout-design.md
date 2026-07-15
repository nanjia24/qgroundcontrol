# Windows Controller and Initial Connect Timeout Design

## Goal

Close the two timeout failures found by the post-Phase-2 full Windows CTest
run without raising shared timeout policy or hiding a stalled test. A complete
full CTest pass is required before downstream development branches are created.

## Evidence

The full Debug CTest attempt registered both tests as ordinary Integration
tests with `TIMEOUT 120`:

- `MissionControllerTest` timed out at 120.06 seconds.
- `InitialConnectTest` timed out at 120.06 seconds.

A reversible generated-metadata probe changed only those build-tree timeout
properties to 360 seconds. The source tree and CMake inputs were unchanged,
and the generated file was restored afterward.

- `MissionControllerTest` passed in 133.82 seconds. Its CTest JUnit had no
  failure or error node.
- `InitialConnectTest` still timed out at 360.04 seconds. Its CTest JUnit
  contained a timeout failure.

The current local environment does not set `CI` or `GITHUB_ACTIONS`, so the
test framework uses its local 30-second long-wait value rather than the
CI-specific 60-second value. The `InitialConnectTest` class contains several
independent test functions plus data-driven matrices; aggregate runtime and
cross-case lifecycle contamination must be separated from a single stalled
case before any durable timeout change is considered.

## Options Considered

1. Use a scoped timeout for the proven-slow Mission controller test and
   isolate Initial Connect functions/data rows before fixing it (chosen).
   This matches the prior Windows-only Mission editor correction and preserves
   evidence about real stalls.
2. Raise `QGC_TEST_TIMEOUT_INTEGRATION` globally. This would change every
   Integration test and mask unrelated regressions, so it is rejected.
3. Assign 360 or 540 seconds to `InitialConnectTest` immediately. The test did
   not finish within 360 seconds, so this would be guesswork and is rejected.
4. Split `InitialConnectTest` immediately. Splitting may be appropriate if
   isolated rows all pass and aggregate runtime alone exceeds the limit, but
   doing it before localization could preserve a lifecycle bug in smaller
   suites.

## Design

### MissionControllerTest

On Windows only, register `MissionControllerTest` with the existing normal
extended timeout value of 180 seconds. Non-Windows registrations and shared
Unit, Integration, Slow, and default timeout values remain unchanged.

The acceptance run must use the generated CTest registration and pass within
180 seconds with no JUnit failure/error node. A result close enough to 180
seconds to lack practical margin must reopen the timeout choice rather than
silently increasing it.

### InitialConnectTest Localization

Use a temporary, uncommitted test-runner instrumentation patch that reads an
optional `QGC_TEST_FUNCTION` environment variable only when one registered
test class is selected. When set, `UnitTest::run` appends that value after
QTest's existing program-name argument as the function selector. QTest
data tags such as `_stateRunMatrix:HL_0_LR_0_Fly_0` remain supported by the
standard selector syntax.

The instrumentation is diagnostic only:

- It must not change production code or command-line parsing.
- It must return a diagnostic error when the variable is set without exactly
  one registered test class selected.
- It must be removed before the durable fix is committed unless a separate
  review explicitly approves it as a permanent test-framework feature.
- Each function and data row must run through CTest with the same offscreen,
  logging, font, Qt, and GStreamer environment as the failing registration.

The localization result determines the durable correction:

- If one function or data row stalls, fix only the proven test fixture
  lifecycle, cleanup, signal ordering, or bounded wait responsible for that
  row. Production initial-connect behavior is out of scope unless evidence
  proves the defect originates there and the user approves that expansion.
- If every isolated row passes and their measured sum explains the aggregate
  runtime, split the monolithic test registration into independently bounded
  suites rather than assigning an arbitrary multi-minute timeout.
- If runtime depends on execution order, identify and fix the state leaked by
  the preceding row, then prove the failing order and isolated rows both pass.

No warning whitelist, skipped row, forced locale, global timeout increase, or
unbounded wait is acceptable.

## Verification

1. Rebuild with VS2022 `VsDevCmd` loaded, x64 host/target, Qt 6.10.3, Ninja,
   and the validated GStreamer short path.
2. Run the Windows `MissionControllerTest` CTest registration and capture CTest
   JUnit plus console output.
3. Run every Initial Connect function/data row using the temporary selector,
   recording exit code, elapsed time, JUnit counts, and the last successful
   selector before any failure.
4. Remove the instrumentation and verify the Git diff contains only the
   approved durable timeout and evidence-backed Initial Connect correction.
5. Run focused CTest for both suites with no failure/error nodes or prohibited
   console diagnostics.
6. Run all 186 CTest registrations to completion in a background wrapper that
   writes an exit-code file, so the controller's 30-minute command limit cannot
   terminate CTest. Full CTest must report zero failed tests.

## Delivery Gate

Only after the fresh Debug build and full CTest both exit zero may the result
be recorded as a verified Windows development baseline. Then:

1. Create and push `codex/joystick-aux-px4-development` from the verified
   hardening commit.
2. Read the QGC development version from the repository's authoritative
   version source.
3. Create and push `change1_crocodile_<version>` from
   `codex/joystick-aux-px4-development`, using the version text as a valid Git
   ref suffix joined by an underscore.

Do not force-push, rewrite `codex/joystick-aux-px4`, or create a PR.
