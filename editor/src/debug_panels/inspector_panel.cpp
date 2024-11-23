#include "inspector_panel.h"

#include <imgui.h>
#include <IconsFontAwesome5.h>

#include "scene/components.h"
#include "core/helpers/imgui.h"

#define ADD_COMPONENT_MENU(type, name)			\
	if (!m_context.hasComponent<type>())		\
	{											\
		if (ImGui::MenuItem(name))				\
		{										\
			m_context.addComponent<type>();		\
			ImGui::CloseCurrentPopup();			\
		}										\
	}											\

namespace sky
{
bool renameRequested = false;

void InspectorPanel::render() 
{
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10, 0));
	ImGui::Begin("Inspector");

	if (!m_context.isNull())
    {
		ImGui::PushFont(gfx::ImGuiBackend::s_fonts["h4"]);
		float containerHeight = 70.0f;

		if (ImGui::BeginChild("CenteredContainer", ImVec2(0, containerHeight), false,
							  ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse))
		{
			ImGui::SetCursorPosY((containerHeight - ImGui::GetTextLineHeight()) * 0.5f); // Vertically center content

			if (renameRequested)
			{
				ImGui::SetKeyboardFocusHere();

				static char name[128] = "\0";
				strcpy(name, m_context.getComponent<TagComponent>().c_str());
				ImGui::PushItemWidth(220);

				// Set frame height alignment
				ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
				ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
				ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 2.0f);
				ImGui::InputText("##rename", name, IM_ARRAYSIZE(name));
				ImGui::PopStyleVar(2);
				ImGui::PopStyleColor(2);
				ImGui::PopItemWidth();

				if (ImGui::IsKeyPressed(ImGuiKey_Escape)) renameRequested = false;

				if (ImGui::IsKeyPressed(ImGuiKey_Enter, false))
				{
					if (strlen(name) >= 1) renameRequested = false;
					m_context.getComponent<TagComponent>() = name;
				}
			}
			else
			{
				ImGui::PushStyleColor(ImGuiCol_Button, {0, 0, 0, 0});
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, {0, 0, 0, 0});
				ImGui::PushStyleColor(ImGuiCol_ButtonActive, {0, 0, 0, 0});

				if (ImGui::Button(ICON_FA_PENCIL_ALT, {40, 40})) renameRequested = true;
				ImGui::PopStyleColor(3);
				ImGui::SameLine();

				// Align text in the middle
				ImGui::SetCursorPosY((containerHeight - ImGui::GetTextLineHeight()) * 0.5f);
				ImGui::Text("%s", m_context.getComponent<TagComponent>().c_str());
			}
		}
		ImGui::PopFont();

		ImGui::SameLine();
		// Move cursor to the far right
		ImGui::SetCursorPosX((ImGui::GetWindowWidth() - 50 * 2)); // Set the cursor position for the last text
		ImGui::SetCursorPosY(((containerHeight - 50) * 0.5f)); // Vertically center content

		if (ImGui::Button("ADD  " ICON_FA_PLUS, {100, 50}))
		{
			ImVec2 buttonPos = ImGui::GetItemRectMin();
			ImVec2 buttonSize = ImGui::GetItemRectSize();

			// Set the popup position just below the button
			ImGui::SetNextWindowPos(ImVec2(buttonPos.x, buttonPos.y + buttonSize.y + 10));
			ImGui::OpenPopup("add_component"); 
		}
		if (ImGui::BeginPopup("add_component"))
        {
			ADD_COMPONENT_MENU(MeshComponent, ICON_FA_CUBE "  Mesh");
			//ADD_COMPONENT_MENU(CameraComponent, "Camera");

			ImGui::EndPopup();        
		}
		ImGui::EndChild();

		helper::imguiCollapsingHeaderStyle("TRANSFORM", [&](){
			drawTransformComponent();	
		});

		helper::imguiCollapsingHeaderStyle("MESH", [&](){
			drawMeshComponent();	
		}, m_context.hasComponent<MeshComponent>());
	}

	ImGui::End();
	ImGui::PopStyleVar();
}

void InspectorPanel::drawTransformComponent() 
{
	if (ImGui::BeginTable("TransformTable", 2, ImGuiTableFlags_Resizable))
	{
		auto &transform = m_context.getComponent<TransformComponent>();

		ImVec2 headerPadding = ImVec2(10, 10); // Increase padding for header row
		ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, headerPadding);

		// Draw Translation row
		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::Text("Translation");
		ImGui::TableNextColumn();
		helper::imguiDrawVec3Control("Translation", transform.getPosition());

		// Draw Rotation row
		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::Text("Rotation");
		ImGui::TableNextColumn();
		helper::imguiDrawVec3Control("Rotation", transform.getPosition());

		// Draw Scale row
		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::Text("Scale");
		ImGui::TableNextColumn();
		helper::imguiDrawVec3Control("Scale", transform.getScale(), 1.f);

		ImGui::PopStyleVar();
		ImGui::EndTable();
	}
}

void InspectorPanel::drawMeshComponent() 
{
}
} // namespace sky