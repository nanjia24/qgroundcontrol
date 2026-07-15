# Initial Connect Board Teardown Design

## Goal

Prevent the Windows `InitialConnectTest::_boardVendorProductId` test from
crashing during MockLink teardown, so the complete Initial Connect suite can
finish without requiring an inflated timeout.

## Evidence

Function/data-row localization ran all 22 Initial Connect selectors. Twenty-one
passed; `_boardVendorProductId` exited with CTest code 8 after 13.60 seconds.
The Windows exception was `0xc0000005`. The captured stack passed through
`LinkManager::_linkDisconnected`, `MockLink::~MockLink`, and
`LinkManager::~LinkManager` during teardown.

The failing test creates a custom `MockConfiguration`, calls
`LinkManager::createConnectedLink`, and later calls
`LinkManager::disconnectAll()` directly. The surrounding `VehicleTest` fixture
already provides `_mockLink`, `_vehicle`, and `_disconnectMockLink()`, which
disconnects the tracked MockLink, waits for `activeVehicleChanged`, verifies
the vehicle is gone, and settles the event loop. The failing test bypasses that
lifecycle path and never assigns its MockLink to `_mockLink`.

## Options Considered

1. Track the custom MockLink in `_mockLink` and call the existing
   `_disconnectMockLink()` helper (chosen). This is the smallest test-only
   change and reuses the cleanup contract already used by the fixture.
2. Add a production `LinkManager` teardown workaround. This would broaden the
   change without evidence of a production lifecycle defect and is rejected.
3. Keep `disconnectAll()` and add an arbitrary delay or increase the test
   timeout. This would mask the cleanup race and is rejected.

## Design

In `_boardVendorProductId`:

1. After `createConnectedLink(linkConfig)` succeeds, assign the created link
   to the fixture's `_mockLink` member with a checked `qobject_cast<MockLink*>`.
2. Retain the existing board vendor/product assertions.
3. Replace the direct `LinkManager::disconnectAll()` call and its duplicated
   vehicle-removal wait with `_disconnectMockLink()`.

The test's custom `SharedLinkConfigurationPtr` remains alive until the local
scope ends. The fixture helper must complete vehicle removal and event-loop
settling before that scope releases the configuration. No production source,
global timeout, warning whitelist, or test skip changes.

## Validation

- Add no new test class; the existing `_boardVendorProductId` selector is the
  regression test.
- Build `QGroundControl` with VS2022 x64, Qt 6.10.3, Ninja, and GStreamer.
- Run `_boardVendorProductId` through the temporary selector if still needed;
  it must exit 0 with no CTest failure/error node.
- Run the complete `InitialConnectTest` registration with its normal 120-second
  timeout. It must finish without the teardown crash; the suite must complete
  within the timeout or receive a separately justified scoped timeout only if
  all rows pass.
- Run the full 186-test CTest suite to completion after this fix. Do not create
  downstream branches until it reports zero failures.
