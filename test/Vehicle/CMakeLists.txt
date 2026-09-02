# ============================================================================
# Vehicle Unit Tests
# Tests for vehicle connection, FTP, MAVLink commands, and link management
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        FTPManagerTest.cc
        FTPManagerTest.h
        FTPControllerTest.cc
        FTPControllerTest.h
        FirmwareUpgradeControllerTest.cc
        FirmwareUpgradeControllerTest.h
        InitialConnectTest.cc
        InitialConnectTest.h
        InitialConnectPeripheralStartupTest.cc
        InitialConnectPeripheralStartupTest.h
        HybridVehicleStateTest.cc
        HybridVehicleStateTest.h
        HybridVehicleIngressTest.cc
        HybridVehicleIngressTest.h
        HybridTransitionControllerTest.cc
        HybridTransitionControllerTest.h
        HybridFlightModePolicyTest.cc
        HybridFlightModePolicyTest.h
        MAVLinkLogManagerTest.cc
        MAVLinkLogManagerTest.h
        RequestMessageTest.cc
        RequestMessageTest.h
        RoverTuningFactGroupTest.cc
        RoverTuningFactGroupTest.h
        SendMavCommandWithHandlerTest.cc
        SendMavCommandWithHandlerTest.h
        SendMavCommandWithSignallingTest.cc
        SendMavCommandWithSignallingTest.h
        VehicleLinkManagerTest.cc
        VehicleLinkManagerTest.h
        VehiclePIDTuningTelemetryTest.cc
)

if(NOT QGC_DISABLE_APM_PLUGIN)
    target_sources(${CMAKE_PROJECT_NAME}
        PRIVATE
            APMAirframeComponentControllerTest.cc
            APMAirframeComponentControllerTest.h
    )
endif()

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# ----------------------------------------------------------------------------
# Component Information Tests
# ----------------------------------------------------------------------------
add_subdirectory(ComponentInformation)
