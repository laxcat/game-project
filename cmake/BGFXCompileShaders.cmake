#
# Loops through subdirectories and looks for BGFX shaders and compiles them.
# 
# Output:
# BGFXShader_target
# Use this target to add the compiled shaders as dependencies of another target.
#
function(BGFXCompileShaders
    shader_input_dir
    shader_output_dir
    shaderc
    include_dir
    profile_list
)
    # vars
    set(outputs)

    # this macro gets run for each frag/vert/comp, for each shader, for each profile
    macro(compile 
        name        # name of shader. should be name of subdir and shaders
        type        # v/f/c
        profile     # see shaderc help
        output_dir  # output directory
    )
        set(output ${output_dir}/${type}s_${name}.bin)
        set(input_dir ${shader_input_dir}/${name})
        set(input ${input_dir}/${type}s_${name}.sc)
        set(varying ${input_dir}/varying.def.sc)

        if(EXISTS ${input})
            # easier to read output using relative paths
            set(input_display ${input})
            set(output_display ${output})
            if (${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.20")
                cmake_path(RELATIVE_PATH input BASE_DIRECTORY ${CMAKE_BINARY_DIR} OUTPUT_VARIABLE input_display)
                cmake_path(RELATIVE_PATH output BASE_DIRECTORY ${CMAKE_BINARY_DIR} OUTPUT_VARIABLE output_display)
            endif()

            # build shader
            add_custom_command(
                OUTPUT  ${output}
                DEPENDS ${shaderc} ${input} ${varying} "create_directory_${profile}"
                COMMAND ${shaderc}
                ARGS    --platform osx -p ${profile} --type ${type} -f ${input} -o ${output} -i ${include_dir}
                COMMENT "Compiling ${type} shader ${input_display} for ${profile} to ${output_display}"
            )
            list(APPEND outputs ${output})
        endif()
    endmacro()

    # loop through subdirectories
    file(GLOB children RELATIVE "${shader_input_dir}" "${shader_input_dir}/*")
    # for each output profile
    foreach(profile ${profile_list})
        # determine output directory
        if ("${profile}" MATCHES "^(120|130|140|150|330|400|410|420|430|440)$")
            set(api_name "glsl")
        elseif ("${profile}" MATCHES "^(100_es|300_es|310_es|320_es)$")
            set(api_name "essl")
        elseif ("${profile}" MATCHES "metal")
            set(api_name "metal")
        # TODO: add direct3d detection for windows builds
        else()
            message(STATUS "WARNING UKNOWN PROFILE. SEE shaderc HELP FOR PROFILE LIST.")
            return()
        endif()

        # set ouput directory
        set(output_dir "${shader_output_dir}/${api_name}")

        # create rule to make output directory if necessary
        add_custom_target("create_directory_${profile}"
            ALL
            COMMAND ${CMAKE_COMMAND} -E make_directory ${output_dir}
        )

        # for each shader input directory
        foreach(child ${children})
            compile(${child} v ${profile} ${output_dir})
            compile(${child} f ${profile} ${output_dir})
            compile(${child} c ${profile} ${output_dir})
        endforeach()
    endforeach()

    # create custom target
    add_custom_target(
        BGFXShader_target
        DEPENDS ${outputs}
    )
endfunction()
