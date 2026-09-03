#pragma once

#include "BaseClasses/VehicleTest.h"

class PX4TuningComponentQmlTest : public VehicleTest
{
    Q_OBJECT

public:
    explicit PX4TuningComponentQmlTest(QObject* parent = nullptr);

private slots:
    void _quadRoverRatePageHasUsablePlotLayout();
};
