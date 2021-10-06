set(SHADERC build/_deps/bgfx_content-build/shaderc)
set(INCLUDE_DIR build/_deps/bgfx_content-src/bgfx/src)
set(OUTPUT_DIR src/shaders/main)
set(OUTPUT ${OUTPUT_DIR}/vs_main.sc.bin)

macro(BGFXShader_compile)
add_custom_command(
    DEPENDS ${SHADERC} ${INCLUDE_DIR}
    OUTPUT  ${OUTPUT}
    COMMAND ${SHADERC}
            --platform osx
            -p metal
            --type f
            -f ${OUTPUT_DIR}/vs_main.sc
            -o ${OUTPUT}
            -i ${INCLUDE_DIR}
)
endmacro()
