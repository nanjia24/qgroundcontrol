# Windows Desktop QGC UX and Functional Assessment

**Date:** 2026-08-04
**QGC baseline:** `0baf101a01fd9a8044f9200e559ba224cf74ff14`
**Target:** Windows desktop QGroundControl for the PX4 Quad-Rover airframe 22001
**MAVLink contract:** `qgc_hybrid`, tag `qgc-hybrid-change1-v1.16.1-r2`, peeled commit `04ad1d63e9c11ed6767a35dae4e52adaca3538c5`

## 1. Scope and evidence policy

This assessment covers only the native Windows desktop QGC application. Android, iOS, handheld-controller layouts, and mobile-only products are out of scope. It assesses both existing QGC usability and gaps introduced by treating the Quad-Rover as a first-class vehicle.

Evidence is classified as follows:

- **S:** locally inspectable source, generated protocol headers, or open-source implementation.
- **D:** an official product manual or documentation page describing an operator-visible function.
- **M:** a vendor marketing claim that was not independently executed.
- **O:** a dated live-product observation without an immutable build or archived artifact.

Public claims are not treated as hardware acceptance. The local QGC findings are source-backed, but visual and physical-aircraft acceptance still require the Windows executable and target aircraft.

## 2. Executive finding

The protocol and state-recovery layer is substantially stronger than the operator workflow built on top of it. The current Windows UI exposes a compact transform control, but it does not yet make the Quad-Rover feel like a coherent QGC vehicle class. Several gaps are functional or safety-relevant rather than cosmetic:

1. Airframe 22001 is presented as a custom airframe even though its image is already packaged.
2. A transform is a single-click action, unlike QGC's confirmed VTOL transition control.
3. Non-idle recovery states can leave transform buttons visually enabled even though the controller rejects the request.
4. Stable Rover shape still receives flight-style virtual-throttle behavior.
5. Ground operation is filtered through the generic `flying` predicate, hiding useful actions and exposing `Land` for a Rover shape.

These items are P0. Search, workspace persistence, richer planning, and consolidated diagnostics matter, but they should follow correction of the vehicle identity and command/control policy.

## 3. Existing strengths to preserve

- The combined `qgc_hybrid` dialect retains PX4 and ArduPilot compatibility and has a pinned, executable release contract.
- Hybrid state freshness, reboot recovery, command correlation, transaction reservation, and effective mode filtering have focused Windows tests.
- Parameter editing already includes search, Modified/Favorites tabs, file save/load, a selectable diff preview, and per-parameter editing (`src/QmlControls/ParameterEditor.qml:42`, `src/QmlControls/ParameterDiffDialog.qml:9`).
- Analyze pages can be popped into independent Windows (`src/UI/MainWindow.qml:653`, `src/AnalyzeView/AnalyzeView.qml:130`).
- Existing QGC confirmation primitives such as `QGCDelayButton` can be reused instead of introducing another interaction framework (`src/UI/toolbar/FlightModeIndicator.qml:136`).

## 4. Native Windows QGC inconvenience assessment

| ID | Priority | Finding | Source evidence | Operator impact | Direction |
| --- | --- | --- | --- | --- | --- |
| N-01 | P1 | Main navigation is a two-column dropdown; Fly, Plan, Analyze, Configure, and Settings have no registered desktop keyboard shortcuts. | `src/UI/toolbar/SelectViewDropdown.qml:15`; no `Shortcut`, `QShortcut`, or `QKeySequence` registration exists under `src`. | Repeated view switching costs mouse travel and hides the application information architecture. | Add discoverable Windows shortcuts and command search after the Hybrid P0 work. |
| N-02 | P1 | Mission command selection has category filtering only. It has no text search, recent commands, or favorites. | `src/QmlControls/MissionCommandDialog.qml:21` | Long command catalogs are slow to scan and error-prone. | Add search first; recents/favorites only if usage data justifies them. |
| N-03 | P1 | An incomplete mission item shows only `?`; explanatory text exists but is commented out. | `src/PlanView/MissionItemEditor.qml:112`, `src/PlanView/MissionItemEditor.qml:329` | The planner reports invalid state without telling the operator what to fix. | Restore a compact reason with a direct focus/deep-link action. |
| N-04 | P2 | Analyze exposes `Onboard Logs` and `Onboard Logs (FTP)` as separate pages with the same icon. | `src/API/QGCCorePlugin.cc:65` | Operators must know the transfer protocol before choosing a workflow. | Present one log workspace and choose the supported transport internally. |
| N-05 | P2 | Analyze pop-outs are transient; only the main window geometry is persisted. | `src/UI/MainWindow.qml:653`; `src/QmlControls/MainWindowSavedState.qml:9` | Multi-monitor diagnostic layouts must be recreated on every session. | Persist approved pop-out geometry and selected pages as a Windows workspace. |
| N-06 | P2 | High-level warnings, MAVLink messages, log retrieval, and inspection tools are split across toolbar and Analyze surfaces. | `src/UI/toolbar/MainStatusIndicator.qml`; `src/API/QGCCorePlugin.cc:65` | Incident reconstruction requires manual context switching. | Add a unified event timeline and diagnostic bundle without removing expert tools. |

## 5. Quad-Rover discontinuity assessment

| ID | Priority | Finding | Source evidence | Operator impact | Direction |
| --- | --- | --- | --- | --- | --- |
| H-01 | P0 | Airframe metadata has no 22001 entry. Unknown `SYS_AUTOSTART` values are classified as custom and offered a Reset path. A previously downloaded firmware metadata cache takes priority over the packaged XML. | `src/AutoPilotPlugins/PX4/PX4AirframeLoader.cc:49`; `src/AutoPilotPlugins/PX4/AirframeComponentController.cc:35`; `src/AutoPilotPlugins/PX4/AirframeComponent.qml:17`; `src/AutoPilotPlugins/Common/CMakeLists.txt:96` | A supported aircraft can remain unsupported after an application update and an operator is led toward a destructive reset. | Register 22001 in an always-loaded QGC overlay, independent of the firmware metadata cache, and test metadata/resource identity. |
| H-02 | P0 | Transform buttons are immediate `QGCButton` actions, while the existing VTOL transition uses `QGCDelayButton`. | `src/UI/toolbar/HybridTransitionIndicator.qml:64`; `src/UI/toolbar/FlightModeIndicator.qml:136` | A safety-significant mechanical action is easier to trigger accidentally than the analogous native action. | Use hold-to-confirm with a clear target shape. |
| H-03 | P0 | Button enablement checks `!controller.busy`, but recovery/fault states are not busy and `requestTransform` accepts only `Idle`. The QML ignores the returned `false`. | `src/Vehicle/HybridTransitionController.cc:10`; `src/Vehicle/HybridTransitionController.cc:46`; `src/UI/toolbar/HybridTransitionIndicator.qml:66` | Controls can look enabled yet do nothing. | Gate on exact `Idle`, surface rejection/recovery state, and never discard a failed request silently. |
| H-04 | P0 | The static Quad-Rover class is neither `rover()` nor `multiRotor()`. Manual-control throttle policy reads only the static class. | `src/Vehicle/Vehicle.cc:1800`; `src/FlyView/VirtualJoystick.qml:20`; `src/Joystick/Joystick.cc:1204` | Stable Rover shape receives flight-style throttle or loses reverse, while transient Hybrid-status loss can disrupt control if mode availability is reused as an input gate. | Preserve a separate last-confirmed manual-control shape profile for both Windows input paths; gate only until the first stable shape is known. |
| H-05 | P0 | Guided actions use `flying` for RTL, Pause, Goto, and related actions, but show Land for any armed non-fixed-wing vehicle. | `src/FlyView/GuidedActionsController.qml:129`; `src/Vehicle/Vehicle.cc:1083` | Ground-mobile operation can lose useful actions and show a meaningless Land action. | Use ground-mobile/effective-shape capability policy rather than landed-state semantics. |
| H-06 | P1 | PID Tuning and Flight Behavior omit `MAV_TYPE_QUAD_ROVER`; an empty setup source is hidden by Vehicle Setup. | `src/AutoPilotPlugins/PX4/PX4TuningComponent.cc:45`; `src/AutoPilotPlugins/PX4/PX4FlightBehavior.cc:45`; `src/Vehicle/VehicleSetup/VehicleConfigView.qml:388` | Standard PX4 setup pages disappear for the new vehicle type. | Route Quad-Rover to the applicable PX4 setup pages, then add dedicated transform setup. |
| H-07 | P1 | Preflight checklist selection does not recognize Quad-Rover, so it loads the generic checklist. | `src/FlyView/PreFlightCheckList.qml:61`; `src/FlyView/DefaultChecklist.qml` | The operator is asked about incompatible airframe parts and receives no transform-mechanism checks. | Add a Hybrid checklist with automatic status checks and manual obstruction/lock checks. |
| H-08 | P1 | `Stale` text is unreachable because `hasValidStatus` is checked first; faults are rendered as raw integers. | `src/UI/toolbar/HybridTransitionIndicator.qml:18`; generated `HYBRID_VEHICLE_FAULT` enum has 11 non-zero reasons. | Operators cannot distinguish telemetry loss, sensor conflict, timeout, actuator protection, or configuration failure. | Provide translated state, fault, transaction, and disabled-reason strings. |
| H-09 | P1 | Controller transaction/result, sensor source, actuator backend, protection flags, and health fields exist but are not exposed as an operator diagnostic surface. | `src/Vehicle/HybridTransitionController.h:13`; `src/Vehicle/HybridVehicleState.h:10` | Recovery states become a silent disabled control rather than an actionable diagnosis. | Add a compact status drawer with recovery guidance and a Setup diagnostics page. |
| H-10 | P1 | Quad-Rover is absent from preferred/offline vehicle classes. | `src/Settings/App.SettingsGroup.json:17`, `src/Settings/App.SettingsGroup.json:37` | A disconnected operator cannot author or validate a native Quad-Rover plan. | Add an offline class only after mission-command availability and vehicle construction are proven. |
| H-11 | P1 | Mission command 50000 has only target selection. Mission time/status calculation models VTOL transitions but not Hybrid shape, transition time, or shape-specific speed. | `src/MissionManager/MavCmdInfoCommon.json:1294`; `src/MissionManager/MissionFlightStatusCalculator.cc:224` | ETA and feasibility can be wrong, and an invalid shape sequence is not localized to a mission item. | Build a shape timeline, use the actual `HYBRID_TRANS_T` Fact, and highlight the first invalid segment. |
| H-12 | P1 | Hybrid mode filtering can return an empty list during stale/fault/transaction state, but the mode UI does not explain why. | `src/FirmwarePlugin/PX4/PX4FirmwarePlugin.cc:100`; `src/UI/toolbar/FlightModeIndicator.qml` | Modes appear to disappear unpredictably. | Keep controls disabled with the effective-shape reason and recovery status. |
| H-13 | P2 | Hybrid text has no entries in the checked-in `.ts` catalogs. | Zero matches for the new Hybrid strings under `translations/*.ts`. | Chinese Windows builds mix English into core flight controls. | Run the established translation update workflow after operator text stabilizes. |
| H-14 | P2 | Fly toolbar is the only first-class Hybrid surface. Plan, Setup, Main Status, Multi-Vehicle, and Analyze lack shape semantics. | Hybrid QML integration is limited to `src/UI/toolbar/FlyViewToolBar.qml`. | The vehicle identity changes across workflows. | Reuse one effective-shape model and one presentation vocabulary across views. |

## 6. Functional gap assessment

### 6.1 Native gaps

- No global Windows shortcut/command layer for core view switching or command discovery.
- No direct mission-command search and no visible reason attached to an incomplete item.
- No persisted multi-window Analyze workspace, despite supporting transient pop-outs.
- No single operator timeline joining warnings, command outcomes, logs, and connection transitions.
- Parameter file diff exists and must be preserved; the later opportunity is acknowledged/staged writes and rollback evidence, not reimplementing basic diff.

### 6.2 Hybrid-specific gaps

- No normal airframe identity or dedicated transform setup/calibration page.
- No operator-facing mapping of the protocol state machine, fault enum, actuator health, sensor source, or recovery state.
- No shape-aware virtual/USB joystick, guided-action, checklist, offline-planning, or mission-estimation policy.
- No mission validation that proves the requested shape is stable before a shape-dependent leg.
- No transition history or diagnostic export containing sequence, ACK result, status result, elapsed time, source component, and recovery reason.

## 7. Competitor and adjacent-product evidence

### 7.1 MicoAir / “微空”

Identity is high-confidence: the [MicoAir about page](https://micoair.cn/zh/about) identifies the Shenzhen drone technology team, and the [ArduPilot partners page](https://ardupilot.org/dev/docs/common-partners.html) independently links MicoAir into the autopilot ecosystem.

- **D:** [MicoConfigurator](https://micoair.cn/configurator/) supports ArduPilot/PX4 in Windows Chrome/Edge without installation.
- **O (live observation, 2026-08-04):** the deployed UI included setup progress/deep links, sensor views, parameter diff, MAVLink messages, routes, logs, plots, firmware, hardware, and RTK sections.
- **O (live observation, 2026-08-04):** parameter import distinguished matching, file-only, vehicle-only, and changed values; motor mapping included propeller-removal confirmation, per-output execution, timeout, and PX4 rejection reasons.
- **Limit:** these functions were inspected in the deployed application and resources, not executed against hardware. It is a browser/PWA product, not a native Windows GCS. Its log page still advertises unfinished functionality and web mission planning reports terrain limitations.

**Adopt:** setup completion with deep links, staged parameter explanation, mechanism diagnostics, explicit preconditions, and phase-by-phase recovery.
**Do not adopt:** Web Serial device selection, store/catalog content, or mobile MicoPilot scope.

### 7.2 CUAV / “雷迅”

- **D/M:** the [CUAV LBA3 manual](https://doc.cuav.net/link/lba3/zh-hans/) documents a 30 Mbps maximum link, AES encryption, cloud/offline modes, and formation support when paired with CUAV LGC.
- **D:** the same manual instructs operators to create a QGC TCP link manually and use port 5760; payload video/control is configured through separate addresses/software.

**Adopt:** first-class link profile health, offline operation, encryption/link-state visibility, and future multi-vehicle control ownership.
**Do not claim:** a superior public Windows LGC workflow; the accessible evidence does not establish one. Avoid reproducing manual TCP and separate-payload fragmentation.

### 7.3 Mission Planner

- **S/D:** the [official overview](https://ardupilot.org/planner/docs/mission-planner-overview.html) and [open-source repository](https://github.com/ArduPilot/MissionPlanner) establish a Windows-only GCS with vehicle setup/tuning, mission planning, onboard/telemetry logs, HIL simulation, and FPV support.
- **D:** [configuration and tuning documentation](https://ardupilot.org/planner/docs/mission-planner-configuration-and-tuning.html) describes parameter file save/restore, diff selection, search, and changed-value workflows.
- **D:** [Flight Data documentation](https://ardupilot.org/planner/docs/mission-planner-flight-data.html) documents message, live-graph, video, and detachable Quick views.

**Adopt:** persistent message visibility and separable high-value diagnostic/telemetry views. Preserve QGC's existing selectable parameter diff and improve transaction feedback rather than duplicating it.
**Do not adopt:** dense tab/right-click interaction or ArduPilot-specific assumptions.

### 7.4 UgCS Desktop

- **D:** [installation documentation](https://manuals.ugcs.com/docs/installation) supports 64-bit Windows 10/11.
- **D:** [route calculation documentation](https://manuals.ugcs.com/docs/route-calculation) validates route feasibility, selects the first error segment, and recalculates after correction.
- **D:** [multi-operator documentation](https://manuals.ugcs.com/docs/ugcs-user-and-multioperator-work) describes mission editing locks and explicit vehicle-control ownership.
- **M:** the [product page](https://www.sphengineering.com/ugcs) claims full 3D preview, DEM/DSM import, offline workflows, mixed fleets, and specialized survey tools. “Only” and “industry-leading” language is not independently verified.

**Adopt:** locate Hybrid mission failures on the exact item/segment, show a shape/elevation timeline, and make control ownership explicit before multi-vehicle expansion.
**Do not adopt:** mandatory multi-service deployment or a 1080p-only layout.

### 7.5 Auterion Mission Control

- **D:** [Windows installation](https://docs.auterion.com/vehicle-operation/auterion-mission-control/installation) and the [product overview](https://auterion.com/product/mission-control/) establish the Windows-capable operator application.
- **D:** [high-level status documentation](https://docs.auterion.com/vehicle-operation/auterion-mission-control/ui-breakdown/fly/high-level-status) presents green/orange/red readiness with concrete causes and current VTOL shape.
- **D:** [preflight checklist documentation](https://docs.auterion.com/vehicle-operation/auterion-mission-control/useful-resources/mission-planning/preflight-checklist) combines automatic and manual checks and can block arming.
- **D:** camera/gimbal controls are capability-driven, and live PX4 log streaming covers arm-to-disarm sessions.

**Adopt:** readiness reasons, current shape in high-level state, automatic plus manual Hybrid checks, and capability-driven surfaces.
**Do not adopt:** cloud/vehicle binding, hiding all advanced diagnostics, or a tablet-first single-window constraint.

### 7.6 DJI Terra and FlightHub 2

- **D/M:** [DJI Terra](https://enterprise.dji.com/dji-terra) is now primarily reconstruction software; current documentation no longer supports treating it as a general GCS. Its useful interaction pattern is direct visual marking of risky geometry and actionable diagnostics.
- **D:** [FlightHub 2 documentation](https://fh.dji.com/user-manual/en/overview.html) describes roles, route libraries, map annotations, multi-stream video, and analysis. [Airspace alerts](https://fh.dji.com/user-manual/en/task-area-management/airspace-alert.html) are deduplicated, severity-ranked, historized, and exportable.

**Adopt later:** risk coloring, an alert history, and integrated payload context.
**Do not adopt:** DJI hardware lock-in, cloud dependency, reconstruction inside the real-time GCS, or a browser platform rewrite.

## 8. Prioritized Windows implementation backlog

### P0: correct unsafe or misleading behavior

1. Register airframe 22001 and expose applicable PX4 setup pages.
2. Centralize `effectiveMultiRotor` / `effectiveRover` in `Vehicle`; use it for PX4 modes, virtual joystick policy, and subsequent UI policy.
3. Give virtual and USB joysticks one last-confirmed manual-control shape profile, block only before the first stable shape is known, and use centered forward/reverse throttle in Rover shape.
4. Replace immediate transform clicks with hold-to-confirm, require exact controller `Idle`, and present transaction/fault/disabled reasons.
5. Correct Guided Actions for ground-mobile operation: no Land, and capability-appropriate RTL/Pause/Goto behavior.

### P1: close first-class workflow gaps

1. Add a Quad-Rover checklist with automatic status checks and explicit obstruction/lock confirmation.
2. Add a Hybrid Setup/Diagnostics page for sensors, actuator backend/health, protection flags, configuration, calibration, and recent command outcome.
3. Add shape-aware mission timeline, item validation, transition duration from the actual `HYBRID_TRANS_T` Fact, and first-error localization.
4. Add offline Quad-Rover planning only after the offline vehicle and command tree are proven end to end.
5. Explain unavailable flight modes and add Hybrid state to high-level readiness.
6. Add mission command search and restore actionable incomplete-item text.

### P2: desktop productivity and incident response

1. Add Windows shortcuts and a command/search surface.
2. Consolidate onboard-log transport selection and add a one-click diagnostic bundle.
3. Persist Analyze pop-out workspaces for multi-monitor use.
4. Add an event/transition history with export.
5. Complete translations after operator terminology stabilizes.

## 9. First implementation slice

The first code slice should be small enough to verify independently but broad enough to remove the immediate control errors:

1. Airframe 22001 identity and applicable PX4 setup visibility.
2. Central effective-shape predicates and virtual joystick integration.
3. Hold-to-confirm Hybrid controls with exact-idle gating and human-readable state/fault/recovery text.
4. Focused C++ tests, XML/resource checks, QML static checks, and a Windows Debug build of affected targets.

Guided Actions and the Hybrid checklist follow as the second slice because they need a shared, reviewed ground-mobile policy rather than isolated QML conditions.

## 10. Acceptance boundaries

- No PX4 source change is required for the backlog above. Mission transition duration must read the existing parameter; QGC must not hard-code it.
- The assessment does not claim target-aircraft validation, physical transform safety, or hardware calibration coverage.
- Public competitor features remain documentation/marketing evidence unless explicitly marked source-inspectable.
- A P0 implementation is not complete until stable Quad, stable Rover, transition/unavailable, stale, fault, and controller-recovery states are covered by tests or reproducible Windows UI evidence.

## 11. Implemented Windows slice

The branch implements the P0 backlog and the first checklist slice without changing PX4:

- Airframe 22001 has a QGC-owned metadata overlay that replaces stale cached identity and group imagery without importing an incompatible full catalog.
- Quad-Rover setup exposes the applicable PX4 flight-behavior and multirotor-tuning pages.
- One last-confirmed manual-control shape profile drives virtual and USB joystick throttle policy; input remains blocked until the first healthy stable shape and survives later stale, fault, transition, and reboot-recovery states.
- Virtual joystick resets are send-gated and synchronize cached axes even while hidden, preventing old values from acquiring a new vehicle or shape meaning.
- Transform actions use hold-to-confirm, exact controller availability, translated state/fault/transaction reasons, and null-safe vehicle switching.
- Guided actions distinguish retained Quad and Rover operation, preserve safety actions during Hybrid-status degradation, and bind every confirmed action to the vehicle that opened the confirmation.
- The Hybrid checklist combines automatic controller/actuator/landed checks with manual mechanism, route, payload, weather, and operating-area checks; resetting the UI also invalidates the enforced vehicle checklist state.

Still deferred are the dedicated Hybrid diagnostics page, shape-aware mission timeline, offline Quad-Rover planning, unavailable-mode explanation, translations, command search, persistent workspaces, and the diagnostic/event bundle. Windows automation does not replace physical target-aircraft acceptance.
