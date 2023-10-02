# https://stackoverflow.com/a/7788165
MACRO(SUBDIRLIST result curdir)
    FILE(GLOB children RELATIVE ${curdir} ${curdir}/*)
    SET(dirlist "")
    FOREACH(child ${children})
        IF(IS_DIRECTORY ${curdir}/${child})
            LIST(APPEND dirlist ${child})
        ENDIF()
    ENDFOREACH()
    SET(${result} ${dirlist})
ENDMACRO()

# to avoid using CMAKE_BUILD_TYPE (which can't be set per-target(?)),
# set debug build defs and options manually
# Pulled settings from here: https://stackoverflow.com/a/59314670
# TODO set windows settings
function(target_build_type target mode BUILD_TYPE_PARAM)
    message(STATUS "SETTING ${BUILD_TYPE_PARAM} on target: ${target}")
    string(TOLOWER ${BUILD_TYPE_PARAM} test_build_type)
    # Release
    if(test_build_type STREQUAL release)
        target_compile_definitions(${target} ${mode} NDEBUG)
        target_compile_options(${target} ${mode} -O3)
    # Debug
    elseif(test_build_type STREQUAL debug)
        target_compile_options(${target} ${mode} -g -O0)
    # RelWithDebInfo
    elseif(test_build_type STREQUAL relwithdebinfo)
        target_compile_definitions(${target} ${mode} NDEBUG)
        target_compile_options(${target} ${mode} -g -O2)
    # MinSizeRel
    elseif(test_build_type STREQUAL minsizerel)
        target_compile_definitions(${target} ${mode} NDEBUG)
        target_compile_options(${target} ${mode} -Os)
    endif()
endfunction()
