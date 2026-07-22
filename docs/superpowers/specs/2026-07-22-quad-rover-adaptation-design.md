# Quad-Rover PX4 Adaptation Design

## Goal

Adapt QGroundControl to the independent PX4 Quad-Rover vehicle implemented by
`change1_v1.16.1`. The application must recognize the vehicle as a permanent,
non-VTOL type, display its authoritative transformation state, safely request
shape transitions, filter shape-dependent flight modes, and preserve the
hybrid transition mission command.

The implementation must meet the protocol and acceptance contract in
`qgc-quad-rover-agent-guide.md`. This document turns that contract into QGC
ownership boundaries and testable behavior.

## Baseline And Dependency Contract

The protocol reference remains PX4 interface revision `82478dbdf2` (or a
contract-preserving descendant) and the `zeroone_x6_hybrid` target. The user
selected `codex/joystick-aux-px4-development` at
`754135601a53d7650ddeb6562ca5a5cd2167880c` as the QGC implementation base.
It intentionally contains the validated downstream work needed by this team;
this change must not rebase it onto upstream QGC main as part of the feature.

All hybrid builds use MAVLink 2 and the published source below:

| Setting | Required value |
| --- | --- |
| Repository | `https://github.com/QQgdiw/mavlink.git` |
| Release tag | `hybrid-change1-v1.16.1` |
| Expected peeled commit | `3b84efb97a7c0b4767868e8725bd6902c0d884e8` |
| Dialect | `hybrid_vehicle` |
| MAVLink version | `2.0` |

The tag is the release input. The maintenance branch
`change1_v1.16.1-hybrid` is never a release-build dependency. CMake must fail
early when the configured repository, tag, dialect, protocol version, or
resolved source commit differ from this contract. It must not copy generated
MAVLink headers into the repository or fall back to QGC's stock `all` dialect.

On this dedicated integration branch, make the four values the hybrid-build
defaults and add an early configure-time contract check. Verify the fetched
MAVLink source HEAD after CPM resolves the tag and record the resolved commit
in the build deliverable. A build that overrides this branch back to a stock
dialect is unsupported and must fail at configure time, rather than compiling
feature sources against headers that lack type 200, command 50000, and message
60000. Upstream QGC remains unaffected because this branch is not merged or
rebased as part of the task.

The configure test suite includes negative cases for an incorrect repository,
tag, resolved commit, dialect, and MAVLink version. Each must stop before C++
compilation; a successful hybrid configure alone is not sufficient evidence.

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
- transition sequence and elapsed milliseconds
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
only for a MAVLink 2 Quad-Rover and never parses its payload manually. A local
timer changes only the derived stale notification after three seconds; it does
not alter any decoded wire field or infer Quad/Rover state. This resolves the
distinction between immutable telemetry and local freshness.

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

The existing `MavCommandQueue` owns only reliable initial dispatch. Its normal
post-ACK window is about 1.2 seconds, while PX4's `HYBRID_TRANS_T` defaults to
six seconds and PX4 sends one initial `IN_PROGRESS` plus one eventual terminal
ACK. Leaving this command in the generic queue would therefore create a false
no-response failure during a healthy transformation.

Generalize the existing Autotune detach-on-progress pattern into an explicit
handler policy with a safe default that retains current queue behavior. The
Hybrid controller opts into detach-on-progress: the queue applies its existing
dispatch/retry policy only while waiting for the first valid ACK (the current
safety policy sends command 50000 once), removes its timed entry when it
receives `IN_PROGRESS`, and passes that progress ACK to the controller.
`Vehicle` routes a later terminal command-50000 ACK only while that controller
has the matching detached transaction. A direct terminal ACK before progress
stays on the normal queue result-handler path. This avoids raw `COMMAND_LONG`
packing, global timeout changes, duplicate ACK delivery, duplicate command
queue entries, and a hidden command-specific queue hack.

Detached ACK routing validates command 50000, the active vehicle system id,
the originally addressed autopilot component, and the QGC source
system/component named by the ACK. The generic queue currently matches only
command and sending component, so the controller must retain this stricter
transaction correlation rather than accepting an unrelated same-command ACK.

After detach, fresh message 60000 telemetry owns physical-state liveness. The
controller adds the hybrid-specific behavior that neither generic dispatch nor
telemetry owns alone:

- Store `COMMAND_ACK.result_param2` from the first in-progress ACK as the
  transition sequence.
- Treat a repeated in-progress ACK with that same sequence as a retransmission,
  not a second local request.
- Store an optional exact `uint32` sequence rather than using zero as an
  absence sentinel. Terminal ACK and confirming status sequence values must
  equal the stored value; no numeric "newer than" comparison is used, so
  unsigned wrap is safe.
- A direct `ACCEPTED` for an already stable requested shape has no local
  transition sequence and is rendered as no motion only when the existing
  fresh status already confirms that shape. It does not wait for a terminal
  ACK or manufacture a sequence.
- Keep one transaction only; an opposite target while transforming is rejected
  and no request is queued.
- Treat accepted-without-motion as complete only when PX4 reports it as such.
- After terminal `ACCEPTED`, wait for fresh status with the requested stable
  shape and matching exact sequence before reporting command success. If no
  such status is already fresh, wait at most the three-second freshness
  interval, then report an unconfirmed outcome and keep shape controls
  disabled until a matching fresh state arrives.
- After terminal `FAILED` or a status fault, report command failure/fault
  presentation and wait for a later fresh fault-free stable status before
  allowing another request.
- Do not automatically retry denied, temporarily rejected, invalid, or faulted
  requests.

If no first ACK arrives, the generic queue reports its existing no-response
failure. After detach, a fresh transitioning status keeps the controller in
progress without an arbitrary timer because PX4 is still reporting its physical
state. A stale status or a fault clears the transaction as unconfirmed and
keeps shape-dependent controls disabled. An out-of-order status with a
different sequence cannot confirm success, but it does not abort a still-fresh
transition. A candidate stable requested-shape status with a different sequence
is an unconfirmed result and keeps those controls disabled. If a terminal ACK
is absent but fresh status reaches the requested stable shape with the expected
sequence, QGC reports completion ACK missing but enables physical mode controls
from that authoritative state. A terminal ACK alone never enables those
controls. No path retries automatically or stays pending after stale/fault
state.

The UI may pre-disable a transform when status is stale, the vehicle is not
landed, land detection is stale, a fault exists, or a transition is active.
That is usability feedback only; the command ACK remains the authoritative
safety result.

## PX4 Plugin, Modes, And QML

Add a hybrid-specific policy layer beneath the PX4 firmware plugin and the
existing Flight Mode menu/toolbar patterns. It consumes the fresh
`HybridVehicleState`, not heartbeat type, VTOL fields, or stale telemetry.

- Stable Quad offers PX4 multirotor-supported modes subject to normal PX4
  availability checks.
- Stable Rover offers only Manual, Position, Mission, Hold/Loiter, Return,
  Acro, Offboard, and Stabilized.
- Transitioning, unknown, stale, and fault states disable shape-dependent
  requests and never queue a later request.

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
hybrid tag, wrong resolved MAVLink commit, wrong dialect, or wrong MAVLink
version. No log may contain credentials or tokens.

## Test Strategy And Acceptance Evidence

Add deterministic unit/protocol fixtures and focused QML or controller tests
that prove all of the following:

1. The published dialect generates and compiles type 200, command 50000, and
   message 60000; every wrong repository, tag, resolved commit, dialect, or
   MAVLink version configure input fails before C++ compilation; a MAVLink 1
   path never claims hybrid support.
2. Type 200 selects `VehicleClassQuadRover`, leaves `vtol` false, and does not
   enter existing VTOL behavior.
3. The initial no-status state, each status enum, every independent flag bit,
   a no-fault false diagnostic health flag, NaN position, and the three-second
   stale transition are represented without shape inference or synthetic fault.
4. The controller covers denied, temporarily rejected, no-motion accepted,
   in-progress retransmission, opposite-target rejection, terminal accepted,
   terminal failed, missing initial ACK, missing terminal ACK, a healthy
   six-second transformation without queue no-response failure,
   missing/mismatched confirmation status, exact sequence correlation including
   zero and unsigned wrap, wrong ACK sender/address rejection, and later
   fault-free stable recovery.
5. A terminal accepted ACK alone cannot enable Quad/Rover controls; fresh
   matching stable status is required.
6. Mode filtering offers the exact Rover list and disables all shape-dependent
   actions in transitioning, unknown, stale, and fault states.
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
does not change PX4 firmware, MAVLink numeric IDs, the upstream QGC branch, or
existing standard VTOL behavior.
