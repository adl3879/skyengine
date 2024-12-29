#include "inspector_panel.h"

#include <imgui.h>
#include <IconsFontAwesome5.h>
#include <tracy/Tracy.hpp>

#include "scene/components.h"
#include "core/helpers/imgui.h"
#include "asset_management/asset_manager.h"
#include "core/events/event_bus.h"
#include "renderer/texture.h"
#include "core/helpers/image.h"
#include "core/resource/material_serializer.h"

#define ADD_COMPONENT_MENU(type, name, fn)		\
	if (!entity.hasComponent<type>())			\
	{											\
		if (ImGui::MenuItem(name))				\
		{										\
			entity.addComponent<type>(); fn();	\
			ImGui::CloseCurrentPopup();			\
		}										\
	}											\

#define HANDLE_DRAG_DROP_TEXTURE(materialParam, format)                                                                \
    if (ImGui::BeginDragDropTarget())                                                                                  \
    {                                                                                                                  \
        if (const auto *payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))                                \
        {                                                                                                              \
            fs::path path = (const char *)payload->Data;                                                               \
            auto assetType = getAssetTypeFromFileExtension(path.extension());                                          \
            if (assetType == AssetType::Texture2D)                                                                     \
            {                                                                                                          \
                const auto handle = AssetManager::getOrCreateAssetHandle(path, AssetType::Texture2D);                  \
                auto texture = AssetManager::getAsset<Texture2D>(handle);                                              \
                materialParam = helper::loadImageFromTexture(texture, format, VK_IMAGE_USAGE_SAMPLED_BIT, true);       \
            }                                                                                                          \
        }                                                                                                              \
        ImGui::EndDragDropTarget();                                                                                    \
    }


namespace sky
{
void InspectorPanel::reset() {}

void InspectorPanel::render()
{
	ZoneScopedN("Inspector panel");
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10, 0));
	ImGui::Begin("Inspector");

	if (m_view != InspectorPanelView::Default)
    {
		if (ImGui::Button("Back", {80, 40}))
			m_view = InspectorPanelView::Default;
    }

	switch (m_view)
	{
		case sky::InspectorPanelView::Default: drawDefaultView(); break;
		case sky::InspectorPanelView::MaterialEditor: drawMaterialEditor(); break;
		default: break;
	}
	
	ImGui::End();
	ImGui::PopStyleVar();
}

void InspectorPanel::drawDefaultView() 
{
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

		helper::imguiCollapsingHeaderStyle("MESH RENDERER", [&](){
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
}

void InspectorPanel::drawMaterialEditor() 
{
    auto renderer = Application::getRenderer();
	const auto imageSize = ImVec2{100, 100};
	bool show = true;

	const auto getTextureOrElse = [=](ImageID imageId) { 
		return imageId != NULL_IMAGE_ID ? imageId : renderer->getCheckerboardTexture();
	};

	auto material = Material{};
	MaterialID materialId = NULL_MATERIAL_ID;
	auto isBuiltinMaterial = false;
	if (!m_materialContext.isCustom) 
	{
		material = renderer->getMaterial(m_materialContext.materialId);
		materialId = m_materialContext.materialId;
		isBuiltinMaterial = true;
	}
	else
    {
		materialId = AssetManager::getAsset<MaterialAsset>(m_materialContext.assetHandle)->material;
		material = renderer->getMaterial(materialId);
    }

	helper::imguiCollapsingHeaderStyle2("Albedo", [&](){
		if (ImGui::BeginTable("Albedo", 3, ImGuiTableFlags_Resizable))
		{
			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::Text("Texture");
			ImGui::TableNextColumn();
			ImGui::Image(getTextureOrElse(material.albedoTexture), imageSize);
			HANDLE_DRAG_DROP_TEXTURE(material.albedoTexture, VK_FORMAT_R8G8B8A8_SRGB);
			ImGui::TableNextColumn();
			ImGui::Button(ICON_FA_UNDO);
			ImGui::SameLine();
			ImGui::Button(ICON_FA_TRASH);

			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::Text("Value");
			ImGui::TableNextColumn();
			float col[] = { material.baseColor.r, material.baseColor.g, material.baseColor.b };
			ImGui::ColorEdit3("##dl", col);
			material.baseColor = { col[0], col[1], col[2], 1.f };
			ImGui::TableNextColumn();
			ImGui::Button(ICON_FA_UNDO);

			ImGui::EndTable();
		}
	}, show, isBuiltinMaterial);
	helper::imguiCollapsingHeaderStyle2("Normal Map", [&](){
		if (ImGui::BeginTable("Normal", 3, ImGuiTableFlags_Resizable))
		{
			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::Text("Texture");
			ImGui::TableNextColumn();
			ImGui::Image(getTextureOrElse(material.normalMapTexture), imageSize);
			HANDLE_DRAG_DROP_TEXTURE(material.normalMapTexture, VK_FORMAT_R8G8B8A8_UNORM);
			ImGui::TableNextColumn();
			ImGui::Button(ICON_FA_UNDO);
			ImGui::SameLine();
			ImGui::Button(ICON_FA_TRASH);

			ImGui::EndTable();
		}
	}, show, isBuiltinMaterial);
	helper::imguiCollapsingHeaderStyle2("Metallic", [&](){
		if (ImGui::BeginTable("Metallic", 3, ImGuiTableFlags_Resizable))
		{
			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::Text("Texture");
			ImGui::TableNextColumn();
			ImGui::Image(getTextureOrElse(material.metallicTexture), imageSize);
			HANDLE_DRAG_DROP_TEXTURE(material.metallicTexture, VK_FORMAT_R8G8B8A8_UNORM);
			ImGui::TableNextColumn();
			ImGui::Button(ICON_FA_UNDO);
			ImGui::SameLine();
			ImGui::Button(ICON_FA_TRASH);

			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::Text("Value");
			ImGui::TableNextColumn();
			ImGui::SliderFloat("##dd", &material.metallicFactor, 0.f, 1.f);
			ImGui::TableNextColumn();
			ImGui::Button(ICON_FA_UNDO);

			ImGui::EndTable();
		}
	}, show, isBuiltinMaterial);
	helper::imguiCollapsingHeaderStyle2("Roughness", [&](){
		if (ImGui::BeginTable("Roughness", 3, ImGuiTableFlags_Resizable))
		{
			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::Text("Texture");
			ImGui::TableNextColumn();
			ImGui::Image(getTextureOrElse(material.roughnessTexture), imageSize);
			HANDLE_DRAG_DROP_TEXTURE(material.roughnessTexture, VK_FORMAT_R8G8B8A8_UNORM);
			ImGui::TableNextColumn();
			ImGui::Button(ICON_FA_UNDO);
			ImGui::SameLine();
			ImGui::Button(ICON_FA_TRASH);

			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::Text("Value");
			ImGui::TableNextColumn();
			ImGui::SliderFloat("##dd", &material.roughnessFactor, 0.f, 1.f);
			ImGui::TableNextColumn();
			ImGui::Button(ICON_FA_UNDO);

			ImGui::EndTable();
		}
	}, show, isBuiltinMaterial);
	helper::imguiCollapsingHeaderStyle2("Ambient Occlusion", [&](){
		if (ImGui::BeginTable("Ambient Occlusion", 3, ImGuiTableFlags_Resizable))
		{
			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::Text("Texture");
			ImGui::TableNextColumn();
			ImGui::Image(getTextureOrElse(material.ambientOcclusionTexture), imageSize);
			HANDLE_DRAG_DROP_TEXTURE(material.ambientOcclusionTexture, VK_FORMAT_R8G8B8A8_UNORM);
			ImGui::TableNextColumn();
			ImGui::Button(ICON_FA_UNDO);
			ImGui::SameLine();
			ImGui::Button(ICON_FA_TRASH);

			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::Text("Value");
			ImGui::TableNextColumn();
			ImGui::SliderFloat("##dd", &material.ambientOcclusionFactor, 0.f, 1.f);
			ImGui::TableNextColumn();
			ImGui::Button(ICON_FA_UNDO);

			ImGui::EndTable();
		}
	}, show, isBuiltinMaterial);
	helper::imguiCollapsingHeaderStyle2("Emissive", [&](){
		if (ImGui::BeginTable("Emissive", 3, ImGuiTableFlags_Resizable))
		{
			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::Text("Texture");
			ImGui::TableNextColumn();
			ImGui::Image(getTextureOrElse(material.emissiveTexture), imageSize);
			HANDLE_DRAG_DROP_TEXTURE(material.albedoTexture, VK_FORMAT_R8G8B8A8_SRGB);
			ImGui::TableNextColumn();
			ImGui::Button(ICON_FA_UNDO);
			ImGui::SameLine();
			ImGui::Button(ICON_FA_TRASH);

			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::Text("Value");
			ImGui::TableNextColumn();
			ImGui::SliderFloat("##dd", &material.emissiveFactor, 0.f, 1.f);
			ImGui::TableNextColumn();
			ImGui::Button(ICON_FA_UNDO);

			ImGui::EndTable();
		}
	}, show, isBuiltinMaterial);


	renderer->updateMaterial(materialId, material);
}

void InspectorPanel::openView(InspectorPanelView view) 
{
	m_previousView = m_view;
	m_view = view;
}

void InspectorPanel::drawTransformComponent()
{
	auto entity = m_context->getSelectedEntity();
	if (ImGui::BeginTable("TransformTable", 2, ImGuiTableFlags_Resizable))
	{
		auto &transform = entity.getComponent<TransformComponent>();
		ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);

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
{
	auto entity = m_context->getSelectedEntity();
	auto &model = entity.getComponent<ModelComponent>();

	if (ImGui::BeginTable("MeshTable", 2, ImGuiTableFlags_Resizable) && model.type == ModelType::Custom)
	{

		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::Text("UUID");
		ImGui::TableNextColumn();
		ImGui::Button(model.handle.toString().c_str(), {-1, 40});

		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::Text("Path");
		ImGui::TableNextColumn();
		auto path = AssetManager::getMetadata(model.handle).filepath;
		ImGui::Button(path.string().c_str(), {-1, 40});
	
		ImGui::EndTable();
	}
	else
    {
    }

	if (ImGui::TreeNode("Surfaces") && model.type == ModelType::Custom)
	{
		auto renderer = Application::getRenderer();
		AssetManager::getAssetAsync<Model>(model.handle, [&](const Ref<Model> &m){
			for (size_t i = 0; i < m->meshes.size(); i++)
			{
				auto mesh = renderer->getMesh(m->meshes[i]);
				std::string surfaceName = "Surface_" + std::to_string(i);

				const auto material = model.customMaterialOverrides.contains(i) 
					?  AssetManager::getAsset<MaterialAsset>(model.customMaterialOverrides.at(i))->material 
					: mesh.material;

				if (ImGui::TreeNode(surfaceName.c_str()))
				{
					if (ImGui::BeginTable("MeshSurfacesTable", 2, ImGuiTableFlags_Resizable))
					{
						ImGui::TableNextRow();
						ImGui::TableNextColumn();
						ImGui::Text("Mesh");
						ImGui::TableNextColumn();
						ImGui::Button(mesh.name.c_str(), ImVec2(-1, 40));

						ImGui::TableNextRow();
						ImGui::TableNextColumn();
						ImGui::Text("Material");
						ImGui::TableNextColumn();

						if (ImGui::Button("...", {0, 40})) ImGui::OpenPopup("materialMenu");
						if (ImGui::BeginPopup("materialMenu"))
						{
							if (ImGui::MenuItem(ICON_FA_PENCIL_ALT "	Edit")) 
							{
								MaterialContext ctx;
								if (model.customMaterialOverrides.contains(i))
								{
									ctx.isCustom = true;
									ctx.assetHandle = model.customMaterialOverrides.at(i); 
								}
								else
								{
									ctx.materialId = mesh.material;
								}
								EditorEventBus::get().pushEvent({EditorEventType::OpenMaterialEditor, ctx});
							}	
							ImGui::Separator();
							if (ImGui::MenuItem("Create New Default Material")) 
							{
							}	
							if (ImGui::MenuItem("Create New Material From Current")) {}	
							if (ImGui::MenuItem("Reset to Default")) {}	
							ImGui::EndPopup();
						}

						ImGui::SameLine();
						const auto materialName = renderer->getMaterial(material).name; 

						ImGui::Button(materialName.c_str(), ImVec2(-1, 40));
						if (ImGui::BeginDragDropTarget())
						{
							if (const auto *payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
							{
								fs::path path = (const char *)payload->Data;
								auto assetType = getAssetTypeFromFileExtension(path.extension());
								if (assetType == AssetType::Material)
								{
									const auto handle = AssetManager::getOrCreateAssetHandle(path, AssetType::Material);
									model.customMaterialOverrides[i] = handle;
								}
							}
							ImGui::EndDragDropTarget();
						}

						ImGui::EndTable();
					}

					ImGui::TreePop();
				}
			}
		});
		ImGui::TreePop();
	}


	}

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
		ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
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
		ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
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
		ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
		float col[] = { (float)dl.color.r, (float)dl.color.g, (float)dl.color.b };
		ImGui::ColorEdit3("##dl", col);
		dl.color = { col[0], col[1], col[2] };

		ImGui::EndTable();
	}
}
} // namespace sky