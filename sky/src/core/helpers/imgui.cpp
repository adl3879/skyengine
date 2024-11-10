#include "core/helpers/imgui.h"

namespace sky
{
namespace helper
{
bool imguiButton(std::string text, ImVec2 size, bool disabled)
{
    if (disabled)
    {
        return ImGui::Button(text.c_str(), size);
    }
    else
    {
        ImGui::BeginDisabled();
        ImGui::Button(text.c_str(), size);
        ImGui::EndDisabled();
    }
    return false;
}
}
}