#include "inspector_panel.h"

#include <imgui.h>
#include <IconsFontAwesome5.h>
#include <tracy/Tracy.hpp>

#include "renderer/material.h"
#include "scene/components.h"
#include "core/helpers/imgui.h"
#include "asset_management/asset_manager.h"
#include "core/events/event_bus.h"
#include "renderer/texture.h"
#include "core/helpers/image.h"
#include "core/resource/material_serializer.h"
#include "asset_browser_popup.h"
#include "core/resource/custom_thumbnail.h"

#define ADD_COMPONENT_MENU(type, name, fn)		\
	if (!entity.hasComponent<type>())			\
	{											\
		if (ImGui::MenuItem(name))				\
		{										\
			entity.addComponent<type>(); fn();	\
			ImGui::CloseCurrentPopup();			\
		}										\
	}											\

#define HANDLE_DRAG_DROP_TEXTURE(materialParam, texHandle, format)                                                     \
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
                texHandle = handle;                                                                                    \
            }                                                                                                          \
        }                                                                                                              \
        ImGui::EndDragDropTarget();                                                                                    \
    }

namespace sky
{
InspectorPanel::InspectorPanel() 
{
    m_icons["save"] = helper::loadImageFromFile("res/icons/save.png");
}

void InspectorPanel::reset() {}

void InspectorPanel::render()
{
	ZoneScopedN("Inspector panel");
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10, 0));
	ImGui::Begin("Inspector");

	if (m_view != InspectorPanelView::Default)
    {
		if (ImGui::Button("Back", {80, 40}))
        {
			m_view = InspectorPanelView::Default;
            CustomThumbnail::get().setMaterialPreviewAssetHandle(NULL_UUID);
        }
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
			});
			ADD_COMPONENT_MENU(SpotLightComponent, ICON_FA_LIGHTBULB "  Spot Light", [&]{
				entity.getComponent<PointLightComponent>().light.type = LightType::Point;
			});
            ADD_COMPONENT_MENU(RigidBodyComponent, ICON_FA_CUBE "  Rigid Body", []{});
            ADD_COMPONENT_MENU(BoxColliderComponent, ICON_FA_CUBE "   Box Collider", []{});
            ADD_COMPONENT_MENU(SphereColliderComponent, ICON_FA_CIRCLE "   Sphere Collider", []{});

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
		helper::imguiCollapsingHeaderStyle(
			"SPRITE RENDERER", [&]() { drawSpriteRendererComponent(); 
		}, entity.hasComponent<SpriteRendererComponent>());
        helper::imguiCollapsingHeaderStyle("CAMERA", [&](){
            drawCameraComponent();
        }, entity.hasComponent<CameraComponent>());
        helper::imguiCollapsingHeaderStyle("RIGID BODY", [&](){
            drawRigidBodyComponent();
        }, entity.hasComponent<RigidBodyComponent>());
        helper::imguiCollapsingHeaderStyle("BOX COLLIDER", [&](){
            drawBoxColliderComponent();
        }, entity.hasComponent<BoxColliderComponent>());
        helper::imguiCollapsingHeaderStyle("SPHERE COLLIDER", [&](){
            drawSphereColliderComponent();
        }, entity.hasComponent<SphereColliderComponent>());
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
	static std::optional<Material> previousMaterial;
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
		// only set previous material if it's not set
		if (!previousMaterial.has_value()) previousMaterial = material;
    }

	{
		float imgSize = 255;
		ImGui::SetCursorPosX(ImGui::GetWindowWidth() / 2 - imgSize / 2);
		ImGui::Image(CustomThumbnail::get().getMaterialPreview(), {imgSize, imgSize}, 
			/*vertical flip*/ {0, 1}, {1, 0});
		ImGui::Dummy({0, 10});
	}

	if (ImGui::ImageButton("##rss", m_icons["save"], {35, 35}))
    {
		MaterialSerializer serializer;
		const auto path = AssetManager::getMetadata(m_materialContext.assetHandle).filepath;
        serializer.serialize(ProjectManager::getConfig().getAssetDirectory() / path, material);
		previousMaterial.reset();

		CustomThumbnail::get().refreshThumbnail(material.name);
    }
    // button tooltip
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::Text("Save Material");
        ImGui::EndTooltip();
	}
	ImGui::SameLine();
	if (ImGui::Button(material.name.c_str(), {-1, 40})) {}

	helper::imguiCollapsingHeaderStyle2("Albedo", [&](){
		if (ImGui::BeginTable("Albedo", 3, ImGuiTableFlags_Resizable))
		{
			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::Text("Texture");
			ImGui::TableNextColumn();
			ImGui::Image(getTextureOrElse(material.albedoTexture), imageSize);
			HANDLE_DRAG_DROP_TEXTURE(material.albedoTexture, material.albedoTextureHandle, VK_FORMAT_R8G8B8A8_SRGB);
			ImGui::TableNextColumn();
			ImGui::PushID("albedo_tex_undo");
			if (ImGui::Button(ICON_FA_UNDO)) material.albedoTexture = previousMaterial->albedoTexture;
			ImGui::PopID();
			ImGui::SameLine();
			ImGui::PushID("albedo_tex_undo");
			if (ImGui::Button(ICON_FA_TRASH)) material.albedoTexture = NULL_IMAGE_ID;
			ImGui::PopID();

			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::Text("Value");
			ImGui::TableNextColumn();
			float col[] = { material.baseColor.r, material.baseColor.g, material.baseColor.b };
			ImGui::ColorEdit3("##dl", col);
			material.baseColor = { col[0], col[1], col[2], 1.f };
			ImGui::TableNextColumn();
			ImGui::PushID("albedo_undo");
			if (ImGui::Button(ICON_FA_UNDO)) material.baseColor = previousMaterial->baseColor;
			ImGui::PopID();

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
			HANDLE_DRAG_DROP_TEXTURE(material.normalMapTexture, material.normalMapTextureHandle, VK_FORMAT_R8G8B8A8_UNORM);
			ImGui::TableNextColumn();
			ImGui::PushID("normal_tex_undo");
			if (ImGui::Button(ICON_FA_UNDO)) material.normalMapTexture = previousMaterial->normalMapTexture;
			ImGui::PopID();
			ImGui::SameLine();
			ImGui::PushID("normal_tex_trash");
			if (ImGui::Button(ICON_FA_TRASH)) material.normalMapTexture = NULL_IMAGE_ID;
			ImGui::PopID();

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
			HANDLE_DRAG_DROP_TEXTURE(material.metallicTexture, material.metallicTextureHandle, VK_FORMAT_R8G8B8A8_UNORM);
			ImGui::TableNextColumn();
			ImGui::PushID("metallic_tex_undo");
			if (ImGui::Button(ICON_FA_UNDO)) material.metallicTexture = previousMaterial->metallicTexture;
			ImGui::PopID();
			ImGui::SameLine();
			ImGui::PushID("metallic_tex_trash");
			if (ImGui::Button(ICON_FA_TRASH)) material.metallicTexture = NULL_IMAGE_ID;
			ImGui::PopID();

			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::Text("Value");
			ImGui::TableNextColumn();
			ImGui::SliderFloat("##dd", &material.metallicFactor, 0.f, 1.f);
			ImGui::TableNextColumn();
			ImGui::PushID("metallic_undo");
			if (ImGui::Button(ICON_FA_UNDO)) material.metallicFactor = previousMaterial->metallicFactor;
			ImGui::PopID();

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
			HANDLE_DRAG_DROP_TEXTURE(material.roughnessTexture, material.roughnessTextureHandle, VK_FORMAT_R8G8B8A8_UNORM);
			ImGui::TableNextColumn();
			ImGui::PushID("roughness_tex_undo");
			if (ImGui::Button(ICON_FA_UNDO)) material.roughnessTexture = previousMaterial->roughnessTexture;
			ImGui::PopID();
			ImGui::SameLine();
			ImGui::PushID("roughness_tex_trash");
			if (ImGui::Button(ICON_FA_TRASH)) material.roughnessTexture = NULL_IMAGE_ID;
			ImGui::PopID();

			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::Text("Value");
			ImGui::TableNextColumn();
			ImGui::SliderFloat("##dd", &material.roughnessFactor, 0.f, 1.f);
			ImGui::TableNextColumn();
			ImGui::PushID("roughness_undo");
			if (ImGui::Button(ICON_FA_UNDO)) material.roughnessFactor = previousMaterial->roughnessFactor;
			ImGui::PopID();

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
			HANDLE_DRAG_DROP_TEXTURE(material.ambientOcclusionTexture, material.ambientOcclusionTextureHandle, 
				VK_FORMAT_R8G8B8A8_UNORM);
			ImGui::TableNextColumn();
			ImGui::PushID("ambient_occlusion_tex_undo");
			if (ImGui::Button(ICON_FA_UNDO))
				material.ambientOcclusionTexture = previousMaterial->ambientOcclusionTexture;
			ImGui::PopID();
			ImGui::SameLine();
			ImGui::PushID("ambient_occlusion_tex_trash");
			if (ImGui::Button(ICON_FA_TRASH)) material.ambientOcclusionTexture = NULL_IMAGE_ID;
			ImGui::PopID();

			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::Text("Value");
			ImGui::TableNextColumn();
			ImGui::SliderFloat("##dd", &material.ambientOcclusionFactor, 0.f, 1.f);
			ImGui::TableNextColumn();
			ImGui::PushID("ambient_occlusion_undo");
			if (ImGui::Button(ICON_FA_UNDO))
				material.ambientOcclusionFactor = previousMaterial->ambientOcclusionFactor;
			ImGui::PopID();

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
			HANDLE_DRAG_DROP_TEXTURE(material.emissiveTexture, material.emissiveTextureHandle, VK_FORMAT_R8G8B8A8_SRGB);
			ImGui::TableNextColumn();
			ImGui::PushID("emissive_tex_undo");
			if (ImGui::Button(ICON_FA_UNDO)) material.emissiveTexture = previousMaterial->emissiveTexture;
			ImGui::PopID();
			ImGui::SameLine();
			ImGui::Button(ICON_FA_TRASH);

			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::Text("Value");
			ImGui::TableNextColumn();
			ImGui::SliderFloat("##dd", &material.emissiveFactor, 0.f, 1.f);
			ImGui::TableNextColumn();
			ImGui::PushID("emissive_undo");
			if (ImGui::Button(ICON_FA_UNDO)) material.emissiveFactor = previousMaterial->emissiveFactor;
			ImGui::PopID();

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
		glm::vec3 eulerDegrees = transform.getRotationDegreesSafe();
		helper::imguiDrawVec3Control("Rotation", eulerDegrees);
		if (eulerDegrees != transform.getRotationDegrees())
		{
            transform.setRotationDegrees(eulerDegrees);
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
    auto renderer = Application::getRenderer();
	auto entity = m_context->getSelectedEntity();
	auto &model = entity.getComponent<ModelComponent>();

	if (model.type == ModelType::Custom)
    {
		if (ImGui::BeginTable("MeshTable", 2, ImGuiTableFlags_Resizable))
		{
			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::Text("UUID");
			ImGui::TableNextColumn();
			ImGui::Button(model.handle.toString().c_str(), {-1, 40});

			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::Text("Model");
			ImGui::TableNextColumn();
			const auto path = AssetManager::getMetadata(model.handle).filepath;
			ImGui::Button(path.string().c_str(), {-1, 40});
		
			ImGui::EndTable();
		}
	}
	else
    {
        if (ImGui::BeginTable("MeshTable", 2, ImGuiTableFlags_Resizable))
		{
			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::Text("Type");
			ImGui::TableNextColumn();
			ImGui::Button("Model type", {-1, 40});

			ImGui::EndTable();
		}
    }

	if (model.type == ModelType::Custom)
    {
		if (ImGui::TreeNode("Surfaces"))
		{
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
								if (model.customMaterialOverrides.contains(i))
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
								}
								if (ImGui::MenuItem("Create New Default Material")) 
								{
									EditorEventBus::get().pushEvent({EditorEventType::CreateDefaultMaterial, "default"});
								}	
								if (ImGui::MenuItem("Create New Material From Current")) 
								{
									EditorEventBus::get().pushEvent({EditorEventType::CreateNewMaterialFrom, "fromMaterial"});
								}	
								if (ImGui::MenuItem("Reset to Default")) 
								{
									model.customMaterialOverrides.erase(i);
								}	
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
						{
							auto &p = AssetBrowserPopup::s_defaultMaterialSavedFile;
							if (p.has_value())
							{
								auto handle = AssetManager::getOrCreateAssetHandle(p.value(), AssetType::Material);
								model.customMaterialOverrides[i] = handle;
								p.reset();
							}
						}
						{
							auto &p = AssetBrowserPopup::s_fromMaterialSavedFile;
							if (p.has_value())
							{
								auto handle = AssetManager::getOrCreateAssetHandle(p.value(), AssetType::Material);
								MaterialSerializer serializer;
								const auto path = ProjectManager::getConfig().getAssetDirectory() / p.value();
								serializer.serialize(path, renderer->getMaterial(mesh.material));
								model.customMaterialOverrides[i] = handle;
								p.reset();
							}
						}

						ImGui::TreePop();
					}
				}
			});
			ImGui::TreePop();
		}
	}
    else 
    {
        if (ImGui::BeginTable("BuiltinMeshTable", 2, ImGuiTableFlags_Resizable))
		{
            const auto material = model.builtinMaterial
                ?  AssetManager::getAsset<MaterialAsset>(model.builtinMaterial)->material 
                : renderer->getMaterialCache().getDefaultMaterial();

            ImGui::TableNextRow();
			ImGui::TableNextColumn();
            ImGui::Text("Material");
			ImGui::TableNextColumn();
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
                        model.builtinMaterial = handle;
                    }
                }
                ImGui::EndDragDropTarget();
            }
        }
        ImGui::EndTable();
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

void InspectorPanel::drawSpriteRendererComponent() 
{
	auto entity = m_context->getSelectedEntity();
    if (ImGui::BeginTable("SpriteRendererTable", 2, ImGuiTableFlags_Resizable))
    {
        auto &sr = entity.getComponent<SpriteRendererComponent>();
		
		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::Text("Sprite");
		ImGui::TableNextColumn();
		auto path = AssetManager::getMetadata(sr.textureHandle).filepath;
		ImGui::Button(path.string().c_str(), ImVec2(-1, 40));
		if (ImGui::BeginDragDropTarget())
		{
			if (const auto *payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
			{
				fs::path path = (const char *)payload->Data;
				auto assetType = getAssetTypeFromFileExtension(path.extension());
				if (assetType == AssetType::Texture2D)
				{
					const auto handle = AssetManager::getOrCreateAssetHandle(path, AssetType::Texture2D);
					sr.textureHandle = handle;
				}
			}
			ImGui::EndDragDropTarget();
		}

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::Text("Color");
        ImGui::TableNextColumn();
        ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
        float col[] = {(float)sr.tint.r, (float)sr.tint.g, (float)sr.tint.b};
        ImGui::ColorEdit3("##dl", col);
        sr.tint = {col[0], col[1], col[2], 1.f};

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

void InspectorPanel::drawCameraComponent()
{
    auto entity = m_context->getSelectedEntity();
    auto camSystem = m_context->getCameraSystem();
    if (ImGui::BeginTable("CameraTable", 2, ImGuiTableFlags_Resizable))
    {
        auto& camera = entity.getComponent<CameraComponent>();

        ImVec2 headerPadding = ImVec2(10, 10);
        ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, headerPadding);

        // Clear Flags
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::Text("Clear Flags");
        ImGui::TableNextColumn();
        
        ClearFlags currentClearFlags = camera.camera.getClearFlags();
        const char* clearFlagTypes[] = { "Skybox", "Solid Color", "Depth Only", "Don't Clear" };
        int clearFlagsItem = static_cast<int>(currentClearFlags);
        
        ImGui::SetNextItemWidth(-FLT_MIN);
        if (ImGui::Combo("##ClearFlags", &clearFlagsItem, clearFlagTypes, 4))
        {
            camera.camera.setClearFlags(static_cast<ClearFlags>(clearFlagsItem));
        }

        // Background Color (only show if Solid Color is selected)
        if (currentClearFlags == ClearFlags::SolidColor)
        {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("Background");
            ImGui::TableNextColumn();
            glm::vec3 backgroundColor = camera.camera.getBackgroundColor();
            float color[3] = { backgroundColor.r, backgroundColor.g, backgroundColor.b };

            ImGui::SetNextItemWidth(-FLT_MIN);
            if (ImGui::ColorEdit3("##Background", color))
            {
                camera.camera.setBackgroundColor({color[0], color[1], color[2], 1.f});
            }
        }

        // Camera Settings Header
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::Text("Primary Camera");
        ImGui::TableNextColumn();

        ImGui::SetNextItemWidth(-FLT_MIN);
        if (ImGui::Checkbox("##Primary", &camera.isPrimary)) 
            camSystem->findAndSetPrimaryCamera();

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::Text("Active");
        ImGui::TableNextColumn();

        ImGui::SetNextItemWidth(-FLT_MIN);
        if (ImGui::Checkbox("##Active", &camera.isActive))
            camSystem->findAndSetPrimaryCamera();

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::Text("Render Order");
        ImGui::TableNextColumn();

        ImGui::SetNextItemWidth(-FLT_MIN);
        ImGui::DragInt("##RenderOrder", &camera.renderOrder, 1.0f, -100, 100);

        // Projection Type
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::Text("Projection");
        ImGui::TableNextColumn();
        
        ProjectionType currentProjection = camera.camera.getProjectionType();
        const char* projectionTypes[] = { "Perspective", "Orthographic" };
        int currentItem = (currentProjection == ProjectionType::Perspective) ? 0 : 1;
        ImGui::SetNextItemWidth(-FLT_MIN);
        if (ImGui::Combo("##Projection", &currentItem, projectionTypes, 2))
        {
            ProjectionType newProjection = (currentItem == 0) ? ProjectionType::Perspective : ProjectionType::Orthographic;
            if (newProjection != currentProjection)
            {
                if (newProjection == ProjectionType::Perspective)
                {
                    camera.makePerspective();
                }
                else
                {
                    camera.makeOrthographic();
                }
            }
        }

        // Projection-specific settings
        if (currentProjection == ProjectionType::Perspective)
        {
            // Field of View
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("Field of View");
            ImGui::TableNextColumn();
            float fov = camera.camera.getFieldOfView();
            ImGui::SetNextItemWidth(-FLT_MIN);
            if (ImGui::SliderFloat("##FOV", &fov, 1.0f, 179.0f, "%.1f°"))
            {
                camera.camera.setFieldOfView(fov);
            }
        }
        else // Orthographic
        {
            // Orthographic Size
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("Size");
            ImGui::TableNextColumn();
            float orthoSize = camera.camera.getOrthographicSize();
            ImGui::SetNextItemWidth(-FLT_MIN);
            if (ImGui::DragFloat("##OrthoSize", &orthoSize, 0.1f, 0.1f, 100.0f, "%.2f"))
            {
                camera.camera.setOrthographicSize(orthoSize);
            }
        }

        // Near and Far Planes
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::Text("Near Plane");
        ImGui::TableNextColumn();
        float nearPlane = camera.camera.getNear();
        ImGui::SetNextItemWidth(-FLT_MIN);
        if (ImGui::DragFloat("##NearPlane", &nearPlane, 0.01f, 0.001f, 1000.0f, "%.3f"))
        {
            camera.camera.setNearClipPlane(nearPlane);
        }

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::Text("Far Plane");
        ImGui::TableNextColumn();
        float farPlane = camera.camera.getFar();
        ImGui::SetNextItemWidth(-FLT_MIN);
        if (ImGui::DragFloat("##FarPlane", &farPlane, 1.0f, nearPlane + 0.1f, 10000.0f, "%.1f"))
        {
            camera.camera.setFarClipPlane(farPlane);
        }

        ImGui::PopStyleVar();

        // Viewport Settings Section
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::Text("Viewport Position");
        ImGui::TableNextColumn();

        // Get viewport settings (assuming you have these in your camera)
        glm::vec4 viewportRect = camera.camera.getViewport(); // x, y, width, height (normalized 0-1)
        float viewport[4] = { viewportRect.x, viewportRect.y, viewportRect.z, viewportRect.w };

        ImGui::SetNextItemWidth(-FLT_MIN);
        if (ImGui::DragFloat2("##ViewportPosition", viewport, 0.01f, 0.0f, 1.0f, "%.2f"))
        {
            camera.camera.setViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
        }

        // Viewport Size Section
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::Text("Viewport Size");
        ImGui::TableNextColumn();

        ImGui::SetNextItemWidth(-FLT_MIN);
        if (ImGui::DragFloat2("##ViewportSize", &viewport[2], 0.01f, 0.0f, 1.0f, "%.2f"))
        {
            camera.camera.setViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
        }

        ImGui::EndTable();
    }
}

void InspectorPanel::drawRigidBodyComponent()
{
    auto entity = m_context->getSelectedEntity();
    if (!entity.hasComponent<RigidBodyComponent>())
        return;

    auto &rb = entity.getComponent<RigidBodyComponent>();

    if (ImGui::BeginTable("RigidBodyTable", 2, ImGuiTableFlags_Resizable))
    {
        ImVec2 headerPadding = ImVec2(10, 10);
        ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, headerPadding);

        // Motion Type Row
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::Text("Motion Type");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-FLT_MIN);
        if (ImGui::BeginCombo(("##MotionType"), physics::motionTypeToString(rb.MotionType).c_str()))
        {
            for (int i = 0; i < 2; i++) // assuming enum has Count
            {
                physics::MotionType mt = (physics::MotionType)i;
                bool isSelected = rb.MotionType == mt;
                if (ImGui::Selectable(physics::motionTypeToString(mt).c_str(), isSelected))
                    rb.MotionType = mt;
                if (isSelected) ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }

        if (rb.MotionType == physics::MotionType::Dynamic)
        {
            // Mass
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("Mass");
            ImGui::TableNextColumn();
            ImGui::SetNextItemWidth(-FLT_MIN);
            ImGui::DragFloat("##Mass", &rb.Mass, 0.1f, 0.0f, 10000.0f);

            // Linear Damping
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("Linear Damping");
            ImGui::TableNextColumn();
            ImGui::SetNextItemWidth(-FLT_MIN);
            ImGui::DragFloat("##LinearDamping", &rb.LinearDamping, 0.01f, 0.0f, 1.0f);

            // Angular Damping
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("Angular Damping");
            ImGui::TableNextColumn();
            ImGui::SetNextItemWidth(-FLT_MIN);
            ImGui::DragFloat("##AngularDamping", &rb.AngularDamping, 0.01f, 0.0f, 1.0f);

            // Use Gravity
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("Use Gravity");
            ImGui::TableNextColumn();
            ImGui::SetNextItemWidth(-FLT_MIN);
            ImGui::Checkbox("##Use Gravity", &rb.UseGravity);

            // Is Kinematic
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("Is Kinematic");
            ImGui::TableNextColumn();
            ImGui::SetNextItemWidth(-FLT_MIN);
            ImGui::Checkbox("##IsKinematic", &rb.IsKinematic);

            // Constraints
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("Constraints");
            ImGui::TableNextColumn();
            if (ImGui::TreeNodeEx((void*)typeid(RigidBodyComponent).hash_code(),
                ImGuiTreeNodeFlags_DefaultOpen, "Constraints"))
            {
                // TODO: constraint UI
                ImGui::TreePop();
            }
        }

        ImGui::PopStyleVar();
        ImGui::EndTable();
    }
}

void InspectorPanel::drawBoxColliderComponent()
{
    auto entity = m_context->getSelectedEntity();
    if (!entity.hasComponent<BoxColliderComponent>()) return;

    auto &collider = entity.getComponent<BoxColliderComponent>();

    if (ImGui::BeginTable("BoxColliderTable", 2, ImGuiTableFlags_Resizable))
    {
        ImVec2 headerPadding = ImVec2(10, 10);
        ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, headerPadding);

        // Size
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::Text("Size");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-FLT_MIN);
        ImGui::DragFloat3("##Size", &collider.Size.x, 0.1f, 0.0f, 1000.0f);

        // Is Trigger
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::Text("Is Trigger");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-FLT_MIN);
        ImGui::Checkbox("##IsTrigger", &collider.IsTrigger);

        ImGui::PopStyleVar();
        ImGui::EndTable();
    }
}

void InspectorPanel::drawSphereColliderComponent()
{
    auto entity = m_context->getSelectedEntity();
    if (!entity.hasComponent<SphereColliderComponent>()) return;

    auto &collider = entity.getComponent<SphereColliderComponent>();

    if (ImGui::BeginTable("SphereColliderTable", 2, ImGuiTableFlags_Resizable))
    {
        ImVec2 headerPadding = ImVec2(10, 10);
        ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, headerPadding);

        // Radius
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::Text("Radius");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-FLT_MIN);
        ImGui::DragFloat("##Radius", &collider.Radius, 0.1f, 0.0f, 10000.0f);

        // Is Trigger
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::Text("Is Trigger");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-FLT_MIN);
        ImGui::Checkbox("##IsTrigger", &collider.IsTrigger);

        ImGui::PopStyleVar();
        ImGui::EndTable();
    }
}
} // namespace sky