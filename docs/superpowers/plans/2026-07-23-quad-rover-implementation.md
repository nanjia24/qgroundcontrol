# Quad-Rover PX4 Adaptation Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add first-class QGC support for the PX4 Quad-Rover vehicle while preserving ordinary PX4, VTOL, Rover, and ArduPilot behavior.

**Architecture:** Lock this integration branch to the published `qgc_hybrid` MAVLink release, then model the physical shape as per-vehicle telemetry state rather than a changing MAV type. A focused state object owns status freshness and reboot epochs; a controller owns command 50000's strict ACK lifecycle; PX4 mode policy projects that state into a temporary effective shape profile without changing QGC's global vehicle classification.

**Tech Stack:** CMake 3.31, Ninja, MSVC v143, Qt 6.10.3, MAVLink 2, Qt Test, CTest, QML, PX4 v1.16.1.

## Global Constraints

- Work only in `E:\workspace\QGC\qgroundcontrol-worktrees\quad-rover-design` on `codex/quad-rover-design`; its required base is `codex/joystick-aux-px4-development` commit `754135601a53d7650ddeb6562ca5a5cd2167880c`.
- Do not rebase this branch onto upstream QGC, modify the protected baseline, create a PR, force-push, commit build output, or stage the unrelated untracked `NUL` entry.
- Use exactly `QGC_MAVLINK_GIT_REPO=https://github.com/QQgdiw/mavlink.git`, `QGC_MAVLINK_GIT_TAG=qgc-hybrid-change1-v1.16.1-r2`, `QGC_MAVLINK_DIALECT=qgc_hybrid`, `QGC_MAVLINK_VERSION=2.0`, and keep `QGC_DISABLE_APM_MAVLINK=OFF` and `QGC_DISABLE_APM_PLUGIN=OFF`. The released peeled commit is `04ad1d63e9c11ed6767a35dae4e52adaca3538c5`; old hybrid-only tags and raw `all` or `hybrid_vehicle` are invalid inputs.
- The firmware contract is PX4 `change1_v1.16.1` at `82478dbdf2`: type 200, command 50000, and message 60000. Do not change PX4, MAVLink numeric IDs, or substitute VTOL command 3000.
- `VehicleClassQuadRover` is permanently distinct from multirotor, rover, and VTOL. `Vehicle::vtol()` remains false. `HYBRID_VEHICLE_STATUS` is the sole physical-shape authority.
- Never hardcode or wait on `HYBRID_TRANS_T`; 2.0 seconds is an airframe value, not a QGC control timeout. The only applicable telemetry freshness window is three seconds.
- Preserve QGC patterns: generated MAVLink decoders, `Vehicle::sendMavCommandWithHandler`, Fact-based parameters, `QGCPalette`, `ScreenTools`, active-vehicle null checks, and no direct raw `COMMAND_LONG` packing from QML.
- Build on Windows with VS2022 v143 x64, Qt `E:\Qt\6.10.3\msvc2022_64`, GStreamer `E:\PROGRA~1\GSTREA~1\1.0\MSVC_X~1`, CMake/Ninja, and `--parallel 2`. Redirect large build/test output to `.tmp` logs and inspect concise summaries.

---

### Task 1: Pin and prove the composite MAVLink dependency

**Files:**
- Create: `cmake/HybridMavlinkContract.cmake`
- Create: `test/CMake/CMakeLists.txt`
- Create: `test/CMake/HybridMavlinkContractTest.cmake`
- Modify: `cmake/CustomOptions.cmake:95-107`
- Modify: `CMakeLists.txt:41-49`
- Modify: `src/MAVLink/CMakeLists.txt:37-78`
- Modify: `test/CMakeLists.txt:42-55`

**Interfaces:**
- Consumes: the fixed r2 repository, tag, dialect, MAVLink version, enabled APM options, and peeled commit.
- Produces: `qgc_validate_hybrid_mavlink_inputs()` for configure-time input validation and `qgc_verify_hybrid_mavlink_checkout(<source-dir>)` for CPM checkout verification.

- [ ] **Step 1: Add an expected-failure CMake contract matrix**

Create `test/CMake/HybridMavlinkContractTest.cmake` so `CASE=valid` sets the exact six accepted inputs and invokes `qgc_validate_hybrid_mavlink_inputs()`. Each of `bad_repository`, `bad_tag`, `bad_dialect`, `bad_version`, `apm_dialect_disabled`, and `apm_plugin_disabled` changes exactly one input before the same call.

```cmake
include("${QGC_SOURCE_DIR}/cmake/HybridMavlinkContract.cmake")

set(QGC_MAVLINK_GIT_REPO "https://github.com/QQgdiw/mavlink.git")
set(QGC_MAVLINK_GIT_TAG "qgc-hybrid-change1-v1.16.1-r2")
set(QGC_MAVLINK_DIALECT "qgc_hybrid")
set(QGC_MAVLINK_VERSION "2.0")
set(QGC_DISABLE_APM_MAVLINK OFF)
set(QGC_DISABLE_APM_PLUGIN OFF)

if(CASE STREQUAL "bad_dialect")
    set(QGC_MAVLINK_DIALECT "hybrid_vehicle")
endif()

qgc_validate_hybrid_mavlink_inputs()
```

Register a passing `valid` test and six `WILL_FAIL` negative tests in `test/CMake/CMakeLists.txt`, then add that directory from `test/CMakeLists.txt` after `include(QGCTest)`.

- [ ] **Step 2: Run the contract matrix before implementation**

Run:

```powershell
cmake -DQGC_SOURCE_DIR="$PWD" -DCASE=bad_dialect -P test\CMake\HybridMavlinkContractTest.cmake
```

Expected: it fails because `HybridMavlinkContract.cmake` does not exist yet. Do not configure QGC with the old default dialect.

- [ ] **Step 3: Implement the configuration and checkout checks**

Set the four MAVLink defaults in `cmake/CustomOptions.cmake` to the r2 values. Put immutable expected values in `cmake/HybridMavlinkContract.cmake`; call the input function after `CustomOptions` and optional `CustomOverrides`, then call the checkout function after `CPMAddPackage` in `src/MAVLink/CMakeLists.txt`.

```cmake
function(qgc_validate_hybrid_mavlink_inputs)
    if(NOT "${QGC_MAVLINK_GIT_REPO}" STREQUAL "https://github.com/QQgdiw/mavlink.git")
        message(FATAL_ERROR "QGC_MAVLINK_GIT_REPO must be the released qgc_hybrid repository")
    endif()
    if(NOT "${QGC_MAVLINK_GIT_TAG}" STREQUAL "qgc-hybrid-change1-v1.16.1-r2")
        message(FATAL_ERROR "QGC_MAVLINK_GIT_TAG must be qgc-hybrid-change1-v1.16.1-r2")
    endif()
    if(NOT "${QGC_MAVLINK_DIALECT}" STREQUAL "qgc_hybrid" OR NOT "${QGC_MAVLINK_VERSION}" STREQUAL "2.0")
        message(FATAL_ERROR "QGC requires MAVLink 2 qgc_hybrid")
    endif()
    if(QGC_DISABLE_APM_MAVLINK OR QGC_DISABLE_APM_PLUGIN)
        message(FATAL_ERROR "qgc_hybrid builds keep the APM MAVLink dialect and plugin enabled")
    endif()
endfunction()

function(qgc_verify_hybrid_mavlink_checkout source_dir)
    execute_process(COMMAND "${GIT_EXECUTABLE}" -C "${source_dir}" rev-parse HEAD
                    OUTPUT_VARIABLE resolved_commit OUTPUT_STRIP_TRAILING_WHITESPACE
                    RESULT_VARIABLE git_result)
    if(NOT git_result EQUAL 0 OR NOT "${resolved_commit}" STREQUAL "04ad1d63e9c11ed6767a35dae4e52adaca3538c5")
        message(FATAL_ERROR "CPM MAVLink checkout does not match the qgc_hybrid r2 peeled commit")
    endif()
endfunction()
```

Use `find_package(Git REQUIRED)` inside the module or before the checkout call. Never log a credential-bearing URL.

- [ ] **Step 4: Run focused CMake verification**

Run the script for `valid` and every negative case, then configure a fresh app-only tree with a fresh CPM cache:

```powershell
$env:CPM_SOURCE_CACHE = (Join-Path $PWD '.tmp\cpm-qgc-hybrid-r2')
& 'E:\Qt\6.10.3\msvc2022_64\bin\qt-cmake.bat' -S . -B build\quad-rover-contract -G Ninja `
    -DCMAKE_BUILD_TYPE=Release -DQGC_BUILD_TESTING=OFF `
    -DGStreamer_ROOT_DIR=E:/PROGRA~1/GSTREA~1/1.0/MSVC_X~1 `
    -DQGC_MAVLINK_GIT_REPO=https://github.com/QQgdiw/mavlink.git `
    -DQGC_MAVLINK_GIT_TAG=qgc-hybrid-change1-v1.16.1-r2 `
    -DQGC_MAVLINK_DIALECT=qgc_hybrid -DQGC_MAVLINK_VERSION=2.0
```

Expected: valid configuration succeeds, every altered input fails before C++ compilation, `CMakeCache.txt` contains r2/qgc_hybrid, and the CPM checkout resolves to `04ad1d63e9c11ed6767a35dae4e52adaca3538c5`.

- [ ] **Step 5: Commit the dependency contract**

```powershell
git add cmake/HybridMavlinkContract.cmake cmake/CustomOptions.cmake CMakeLists.txt src/MAVLink/CMakeLists.txt test/CMake test/CMakeLists.txt
git commit -m "chore[mavlink]: pin qgc hybrid r2 contract"
```

### Task 2: Register type 200 as a permanent Quad-Rover class

**Files:**
- Modify: `src/MAVLink/QGCMAVLink.h:29-60`
- Modify: `src/MAVLink/QGCMAVLink.cc:91-260`
- Modify: `src/Vehicle/Vehicle.h:162-168,458-470`
- Modify: `src/Vehicle/Vehicle.cc:1750-1795`
- Modify: `test/MAVLink/QGCMAVLinkTest.h`
- Modify: `test/MAVLink/QGCMAVLinkTest.cc`

**Interfaces:**
- Consumes: generated `MAV_TYPE_QUAD_ROVER` from `mavlink/qgc_hybrid/mavlink.h`.
- Produces: `QGCMAVLink::VehicleClassQuadRover`, `QGCMAVLink::isQuadRover(MAV_TYPE)`, and `Vehicle::quadRover()`.

- [ ] **Step 1: Add failing classification assertions**

Extend `QGCMAVLinkTest` with a slot that asserts type 200 maps only to the new class and that its public/internal names round-trip.

```cpp
QCOMPARE(QGCMAVLink::vehicleClass(MAV_TYPE_QUAD_ROVER), QGCMAVLink::VehicleClassQuadRover);
QVERIFY(QGCMAVLink::isQuadRover(MAV_TYPE_QUAD_ROVER));
QVERIFY(!QGCMAVLink::isMultiRotor(MAV_TYPE_QUAD_ROVER));
QVERIFY(!QGCMAVLink::isRoverBoat(MAV_TYPE_QUAD_ROVER));
QVERIFY(!QGCMAVLink::isVTOL(MAV_TYPE_QUAD_ROVER));
QCOMPARE(QGCMAVLink::vehicleTypeFromString(QStringLiteral("Quad-Rover")), MAV_TYPE_QUAD_ROVER);
```

- [ ] **Step 2: Confirm the test is red against the old classification code**

Configure Task 1's r2 build with testing enabled and run:

```powershell
cmake --build build\quad-rover-debug --target QGroundControl --parallel 2 *> .tmp\quad-rover-classification-red.log
& '.\build\quad-rover-debug\Debug\QGroundControl.exe' '--unittest:QGCMAVLinkTest' '--allow-multiple'
```

Expected: the new test does not compile or fails because the type/class helpers do not yet exist.

- [ ] **Step 3: Implement the first-class mapping**

Add the exact class constant and helper beside existing vehicle classes, map only `MAV_TYPE_QUAD_ROVER` in `vehicleClass`, and add it to all class lists and string conversions.

```cpp
static constexpr const VehicleClass_t VehicleClassQuadRover = MAV_TYPE_QUAD_ROVER;

bool QGCMAVLink::isQuadRover(MAV_TYPE mavType)
{
    return vehicleClass(mavType) == VehicleClassQuadRover;
}

bool Vehicle::quadRover() const
{
    return QGCMAVLink::isQuadRover(vehicleType());
}
```

Expose `quadRover` as a read-only `Q_PROPERTY`. Do not make `multiRotor`, `rover`, or `vtol` return true for this type.

- [ ] **Step 4: Run the classification regression test**

Run `QGCMAVLinkTest`, then inspect the generated `MAVLinkEnums.h` in the r2 build for `MAV_TYPE_QUAD_ROVER`, `MAV_CMD_DO_HYBRID_TRANSITION`, and `MAVLINK_MSG_ID_HYBRID_VEHICLE_STATUS`.

Expected: all type-200 assertions pass, while pre-existing VTOL/multirotor/rover assertions stay unchanged.

- [ ] **Step 5: Commit the isolated classification change**

```powershell
git add src/MAVLink/QGCMAVLink.h src/MAVLink/QGCMAVLink.cc src/Vehicle/Vehicle.h src/Vehicle/Vehicle.cc test/MAVLink/QGCMAVLinkTest.h test/MAVLink/QGCMAVLinkTest.cc
git commit -m "feat[vehicle]: classify px4 quad rover"
```

### Task 3: Build an authoritative HybridVehicleState with deterministic freshness and reboot epochs

**Files:**
- Create: `src/Vehicle/HybridVehicleState.h`
- Create: `src/Vehicle/HybridVehicleState.cc`
- Modify: `src/Vehicle/CMakeLists.txt:6-45`
- Modify: `test/Vehicle/CMakeLists.txt:6-35`
- Create: `test/Vehicle/HybridVehicleStateTest.h`
- Create: `test/Vehicle/HybridVehicleStateTest.cc`
- Modify: `test/CMakeLists.txt:363-390`

**Interfaces:**
- Consumes: decoded `mavlink_hybrid_vehicle_status_t`, selected autopilot component id, decoded `mavlink_system_time_t`, and MAVLink 2-only ingress.
- Produces: read-only QML properties, `freshStatusChanged`, `statusAccepted`, `rebootConfirmed`, and two `requestQGCTimeSync` signals for the invalid-RTC recovery path.

- [ ] **Step 1: Write state-machine tests before the class exists**

Add `HybridVehicleStateTest` slots for initial unknown/stale state, every state enum and independent flag, NaN position, stale transition, monotonic timestamp rejection, natural `uint32_t` boot-time wrap, both reboot-candidate arrival orders, candidate expiry, wrong component rejection, and the three low-HRT fallback samples.

```cpp
void HybridVehicleStateTest::_rebootNeedsSameComponentAndBothStreams()
{
    _state->handleSystemTime(MAV_COMP_ID_AUTOPILOT1, 5000);
    _state->handleStatus(MAV_COMP_ID_AUTOPILOT1, makeStatus(900000));
    _state->handleStatus(MAV_COMP_ID_AUTOPILOT1, makeStatus(100));
    QVERIFY(!_rebootSpy->count());
    _state->handleSystemTime(MAV_COMP_ID_AUTOPILOT1, 20);
    QCOMPARE(_rebootSpy->count(), 1);
    QVERIFY(!_state->hasValidStatus());
}
```

Use a test freshness duration only under `QGC::runningUnitTests()`; production values remain 3000 milliseconds.

- [ ] **Step 2: Run the new state test to establish RED**

Run:

```powershell
& '.\build\quad-rover-debug\Debug\QGroundControl.exe' '--unittest:HybridVehicleStateTest' '--allow-multiple' *> .tmp\hybrid-state-red.log
```

Expected: the test target reports that `HybridVehicleState` is missing.

- [ ] **Step 3: Implement the owned protocol state object**

Use one focused QObject, with enums and independent scalar/boolean properties. It receives only already-decoded data and never reads PX4 parameters or infers shape from another field.

```cpp
class HybridVehicleState final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(CurrentState currentState READ currentState NOTIFY stateChanged)
    Q_PROPERTY(TargetState targetState READ targetState NOTIFY stateChanged)
    Q_PROPERTY(bool hasValidStatus READ hasValidStatus NOTIFY statusValidityChanged)
    Q_PROPERTY(bool stale READ stale NOTIFY freshnessChanged)
    Q_PROPERTY(bool positionNormalizedValid READ positionNormalizedValid NOTIFY stateChanged)
    Q_PROPERTY(double positionNormalized READ positionNormalized NOTIFY stateChanged)
public:
    enum CurrentState { Quad = 0, Transitioning = 1, Rover = 2, Unknown = 3, TransitionFault = 4 };
    enum TargetState { None = 0, TargetQuad = 1, TargetRover = 2 };
    void handleStatus(uint8_t componentId, const mavlink_hybrid_vehicle_status_t& status);
    void handleSystemTime(uint8_t componentId, uint32_t timeBootMs);
    void resetForNewVehicleSession();
signals:
    void requestQGCTimeSync();
    void rebootConfirmed();
};
```

Expose these decoded fields as read-only properties with one change signal per
coherent update: `currentState`, `targetState`, `transitionSequence`,
`transitionElapsedMs`, `positionNormalized`, `positionNormalizedValid`,
`faultReason`, `commandResult`, `sensorSource`, `actuatorBackend`,
`actuatorProtectionFlags`, raw `flags`, each independent bit 0-7, `landed`,
`landDetectionSampleFresh`, `commandTimestamp`, and local monotonic status
receipt time. Also expose derived `hasValidStatus`, `stale`, and
`resetCandidateActive`. `canRequestTransform()` is true only for a fresh,
fault-free stable shape with landed and fresh land detection and no active
reset candidate; it must not synthesize a fault from diagnostic actuator bits.

Implement serial arithmetic as `delta = candidate - baseline; delta != 0 && delta < 0x80000000U`. Pair only a boot-time rollback and a lower HRT candidate from the same selected component within three local seconds. A normal forward time sample clears only its time candidate; newer HRT clears only its status candidate. On lower HRT, emit `requestQGCTimeSync()` twice; only then allow exactly three strictly increasing low-HRT samples in the same bounded window to confirm a no-RTC reboot.

- [ ] **Step 4: Run focused state coverage**

Run `HybridVehicleStateTest` and `QGCMAVLinkTest`. Inspect its signal results so that a candidate alone fails closed but expiry restores only ordinary freshness behavior, never a guessed Quad or Rover shape.

Expected: all new state cases pass; no normal status ordering or uint32 wrap is interpreted as a reboot.

- [ ] **Step 5: Commit the state object and its tests**

```powershell
git add src/Vehicle/HybridVehicleState.h src/Vehicle/HybridVehicleState.cc src/Vehicle/CMakeLists.txt test/Vehicle/HybridVehicleStateTest.h test/Vehicle/HybridVehicleStateTest.cc test/Vehicle/CMakeLists.txt test/CMakeLists.txt
git commit -m "feat[vehicle]: add hybrid telemetry state"
```

### Task 4: Route MAVLink 2 status and SYSTEM_TIME through Vehicle safely

**Files:**
- Modify: `src/Vehicle/Vehicle.h:141-230,825-850,1040-1060`
- Modify: `src/Vehicle/Vehicle.cc:260-310,610-690,2205-2270`
- Modify: `src/Comms/MockLink/MockLink.h:140-160`
- Modify: `src/Comms/MockLink/MockLink.cc:1680-1710`
- Create: `test/Vehicle/HybridVehicleIngressTest.h`
- Create: `test/Vehicle/HybridVehicleIngressTest.cc`
- Modify: `test/Vehicle/CMakeLists.txt`

**Interfaces:**
- Consumes: a type-200 PX4 MockLink and `MockLink::respondWithMavlinkMessage` test injection.
- Produces: `Vehicle::hybridVehicleState()` and source-filtered calls to `HybridVehicleState::handleStatus` / `handleSystemTime`.

- [ ] **Step 1: Add vehicle ingress tests**

Add `MockLink::startPX4QuadRoverMockLink(...)` and a `VehicleTest` subclass that connects it. Test that a valid MAVLink 2 type-200 status is decoded using `mavlink_msg_hybrid_vehicle_status_decode`, while a payload component, `MAV_COMP_ID_ALL`, wrong sysid, MAVLink 1 frame, and out-of-order HRT cannot alter the state.

```cpp
mavlink_message_t message{};
mavlink_hybrid_vehicle_status_t status = makeStatus(HybridVehicleState::Rover, 101);
mavlink_msg_hybrid_vehicle_status_encode_chan(vehicle()->id(), MAV_COMP_ID_AUTOPILOT1,
                                                mockLink()->mavlinkChannel(), &message, &status);
mockLink()->respondWithMavlinkMessage(message);
QTRY_COMPARE(vehicle()->hybridVehicleState()->currentState(), HybridVehicleState::Rover);
```

- [ ] **Step 2: Run the ingress fixture as RED**

Run `HybridVehicleIngressTest` before adding Vehicle members or switch cases.

Expected: it cannot find the type-200 fixture/property or the message is ignored.

- [ ] **Step 3: Wire Vehicle without manual payload parsing**

Add a constant `HybridVehicleState*` property and initialize it as a Vehicle child. In the existing message switch, decode only after confirming type 200, v2 magic, matching system id, a concrete default autopilot component, and exact `message.compid == defaultComponentId()`.

```cpp
case MAVLINK_MSG_ID_HYBRID_VEHICLE_STATUS: {
    if (_isHybridAutopilotMessage(message)) {
        mavlink_hybrid_vehicle_status_t status{};
        mavlink_msg_hybrid_vehicle_status_decode(&message, &status);
        _hybridVehicleState->handleStatus(message.compid, status);
    }
    break;
}
case MAVLINK_MSG_ID_SYSTEM_TIME: {
    if (_isHybridAutopilotMessage(message)) {
        mavlink_system_time_t time{};
        mavlink_msg_system_time_decode(&message, &time);
        _hybridVehicleState->handleSystemTime(message.compid, time.time_boot_ms);
    }
    break;
}
```

Connect `requestQGCTimeSync` to the existing `_sendQGCTimeToVehicle`. On `rebootConfirmed`, clear state through the state object's reset path and defer controller cancellation to Task 6. Do not parse any payload bytes manually.

- [ ] **Step 4: Run ingress and parameter-discovery checks**

Run `HybridVehicleIngressTest` with the existing `InitialConnectTest` and a type-200 PX4 mock fixture that exposes `HYBRID_TRANS_T`, `HYB_SENS_EN`, and `HYB_ACT_TYPE` through `ParameterManager` Facts.

Expected: state starts unknown/stale, valid status changes it, invalid source data does not, and no private parameter store is introduced.

- [ ] **Step 5: Commit vehicle ingress**

```powershell
git add src/Vehicle/Vehicle.h src/Vehicle/Vehicle.cc src/Comms/MockLink/MockLink.h src/Comms/MockLink/MockLink.cc test/Vehicle/HybridVehicleIngressTest.h test/Vehicle/HybridVehicleIngressTest.cc test/Vehicle/CMakeLists.txt
git commit -m "feat[vehicle]: decode hybrid status telemetry"
```

### Task 5: Generalize the command queue for strict matching and detach-on-progress

**Files:**
- Modify: `src/Vehicle/VehicleTypes.h:14-50`
- Modify: `src/Vehicle/MavCommandQueue.h:30-110`
- Modify: `src/Vehicle/MavCommandQueue.cc:170-510`
- Modify: `src/Vehicle/Vehicle.cc:2222-2265`
- Modify: `test/Vehicle/SendMavCommandWithHandlerTest.h`
- Modify: `test/Vehicle/SendMavCommandWithHandlerTest.cc`

**Interfaces:**
- Consumes: per-entry `MavCmdAckMatcher`, strict `mavlink_message_t`/`mavlink_command_ack_t` tuples, and `detachOnProgress` policy.
- Produces: `bool MavCommandQueue::handleCommandAck(...)`, `cancelCommand(int, MAV_CMD)`, and existing retry behavior until the first matching ACK.

- [ ] **Step 1: Add queue tests that fail on the current loose matcher**

Add a test handler with a matcher that accepts only a given sender, command, target system, and target component. Inject a wrong-target terminal ACK, a wrong-sender `IN_PROGRESS`, then a correct `IN_PROGRESS` and correct terminal ACK.

```cpp
QVERIFY(vehicle()->isMavCommandPending(MAV_COMP_ID_AUTOPILOT1, command));
injectAck(command, MAV_RESULT_ACCEPTED, wrongSender, qgcSystemId, qgcComponentId);
QVERIFY(vehicle()->isMavCommandPending(MAV_COMP_ID_AUTOPILOT1, command));
injectAck(command, MAV_RESULT_IN_PROGRESS, MAV_COMP_ID_AUTOPILOT1, qgcSystemId, qgcComponentId);
QVERIFY(!vehicle()->isMavCommandPending(MAV_COMP_ID_AUTOPILOT1, command));
```

Also assert an ordinary command retains its current behavior, including progress timeout/retry semantics.

- [ ] **Step 2: Run the test as RED**

Run `SendMavCommandWithHandlerTest` before altering queue code.

Expected: an address-mismatched ACK is consumed or an `IN_PROGRESS` entry remains queued, showing why the new entry policy is needed.

- [ ] **Step 3: Add an opt-in entry policy with safe defaults**

Extend `MavCmdAckHandlerInfo_t` without changing ordinary callers:

```cpp
typedef bool (*MavCmdAckMatcher)(void* matcherData, const mavlink_message_t& message,
                                 const mavlink_command_ack_t& ack);

MavCmdAckMatcher ackMatcher = nullptr;
void*             ackMatcherData = nullptr;
bool              detachOnProgress = false;
```

In `handleCommandAck`, locate a candidate by target component and command, run its matcher before changing timer/retry/list state, and return `false` when none matches. For `detachOnProgress`, `takeAt` the one entry before calling the progress handler; otherwise preserve current behavior. Return `true` only after a matching entry has been consumed or handled. Add `cancelCommand` to remove one pending entry silently for confirmed reboot cleanup.

Make `MAV_CMD_DO_HYBRID_TRANSITION` a narrow `_shouldRetry` exception. It uses the existing `kMaxRetryCount`, ACK timeout, resend path, and original parameters; it must never create a second entry or a command-specific timer.

- [ ] **Step 4: Run queue regression coverage**

Run `SendMavCommandWithHandlerTest`, `SendMavCommandWithSignallingTest`, and `RequestMessageTest`.

Expected: mismatched ACKs cause no handler, retry, list, or UI side effect; correct first ACK stops initial retry; existing non-hybrid commands retain their current results.

- [ ] **Step 5: Commit the queue primitive**

```powershell
git add src/Vehicle/VehicleTypes.h src/Vehicle/MavCommandQueue.h src/Vehicle/MavCommandQueue.cc src/Vehicle/Vehicle.cc test/Vehicle/SendMavCommandWithHandlerTest.h test/Vehicle/SendMavCommandWithHandlerTest.cc
git commit -m "feat[vehicle]: add strict command ack queue policy"
```

### Task 6: Implement the HybridTransitionController transaction lifecycle

**Files:**
- Create: `src/Vehicle/HybridTransitionController.h`
- Create: `src/Vehicle/HybridTransitionController.cc`
- Modify: `src/Vehicle/CMakeLists.txt`
- Modify: `src/Vehicle/Vehicle.h`
- Modify: `src/Vehicle/Vehicle.cc`
- Modify: `src/Comms/MockLink/MockLink.h`
- Modify: `src/Comms/MockLink/MockLink.cc`
- Create: `test/Vehicle/HybridTransitionControllerTest.h`
- Create: `test/Vehicle/HybridTransitionControllerTest.cc`
- Modify: `test/Vehicle/CMakeLists.txt`
- Modify: `test/CMakeLists.txt`

**Interfaces:**
- Consumes: `Vehicle`, `HybridVehicleState`, strict queue handler policy, status snapshots, and command 50000 ACKs.
- Produces: QML-callable `requestTransform(int targetState)`, read-only busy/result properties, `handleDetachedAck`, and `handleVehicleReboot`.

- [ ] **Step 1: Add protocol fixtures for the full ACK/status matrix**

Extend MockLink with a type-200 factory and a test-only command-50000 response mode that can hold automatic ACKs. Add helpers that inject a `COMMAND_ACK` with arbitrary sender/target/result/result_param2 and an encoded `HYBRID_VEHICLE_STATUS`.

Cover direct terminal accepted, denied, temporarily rejected, no first ACK, `IN_PROGRESS` then terminal accepted/failed, repeated same sequence, sequence zero, unsigned sequence wrap, wrong sender/target, status-only completion, mismatching status result, superseding sequence, state fault, stale status, same-target retry, opposite-target rejection, and confirmed reboot.

```cpp
void HybridTransitionControllerTest::_wrongAddressDoesNotDetach()
{
    QVERIFY(_controller->requestTransform(HybridVehicleState::TargetRover));
    injectAck(MAV_RESULT_IN_PROGRESS, 17, MAV_COMP_ID_AUTOPILOT1,
              MAVLinkProtocol::instance()->getSystemId() + 1, MAVLinkProtocol::getComponentId());
    QVERIFY(vehicle()->isMavCommandPending(MAV_COMP_ID_AUTOPILOT1, MAV_CMD_DO_HYBRID_TRANSITION));
    QCOMPARE(_controller->transactionState(), HybridTransitionController::Queued);
}
```

- [ ] **Step 2: Run controller tests as RED**

Run `HybridTransitionControllerTest` before adding the controller.

Expected: compilation fails because no controller owns command 50000; do not add a QML raw-command shortcut.

- [ ] **Step 3: Implement one logical transaction per UI action**

Create the controller as a Vehicle child and expose it through a constant Vehicle property. It accepts only `TargetQuad` (1) or `TargetRover` (2), rejects any queued or detached transaction, and calls only `Vehicle::sendMavCommandWithHandler` to concrete `MAV_COMP_ID_AUTOPILOT1` with `param1=target` and finite zeroes for parameters 2-7.

```cpp
bool HybridTransitionController::requestTransform(int target)
{
    if (_transactionState != Idle || !_state->canRequestTransform() ||
        (target != HybridVehicleState::TargetQuad && target != HybridVehicleState::TargetRover)) {
        return false;
    }
    _preRequestTimestamp = _state->timestamp();
    _preRequestCommandTimestamp = _state->commandTimestamp();
    Vehicle::MavCmdAckHandlerInfo_t handlerInfo{};
    handlerInfo.resultHandler = _resultHandler;
    handlerInfo.progressHandler = _progressHandler;
    handlerInfo.ackMatcher = _ackMatcher;
    handlerInfo.ackMatcherData = this;
    handlerInfo.detachOnProgress = true;
    _vehicle->sendMavCommandWithHandler(&handlerInfo, MAV_COMP_ID_AUTOPILOT1,
        MAV_CMD_DO_HYBRID_TRANSITION, static_cast<float>(target), 0.f, 0.f, 0.f, 0.f, 0.f, 0.f);
    _transactionState = Queued;
    return true;
}
```

The matcher requires command 50000, selected autopilot sender, and ACK `target_system`/`target_component` equal to QGC's local MAVLink source. From the first matching ACK, it owns semantics: a direct terminal result arrives through the queue callback; `IN_PROGRESS` detaches and stores exact `uint32_t(result_param2)`; later ACKs route from `Vehicle::_handleCommandAck` only while detached and pass the same matcher again.

- [ ] **Step 4: Implement status confirmation and reboot outcomes**

Make transaction state explicit so QML and tests cannot mistake a terminal ACK
for a physical success: `Idle`, `Queued`, `Detached`, `AwaitingStatus`,
`NoMotionAccepted`, `Unconfirmed`, `SupersededUnconfirmed`,
`VehicleRebootedUnconfirmed`, and `Faulted`. Keep `busy` true for Queued,
Detached, and AwaitingStatus, and keep shape controls unavailable for both
unconfirmed states until the state object independently resynchronizes.

On accepted progress, latch a status `command_timestamp` only after matching sequence plus `MAV_RESULT_IN_PROGRESS`. Complete with status only when timestamp is post-request, sequence is exact, current shape is requested/stable, fault is clear, result is `MAV_RESULT_ACCEPTED`, and the command-timestamp association is valid. A direct stable accepted response is no-motion only when the pre-existing fresh state already matches. Do not invent a sequence.

Use a three-second telemetry freshness watchdog only after a terminal accepted ACK. On a newer status with a different sequence, finish `SupersededUnconfirmed`, keep controls unavailable, and require independent later stable resynchronization. On state fault, stale state, terminal failed, or reboot, finish without success; reboot cancels the queue entry without a retry and reports `VehicleRebootedUnconfirmed`.

- [ ] **Step 5: Run complete controller coverage and commit**

Run `HybridTransitionControllerTest` plus Task 3/4 tests. Confirm that a second same-target or opposite request while queued/detached sends no packet and creates no queue entry; confirm a healthy transition can outlive the generic 1.2-second queue window without reading `HYBRID_TRANS_T`.

```powershell
git add src/Vehicle/HybridTransitionController.h src/Vehicle/HybridTransitionController.cc src/Vehicle/Vehicle.h src/Vehicle/Vehicle.cc src/Vehicle/CMakeLists.txt src/Comms/MockLink/MockLink.h src/Comms/MockLink/MockLink.cc test/Vehicle/HybridTransitionControllerTest.h test/Vehicle/HybridTransitionControllerTest.cc test/Vehicle/CMakeLists.txt test/CMakeLists.txt
git commit -m "feat[vehicle]: control hybrid transitions safely"
```

### Task 7: Apply the effective shape profile to PX4 flight modes and command gates

**Files:**
- Modify: `src/FirmwarePlugin/PX4/PX4FirmwarePlugin.h`
- Modify: `src/FirmwarePlugin/PX4/PX4FirmwarePlugin.cc:99-165`
- Modify: `src/Vehicle/Vehicle.cc:1490-1540`
- Modify: `test/Vehicle/HybridTransitionControllerTest.cc`
- Create: `test/Vehicle/HybridFlightModePolicyTest.h`
- Create: `test/Vehicle/HybridFlightModePolicyTest.cc`
- Modify: `test/Vehicle/CMakeLists.txt`
- Modify: `test/CMakeLists.txt`

**Interfaces:**
- Consumes: `Vehicle::quadRover()`, fresh `HybridVehicleState`, and PX4 `FlightModeList` custom-mode identities.
- Produces: `EffectiveShapeProfile { QuadMultiRotor, Rover, Unavailable }`, filtered `Vehicle::flightModes`, and a no-send gate for unavailable/disallowed hybrid modes.

- [ ] **Step 1: Add mode-policy tests**

Use the type-200 fixture to assert stable Quad receives exactly the modes whose existing `mode.multiRotor` flag is true. Assert stable Rover receives only Manual, Position, Mission, Hold/Loiter, Return, Acro, Offboard, and Stabilized. Assert transitioning, unknown, stale, fault, and superseded states return no selectable shape-dependent modes and `Vehicle::setFlightMode` emits no `MAV_CMD_DO_SET_MODE`/`SET_MODE` packet.

```cpp
const QStringList roverModes = vehicle()->flightModes();
QVERIFY(roverModes.contains(QStringLiteral("Manual")));
QVERIFY(roverModes.contains(QStringLiteral("Mission")));
QVERIFY(!roverModes.contains(QStringLiteral("Altitude")));
QVERIFY(!roverModes.contains(QStringLiteral("Takeoff")));
```

- [ ] **Step 2: Run the new policy test as RED**

Expected: the current PX4 `other` branch exposes broad modes because type 200 is neither fixed-wing nor global multirotor.

- [ ] **Step 3: Implement a policy-only projection**

Add a private PX4 helper that returns `QuadMultiRotor` only for fresh, fault-free stable Quad; `Rover` only for fresh, fault-free stable Rover; otherwise `Unavailable`. Do not change `VehicleClassQuadRover`, `multiRotor`, `fixedWing`, or `vtol`.

```cpp
if (vehicle->quadRover()) {
    switch (_effectiveShapeProfile(vehicle)) {
    case EffectiveShapeProfile::QuadMultiRotor:
        if (mode.multiRotor) { flightModesList += mode.mode_name; }
        break;
    case EffectiveShapeProfile::Rover:
        if (_isHybridRoverMode(mode.custom_mode)) { flightModesList += mode.mode_name; }
        break;
    case EffectiveShapeProfile::Unavailable:
        break;
    }
    continue;
}
```

Use the same profile in PX4 capability checks currently based on `vehicle->multiRotor()` so stable Quad retains multicopter-only capabilities. In `Vehicle::setFlightMode`, reject a Quad-Rover request not present in its currently filtered `flightModes()` before packing/sending anything. Connect hybrid state/controller changes to `flightModesChanged`.

- [ ] **Step 4: Run PX4 and non-hybrid regression tests**

Run `HybridFlightModePolicyTest`, `QGCMAVLinkTest`, and relevant existing PX4/Vehicle tests.

Expected: normal PX4 multirotor, VTOL, and rover lists remain unchanged; type 200 never obtains VTOL controls or a hidden automatic transform.

- [ ] **Step 5: Commit the mode policy**

```powershell
git add src/FirmwarePlugin/PX4/PX4FirmwarePlugin.h src/FirmwarePlugin/PX4/PX4FirmwarePlugin.cc src/Vehicle/Vehicle.cc test/Vehicle/HybridFlightModePolicyTest.h test/Vehicle/HybridFlightModePolicyTest.cc test/Vehicle/HybridTransitionControllerTest.cc test/Vehicle/CMakeLists.txt test/CMakeLists.txt
git commit -m "feat[px4]: filter modes by hybrid shape"
```

### Task 8: Preserve and validate the hybrid transition mission item

**Files:**
- Create: `src/MissionManager/MavCmdInfoQuadRover.json`
- Modify: `src/MissionManager/MavCmdInfoCommon.json`
- Create: `src/FirmwarePlugin/PX4/PX4-MavCmdInfoQuadRover.json`
- Modify: `src/FirmwarePlugin/FirmwarePlugin.cc:94-112`
- Modify: `src/FirmwarePlugin/PX4/PX4FirmwarePlugin.cc:181-250`
- Modify: `src/MissionManager/MissionCommandTree.cc:15-45`
- Modify: `src/MissionManager/MissionItem.h`
- Modify: `src/MissionManager/MissionItem.cc`
- Modify: `src/MissionManager/PlanManager.cc:57-100`
- Modify: `src/MissionManager/SimpleMissionItem.cc:789-805`
- Create: `test/MissionManager/UT-MavCmdInfoQuadRover.json`
- Modify: `test/MissionManager/CMakeLists.txt`
- Modify: `test/MissionManager/MissionCommandTreeTest.h`
- Modify: `test/MissionManager/MissionCommandTreeTest.cc`
- Create: `test/MissionManager/HybridMissionItemTest.h`
- Create: `test/MissionManager/HybridMissionItemTest.cc`

**Interfaces:**
- Consumes: `MAV_CMD_DO_HYBRID_TRANSITION`, type-200 mission class, `MissionItem` params, and MissionManager upload flow.
- Produces: a type-200-only mission command with integral param1 {1,2}, reserved finite zero params2-7, and explicit pre-upload rejection.

- [ ] **Step 1: Add mission-tree and invalid-upload tests**

Assert command 50000 is listed for Quad-Rover PX4 only, never for normal PX4 multirotor/VTOL/rover. Add plan load/save round-trip cases for both values of param1. Add upload tests for `1.5`, NaN, infinity, zero, three, nonzero reserved params, and NaN/infinite reserved params; each must leave the MissionManager idle without a MISSION_COUNT packet.

```cpp
QVERIFY(_missionItemIsRejected(MAV_CMD_DO_HYBRID_TRANSITION, 1.5, 0, 0, 0, 0, 0, 0));
QVERIFY(_missionItemIsRejected(MAV_CMD_DO_HYBRID_TRANSITION, 1, qQNaN(), 0, 0, 0, 0, 0));
QVERIFY(_missionItemIsAccepted(MAV_CMD_DO_HYBRID_TRANSITION, 2, 0, 0, 0, 0, 0, 0));
```

- [ ] **Step 2: Run the mission tests as RED**

Expected: no type-200 tree exists and malformed values can reach generic upload handling.

- [ ] **Step 3: Implement metadata and one shared validator**

Add command 50000 to common metadata as a non-positional action with enum param1 `1 Quad,2 Rover`. Add the empty generic Quad-Rover override resource and a PX4 Quad-Rover override that hides reserved params and removes coordinate/frame/altitude semantics. Add the new class to both base and PX4 `missionCommandOverrides`, the production tree, and the unit-test tree/resources.

Implement one `MissionItem`-level validator used by `SimpleMissionItem::readyForSaveState` and by `PlanManager::writeMissionItems` before ownership transfer:

```cpp
bool MissionItem::isValidHybridTransition(QString* error) const
{
    if (command() != MAV_CMD_DO_HYBRID_TRANSITION) { return true; }
    const double target = param1();
    if (!std::isfinite(target) || (target != 1.0 && target != 2.0)) {
        if (error) { *error = tr("Hybrid transition target must be Quad or Rover."); }
        return false;
    }
    for (const double reserved : {param2(), param3(), param4(), param5(), param6(), param7()}) {
        if (!std::isfinite(reserved) || reserved != 0.0) { return false; }
    }
    return true;
}
```

The PlanManager guard applies only to Quad-Rover missions and emits a clear expected error before any transfer packet. It preserves invalid loaded items for user correction but never uploads them. Do not translate this command to command 3000.

- [ ] **Step 4: Run mission round-trip and transfer tests**

Run `MissionCommandTreeTest`, `HybridMissionItemTest`, `MissionItemTest`, and `SimpleMissionItemTest`.

Expected: valid 50000 items serialize/deserialize/upload/download intact; invalid values stop before transfer; ordinary vehicle classes do not advertise or create the command.

- [ ] **Step 5: Commit mission integration**

```powershell
git add src/MissionManager/MavCmdInfoQuadRover.json src/MissionManager/MavCmdInfoCommon.json src/FirmwarePlugin/PX4/PX4-MavCmdInfoQuadRover.json src/FirmwarePlugin/FirmwarePlugin.cc src/FirmwarePlugin/PX4/PX4FirmwarePlugin.cc src/MissionManager/MissionCommandTree.cc src/MissionManager/PlanManager.cc src/MissionManager/SimpleMissionItem.cc test/MissionManager
git commit -m "feat[mission]: support hybrid transition items"
```

### Task 9: Add compact, state-driven toolbar controls without VTOL reuse

**Files:**
- Create: `src/UI/toolbar/HybridTransitionIndicator.qml`
- Modify: `src/UI/toolbar/CMakeLists.txt:4-40`
- Modify: `src/UI/toolbar/FlyViewToolBar.qml:68-108`
- Modify: `src/QmlControls/FlightModeMenu.qml` only if a stale menu needs an explicit active-vehicle refresh
- Test: `src/UI/toolbar/HybridTransitionIndicator.qml` with `qmllint`

**Interfaces:**
- Consumes: `activeVehicle.quadRover`, `activeVehicle.hybridVehicleState`, and `activeVehicle.hybridTransitionController`.
- Produces: visible state/fault presentation and explicit Quad/Rover actions which call only `requestTransform`.

- [ ] **Step 1: Write a QML static check and controller-driven interaction expectation**

Create the QML file in the toolbar module and run `qmllint` before it exists. The controller test from Task 6 is the behavior test: it must prove that no second action sends a MAVLink packet while the first transaction is queued/detached.

```powershell
& 'E:\Qt\6.10.3\msvc2022_64\bin\qmllint.exe' src\UI\toolbar\HybridTransitionIndicator.qml
```

Expected: file-not-found before implementation.

- [ ] **Step 2: Implement the compact indicator**

Use a `RowLayout` visible only for a non-null active Quad-Rover. Render `Unknown`, `Transitioning`, `Quad`, `Rover`, and fault state from decoded properties; render normalized position only when `positionNormalizedValid`. Use `QGCPalette` colors and `ScreenTools` dimensions. Use two clear command buttons, not decorative cards; both bind `enabled` to the controller/state readiness and call no MAVLink methods directly.

```qml
QGCButton {
    text: qsTr("Rover")
    enabled: state.canRequestTransform && !controller.busy
    onClicked: controller.requestTransform(state.TargetRover)
    ToolTip.text: qsTr("Transform to rover shape")
}
```

Use generated enum values or the state object's QML enum instead of a magic numeric literal. There must be no `MAV_CMD_DO_VTOL_TRANSITION`, no `vehicle.vtol` inference, and no automatic transform to satisfy a mode selection.

- [ ] **Step 3: Add the component to the toolbar module and Fly toolbar**

List the QML file in `ToolbarModule` and add it adjacent to `FlightModeIndicator` in `FlyViewToolBar.qml`. Bind it to the active vehicle with null guards; do not make it a nested floating card.

- [ ] **Step 4: Validate UI behavior**

Run `qmllint`, `HybridTransitionControllerTest`, and a debug build. Exercise stable Quad, stable Rover, transitioning, landed rejection, fault, and stale status using the MockLink fixture; capture screenshots/logs only after the state is actually injected.

Expected: controls are disabled during unknown/stale/fault/reset; only explicit user clicks reach the controller; existing toolbar layout remains stable on desktop and compact width.

- [ ] **Step 5: Commit the QML surface**

```powershell
git add src/UI/toolbar/HybridTransitionIndicator.qml src/UI/toolbar/CMakeLists.txt src/UI/toolbar/FlyViewToolBar.qml src/QmlControls/FlightModeMenu.qml
git commit -m "feat[toolbar]: add hybrid transition controls"
```

Omit `src/QmlControls/FlightModeMenu.qml` from the staged set if its existing `flightModesChanged` binding needs no source change.

### Task 10: Run integration gates, record evidence, and prepare handoff

**Files:**
- Modify: `state/README.md`
- Modify: `state/TODO.md`
- Modify: `state/LOG.md`
- Test: fresh CMake cache, targeted Qt tests, CTest labels, release executable, generated MAVLink header proof

**Interfaces:**
- Consumes: all preceding commits and the immutable release contract.
- Produces: auditable build/test evidence, resolved cache/CPM commit, screenshots/logs, and accurate state tracking.

- [ ] **Step 1: Run static integrity checks**

Run:

```powershell
git diff --check 754135601a53d7650ddeb6562ca5a5cd2167880c...HEAD
git status --short
git ls-files --others --exclude-standard
```

Expected: no whitespace errors; the only unrelated untracked entry remains `NUL`; no build artifacts are staged.

- [ ] **Step 2: Build the Debug test target with an isolated CPM cache**

Use the fixed r2 CMake values and redirected logs:

```powershell
$env:CPM_SOURCE_CACHE = (Join-Path $PWD '.tmp\cpm-qgc-hybrid-r2-tests')
& 'E:\Qt\6.10.3\msvc2022_64\bin\qt-cmake.bat' -S . -B build\quad-rover-debug -G Ninja `
    -DCMAKE_BUILD_TYPE=Debug -DQGC_BUILD_TESTING=ON `
    -DGStreamer_ROOT_DIR=E:/PROGRA~1/GSTREA~1/1.0/MSVC_X~1 *> .tmp\quad-rover-configure.log
cmake --build build\quad-rover-debug --parallel 2 *> .tmp\quad-rover-debug-build.log
```

Expected: CMake cache and CPM checkout both record r2/`04ad1d63e9c11ed6767a35dae4e52adaca3538c5`; generated root headers contain hybrid and ArduPilot symbols.

- [ ] **Step 3: Run focused then broad automated tests**

Run all new test classes first, then their affected labels:

```powershell
& '.\build\quad-rover-debug\Debug\QGroundControl.exe' '--unittest:QGCMAVLinkTest,HybridVehicleStateTest,HybridVehicleIngressTest,HybridTransitionControllerTest,HybridFlightModePolicyTest,HybridMissionItemTest' '--allow-multiple' *> .tmp\quad-rover-focused-tests.log
ctest --test-dir build\quad-rover-debug -L 'Unit|Vehicle|MissionManager|MAVLink' --output-on-failure --parallel 2 *> .tmp\quad-rover-ctest.log
```

If a pre-existing test fails, reproduce it on the documented base before assigning causality. Do not skip, weaken, or whitelist a failure.

- [ ] **Step 4: Build and inspect the release application**

Configure a fresh Release tree with the same r2 inputs and build the application target. Verify `QGroundControl.exe` exists and launches with inherited Qt/GStreamer PATH. Keep the full build output in `.tmp`; record only target completion, executable path, cache values, generated root header, and CPM `HEAD` in `state/LOG.md`.

- [ ] **Step 5: Run linters, update state, and commit evidence**

Run `pre-commit run --all-files` only when the executable is available; otherwise record that it was unavailable, not as a passing check. Run `qmllint` for changed QML. Update state files with exact test counts, failed-baseline comparison if any, tag/object/peeled values, and remaining physical acceptance evidence. Then commit only source/tests/docs:

```powershell
git add state/README.md state/TODO.md state/LOG.md
git commit -m "test[quad-rover]: record integration evidence"
```

Physical sign-off remains separate: attach the source commit, CMake cache, resolved MAVLink commit, desktop binary, automated output, MAVLink 2 capture, and evidence for Quad, Rover, transform, landed rejection, fault, and a mission item. Do not claim physical acceptance from a successful desktop build alone.
