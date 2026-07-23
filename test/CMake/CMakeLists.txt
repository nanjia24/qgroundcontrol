set(hybrid_mavlink_contract_cases
    valid
    bad_repository
    bad_tag
    bad_dialect
    bad_version
    apm_dialect_disabled
    apm_plugin_disabled
)

foreach(case IN LISTS hybrid_mavlink_contract_cases)
    set(test_name HybridMavlinkContract_${case})
    add_test(
        NAME ${test_name}
        COMMAND
            ${CMAKE_COMMAND}
            "-DQGC_SOURCE_DIR=${CMAKE_SOURCE_DIR}"
            "-DCASE=${case}"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/HybridMavlinkContractTest.cmake"
    )

    if(NOT case STREQUAL "valid")
        set_tests_properties(${test_name} PROPERTIES WILL_FAIL TRUE)
    endif()
endforeach()
