#pragma once
#include "../common/imgui_bgfx_glfw/imgui_bgfx_glfw.h"
#include "../memory/Gobj.h"

inline bgfx::ProgramHandle createBGFXProgram(
    unsigned char * vdata, unsigned int vlen,
    unsigned char * fdata, unsigned int flen
) {
    bgfx::ShaderHandle vh = bgfx::createShader(bgfx::makeRef(vdata, vlen));
    bgfx::ShaderHandle fh = bgfx::createShader(bgfx::makeRef(fdata, flen));
    return bgfx::createProgram(vh, fh, true);
}

// call createBGFXProgram with the usual name pattern
#define CREATE_BGFX_PROGRAM(_shader_name) createBGFXProgram( \
    vs_##_shader_name##_bin, vs_##_shader_name##_bin_len, \
    fs_##_shader_name##_bin, fs_##_shader_name##_bin_len)

inline bgfx::AttribType::Enum bgfxAttribTypeFromAccessorComponentType(Gobj::Accessor::ComponentType componentType) {
    switch (componentType) {
    case Gobj::Accessor::COMPTYPE_BYTE           : assert(false);
    case Gobj::Accessor::COMPTYPE_UNSIGNED_BYTE  : return bgfx::AttribType::Uint8;
    case Gobj::Accessor::COMPTYPE_SHORT          : assert(false);
    case Gobj::Accessor::COMPTYPE_UNSIGNED_SHORT : return bgfx::AttribType::Int16;
    case Gobj::Accessor::COMPTYPE_UNSIGNED_INT   : assert(false);
    case Gobj::Accessor::COMPTYPE_FLOAT          : return bgfx::AttribType::Float;
    }
}
