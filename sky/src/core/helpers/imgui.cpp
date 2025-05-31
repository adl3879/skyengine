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

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.8f, 0.1f, 0.15f, 0.8f});
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

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.2f, 0.7f, 0.2f, 0.8f});
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

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.1f, 0.25f, 0.8f, 0.8f});
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

void imguiCollapsingHeaderStyle2(const char* label, std::function<void()> fn, bool closed, bool disabled)
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
		//ImGui::Dummy({0, 8});
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 20);
        if (disabled) ImGui::BeginDisabled();
		fn();
        if (disabled) ImGui::EndDisabled();
		//ImGui::Dummy({0, 8});
		ImGui::PopFont();
	}
	ImGui::PopFont();

	// Restore original colors
	style.Colors[ImGuiCol_Header] = originalHeaderColor;
	style.Colors[ImGuiCol_HeaderHovered] = originalHeaderHoveredColor;
	style.Colors[ImGuiCol_HeaderActive] = originalHeaderActiveColor;
}

void drawButtonImage(ImageID img, ImU32 tintNormal, ImU32 tintHovered, ImU32 tintPressed) 
{
    auto *drawList = ImGui::GetForegroundDrawList();
    auto rectMin = ImGui::GetItemRectMin();
    auto rectMax = ImGui::GetItemRectMax();

    if (ImGui::IsItemActive())
        drawList->AddImage(img, rectMin, rectMax, ImVec2(0, 1), ImVec2(1, 0), tintPressed);
    else if (ImGui::IsItemHovered())
        drawList->AddImage(img, rectMin, rectMax, ImVec2(0, 1), ImVec2(1, 0), tintHovered);
    else 
        drawList->AddImage(img, rectMin, rectMax, ImVec2(0, 1), ImVec2(1, 0), tintNormal);
}

int showSelectableImagePopup(const char* title, const std::vector<SelectableImageItem>& items, 
    ImVec2 thumbnailSize, int columns)
{
    ImGui::OpenPopup(title);
    
    int selectedIndex = -1;
    
    // Center the popup
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    
    // Calculate popup size based on content
    float popupWidth = (thumbnailSize.x + 20) * std::min(columns, static_cast<int>(items.size())) + 20;
    float popupHeight = ((thumbnailSize.y + 40) * (items.size() / columns + 1)) + 80;
    ImGui::SetNextWindowSize(ImVec2(popupWidth, popupHeight), ImGuiCond_Appearing);
    
    if (ImGui::BeginPopupModal(title, nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        // Display items in a grid
        ImGui::BeginChild("ScrollingRegion", ImVec2(0, popupHeight - 60), false);
        
        int itemCount = 0;
        for (size_t i = 0; i < items.size(); i++)
        {
            // Start new row if needed
            if (itemCount % columns != 0)
                ImGui::SameLine();
            
            // Create a group for the item (image + text)
            ImGui::BeginGroup();
            
            // Create a unique ID for this item
            ImGui::PushID(static_cast<int>(i));
            
            // Create a button with the image
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.2f, 0.2f, 0.2f, 0.5f));
            
            if (ImGui::Button("##ImageButton", thumbnailSize))
            {
                selectedIndex = static_cast<int>(i);
                ImGui::CloseCurrentPopup();
            }
            
            // Draw the image on top of the button
            if (items[i].imageId != NULL_IMAGE_ID)
            {
                drawButtonImage(items[i].imageId, 
                    IM_COL32_WHITE, 
                    IM_COL32(220, 220, 220, 255), 
                    IM_COL32(180, 180, 180, 255));
            }
            
            ImGui::PopStyleColor(2);
            
            // Center and display the item name
            float textWidth = ImGui::CalcTextSize(items[i].name.c_str()).x;
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (thumbnailSize.x - textWidth) * 0.5f);
            ImGui::TextWrapped("%s", items[i].name.c_str());
            
            ImGui::PopID();
            ImGui::EndGroup();
            
            itemCount++;
        }
        
        ImGui::EndChild();
        
        ImGui::Separator();
        
        // Add Cancel button at the bottom
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5);
        if (ImGui::Button("Cancel", ImVec2(120, 0)))
        {
            ImGui::CloseCurrentPopup();
        }
        
        ImGui::EndPopup();
    }
    
    return selectedIndex;
}
}
}