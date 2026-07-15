# Full CTest Tail Failures Design

## Context

The first completion-capable Windows Debug CTest run finished all 186
registrations in 1936.82 seconds. It passed 182 tests and failed four. Focused
direct JUnit runs separated two CTest registration timeouts from two internal
test wait failures. The four affected test source files match `origin/master`,
so the failures are not introduced by the joystick feature branch.

## Evidence

| Test | Full CTest | Direct JUnit |
|---|---|---|
| `VehicleCameraControlTest` | timeout at 120.05 s | 15/15 passed in 161.519 s |
| `ParameterManagerTest` | failed in 109.43 s | 11/14 passed; three 1 s progress waits expired |
| `MissionControllerTreeTest` | timeout at 120.048 s | 11/11 passed in 123.426 s |
| `VehicleLinkManagerTest` | failed in 46.458 s | 4/7 passed; three 5 s initial-connect waits expired |

`ParameterManagerTest` fails in `_noFailure`,
`_requestListMissingParamSuccess`, and `_requestListMissingParamFail` while
waiting for `ParameterManager::loadProgressChanged` with
`TestTimeout::shortMs()` (1,000 ms). The no-response test uses the same bound
to prove that no progress signal occurs; that negative assertion is not
failing and must remain unchanged.

`VehicleLinkManagerTest` fails initial-connect waits in `_simpleLinkTest`,
`_simpleCommLossTest`, and `_multiLinkSingleVehicleTest`. Qt reported that
6,100, 9,950, and 10,050 ms would have been sufficient. A fourth test uses the
same initial-connect invariant and bound, so all four equivalent waits should
use one consistent bound.

## Design

### CTest registration timeouts

Keep existing non-Windows behavior. On Windows only:

- register `VehicleCameraControlTest` with a 360-second timeout;
- register `MissionControllerTreeTest` with a 180-second timeout.

The camera test's measured 161.519-second runtime leaves too little margin
under a 180-second limit, so it uses 360 seconds. The mission tree test needs
only the existing extended 180-second class because its measured runtime is
123.426 seconds. Shared timeout constants and unrelated registrations remain
unchanged.

### Parameter progress waits

Change only the three positive `loadProgressChanged` waits from
`TestTimeout::shortMs()` to `TestTimeout::mediumMs()`. This restores the
previous five-second behavioral allowance removed by the upstream test
hardening change while preserving the signal and value assertions. Do not
change the negative no-progress wait or parameter-ready waits.

### Vehicle initial-connect waits

Change the four equivalent `VehicleLinkManagerTest` initial-connect waits from
`TestTimeout::mediumMs()` to `TestTimeout::longMs()`. This matches the proven
InitialConnect test boundary and covers the observed 10.05-second requirement
without arbitrary sleeps. Existing boolean conditions, signal spies, cleanup,
and link behavior remain unchanged.

## Scope Boundaries

- No production source changes.
- No global timeout increase.
- No skipped tests, warning whitelist, retries, or failure suppression.
- No MinGW, qmake, Android, dependency, or toolchain changes.
- No changes to non-Windows CTest registration behavior.
- No downstream development branch until the full suite passes 186/186.

## Verification

1. Rebuild the existing VS2022 v143 x64 Debug test runner and confirm the
   affected objects compile and `QGroundControl.exe` relinks.
2. Verify generated CTest properties: camera 360 seconds on Windows, mission
   tree 180 seconds, and unrelated registrations unchanged.
3. Run direct JUnit plus generated CTest for all four affected suites using the
   established offscreen, font, Qt, and GStreamer environment.
4. Require zero direct JUnit failures/errors and 4/4 focused CTest passes.
5. Run a fresh interruption-safe full CTest and require exit code 0 with
   186 tests, zero failures, and zero errors.
6. Update environment/state evidence, commit and push the hardening result,
   then create the requested downstream branches.

## Delivery

After the 186/186 gate, create and push
`codex/joystick-aux-px4-development` from the verified hardening head. Derive
the development suffix at delivery time with:

```powershell
$version = (git describe --tags --always).Trim() -replace '[^0-9A-Za-z._-]', '_'
```

Create and push `change1_crocodile_<version>` from the development branch.
Do not force-push, modify `codex/joystick-aux-px4`, or create a pull request.
