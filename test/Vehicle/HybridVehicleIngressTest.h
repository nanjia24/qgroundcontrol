#pragma once

#include "BaseClasses/VehicleTestManualConnect.h"

class HybridVehicleIngressTest : public VehicleTestManualConnect
{
    Q_OBJECT

private slots:
    void _quadRoverFixtureExposesHybridParameters();
    void _validMavlink2StatusUpdatesVehicleState();
    void _invalidSourcesAndOutOfOrderStatusAreIgnored();
    void _systemTimeRebootResetsVehicleState();
    void _ordinaryPx4VehicleIgnoresHybridStatus();

private:
    void _connectQuadRoverMockLink();
    void _connectPx4MockLink();
};
