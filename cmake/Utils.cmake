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

# to avoid using CMAKE_BUILD_TYPE, (which I couldn't figure out how to control per-target)
# set debug info manually
# Pulled settings from here: https://stackoverflow.com/a/59314670
# TODO set windows settings
macro(set_target_build_type target mode)
    message(STATUS "SETTING ${BUILD_TYPE} on target: ${target}")
    string(TOLOWER ${BUILD_TYPE} this_build_type)
    # Release
    if(this_build_type STREQUAL release)
        target_compile_definitions(${target} ${mode} NDEBUG)
        target_compile_options(${target} ${mode} -O3)
    # Debug
    elseif(this_build_type STREQUAL debug)
        target_compile_options(${target} ${mode} -g -O0)
    # RelWithDebInfo
    elseif(this_build_type STREQUAL relwithdebinfo)
        target_compile_definitions(${target} ${mode} NDEBUG)
        target_compile_options(${target} ${mode} -g -O2)
    # MinSizeRel
    elseif(this_build_type STREQUAL minsizerel)
        target_compile_definitions(${target} ${mode} NDEBUG)
        target_compile_options(${target} ${mode} -Os)
    endif()
endmacro()
