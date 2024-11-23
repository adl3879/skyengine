#include "core/helpers/imgui.h"

#include <imgui_internal.h>
#include <graphics/vulkan/vk_imgui_backend.h>

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

void imguiDrawVec3Control(const std::string &label, glm::vec3 &values, float resetValue, float columnWidth)
{
    ImGuiIO &io = ImGui::GetIO();
    auto boldFont = gfx::ImGuiBackend::s_fonts["bold"];

    ImGui::PushID(label.c_str());

    ImGui::PushMultiItemsWidths(3, ImGui::CalcItemWidth());
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{0, 0});

    float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
    ImVec2 buttonSize = {lineHeight + 3.0f, lineHeight};

    float itemWidth = (ImGui::GetContentRegionAvail().x / 3.0f) - buttonSize.x;

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.8f, 0.1f, 0.15f, 1.0f});
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{0.9f, 0.2f, 0.2f, 1.0f});
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{0.8f, 0.1f, 0.15f, 1.0f});
    ImGui::PushFont(boldFont);
    if (ImGui::Button("X", buttonSize)) values.x = resetValue;
    ImGui::PopFont();
    ImGui::PopStyleColor(3);

    ImGui::SameLine();
    ImGui::SetNextItemWidth(itemWidth);
    ImGui::DragFloat("##X", &values.x, 0.1f, 0.0f, 0.0f, "%.2f");
    ImGui::PopItemWidth();
    ImGui::SameLine();

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.2f, 0.7f, 0.2f, 1.0f});
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{0.3f, 0.8f, 0.3f, 1.0f});
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{0.2f, 0.7f, 0.2f, 1.0f});
    ImGui::PushFont(boldFont);
    if (ImGui::Button("Y", buttonSize)) values.y = resetValue;
    ImGui::PopFont();
    ImGui::PopStyleColor(3);

    ImGui::SameLine();
    ImGui::SetNextItemWidth(itemWidth);
    ImGui::DragFloat("##Y", &values.y, 0.1f, 0.0f, 0.0f, "%.2f");
    ImGui::PopItemWidth();
    ImGui::SameLine();

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.1f, 0.25f, 0.8f, 1.0f});
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{0.2f, 0.35f, 0.9f, 1.0f});
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{0.1f, 0.25f, 0.8f, 1.0f});
    ImGui::PushFont(boldFont);
    if (ImGui::Button("Z", buttonSize)) values.z = resetValue;
    ImGui::PopFont();
    ImGui::PopStyleColor(3);

    ImGui::SameLine();
    ImGui::SetNextItemWidth(itemWidth);
    ImGui::DragFloat("##Z", &values.z, 0.1f, 0.0f, 0.0f, "%.2f");
    ImGui::PopItemWidth();

    ImGui::PopStyleVar();

    ImGui::PopID();
}

void imguiCollapsingHeaderStyle(const char* label, std::function<void()> fn, bool show, bool closed)
{
    if (show)
    {
		ImGuiStyle &style = ImGui::GetStyle();
		ImFont *defaultFont = ImGui::GetFont();
		auto boldFont = gfx::ImGuiBackend::s_fonts["bold"];

		// Backup current colors
		ImVec4 originalHeaderColor = style.Colors[ImGuiCol_Header];
		ImVec4 originalHeaderHoveredColor = style.Colors[ImGuiCol_HeaderHovered];
		ImVec4 originalHeaderActiveColor = style.Colors[ImGuiCol_HeaderActive];

		// Set custom colors
		style.Colors[ImGuiCol_Header] = ImVec4(0.3f, 0.3f, 0.3f, 1.0f);        // Normal
		style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.3f, 0.3f, 0.3f, 1.0f); // Hovered
		style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.3f, 0.3f, 0.3f, 1.0f);  // Active

		if (!closed) ImGui::SetNextItemOpen(true, ImGuiCond_Once);

		 // Render the CollapsingHeader
		ImGui::PushFont(boldFont);
		if (ImGui::CollapsingHeader(label))
		{
			ImGui::PushFont(defaultFont);
			ImGui::Dummy({0, 8});
			ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 20);
			fn();
			ImGui::Dummy({0, 8});
			ImGui::PopFont();
		}
		ImGui::PopFont();

		// Restore original colors
		style.Colors[ImGuiCol_Header] = originalHeaderColor;
		style.Colors[ImGuiCol_HeaderHovered] = originalHeaderHoveredColor;
		style.Colors[ImGuiCol_HeaderActive] = originalHeaderActiveColor;
    }
}
}
}