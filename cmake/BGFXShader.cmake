#
# Loops through subdirectories and looks for BGFX shaders and compiles them.
#
function(BGFXShader_compile shader_dir_rel_to_root)
    # vars
    set(root_dir ${CMAKE_CURRENT_SOURCE_DIR})
    set(shaderc ${root_dir}/build/_deps/bgfx_content-build/shaderc)
    set(include_dir ${root_dir}/build/_deps/bgfx_content-src/bgfx/src)
    set(shader_dir ${root_dir}/${shader_dir_rel_to_root})
    set(depends)

    # run for each individual shader
    macro(compile dir type)
        set(output_dir ${shader_dir}/${dir})
        set(output ${output_dir}/${type}s_${dir}.sc.bin)
        set(input ${output_dir}/${type}s_${dir}.sc)

        if(EXISTS ${input})
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
