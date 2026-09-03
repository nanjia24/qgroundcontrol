target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        PX4AirframeLoaderTest.cc
        PX4AirframeLoaderTest.h
        PX4TuningComponentTest.cc
        PX4TuningComponentTest.h
        PX4TuningComponentQmlTest.cc
        PX4TuningComponentQmlTest.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
