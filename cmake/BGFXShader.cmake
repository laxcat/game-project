#
# Loops through subdirectories and looks for BGFX shaders and compiles them.
# Depends on FetchContent; provide FetchContent name as bgfx_content parameter.
#
function(BGFXShader_compile
    shader_dir_rel_to_root
    bgfx_content
    )
    # vars
    set(root_dir ${CMAKE_CURRENT_SOURCE_DIR})
    set(bin_dir "${${bgfx_content}_BINARY_DIR}")
    set(src_dir "${${bgfx_content}_SOURCE_DIR}")
    set(shaderc ${bin_dir}/shaderc)
    set(include_dir ${src_dir}/bgfx/src)
    set(shader_dir ${root_dir}/${shader_dir_rel_to_root})
    set(depends)

    # run for each individual shader
    macro(compile dir type)
        set(output_dir ${root_dir}/build/shaders/metal)
        set(output ${output_dir}/${type}s_${dir}.bin)
        set(input ${shader_dir}/${dir}/${type}s_${dir}.sc)

        if(EXISTS ${input})
            if(NOT EXISTS ${output_dir})
                message(STATUS "MAKING DIRECTORY: " ${output_dir})
                file(MAKE_DIRECTORY ${output_dir})
            endif()

            add_custom_command(
                DEPENDS ${shaderc} ${include_dir} ${input}
                OUTPUT  ${output}
                COMMAND ${shaderc}
                ARGS    --platform osx -p metal --type ${type} -f ${input} -o ${output} -i ${include_dir}
            )
            list(APPEND depends ${output})
        endif()
    endmacro()
    
    # loop through subdirectories
    file(GLOB children RELATIVE "${shader_dir}" "${shader_dir}/*")
    foreach(child ${children})
        if(IS_DIRECTORY ${shader_dir}/${child})
            compile(${child} v)
            compile(${child} f)
            compile(${child} c)
        endif()
    endforeach()

    # gathers all output files as dependents of blank target
    add_custom_target(
      BGFXShader_target
      DEPENDS ${depends}
    )
endfunction()
