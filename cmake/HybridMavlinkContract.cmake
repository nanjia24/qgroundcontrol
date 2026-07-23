find_package(Git REQUIRED)

function(qgc_validate_hybrid_mavlink_inputs)
    if(NOT "${QGC_MAVLINK_GIT_REPO}" STREQUAL "https://github.com/QQgdiw/mavlink.git")
        message(FATAL_ERROR "QGC_MAVLINK_GIT_REPO must be the released qgc_hybrid repository")
    endif()
    if(NOT "${QGC_MAVLINK_GIT_TAG}" STREQUAL "qgc-hybrid-change1-v1.16.1-r2")
        message(FATAL_ERROR "QGC_MAVLINK_GIT_TAG must be qgc-hybrid-change1-v1.16.1-r2")
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
    if(NOT git_result EQUAL 0 OR NOT "${resolved_commit}" STREQUAL "04ad1d63e9c11ed6767a35dae4e52adaca3538c5")
        message(FATAL_ERROR "CPM MAVLink checkout does not match the qgc_hybrid r2 peeled commit")
    endif()
endfunction()
