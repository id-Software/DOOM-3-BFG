// dear imgui: wrappers for C++ standard library (STL) types (std::string, etc.)
// This is also an example of how you may wrap your own similar types.

// Changelog:
// - v0.10: Initial version. Added InputText() / InputTextMultiline() calls with std::string

#pragma once

class idStr;

namespace ImGui
{
// ImGui::InputText() with std::string
// Because text input needs dynamic resizing, we need to setup a callback to grow the capacity
IMGUI_API bool  InputText( const char* label, idStr* str, ImGuiInputTextFlags flags = 0, ImGuiInputTextCallback callback = NULL, void* user_data = NULL );
IMGUI_API bool  InputTextMultiline( const char* label, idStr* str, const ImVec2& size = ImVec2( 0, 0 ), ImGuiInputTextFlags flags = 0, ImGuiInputTextCallback callback = NULL, void* user_data = NULL );
IMGUI_API bool  InputTextWithHint( const char* label, const char* hint, idStr* str, ImGuiInputTextFlags flags = 0, ImGuiInputTextCallback callback = NULL, void* user_data = NULL );
}
