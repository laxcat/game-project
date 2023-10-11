#pragma once
#include <bgfx/defines.h>
#include "../common/imgui_bgfx_glfw/imgui_bgfx_glfw.h"
#include "../memory/Gobj.h"

inline bgfx::ProgramHandle createBGFXProgram(
    uint8_t const * vdata, unsigned int vlen,
    uint8_t const * fdata, unsigned int flen
) {
    bgfx::ShaderHandle vh = bgfx::createShader(bgfx::makeRef(vdata, vlen));
    bgfx::ShaderHandle fh = bgfx::createShader(bgfx::makeRef(fdata, flen));
    return bgfx::createProgram(vh, fh, true);
}

// call createBGFXProgram with the usual name pattern
#define CREATE_BGFX_PROGRAM(_shader_name) createBGFXProgram( \
    vs_##_shader_name, sizeof(vs_##_shader_name), \
    fs_##_shader_name, sizeof(fs_##_shader_name))

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

uint64_t bgfxPrimitiveType(Gobj::MeshPrimitive::Mode mode) {
    switch (mode) {
    case Gobj::MeshPrimitive::MODE_POINTS:          return BGFX_STATE_PT_POINTS;
    case Gobj::MeshPrimitive::MODE_LINES:           return BGFX_STATE_PT_LINES;
    case Gobj::MeshPrimitive::MODE_LINE_STRIP:      return BGFX_STATE_PT_LINESTRIP;
    case Gobj::MeshPrimitive::MODE_TRIANGLE_STRIP:  return BGFX_STATE_PT_TRISTRIP;

    case Gobj::MeshPrimitive::MODE_LINE_LOOP:
    case Gobj::MeshPrimitive::MODE_TRIANGLE_FAN:    assert(false && "Unsupported mode.");

    case Gobj::MeshPrimitive::MODE_TRIANGLES:
    default:                                        return 0;
    }
}
