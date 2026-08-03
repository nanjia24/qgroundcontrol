#pragma once

#include "UnitTest.h"

class RoverTuningFactGroupTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _protocolContract_test();
    void _rateValidation_test();
    void _attitudeYawUnwrap_test();
    void _velocityAndPosition_test();
    void _timestampLifecycle_test();
    void _timeout_test();
    void _ownerIsolation_test();
};
