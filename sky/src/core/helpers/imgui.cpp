#include "core/helpers/imgui.h"

namespace sky
{
namespace helper
{
bool imguiButton(std::string text, ImVec2 size, bool disabled, std::string type)
{
    bool btn = false;
    if (type == "danger")
    {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.1f, 0.1f, 1.0f));        // Red button
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.2f, 0.2f, 1.0f)); // Lighter red on hover
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.7f, 0.0f, 0.0f, 1.0f));  // Darker red when pressed
    }
    if (!disabled)
    {
        btn = ImGui::Button(text.c_str(), size);
    }
    else
    {
        ImGui::BeginDisabled();
        ImGui::Button(text.c_str(), size);
        ImGui::EndDisabled();
    }
    if (type == "danger")
    {
        ImGui::PopStyleColor(3); // Pop all three color changes at once
    }
    return btn;
}

void imguiCenteredText(std::string text)
{
    ImVec2 windowSize = ImGui::GetWindowSize();
    ImVec2 textSize = ImGui::CalcTextSize(text.c_str());

    // Calculate the X position for centered text
    float textX = (windowSize.x - textSize.x) / 2.0f;

    // Apply the calculated position and render the text
    ImGui::SetCursorPosX(textX);
    ImGui::Text("%s", text.c_str());
}
}
}