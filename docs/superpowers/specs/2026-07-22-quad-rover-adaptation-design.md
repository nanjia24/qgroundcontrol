# Quad-Rover PX4 Adaptation Design

## Goal

Adapt QGroundControl to the independent PX4 Quad-Rover vehicle implemented by
`change1_v1.16.1`. The application must recognize the vehicle as a permanent,
non-VTOL type, display its authoritative transformation state, safely request
shape transitions, filter shape-dependent flight modes, and preserve the
hybrid transition mission command.

The implementation must meet the protocol and acceptance contract in
`qgc-quad-rover-agent-guide.md`. Until the PX4/MAVLink agent revises that guide
for the composite dialect release, this document supersedes its incompatible
raw-`hybrid_vehicle` build statements and turns the remaining contract into QGC
ownership boundaries and testable behavior.

## Baseline And Dependency Contract

The protocol reference remains PX4 interface revision `82478dbdf2` (or a
contract-preserving descendant) and the `zeroone_x6_hybrid` target. The user
selected `codex/joystick-aux-px4-development` at
`754135601a53d7650ddeb6562ca5a5cd2167880c` as the QGC implementation base.
It intentionally contains the validated downstream work needed by this team;
this change must not rebase it onto upstream QGC main as part of the feature.

QGC must use a combined MAVLink 2 dialect rather than `hybrid_vehicle` alone.
The fixed dialect identifier is `qgc_hybrid`, defined in the QQgdiw MAVLink
fork as `message_definitions/v1.0/qgc_hybrid.xml`:

```xml
<mavlink>
  <include>all.xml</include>
  <include>hybrid_vehicle.xml</include>
</mavlink>
```

`all.xml` retains its `ardupilotmega` inclusion and existing QGC-compatible
dialects; `hybrid_vehicle.xml` contributes type 200, command 50000, and
message 60000. Its transitive `development.xml` inclusion is already present
through `all.xml`; the MAVLink release CI must generate this composite dialect
and reject any duplicate enum or message-id definition. The final generated
headers must expose both `mavlink/qgc_hybrid/mavlink.h` and the included
`ardupilotmega` headers.

| Setting | Required value |
| --- | --- |
| Repository | `https://github.com/QQgdiw/mavlink.git` |
| Dialect | `qgc_hybrid` |
| MAVLink version | `2.0` |
| `QGC_DISABLE_APM_MAVLINK` | `OFF` |
| `QGC_DISABLE_APM_PLUGIN` | `OFF` |

The existing immutable tag `hybrid-change1-v1.16.1` at
`3b84efb97a7c0b4767868e8725bd6902c0d884e8` remains the published
`hybrid_vehicle` contract, but it does not contain `qgc_hybrid` and is not a
valid final QGC build input. A PX4/MAVLink release gate must publish an
immutable tag containing the composite XML, provide its peeled commit, and
update the authoritative integration guide before QGC feature implementation
may be configured. The maintenance branch remains development-only.

Once that release gate is satisfied, this integration branch sets
`QGC_MAVLINK_GIT_REPO`, `QGC_MAVLINK_GIT_TAG`,
`QGC_MAVLINK_DIALECT=qgc_hybrid`, and `QGC_MAVLINK_VERSION=2.0` as its
defaults. An early CMake contract check verifies the repository, immutable tag,
peeled source commit, dialect, protocol version, and both enabled APM options.
It must neither copy generated headers into the repository nor accept a
fallback to raw `all` or raw `hybrid_vehicle`: only `qgc_hybrid` supplies both
compatibility domains. Upstream QGC remains unaffected because this branch is
not merged or rebased as part of the task.

The configure test suite includes negative cases for an incorrect repository,
tag, resolved commit, dialect, MAVLink version, or disabled APM dialect/plugin.
Each must stop before C++ compilation. A positive composite-dialect build must
compile references to type 200, command 50000, message 60000, and an
`ardupilotmega` symbol; successful hybrid-only generation is not sufficient
evidence.

## Options Considered

1. First-class Quad-Rover support with a dedicated state object, transition
   controller, mode policy, and mission metadata (chosen). It matches the PX4
   contract while keeping hybrid behavior out of existing VTOL and rover
   paths.
2. MAVLink compatibility only: compile the dialect and expose raw message 60000
   in the inspector. This is useful only for protocol bring-up and fails the
   required transform, mode, mission, and safety acceptance criteria.
3. Reinterpret type 200 as VTOL or dynamically switch it between multirotor
   and rover. This would issue the wrong command family, conflict with PX4's
   explicit `is_vtol=false` behavior, and create regressions in normal VTOL
   presentation. It is rejected.

### Dialect Strategy

1. `qgc_hybrid` includes `all.xml` and `hybrid_vehicle.xml` (chosen). It
   retains QGC's ArduPilot and ordinary-dialect compatibility while adding the
   hybrid symbols in one generated header family.
2. A `hybrid_vehicle`-only QGC that forces both APM MAVLink and plugin support
   off is rejected. It would make a PX4 hardware adaptation silently change
   the product's supported vehicle set.
3. Keeping QGC on raw `all` and adding ad-hoc hybrid headers is rejected. It
   creates two generated MAVLink universes and weakens the release contract.

## Protocol Model

`HEARTBEAT.type` is permanently `MAV_TYPE_QUAD_ROVER = 200`.
`MAV_CMD_DO_HYBRID_TRANSITION = 50000` is the only transformation command.
`HYBRID_VEHICLE_STATUS = 60000` is the only authority for current shape,
target shape, fault, health, and transition correlation.

`EXTENDED_SYS_STATE.vtol_state` is always undefined for this vehicle and is
never used for Quad-Rover decisions. `Vehicle::vtol` remains false. A MAVLink
1 connection may identify type 200 but is not hybrid-capable: QGC must show the
feature unavailable and must not infer state, enable a transform, or offer
shape-dependent controls.

## Vehicle Classification And State Ownership

`QGCMAVLink` gains `VehicleClassQuadRover`, `isQuadRover(MAV_TYPE)`, type
string conversion, user-visible naming, and inclusion in all presentation
lists. Type 200 maps only to this class. The PX4 firmware plugin remains the
selected firmware plugin because the autopilot is still PX4.

`Vehicle` owns one focused `HybridVehicleState` QObject for each Quad-Rover.
It is exposed read-only to QML and holds these decoded wire values:

- current state and target shape
- PX4 status timestamp, transition sequence, elapsed milliseconds, and command
  timestamp
- normalized position, including an explicit unavailable representation for NaN
- fault reason, command result, sensor source, actuator backend, raw actuator
  protection flags, decoded status flags, and command timestamp
- local monotonic receipt time and derived `stale` state

Before the first valid MAVLink 2 status, the object starts with
`currentState=UNKNOWN`, `targetState=NONE`, `hasValidStatus=false`, and
`stale=true`. It therefore disables transformation and shape-dependent controls
without treating a default enum value as a stable physical shape.

`Vehicle::_mavlinkMessageReceived` handles message 60000 through the generated
`mavlink_msg_hybrid_vehicle_status_decode` function. It accepts the message
only for a MAVLink 2 Quad-Rover and never parses its payload manually. Within a
Vehicle session it ignores a sample whose PX4 `timestamp` is not newer than the
last accepted sample; a vehicle reconnect/reset creates a new state session.
This prevents a late status packet from superseding a newer transition result.
A local timer changes only the derived stale notification after three seconds;
it does not alter any decoded wire field or infer Quad/Rover state. This
resolves the distinction between immutable telemetry and local freshness.

Any nonzero fault reason, `TRANSITION_FAULT`, unknown state, or stale status
blocks new transformations and shape-dependent mode requests. Each status flag
is decoded independently; no health or landed value is inferred from another
flag.

`ACTUATOR_ONLINE`, `ACTUATOR_HEALTHY`, and the other diagnostic flags are shown
independently but are not synthetic QGC fault gates. PX4's transition policy
gates on landed freshness, landed state, transformation fault, current state,
and requested target; QGC must not reject a no-fault request solely because a
diagnostic flag is false. The firmware ACK and fault state remain authoritative.

Quad-Rover parameters remain in the existing PX4 Fact and component-metadata
path. There is no private parameter store or bespoke setup component. The
implementation verifies that `MAV_TYPE=200` metadata is accepted and that
`HYBRID_TRANS_T`, `HYB_SENS_EN`, `HYB_ACT_TYPE`, and related `HYB_*` parameters
remain discoverable through normal PX4 parameter UI. New hybrid telemetry is
not synthesized from those parameters.

## Transition Controller

`Vehicle` owns a `HybridTransitionController` that is exposed to the hybrid
QML surface. It sends command 50000 through `Vehicle::sendMavCommandWithHandler`
with finite zeroes for parameters 2 through 7 and target shape 1 (Quad) or 2
(Rover) in parameter 1.

`MavCommandQueue` owns reliable initial dispatch, but the hybrid controller
owns **all command-50000 ACK semantics from the first ACK**. Its ordinary
post-`IN_PROGRESS` timeout is about 1.2 seconds, while PX4 emits one progress
ACK and one eventual terminal ACK. `HYBRID_TRANS_T` is an airframe parameter,
not a QGC timeout: airframe 22001 currently sets it to 2.0 seconds although the
module-level parameter fallback is 6.0. QGC neither reads nor hardcodes either
value. It tests only that a live transition which outlasts the generic 1.2-second
window does not become a false no-response failure.

Generalize the existing Autotune detach-on-progress pattern into an explicit
entry-level `ackMatcher` and detach-on-progress policy with safe defaults that
retain every existing command's behavior. Command 50000 is sent once through
`MavCommandQueue` to the concrete `MAV_COMP_ID_AUTOPILOT1`, never to
`MAV_COMP_ID_ALL` and never through raw `COMMAND_LONG` packing. Its matcher is
run before the queue consumes **every** ACK, including a direct terminal ACK
that arrives before progress. `Vehicle` already filters inbound messages to its
system id; the hybrid matcher additionally requires command 50000, the sent
autopilot component as `message.compid`, and `COMMAND_ACK.target_system` and
`target_component` equal to QGC's local MAVLink source address. A mismatch
leaves the queue entry intact and has no controller/UI side effect.

On the first strictly matched `IN_PROGRESS`, the queue removes the timed entry
and calls the controller progress handler. `Vehicle` routes later terminal ACKs
only while that controller has a detached transaction, using the same matcher.
A direct terminal ACK instead reaches the controller through the queue result
handler. No `Vehicle::_handleCommandAck` command-50000 side effect may run
before this matching boundary. This avoids raw packing, global timeout changes,
duplicate ACK delivery, duplicate queue entries, and a command-specific queue
hack.

After detach, fresh message 60000 telemetry owns physical-state liveness. The
controller adds the hybrid-specific behavior that neither generic dispatch nor
telemetry owns alone:

- At send time, retain the latest accepted status timestamp and
  `command_timestamp` as the pre-request baseline.
- Store `COMMAND_ACK.result_param2` from the first in-progress ACK as an
  optional exact `uint32` transition sequence. A repeated in-progress ACK with
  that same sequence is a retransmission, not a second local request. Sequence
  zero is valid, and no numeric sequence ordering is used.
- When a fresh status first reports that exact sequence with
  `command_result=MAV_RESULT_IN_PROGRESS`, latch its PX4 `command_timestamp`.
  A subsequent stable-status confirmation must retain that timestamp. If an
  unusually fast transition supplies no observed in-progress status, its
  confirmation must at least have a non-baseline command timestamp.
- A status fallback is successful only when its monotonic PX4 timestamp is
  post-request, its sequence exactly matches, its `current_state` is the
  requested stable Quad/Rover shape, its fault is clear,
  `command_result=MAV_RESULT_ACCEPTED`, and the command-timestamp rule above
  holds. `target_state` is not used as this stable confirmation because PX4 may
  clear it to `NONE` after completion.
- A direct `ACCEPTED` for an already stable requested shape has no local
  transition sequence and is rendered as no motion only when the existing
  fresh status already confirms that shape. It does not manufacture a sequence
  or await a terminal ACK.
- Keep one transaction only; an opposite target while transforming is rejected
  and no request is queued. Do not automatically retry denied, temporarily
  rejected, invalid, faulted, or unconfirmed requests.

If no first ACK arrives, the generic queue reports its existing no-response
failure. After detach, a fresh transitioning status keeps the controller in
progress without an arbitrary transition-duration timer. After a matched
terminal `ACCEPTED`, QGC waits at most the existing three-second *telemetry
freshness* interval for the full status-confirmation predicate above; it then
reports unconfirmed and keeps shape controls disabled. A terminal `FAILED`, a
fault, or stale telemetry ends the transaction without success.

The state object rejects status packets that are older by PX4 timestamp. If a
newer accepted status has a sequence different from the active expected
sequence, the controller immediately ends the old request as
`SupersededUnconfirmed`; it never compares sequence numbers numerically. Shape
controls remain disabled until a later fresh status with that superseding
sequence independently reports a fault-free stable shape and
`MAV_RESULT_ACCEPTED`. That resynchronizes physical presentation but never
rewrites the old request as successful. If a terminal ACK is absent, the same
full status-confirmation predicate reports completion with ACK missing and may
enable physical mode controls. A terminal ACK alone never enables them.

The UI may pre-disable a transform when status is stale, the vehicle is not
landed, land detection is stale, a fault exists, or a transition is active.
That is usability feedback only; the command ACK remains the authoritative
safety result.

## PX4 Plugin, Modes, And QML

Add a hybrid-specific `EffectiveShapeProfile` beneath the PX4 firmware plugin
and existing Flight Mode menu/toolbar patterns. It is a mode-policy-only
projection of fresh `HybridVehicleState`, with `QuadMultiRotor`, `Rover`, and
`Unavailable` values. It never changes `VehicleClassQuadRover`,
`Vehicle::multiRotor()`, `Vehicle::fixedWing()`, or `Vehicle::vtol()`; those
global classifications retain their existing meaning for every other feature.

`PX4FirmwarePlugin::flightModes` must consult this profile before its current
generic `other` branch. In `QuadMultiRotor`, it filters the existing PX4 list
with `mode.multiRotor` exactly as a native multicopter would. In `Rover`, a
localized hybrid policy filters by PX4 custom-mode identity to only Manual,
Position, Mission, Hold/Loiter, Return, Acro, Offboard, and Stabilized. It does
not add a global rover bit to the existing `FirmwareFlightMode` structure. The
same profile is used by PX4 mode-related capability checks that currently rely
on `vehicle->multiRotor()` (for example the Quad-only action capability path),
so a stable Quad retains its multicopter mode capabilities without pretending
to be a global multicopter.

Transitioning, unknown, stale, fault, and `SupersededUnconfirmed` states map to
`Unavailable`: they disable shape-dependent requests and never queue a later
request. The policy consumes no heartbeat-type inference, VTOL fields, or
stale telemetry, and it never silently transforms a vehicle to satisfy a mode
request.

The QML surface contains a compact state/fault indicator and explicit Quad and
Rover transform actions. It uses the existing toolbar and Flight Mode menu
patterns, QGCPalette, ScreenTools sizing, and active-vehicle null checks. It
does not expose a VTOL action, call `MAV_CMD_DO_VTOL_TRANSITION`, or silently
transform to satisfy a mode request.

## Mission Integration

Register command 50000 with generated MAVLink enums, common mission metadata,
and a Quad-Rover-specific PX4 mission override/list so only type 200 offers it.
The command is a non-positional action item with parameter 1 restricted to
integral values 1 and 2. Parameters 2 through 7 must be finite zeroes.
Coordinate, frame, and altitude UI are not exposed.

The mission editor and transfer path preserve the item during load, save,
upload, and download. A focused validator rejects fractional, NaN, or infinite
parameter 1 and any nonzero, NaN, or infinite reserved parameter before upload.
It never translates this command to `MAV_CMD_DO_VTOL_TRANSITION`. Mission
execution progress remains vehicle-reported; upload ACKs and in-progress ACKs
do not advance local mission presentation.

## Error Boundaries

Expected vehicle failures are represented as controller/state results: stale
telemetry, denied/temporarily rejected command ACKs, fault status, invalid
mission item values, and MAVLink 1 incompatibility. They result in clear UI
state and disabled unsafe commands, not process exceptions.

Configuration-contract violations are fatal configure errors: unavailable
combined-dialect tag, wrong resolved MAVLink commit, a dialect other than
`qgc_hybrid`, wrong MAVLink version, or either disabled APM option. No log may
contain credentials or tokens.

## Test Strategy And Acceptance Evidence

Add deterministic unit/protocol fixtures and focused QML or controller tests
that prove all of the following:

1. The published `qgc_hybrid` dialect generates and compiles type 200, command
   50000, message 60000, and an `ardupilotmega` symbol. Every wrong repository,
   tag, resolved commit, dialect, MAVLink version, or disabled APM option fails
   before C++ compilation; raw `all` and raw `hybrid_vehicle` are negative
   inputs. MAVLink release CI proves the XML's include graph has no duplicate
   enum/message definition. A MAVLink 1 path never claims hybrid support.
2. Type 200 selects `VehicleClassQuadRover`, leaves `vtol` false, and does not
   enter existing VTOL behavior.
3. The initial no-status state, each status enum, every independent flag bit,
   a no-fault false diagnostic health flag, NaN position, and the three-second
   stale transition are represented without shape inference or synthetic fault.
   An out-of-order status timestamp is ignored and cannot replace newer state.
4. The controller covers denied, temporarily rejected, no-motion accepted,
   direct terminal ACK and in-progress ACK strict matching from the first ACK,
   retransmission, opposite-target rejection, terminal accepted, terminal
   failed, missing initial ACK, missing terminal ACK, and a healthy transition
   that outlasts the generic 1.2-second queue window without reading or
   hardcoding `HYBRID_TRANS_T`. It covers exact sequence correlation including
   zero and unsigned wrap, ACK sender/address rejection, command-timestamp
   latching, the full status-success predicate, and later fault-free recovery.
5. A terminal accepted ACK alone cannot enable Quad/Rover controls. Status
   fallback requires the matching sequence, requested stable current shape,
   `MAV_RESULT_ACCEPTED`, and the command-timestamp association. A newer status
   with a different sequence ends the old transaction as
   `SupersededUnconfirmed`; only independent later stable-status resync can
   restore physical controls.
6. `VehicleClassQuadRover` remains neither global multirotor nor VTOL. The
   effective profile exposes the normal PX4 multicopter list/capabilities in
   stable Quad, the exact Rover list in stable Rover, and no shape-dependent
   actions in transitioning, unknown, stale, fault, or superseded states.
7. Mission plans preserve command 50000 and reject every invalid parameter
   combination before transfer. No hybrid path emits command 3000, and normal
   PX4 multirotor, VTOL, and rover classes neither offer nor create command
   50000.
8. PX4 parameter metadata accepts type 200 and exposes the `HYB_*` parameters
   through Facts without a parallel parameter store.

Before physical sign-off, provide the QGC source commit, resolved CMake cache,
resolved MAVLink commit, desktop binary, automated-test output, a MAVLink 2
capture, and screenshots or logs for stable Quad, stable Rover, successful
transform, landed rejection, fault, and a mission transform item.

## Out Of Scope

This work does not add camera, video, gimbal, payload, or new component-metadata
protocol support because the PX4 comparison showed no such hybrid contract. It
does not publish or mutate the MAVLink release itself: the PX4/MAVLink agent
owns `qgc_hybrid`, its immutable tag, and its peeled-commit release contract.
It does not change PX4 firmware, MAVLink numeric IDs, the upstream QGC branch,
or existing standard VTOL behavior.
