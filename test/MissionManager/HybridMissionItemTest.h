#pragma once

#include "BaseClasses/VehicleTestManualConnect.h"

class MissionManager;

class HybridMissionItemTest : public VehicleTestManualConnect
{
    Q_OBJECT

private slots:
    void init() override;

    void _planRoundTripsBothHybridTargets();
    void _invalidHybridTransitionsAreRejectedBeforeMissionCount();
    void _validHybridTransitionUploadsAndDownloadsUnchanged();

private:
    void _connectQuadRoverMockLink();
    bool _missionItemIsRejected(double param1, double param2, double param3, double param4, double param5,
                                double param6, double param7);
    bool _missionItemIsAccepted(double param1, double param2, double param3, double param4, double param5,
                                double param6, double param7);

    MissionManager* _missionManager = nullptr;
};
