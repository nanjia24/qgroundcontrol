#pragma once

#include "UnitTest.h"

class PX4TuningComponentTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _vehicleTypeRouting();
    void _quadRoverTabsDoNotDependOnLegacyParameter();
};
