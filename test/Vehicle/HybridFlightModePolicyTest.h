#pragma once

#include "BaseClasses/VehicleTestManualConnect.h"
#include "HybridVehicleState.h"

class HybridFlightModePolicyTest : public VehicleTestManualConnect
{
    Q_OBJECT

private slots:
    void init() override;

    void _stableQuadFiltersToMultirotorModes();
    void _stableRoverFiltersToRoverModes();
    void _unavailableStatesRejectModesAndCommands_data();
    void _unavailableStatesRejectModesAndCommands();
    void _staleStateRejectsModesAndCommands();

private:
    void _connectQuadRover();
    void _injectAck(MAV_RESULT result, uint32_t sequence);
    void _injectStatus(HybridVehicleState::CurrentState currentState, HybridVehicleState::TargetState targetState,
                       uint8_t faultReason = HYBRID_VEHICLE_FAULT_NONE, uint64_t timestamp = 0, uint32_t sequence = 7,
                       MAV_RESULT commandResult = MAV_RESULT_ACCEPTED, uint64_t commandTimestamp = 70);
    void _primeStableState(HybridVehicleState::CurrentState currentState);
    void _verifyUnavailableModeRequestIsNotSent();

    uint64_t _statusTimestamp = 100;
    QString _manualMode;
};
