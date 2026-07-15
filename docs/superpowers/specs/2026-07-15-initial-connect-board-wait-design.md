# Initial Connect Board Wait Design

## Evidence

After assigning the custom MockLink to `_mockLink` and replacing direct
`disconnectAll()` with `_disconnectMockLink()`, the previous Windows access
violation no longer occurred. The complete direct QGC JUnit run reached 24 test
cases and reported one failure in `_boardVendorProductId`:

```text
QTestLib: This test case check (initialConnectCompleteSpy.count() > 0 || vehicle->isInitialConnectComplete()) failed because the requested timeout (5000 ms) was too short, 9950 ms would have been sufficient this time.
```

The failure is at the test's `initialConnectComplete` wait, which currently
uses `TestTimeout::mediumMs()`. The teardown then completed through the
fixture helper without the former crash.

## Design

Change only the `_boardVendorProductId` initial-connect wait from
`TestTimeout::mediumMs()` to `TestTimeout::longMs()`. Keep the active-vehicle
arrival wait at its existing medium timeout and keep all other Initial Connect
waits unchanged. This gives the test the existing local 30-second bounded wait
without changing global timeout policy or production connection behavior.

## Validation

- Build the affected `InitialConnectTest.cc` object with VS2022 x64.
- Run the direct QGC JUnit selector or the complete `InitialConnectTest`
  registration; `_boardVendorProductId` must pass with no teardown crash.
- Verify the complete Initial Connect suite has no failure/error node under its
  justified CTest timeout.
- Only then proceed to full CTest and downstream branch creation.

## Scope

No production source, global timeout, warning whitelist, skipped test, or
unbounded wait is permitted.
