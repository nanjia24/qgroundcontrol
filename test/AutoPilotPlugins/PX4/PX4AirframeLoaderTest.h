#pragma once

#include "UnitTest.h"

class PX4AirframeLoaderTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _qgcOverlayReplacesOnlyDeclaredAirframe();
};
