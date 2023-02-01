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
static ImGuiContext * context = nullptr;
static bgfx::VertexLayout  vertexLayout;
static bgfx::ProgramHandle mainProgram;
static bgfx::ProgramHandle imageProgram;
static bgfx::TextureHandle fontTexture;
static bgfx::UniformHandle uniformSampler;
static bgfx::UniformHandle uniformImageLodEnabled;
static GLFWcursor * mouseCursors[ImGuiMouseCursor_COUNT];

static char const * getClipboardText(void * userData) {
    return glfwGetClipboardString((GLFWwindow *)userData);
}

static void setClipboardText(void * userData, char const * text) {
    glfwSetClipboardString((GLFWwindow *)userData, text);
}

void imguiCreate(GLFWwindow * window, bgfx::ViewId viewId, ImVec2 windowSize) {
    assert(context == nullptr && "Already initialized imgui!");

    context = ImGui::CreateContext();
    ImGuiIO & io = ImGui::GetIO();

    io.DisplaySize = windowSize;
    printl("imguiCreate, io.DisplaySize (%f, %f)", io.DisplaySize.x, io.DisplaySize.y);
    io.DeltaTime   = 1.0f / 60.0f;
    io.IniFilename = NULL;

    bgfx::RendererType::Enum type = bgfx::getRendererType();
    mainProgram = bgfx::createProgram(
        bgfx::createEmbeddedShader(s_embeddedShaders, type, "vs_ocornut_imgui"),
        bgfx::createEmbeddedShader(s_embeddedShaders, type, "fs_ocornut_imgui"),
        true
    );

    uniformImageLodEnabled = bgfx::createUniform("u_imageLodEnabled", bgfx::UniformType::Vec4);
    imageProgram = bgfx::createProgram(
        bgfx::createEmbeddedShader(s_embeddedShaders, type, "vs_imgui_image"),
        bgfx::createEmbeddedShader(s_embeddedShaders, type, "fs_imgui_image"),
        true
    );

    vertexLayout
        .begin()
        .add(bgfx::Attrib::Position,  2, bgfx::AttribType::Float)
        .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
        .add(bgfx::Attrib::Color0,    4, bgfx::AttribType::Uint8, true)
        .end();

    uniformSampler = bgfx::createUniform("s_tex", bgfx::UniformType::Sampler);

    uint8_t* data;
    int32_t width;
    int32_t height;
    io.Fonts->GetTexDataAsRGBA32(&data, &width, &height);
    fontTexture = bgfx::createTexture2D(
        (uint16_t)width,
        (uint16_t)height,
        false,
        1,
        bgfx::TextureFormat::BGRA8,
        0,
        bgfx::copy(data, width*height*4)
    );







    // FROM imgui_impl_glfw.cpp

    io.BackendPlatformName = "imgui_bgfx_glfw";
    io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;         // We can honor GetMouseCursor() values (optional)
    io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;          // We can honor io.WantSetMousePos requests (optional, rarely used)

    io.SetClipboardTextFn = setClipboardText;
    io.GetClipboardTextFn = getClipboardText;
    io.ClipboardUserData = window;

    // Set platform dependent data in viewport
#if defined(_WIN32)
    ImGui::GetMainViewport()->PlatformHandleRaw = (void*)glfwGetWin32Window(window);
#endif

    // Create mouse cursors
    // (By design, on X11 cursors are user configurable and some cursors may be missing. When a cursor doesn't exist,
    // GLFW will emit an error which will often be printed by the app, so we temporarily disable error reporting.
    // Missing cursors will return NULL and our _UpdateMouseCursor() function will use the Arrow cursor instead.)
    GLFWerrorfun prev_error_callback = glfwSetErrorCallback(NULL);
    mouseCursors[ImGuiMouseCursor_Arrow] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
    mouseCursors[ImGuiMouseCursor_TextInput] = glfwCreateStandardCursor(GLFW_IBEAM_CURSOR);
    mouseCursors[ImGuiMouseCursor_ResizeNS] = glfwCreateStandardCursor(GLFW_VRESIZE_CURSOR);
    mouseCursors[ImGuiMouseCursor_ResizeEW] = glfwCreateStandardCursor(GLFW_HRESIZE_CURSOR);
    mouseCursors[ImGuiMouseCursor_Hand] = glfwCreateStandardCursor(GLFW_HAND_CURSOR);
#if GLFW_HAS_NEW_CURSORS
    mouseCursors[ImGuiMouseCursor_ResizeAll] = glfwCreateStandardCursor(GLFW_RESIZE_ALL_CURSOR);
    mouseCursors[ImGuiMouseCursor_ResizeNESW] = glfwCreateStandardCursor(GLFW_RESIZE_NESW_CURSOR);
    mouseCursors[ImGuiMouseCursor_ResizeNWSE] = glfwCreateStandardCursor(GLFW_RESIZE_NWSE_CURSOR);
    mouseCursors[ImGuiMouseCursor_NotAllowed] = glfwCreateStandardCursor(GLFW_NOT_ALLOWED_CURSOR);
#else
    mouseCursors[ImGuiMouseCursor_ResizeAll] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
    mouseCursors[ImGuiMouseCursor_ResizeNESW] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
    mouseCursors[ImGuiMouseCursor_ResizeNWSE] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
    mouseCursors[ImGuiMouseCursor_NotAllowed] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
#endif
    glfwSetErrorCallback(prev_error_callback);

}

void imguiDestroy() {
    ImGui::DestroyContext(context);

    bgfx::destroy(uniformSampler);
    bgfx::destroy(fontTexture);

    bgfx::destroy(uniformImageLodEnabled);
    bgfx::destroy(imageProgram);
    bgfx::destroy(mainProgram);
}

void imguiBeginFrame(size2 windowSize, EventQueue & events, double dt) {
    // printl("imguiBeginFrame dt %f", dt);

    ImGuiIO & io = ImGui::GetIO();

    // set size
    io.DisplaySize = ImVec2((float)windowSize.w, (float)windowSize.h);
    printl("imguiBeginFrame, io.DisplaySize (%f, %f)", io.DisplaySize.x, io.DisplaySize.y);

    // proces events
    for (int i = 0; i < events.count; ++i) {
        Event & e = events.events[i];
        switch (e.type) {

        case Event::Keyboard:
            break;
        case Event::Mouse:
            break;
        case Event::Gamepad:
            break;

        case Event::None:
        default:
            break;
        }
    }

    io.DeltaTime = (float)dt;
    ImGui::NewFrame();
}

bool checkAvailTransientBuffers(uint32_t _numVertices, uint32_t _numIndices) {
    return _numVertices == bgfx::getAvailTransientVertexBuffer(_numVertices, vertexLayout)
        && (0 == _numIndices || _numIndices == bgfx::getAvailTransientIndexBuffer(_numIndices) )
        ;
}

void render(bgfx::ViewId viewId, ImDrawData * drawData) {
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

        bgfx::allocTransientVertexBuffer(&tvb, numVertices, vertexLayout);
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

                bgfx::TextureHandle th = fontTexture;
                bgfx::ProgramHandle program = mainProgram;

                if (NULL != cmd->TextureId) {
                    union { ImTextureID ptr; struct { bgfx::TextureHandle handle; uint8_t flags; uint8_t mip; } s; } texture = { cmd->TextureId };
                    state |= 0 != (IMGUI_FLAGS_ALPHA_BLEND & texture.s.flags)
                        ? BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_SRC_ALPHA, BGFX_STATE_BLEND_INV_SRC_ALPHA)
                        : BGFX_STATE_NONE
                        ;
                    th = texture.s.handle;
                    if (0 != texture.s.mip) {
                        const float lodEnabled[4] = { float(texture.s.mip), 1.0f, 0.0f, 0.0f };
                        bgfx::setUniform(uniformImageLodEnabled, lodEnabled);
                        program = imageProgram;
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
                    encoder->setTexture(0, uniformSampler, th);
                    encoder->setVertexBuffer(0, &tvb, cmd->VtxOffset, numVertices);
                    encoder->setIndexBuffer(&tib, cmd->IdxOffset, cmd->ElemCount);
                    encoder->submit(viewId, program);
                }
            }
        }

        bgfx::end(encoder);

    }
}

void imguiEndFrame(bgfx::ViewId viewId) {
    ImGui::Render();
    render(viewId, ImGui::GetDrawData());
}
