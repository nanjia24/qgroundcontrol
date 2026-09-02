find_package(Git REQUIRED)

function(qgc_validate_hybrid_mavlink_inputs)
    if(NOT "${QGC_MAVLINK_GIT_REPO}" STREQUAL "https://github.com/QQgdiw/mavlink.git")
        message(FATAL_ERROR "QGC_MAVLINK_GIT_REPO must be the released qgc_hybrid repository")
    endif()
    if(NOT "${QGC_MAVLINK_GIT_TAG}" STREQUAL "qgc-hybrid-rover-tuning-v1.16.1-r1")
        message(FATAL_ERROR "QGC_MAVLINK_GIT_TAG must be qgc-hybrid-rover-tuning-v1.16.1-r1")
    endif()
    if(NOT "${QGC_MAVLINK_DIALECT}" STREQUAL "qgc_hybrid" OR NOT "${QGC_MAVLINK_VERSION}" STREQUAL "2.0")
        message(FATAL_ERROR "QGC requires MAVLink 2 qgc_hybrid")
    endif()
    if(QGC_DISABLE_APM_MAVLINK OR QGC_DISABLE_APM_PLUGIN)
        message(FATAL_ERROR "qgc_hybrid builds keep the APM MAVLink dialect and plugin enabled")
    endif()
endfunction()

function(qgc_verify_hybrid_mavlink_checkout source_dir)
    execute_process(
        COMMAND "${GIT_EXECUTABLE}" -C "${source_dir}" rev-parse HEAD
        OUTPUT_VARIABLE resolved_commit
        OUTPUT_STRIP_TRAILING_WHITESPACE
        RESULT_VARIABLE git_result
    )
    if(NOT git_result EQUAL 0 OR NOT "${resolved_commit}" STREQUAL "21922689c6fb113884df0f66582d8e602286fdc1")
        message(FATAL_ERROR "CPM MAVLink checkout does not match the qgc_hybrid Rover tuning r1 peeled commit")
    endif()
endfunction()
