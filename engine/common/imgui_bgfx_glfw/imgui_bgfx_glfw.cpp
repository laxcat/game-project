#include "imgui_bgfx_glfw.h"

/*
Most code derived from:
https://github.com/bkaradzic/bgfx/blob/master/examples/common/imgui/imgui.cpp
https://github.com/ocornut/imgui/blob/master/backends/imgui_impl_glfw.cpp

BGFX shaders, etc:
https://github.com/bkaradzic/bgfx/tree/master/examples/common/imgui

*/

#include <bgfx/bgfx.h>
#include <bgfx/embedded_shader.h>
#include <bx/math.h>
#include <bx/timer.h>
// #include <imgui_internal.h> // TODO: what do we need that isn't publically available?

// #include "imgui_impl_glfw.h"

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



static GLFWwindow *         _window = NULL;
static double               _time;
static GLFWwindow *         _mouseWindow;
static GLFWcursor *         _mouseCursors[ImGuiMouseCursor_COUNT];
static ImVec2               _lastValidMousePos;
static GLFWwindowfocusfun   _prevUserCallbackWindowFocus;
static GLFWcursorenterfun   _prevUserCallbackCursorEnter;


// Functions
static const char * getClipboardText(void * userData)
{
    return glfwGetClipboardString((GLFWwindow *)userData);
}

static void setClipboardText(void * userData, const char * text)
{
    glfwSetClipboardString((GLFWwindow *)userData, text);
}

static ImGuiKey keyToImGuiKey(int key)
{
    switch (key)
    {
        case GLFW_KEY_TAB: return ImGuiKey_Tab;
        case GLFW_KEY_LEFT: return ImGuiKey_LeftArrow;
        case GLFW_KEY_RIGHT: return ImGuiKey_RightArrow;
        case GLFW_KEY_UP: return ImGuiKey_UpArrow;
        case GLFW_KEY_DOWN: return ImGuiKey_DownArrow;
        case GLFW_KEY_PAGE_UP: return ImGuiKey_PageUp;
        case GLFW_KEY_PAGE_DOWN: return ImGuiKey_PageDown;
        case GLFW_KEY_HOME: return ImGuiKey_Home;
        case GLFW_KEY_END: return ImGuiKey_End;
        case GLFW_KEY_INSERT: return ImGuiKey_Insert;
        case GLFW_KEY_DELETE: return ImGuiKey_Delete;
        case GLFW_KEY_BACKSPACE: return ImGuiKey_Backspace;
        case GLFW_KEY_SPACE: return ImGuiKey_Space;
        case GLFW_KEY_ENTER: return ImGuiKey_Enter;
        case GLFW_KEY_ESCAPE: return ImGuiKey_Escape;
        case GLFW_KEY_APOSTROPHE: return ImGuiKey_Apostrophe;
        case GLFW_KEY_COMMA: return ImGuiKey_Comma;
        case GLFW_KEY_MINUS: return ImGuiKey_Minus;
        case GLFW_KEY_PERIOD: return ImGuiKey_Period;
        case GLFW_KEY_SLASH: return ImGuiKey_Slash;
        case GLFW_KEY_SEMICOLON: return ImGuiKey_Semicolon;
        case GLFW_KEY_EQUAL: return ImGuiKey_Equal;
        case GLFW_KEY_LEFT_BRACKET: return ImGuiKey_LeftBracket;
        case GLFW_KEY_BACKSLASH: return ImGuiKey_Backslash;
        case GLFW_KEY_RIGHT_BRACKET: return ImGuiKey_RightBracket;
        case GLFW_KEY_GRAVE_ACCENT: return ImGuiKey_GraveAccent;
        case GLFW_KEY_CAPS_LOCK: return ImGuiKey_CapsLock;
        case GLFW_KEY_SCROLL_LOCK: return ImGuiKey_ScrollLock;
        case GLFW_KEY_NUM_LOCK: return ImGuiKey_NumLock;
        case GLFW_KEY_PRINT_SCREEN: return ImGuiKey_PrintScreen;
        case GLFW_KEY_PAUSE: return ImGuiKey_Pause;
        case GLFW_KEY_KP_0: return ImGuiKey_Keypad0;
        case GLFW_KEY_KP_1: return ImGuiKey_Keypad1;
        case GLFW_KEY_KP_2: return ImGuiKey_Keypad2;
        case GLFW_KEY_KP_3: return ImGuiKey_Keypad3;
        case GLFW_KEY_KP_4: return ImGuiKey_Keypad4;
        case GLFW_KEY_KP_5: return ImGuiKey_Keypad5;
        case GLFW_KEY_KP_6: return ImGuiKey_Keypad6;
        case GLFW_KEY_KP_7: return ImGuiKey_Keypad7;
        case GLFW_KEY_KP_8: return ImGuiKey_Keypad8;
        case GLFW_KEY_KP_9: return ImGuiKey_Keypad9;
        case GLFW_KEY_KP_DECIMAL: return ImGuiKey_KeypadDecimal;
        case GLFW_KEY_KP_DIVIDE: return ImGuiKey_KeypadDivide;
        case GLFW_KEY_KP_MULTIPLY: return ImGuiKey_KeypadMultiply;
        case GLFW_KEY_KP_SUBTRACT: return ImGuiKey_KeypadSubtract;
        case GLFW_KEY_KP_ADD: return ImGuiKey_KeypadAdd;
        case GLFW_KEY_KP_ENTER: return ImGuiKey_KeypadEnter;
        case GLFW_KEY_KP_EQUAL: return ImGuiKey_KeypadEqual;
        case GLFW_KEY_LEFT_SHIFT: return ImGuiKey_LeftShift;
        case GLFW_KEY_LEFT_CONTROL: return ImGuiKey_LeftCtrl;
        case GLFW_KEY_LEFT_ALT: return ImGuiKey_LeftAlt;
        case GLFW_KEY_LEFT_SUPER: return ImGuiKey_LeftSuper;
        case GLFW_KEY_RIGHT_SHIFT: return ImGuiKey_RightShift;
        case GLFW_KEY_RIGHT_CONTROL: return ImGuiKey_RightCtrl;
        case GLFW_KEY_RIGHT_ALT: return ImGuiKey_RightAlt;
        case GLFW_KEY_RIGHT_SUPER: return ImGuiKey_RightSuper;
        case GLFW_KEY_MENU: return ImGuiKey_Menu;
        case GLFW_KEY_0: return ImGuiKey_0;
        case GLFW_KEY_1: return ImGuiKey_1;
        case GLFW_KEY_2: return ImGuiKey_2;
        case GLFW_KEY_3: return ImGuiKey_3;
        case GLFW_KEY_4: return ImGuiKey_4;
        case GLFW_KEY_5: return ImGuiKey_5;
        case GLFW_KEY_6: return ImGuiKey_6;
        case GLFW_KEY_7: return ImGuiKey_7;
        case GLFW_KEY_8: return ImGuiKey_8;
        case GLFW_KEY_9: return ImGuiKey_9;
        case GLFW_KEY_A: return ImGuiKey_A;
        case GLFW_KEY_B: return ImGuiKey_B;
        case GLFW_KEY_C: return ImGuiKey_C;
        case GLFW_KEY_D: return ImGuiKey_D;
        case GLFW_KEY_E: return ImGuiKey_E;
        case GLFW_KEY_F: return ImGuiKey_F;
        case GLFW_KEY_G: return ImGuiKey_G;
        case GLFW_KEY_H: return ImGuiKey_H;
        case GLFW_KEY_I: return ImGuiKey_I;
        case GLFW_KEY_J: return ImGuiKey_J;
        case GLFW_KEY_K: return ImGuiKey_K;
        case GLFW_KEY_L: return ImGuiKey_L;
        case GLFW_KEY_M: return ImGuiKey_M;
        case GLFW_KEY_N: return ImGuiKey_N;
        case GLFW_KEY_O: return ImGuiKey_O;
        case GLFW_KEY_P: return ImGuiKey_P;
        case GLFW_KEY_Q: return ImGuiKey_Q;
        case GLFW_KEY_R: return ImGuiKey_R;
        case GLFW_KEY_S: return ImGuiKey_S;
        case GLFW_KEY_T: return ImGuiKey_T;
        case GLFW_KEY_U: return ImGuiKey_U;
        case GLFW_KEY_V: return ImGuiKey_V;
        case GLFW_KEY_W: return ImGuiKey_W;
        case GLFW_KEY_X: return ImGuiKey_X;
        case GLFW_KEY_Y: return ImGuiKey_Y;
        case GLFW_KEY_Z: return ImGuiKey_Z;
        case GLFW_KEY_F1: return ImGuiKey_F1;
        case GLFW_KEY_F2: return ImGuiKey_F2;
        case GLFW_KEY_F3: return ImGuiKey_F3;
        case GLFW_KEY_F4: return ImGuiKey_F4;
        case GLFW_KEY_F5: return ImGuiKey_F5;
        case GLFW_KEY_F6: return ImGuiKey_F6;
        case GLFW_KEY_F7: return ImGuiKey_F7;
        case GLFW_KEY_F8: return ImGuiKey_F8;
        case GLFW_KEY_F9: return ImGuiKey_F9;
        case GLFW_KEY_F10: return ImGuiKey_F10;
        case GLFW_KEY_F11: return ImGuiKey_F11;
        case GLFW_KEY_F12: return ImGuiKey_F12;
        default: return ImGuiKey_None;
    }
}

static void updateKeyModifiers(int mods)
{
    ImGuiIO & io = ImGui::GetIO();
    io.AddKeyEvent(ImGuiKey_ModCtrl, (mods & GLFW_MOD_CONTROL) != 0);
    io.AddKeyEvent(ImGuiKey_ModShift, (mods & GLFW_MOD_SHIFT) != 0);
    io.AddKeyEvent(ImGuiKey_ModAlt, (mods & GLFW_MOD_ALT) != 0);
    io.AddKeyEvent(ImGuiKey_ModSuper, (mods & GLFW_MOD_SUPER) != 0);
}

static int translateUntranslatedKey(int key, int scancode)
{
#if GLFW_HAS_GET_KEY_NAME && !defined(__EMSCRIPTEN__)
    if (key >= GLFW_KEY_KP_0 && key <= GLFW_KEY_KP_EQUAL)
        return key;
    const char * key_name = glfwGetKeyName(key, scancode);
    if (key_name && key_name[0] != 0 && key_name[1] == 0)
    {
        const char char_names[] = "`-=[]\\,;\'./";
        const int char_keys[] = { GLFW_KEY_GRAVE_ACCENT, GLFW_KEY_MINUS, GLFW_KEY_EQUAL, GLFW_KEY_LEFT_BRACKET, GLFW_KEY_RIGHT_BRACKET, GLFW_KEY_BACKSLASH, GLFW_KEY_COMMA, GLFW_KEY_SEMICOLON, GLFW_KEY_APOSTROPHE, GLFW_KEY_PERIOD, GLFW_KEY_SLASH, 0 };
        IM_ASSERT(IM_ARRAYSIZE(char_names) == IM_ARRAYSIZE(char_keys));
        if (key_name[0] >= '0' && key_name[0] <= '9')               { key = GLFW_KEY_0 + (key_name[0] - '0'); }
        else if (key_name[0] >= 'A' && key_name[0] <= 'Z')          { key = GLFW_KEY_A + (key_name[0] - 'A'); }
        else if (const char * p = strchr(char_names, key_name[0]))   { key = char_keys[p - char_names]; }
    }
#else
    IM_UNUSED(scancode);
#endif
    return key;
}

static void windowFocusCallback(GLFWwindow * window, int focused)
{
    if (_prevUserCallbackWindowFocus != NULL && window == _window)
        _prevUserCallbackWindowFocus(window, focused);

    ImGuiIO & io = ImGui::GetIO();
    io.AddFocusEvent(focused != 0);
}

static void cursorEnterCallback(GLFWwindow * window, int entered)
{
    if (_prevUserCallbackCursorEnter != NULL && window == _window)
        _prevUserCallbackCursorEnter(window, entered);

    ImGuiIO & io = ImGui::GetIO();
    if (entered)
    {
        _mouseWindow = window;
        io.AddMousePosEvent(_lastValidMousePos.x, _lastValidMousePos.y);
    }
    else if (!entered && _mouseWindow == window)
    {
        _lastValidMousePos = io.MousePos;
        _mouseWindow = NULL;
        io.AddMousePosEvent(-FLT_MAX, -FLT_MAX);
    }
}












void imguiCreate(GLFWwindow * window, bgfx::ViewId viewId, ImVec2 windowSize) {
    assert(context == nullptr && "Already initialized imgui!");

    context = ImGui::CreateContext();
    ImGuiIO & io = ImGui::GetIO();

    IM_ASSERT(_window == NULL && "Already initialized!");
    IM_ASSERT(window != NULL && "Passed window is NULL!");

    // Setup backend capabilities flags
    io.BackendPlatformName = "imgui_bgfx_glfw";
    io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;         // We can honor GetMouseCursor() values (optional)
    io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;          // We can honor io.WantSetMousePos requests (optional, rarely used)

    _window = window;
    _time = 0.0;

    io.SetClipboardTextFn = setClipboardText;
    io.GetClipboardTextFn = getClipboardText;
    io.ClipboardUserData = _window;

    // Set platform dependent data in viewport
#if defined(_WIN32)
    ImGui::GetMainViewport()->PlatformHandleRaw = (void *)glfwGetWin32Window(_window);
#endif

    // Create mouse cursors
    // (By design, on X11 cursors are user configurable and some cursors may be missing. When a cursor doesn't exist,
    // GLFW will emit an error which will often be printed by the app, so we temporarily disable error reporting.
    // Missing cursors will return NULL and our _UpdateMouseCursor() function will use the Arrow cursor instead.)
    GLFWerrorfun prev_error_callback = glfwSetErrorCallback(NULL);
    _mouseCursors[ImGuiMouseCursor_Arrow] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
    _mouseCursors[ImGuiMouseCursor_TextInput] = glfwCreateStandardCursor(GLFW_IBEAM_CURSOR);
    _mouseCursors[ImGuiMouseCursor_ResizeNS] = glfwCreateStandardCursor(GLFW_VRESIZE_CURSOR);
    _mouseCursors[ImGuiMouseCursor_ResizeEW] = glfwCreateStandardCursor(GLFW_HRESIZE_CURSOR);
    _mouseCursors[ImGuiMouseCursor_Hand] = glfwCreateStandardCursor(GLFW_HAND_CURSOR);
#if GLFW_HAS_NEW_CURSORS
    _mouseCursors[ImGuiMouseCursor_ResizeAll] = glfwCreateStandardCursor(GLFW_RESIZE_ALL_CURSOR);
    _mouseCursors[ImGuiMouseCursor_ResizeNESW] = glfwCreateStandardCursor(GLFW_RESIZE_NESW_CURSOR);
    _mouseCursors[ImGuiMouseCursor_ResizeNWSE] = glfwCreateStandardCursor(GLFW_RESIZE_NWSE_CURSOR);
    _mouseCursors[ImGuiMouseCursor_NotAllowed] = glfwCreateStandardCursor(GLFW_NOT_ALLOWED_CURSOR);
#else
    _mouseCursors[ImGuiMouseCursor_ResizeAll] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
    _mouseCursors[ImGuiMouseCursor_ResizeNESW] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
    _mouseCursors[ImGuiMouseCursor_ResizeNWSE] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
    _mouseCursors[ImGuiMouseCursor_NotAllowed] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
#endif
    glfwSetErrorCallback(prev_error_callback);


    // Chain GLFW callbacks: our callbacks will call the user's previously installed callbacks, if any.
    IM_ASSERT(_window == window);
    _prevUserCallbackWindowFocus = glfwSetWindowFocusCallback(window, windowFocusCallback);
    _prevUserCallbackCursorEnter = glfwSetCursorEnterCallback(window, cursorEnterCallback);



    io.DisplaySize = windowSize;
    // printl("imguiCreate, io.DisplaySize (%f, %f)", io.DisplaySize.x, io.DisplaySize.y);
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
}

void imguiDestroy() {
    IM_ASSERT(_window != NULL && "Already shutdown imgui?");

    ImGuiIO & io = ImGui::GetIO();

    // restore callbacks
    glfwSetWindowFocusCallback(_window, _prevUserCallbackWindowFocus);
    glfwSetCursorEnterCallback(_window, _prevUserCallbackCursorEnter);
    _prevUserCallbackWindowFocus = NULL;
    _prevUserCallbackCursorEnter = NULL;

    for (ImGuiMouseCursor cursor_n = 0; cursor_n < ImGuiMouseCursor_COUNT; cursor_n++)
        glfwDestroyCursor(_mouseCursors[cursor_n]);

    io.BackendPlatformName = NULL;
    _window = NULL;

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

    io.DisplaySize = ImVec2((float)windowSize.w, (float)windowSize.h);
    // printl("imguiBeginFrame, io.DisplaySize (%f, %f)", io.DisplaySize.x, io.DisplaySize.y);

    io.DeltaTime = (float)dt;

    IM_ASSERT(_window != NULL && "Did you call initForXXX()?");


    // process events
    // printl("imguiBeginFrame event count: %d", events.count);
    for (int i = 0; i < events.count; ++i) {
        Event & e = events.events[i];
        switch (e.type) {

        case Event::MousePos: {
            io.AddMousePosEvent((float)e.x, (float)e.y);
            // printf("cursorPosCallback %f, %f\n", x, y);
            _lastValidMousePos = ImVec2((float)e.x, (float)e.y);
            if (e.consume && io.WantCaptureMouse) e.type = Event::None;
            break;
        }
        case Event::MouseButton: {
            updateKeyModifiers(e.mods);
            if (e.button >= 0 && e.button < ImGuiMouseButton_COUNT)
                io.AddMouseButtonEvent(e.button, (e.action == GLFW_PRESS));
            if (e.consume && io.WantCaptureMouse) e.type = Event::None;
            break;
        }
        case Event::MouseScroll: {
            io.AddMouseWheelEvent((float)e.scrollx, (float)e.scrolly);
            if (e.consume && io.WantCaptureMouse) e.type = Event::None;
            break;
        }

        case Event::Char: {
            io.AddInputCharacter(e.codepoint);
            if (e.consume && io.WantCaptureKeyboard) e.type = Event::None;
            break;
        }
        case Event::Key: {
            if (e.action != GLFW_PRESS && e.action != GLFW_RELEASE)
                break;
            updateKeyModifiers(e.mods);
            int keycode = translateUntranslatedKey(e.key, e.scancode);
            ImGuiKey imgui_key = keyToImGuiKey(keycode);
            io.AddKeyEvent(imgui_key, (e.action == GLFW_PRESS));
            io.SetKeyEventNativeData(imgui_key, keycode, e.scancode);
            if (e.consume && io.WantCaptureKeyboard) e.type = Event::None;
            break;
        }

        case Event::None:
        default:
            break;
        }
    }


    // updateMouseCursor
    if ((io.ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange) || glfwGetInputMode(_window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED)
        return;

    ImGuiMouseCursor imgui_cursor = ImGui::GetMouseCursor();
    if (imgui_cursor == ImGuiMouseCursor_None || io.MouseDrawCursor)
    {
        // Hide OS mouse cursor if imgui is drawing it or if it wants no cursor
        glfwSetInputMode(_window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
    }
    else
    {
        // Show OS mouse cursor
        // FIXME-PLATFORM: Unfocused windows seems to fail changing the mouse cursor with GLFW 3.2, but 3.3 works here.
        glfwSetCursor(_window, _mouseCursors[imgui_cursor] ? _mouseCursors[imgui_cursor] : _mouseCursors[ImGuiMouseCursor_Arrow]);
        glfwSetInputMode(_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }


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
