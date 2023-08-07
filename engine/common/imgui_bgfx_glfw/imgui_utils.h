#include <imgui.h>

// will set didPressEnter to false if enter-key-repeat detected, so even if
// even if user holds enter, didPressEnter will only be true once.
// mostly applicable when using ImGuiInputTextFlags_EnterReturnsTrue flag with
// InputText while keeping focus on the InputText.
// Example:
// bool didPressEnter = InputText("Label", str, MAX, ImGuiInputTextFlags_EnterReturnsTrue);
// if (didPressEtner) SetKeyboardFocusHere(-1);  // label will stay focused...
// removeKeyEnterKeyRepeat(&didPressEnter);      // but didPressEnter only true on initial key-press
inline void removeKeyEnterKeyRepeat(bool * didPressEnter) {
    using namespace ImGui;
    if (!*didPressEnter) {
        return;
    }
    bool first  = IsKeyPressed(ImGuiKey_Enter, false) || IsKeyPressed(ImGuiKey_KeypadEnter, false);
    bool repeat = IsKeyPressed(ImGuiKey_Enter, true)  || IsKeyPressed(ImGuiKey_KeypadEnter, true);
    if (!first && repeat) {
        *didPressEnter = false;
    }
}
