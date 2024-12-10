#include "inspector_panel.h"

#include <imgui.h>
#include <IconsFontAwesome5.h>

#include "scene/components.h"
#include "core/helpers/imgui.h"

#define ADD_COMPONENT_MENU(type, name, fn)			\
	if (!entity.hasComponent<type>())		\
	{											\
		if (ImGui::MenuItem(name))				\
		{										\
			entity.addComponent<type>(); fn();		\
			ImGui::CloseCurrentPopup();			\
		}										\
	}											\

namespace sky
{
void InspectorPanel::render() 
{
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10, 0));
	ImGui::Begin("Inspector");

	auto entity = m_context->getSelectedEntity();

	if (!entity.isNull())
    {
		ImGui::PushFont(gfx::ImGuiBackend::s_fonts["h4"]);
		float containerHeight = 70.0f;

		if (ImGui::BeginChild("CenteredContainer", ImVec2(0, containerHeight), false,
							  ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse))
		{
			ImGui::SetCursorPosY((containerHeight - ImGui::GetTextLineHeight()) * 0.5f); // Vertically center content

			if (m_renameRequested)
			{
				ImGui::SetKeyboardFocusHere();

				static char name[128] = "\0";
				strcpy(name, entity.getComponent<TagComponent>().c_str());
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

				if (ImGui::IsKeyPressed(ImGuiKey_Escape)) m_renameRequested = false;

				if (ImGui::IsKeyPressed(ImGuiKey_Enter, false))
				{
					if (strlen(name) >= 1) m_renameRequested = false;
					entity.getComponent<TagComponent>() = name;
				}
			}
			else
			{
				ImGui::PushStyleColor(ImGuiCol_Button, {0, 0, 0, 0});
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, {0, 0, 0, 0});
				ImGui::PushStyleColor(ImGuiCol_ButtonActive, {0, 0, 0, 0});

				if (ImGui::Button(ICON_FA_PENCIL_ALT, {40, 40})) m_renameRequested = true;
				ImGui::PopStyleColor(3);
				ImGui::SameLine();

				// Align text in the middle
				ImGui::SetCursorPosY((containerHeight - ImGui::GetTextLineHeight()) * 0.5f);
				ImGui::Text("%s", entity.getComponent<TagComponent>().c_str());
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
			ADD_COMPONENT_MENU(ModelComponent, ICON_FA_CUBE "  Mesh", []{});
			ADD_COMPONENT_MENU(PointLightComponent, ICON_FA_LIGHTBULB "  Point Light", [&]{
				entity.getComponent<PointLightComponent>().light.type = LightType::Point;
			})
			ADD_COMPONENT_MENU(SpotLightComponent, ICON_FA_LIGHTBULB "  Spot Light", [&]{
				entity.getComponent<PointLightComponent>().light.type = LightType::Point;
			})

			ImGui::EndPopup();        
		}
		ImGui::EndChild();

		helper::imguiCollapsingHeaderStyle("TRANSFORM", [&](){
			drawTransformComponent();	
		});

		helper::imguiCollapsingHeaderStyle("MESH", [&](){
			drawMeshComponent();	
		}, entity.hasComponent<ModelComponent>());

		helper::imguiCollapsingHeaderStyle("DIRECTIONAL LIGHT", [&](){
			drawDirectionalLightComponent();
		}, entity.hasComponent<DirectionalLightComponent>());
		helper::imguiCollapsingHeaderStyle("POINT LIGHT", [&](){
			drawPointLightComponent();
		}, entity.hasComponent<PointLightComponent>());
		helper::imguiCollapsingHeaderStyle("SPOT LIGHT", [&](){
			drawSpotLightComponent();
		}, entity.hasComponent<SpotLightComponent>());
	}

	ImGui::End();
	ImGui::PopStyleVar();
}

void InspectorPanel::drawTransformComponent() 
{
	auto entity = m_context->getSelectedEntity();
	if (ImGui::BeginTable("TransformTable", 2, ImGuiTableFlags_Resizable))
	{
		auto &transform = entity.getComponent<TransformComponent>();

		ImVec2 headerPadding = ImVec2(10, 10); // Increase padding for header row
		ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, headerPadding);

		// Draw Translation row
		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::Text("Position");
		ImGui::TableNextColumn();
		helper::imguiDrawVec3Control("Position", transform.getPosition());

		// Draw Rotation row
		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::Text("Rotation");
		ImGui::TableNextColumn();
		glm::vec3 eulerAngles = glm::eulerAngles(transform.getRotationQuaternion());
		glm::vec3 eulerDegrees = glm::degrees(eulerAngles);
		helper::imguiDrawVec3Control("Rotation", eulerDegrees);
		if (eulerDegrees != glm::degrees(glm::eulerAngles(transform.getRotationQuaternion())))
		{
			glm::vec3 radians = glm::radians(eulerDegrees);
			transform.setRotationDegrees(glm::quat(radians));
		}

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
{}

void InspectorPanel::drawPointLightComponent() 
{
	auto entity = m_context->getSelectedEntity();
	if (ImGui::BeginTable("PointLightTable", 2, ImGuiTableFlags_Resizable))
	{
		auto &pl = entity.getComponent<PointLightComponent>().light;

		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::Text("Color");
		ImGui::TableNextColumn();
		float col[] = { (float)pl.color.r, (float)pl.color.g, (float)pl.color.b };
		ImGui::ColorEdit3("##dl", col);
		pl.color = { col[0], col[1], col[2] };

		// Edit Intensity
		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::Text("Intensity");
		ImGui::TableNextColumn();
		ImGui::DragFloat("##plIntensity", &pl.intensity, 0.1f, 0.0f, 100.0f);

		// Edit Range
		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::Text("Range");
		ImGui::TableNextColumn();
		ImGui::DragFloat("##plRange", &pl.range, 0.1f, 0.0f, 1000.0f);

		ImGui::EndTable();
	}
}

void InspectorPanel::drawSpotLightComponent() 
{
	auto entity = m_context->getSelectedEntity();
	if (ImGui::BeginTable("SpotLightTable", 2, ImGuiTableFlags_Resizable))
	{
		auto &sl = entity.getComponent<SpotLightComponent>().light;

		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::Text("Color");
		ImGui::TableNextColumn();
		float col[] = { (float)sl.color.r, (float)sl.color.g, (float)sl.color.b };
		ImGui::ColorEdit3("##dl", col);
		sl.color = { col[0], col[1], col[2] };

		// Edit Intensity
		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::Text("Intensity");
		ImGui::TableNextColumn();
		ImGui::DragFloat("##slIntensity", &sl.intensity, 0.1f, 0.0f, 100.0f);

		// Edit Range
		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::Text("Range");
		ImGui::TableNextColumn();
		ImGui::DragFloat("##slRange", &sl.range, 0.1f, 0.0f, 1000.0f);

		// Edit Inner Cone Angle
		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::Text("Inner Cone");
		ImGui::TableNextColumn();
		if (ImGui::SliderAngle("##slInnerCone", &sl.innerConeAngle, 0.0f, 90.f))
        {
			sl.setConeAngles(sl.innerConeAngle, sl.outerConeAngle);
        }

		// Edit Outer Cone Angle
		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::Text("Outer Cone");
		ImGui::TableNextColumn();
		if (ImGui::SliderAngle("##slOuterCone", &sl.outerConeAngle, sl.innerConeAngle, 90.0f))
		{
			sl.setConeAngles(sl.innerConeAngle, sl.outerConeAngle);
        }

		ImGui::EndTable();
	}
}

void InspectorPanel::drawDirectionalLightComponent() 
{
	auto entity = m_context->getSelectedEntity();
	if (ImGui::BeginTable("DirectionalLightTable", 2, ImGuiTableFlags_Resizable))
	{
		auto &dl = entity.getComponent<DirectionalLightComponent>().light;

		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::Text("Color");
		ImGui::TableNextColumn();
		float col[] = { (float)dl.color.r, (float)dl.color.g, (float)dl.color.b };
		ImGui::ColorEdit3("##dl", col);
		dl.color = { col[0], col[1], col[2] };

		ImGui::EndTable();
	}
}
} // namespace sky