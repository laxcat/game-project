#include "imgui_bgfx_glfw_2.h"

#include <bgfx/bgfx.h>
#include <bgfx/embedded_shader.h>
#include <bx/math.h>
#include <bx/timer.h>
// #include <imgui_internal.h> // TODO: what do we need that isn't publically available?

#include "vs_ocornut_imgui.bin.h"
#include "fs_ocornut_imgui.bin.h"
#include "vs_imgui_image.bin.h"
#include "fs_imgui_image.bin.h"

#include "../../dev/print.h"

#define IMGUI_FLAGS_ALPHA_BLEND UINT8_C(0x01)

static const bgfx::EmbeddedShader s_embeddedShaders[] =
{
    BGFX_EMBEDDED_SHADER(vs_ocornut_imgui),
    BGFX_EMBEDDED_SHADER(fs_ocornut_imgui),
    BGFX_EMBEDDED_SHADER(vs_imgui_image),
    BGFX_EMBEDDED_SHADER(fs_imgui_image),

    BGFX_EMBEDDED_SHADER_END()
};
static ImGuiContext *      m_imgui;
static bgfx::VertexLayout  m_layout;
static bgfx::ProgramHandle m_program;
static bgfx::ProgramHandle m_imageProgram;
static bgfx::TextureHandle m_texture;
static bgfx::UniformHandle s_tex;
static bgfx::UniformHandle u_imageLodEnabled;

void imguiCreate(GLFWwindow * window, bgfx::ViewId viewId, ImVec2 windowSize, ImVec2 framebufferSize) {
    m_imgui = ImGui::CreateContext();
    ImGuiIO & io = ImGui::GetIO();

    io.DisplaySize = windowSize;
    if (windowSize.x > 0 && windowSize.y > 0) {
        io.DisplayFramebufferScale = ImVec2(
            framebufferSize.x / windowSize.x,
            framebufferSize.y / windowSize.y
        );
    }
    io.DeltaTime   = 1.0f / 60.0f;
    io.IniFilename = NULL;

    bgfx::RendererType::Enum type = bgfx::getRendererType();
    m_program = bgfx::createProgram(
          bgfx::createEmbeddedShader(s_embeddedShaders, type, "vs_ocornut_imgui")
        , bgfx::createEmbeddedShader(s_embeddedShaders, type, "fs_ocornut_imgui")
        , true
        );

    u_imageLodEnabled = bgfx::createUniform("u_imageLodEnabled", bgfx::UniformType::Vec4);
    m_imageProgram = bgfx::createProgram(
          bgfx::createEmbeddedShader(s_embeddedShaders, type, "vs_imgui_image")
        , bgfx::createEmbeddedShader(s_embeddedShaders, type, "fs_imgui_image")
        , true
        );

    m_layout
        .begin()
        .add(bgfx::Attrib::Position,  2, bgfx::AttribType::Float)
        .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
        .add(bgfx::Attrib::Color0,    4, bgfx::AttribType::Uint8, true)
        .end();

    s_tex = bgfx::createUniform("s_tex", bgfx::UniformType::Sampler);

    uint8_t* data;
    int32_t width;
    int32_t height;
    io.Fonts->GetTexDataAsRGBA32(&data, &width, &height);
    m_texture = bgfx::createTexture2D(
          (uint16_t)width
        , (uint16_t)height
        , false
        , 1
        , bgfx::TextureFormat::BGRA8
        , 0
        , bgfx::copy(data, width*height*4)
        );



    // uint8_t* fontData;
    // int32_t fontw;
    // int32_t fonth;
    // io.Fonts->GetTexDataAsRGBA32(&fontData, &fontw, &fonth);

    // bgfx::RendererType::Enum type = bgfx::getRendererType();
    // m_program = bgfx::createProgram(
    //       bgfx::createEmbeddedShader(s_embeddedShaders, type, "vs_ocornut_imgui")
    //     , bgfx::createEmbeddedShader(s_embeddedShaders, type, "fs_ocornut_imgui")
    //     , true
    //     );

    // u_imageLodEnabled = bgfx::createUniform("u_imageLodEnabled", bgfx::UniformType::Vec4);
    // m_imageProgram = bgfx::createProgram(
    //       bgfx::createEmbeddedShader(s_embeddedShaders, type, "vs_imgui_image")
    //     , bgfx::createEmbeddedShader(s_embeddedShaders, type, "fs_imgui_image")
    //     , true
    //     );

    // vertexLayout
    //     .begin()
    //     .add(bgfx::Attrib::Position,  2, bgfx::AttribType::Float)
    //     .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
    //     .add(bgfx::Attrib::Color0,    4, bgfx::AttribType::Uint8, true)
    //     .end();
}

void imguiDestroy() {
        ImGui::DestroyContext(m_imgui);

        bgfx::destroy(s_tex);
        bgfx::destroy(m_texture);

        bgfx::destroy(u_imageLodEnabled);
        bgfx::destroy(m_imageProgram);
        bgfx::destroy(m_program);
}

void imguiBeginFrame(EventQueue const & events, double dt) {
    printl("imgui begin frame dt %f", dt);
    ImGuiIO & io = ImGui::GetIO();

    for (int i = 0; i < events.count; ++i) {
        switch (events.events[i].type) {
        case Event::Window:
            break;
        case Event::Keyboard:
            break;
        case Event::Mouse:
            break;
        case Event::Gamepad:
            break;
        default:
        case Event::None:
            break;
        }
    }

    // process events

    // handle in processing
    // io.DisplaySize = ImVec2( (float)event.width, (float)event.height);


    io.DeltaTime = (float)dt;
    ImGui::NewFrame();
}

bool checkAvailTransientBuffers(uint32_t _numVertices, uint32_t _numIndices) {
    return _numVertices == bgfx::getAvailTransientVertexBuffer(_numVertices, m_layout)
        && (0 == _numIndices || _numIndices == bgfx::getAvailTransientIndexBuffer(_numIndices) )
        ;
}

void render(bgfx::ViewId viewId, ImDrawData * drawData) {
    // #if 0 // disable until all variables are re-introduced

    // Avoid rendering when minimized, scale coordinates for retina displays (screen coordinates != framebuffer coordinates)
    int fb_width = (int)(drawData->DisplaySize.x * drawData->FramebufferScale.x);
    int fb_height = (int)(drawData->DisplaySize.y * drawData->FramebufferScale.y);
    if (fb_width <= 0 || fb_height <= 0)
        return;

    bgfx::setViewName(viewId, "ImGui");
    bgfx::setViewMode(viewId, bgfx::ViewMode::Sequential);

    bgfx::Caps const * caps = bgfx::getCaps();
    {
        float ortho[16];
        float x = drawData->DisplayPos.x;
        float y = drawData->DisplayPos.y;
        float width = drawData->DisplaySize.x;
        float height = drawData->DisplaySize.y;

        bx::mtxOrtho(ortho, x, x + width, y + height, y, 0.0f, 1000.0f, 0.0f, caps->homogeneousDepth);
        bgfx::setViewTransform(viewId, NULL, ortho);
        bgfx::setViewRect(viewId, 0, 0, uint16_t(width), uint16_t(height) );
    }

    const ImVec2 clipPos   = drawData->DisplayPos;       // (0,0) unless using multi-viewports
    const ImVec2 clipScale = drawData->FramebufferScale; // (1,1) unless using retina display which are often (2,2)

    // Render command lists
    for (int32_t ii = 0, num = drawData->CmdListsCount; ii < num; ++ii) {
        bgfx::TransientVertexBuffer tvb;
        bgfx::TransientIndexBuffer tib;

        const ImDrawList* drawList = drawData->CmdLists[ii];
        uint32_t numVertices = (uint32_t)drawList->VtxBuffer.size();
        uint32_t numIndices  = (uint32_t)drawList->IdxBuffer.size();

        if (!checkAvailTransientBuffers(numVertices, numIndices)) {
            // not enough space in transient buffer just quit drawing the rest...
            break;
        }

        bgfx::allocTransientVertexBuffer(&tvb, numVertices, m_layout);
        bgfx::allocTransientIndexBuffer(&tib, numIndices, sizeof(ImDrawIdx) == 4);

        ImDrawVert* verts = (ImDrawVert*)tvb.data;
        bx::memCopy(verts, drawList->VtxBuffer.begin(), numVertices * sizeof(ImDrawVert) );

        ImDrawIdx* indices = (ImDrawIdx*)tib.data;
        bx::memCopy(indices, drawList->IdxBuffer.begin(), numIndices * sizeof(ImDrawIdx) );

        bgfx::Encoder* encoder = bgfx::begin();

        for (const ImDrawCmd* cmd = drawList->CmdBuffer.begin(), *cmdEnd = drawList->CmdBuffer.end(); cmd != cmdEnd; ++cmd) {
            if (cmd->UserCallback) {
                cmd->UserCallback(drawList, cmd);
            }
            else if (0 != cmd->ElemCount) {
                uint64_t state = 0
                    | BGFX_STATE_WRITE_RGB
                    | BGFX_STATE_WRITE_A
                    | BGFX_STATE_MSAA
                    ;

                bgfx::TextureHandle th = m_texture;
                bgfx::ProgramHandle program = m_program;

                if (NULL != cmd->TextureId) {
                    union { ImTextureID ptr; struct { bgfx::TextureHandle handle; uint8_t flags; uint8_t mip; } s; } texture = { cmd->TextureId };
                    state |= 0 != (IMGUI_FLAGS_ALPHA_BLEND & texture.s.flags)
                        ? BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_SRC_ALPHA, BGFX_STATE_BLEND_INV_SRC_ALPHA)
                        : BGFX_STATE_NONE
                        ;
                    th = texture.s.handle;
                    if (0 != texture.s.mip) {
                        const float lodEnabled[4] = { float(texture.s.mip), 1.0f, 0.0f, 0.0f };
                        bgfx::setUniform(u_imageLodEnabled, lodEnabled);
                        program = m_imageProgram;
                    }
                }
                else {
                    state |= BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_SRC_ALPHA, BGFX_STATE_BLEND_INV_SRC_ALPHA);
                }

                // Project scissor/clipping rectangles into framebuffer space
                ImVec4 clipRect;
                clipRect.x = (cmd->ClipRect.x - clipPos.x) * clipScale.x;
                clipRect.y = (cmd->ClipRect.y - clipPos.y) * clipScale.y;
                clipRect.z = (cmd->ClipRect.z - clipPos.x) * clipScale.x;
                clipRect.w = (cmd->ClipRect.w - clipPos.y) * clipScale.y;

                if (clipRect.x <  fb_width
                &&  clipRect.y <  fb_height
                &&  clipRect.z >= 0.0f
                &&  clipRect.w >= 0.0f) {
                    const uint16_t xx = uint16_t(bx::max(clipRect.x, 0.0f) );
                    const uint16_t yy = uint16_t(bx::max(clipRect.y, 0.0f) );
                    encoder->setScissor(xx, yy
                            , uint16_t(bx::min(clipRect.z, 65535.0f)-xx)
                            , uint16_t(bx::min(clipRect.w, 65535.0f)-yy)
                            );

                    encoder->setState(state);
                    encoder->setTexture(0, s_tex, th);
                    encoder->setVertexBuffer(0, &tvb, cmd->VtxOffset, numVertices);
                    encoder->setIndexBuffer(&tib, cmd->IdxOffset, cmd->ElemCount);
                    encoder->submit(viewId, program);
                }
            }
        }

        bgfx::end(encoder);

    }
    // #endif
}

void imguiEndFrame(bgfx::ViewId viewId) {
    ImGui::Render();
    render(viewId, ImGui::GetDrawData());
}
