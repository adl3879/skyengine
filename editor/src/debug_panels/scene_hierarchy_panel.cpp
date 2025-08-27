#include "scene_hierarchy_panel.h"

#include <imgui.h>
#include <tracy/Tracy.hpp>
#include <IconsFontAwesome5.h>
#include "renderer/camera/game_camera.h"
#include "scene/components.h"
#include "asset_management/asset_manager.h"
#include "scene/scene.h"

namespace sky
{
static bool matchesSearch(const std::string &entityName, const char *query)
{
    std::string lowerName = entityName;
    std::string lowerQuery = query;

    // Convert both to lowercase for case-insensitive comparison
    std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
    std::transform(lowerQuery.begin(), lowerQuery.end(), lowerQuery.begin(), ::tolower);

    return lowerName.find(lowerQuery) != std::string::npos;
}

static std::string getEntityType(Entity &entity) 
{
    if (entity.hasComponent<ModelComponent>()) return "Model";
    return "";
}

bool SceneHierarchyPanel::matchesSearchRecursively(Entity &entity, const char *query)
{
    if (matchesSearch(entity.getComponent<TagComponent>(), query))
        return true;

    // TODO: Check children recursively

    return false;
}

void SceneHierarchyPanel::render() 
{
    ZoneScopedN("Scene hierarchy panel");
	ImGui::Begin("Scene Hierarchy   ");
 
	if (m_context)
	{
		static char searchQuery[128] = ""; // Buffer to hold the search query
        // Set the width to fill the available space
        ImGui::Dummy(ImVec2(0.0f, 5.0f));
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
		ImGui::InputTextWithHint("##Search", ICON_FA_SEARCH "  Search...", searchQuery, IM_ARRAYSIZE(searchQuery));
        ImGui::Dummy(ImVec2(0.0f, 5.0f));

		ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.1f, 0.1f, 0.1f, 0.54f));
        ImGui::BeginChild("SHMainContent");

	    if (ImGui::BeginPopupContextWindow())
		{
			auto e = createEntityPopup();
            m_context->getSceneGraph()->parentEntity(m_context->getRootEntity(), e);
			ImGui::EndPopup();
		}

		if (ImGui::BeginTable("shtable", 3, ImGuiTableFlags_Resizable | ImGuiTableFlags_NoBordersInBody))
		{
			// Set the padding for the header (first row)
			ImVec2 headerPadding = ImVec2(20, 10); // Increase padding for header row
			ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, headerPadding);

			ImGui::TableSetupColumn(" Label", ImGuiTableColumnFlags_WidthStretch);
			ImGui::TableSetupColumn(" Type", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn(" Visibility", ImGuiTableColumnFlags_WidthFixed, 30.f);
			ImGui::TableHeadersRow();
			
			ImGui::PopStyleVar();

            drawEntityNode(m_context->getRootEntity(), searchQuery);
            
			ImGui::EndTable();
		}

        ImGui::EndChild();
        ImGui::PopStyleColor();
	}

	if (ImGui::BeginDragDropTarget())
	{
		if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
		{
			fs::path path = (const char *)payload->Data;
			auto assetType = getAssetTypeFromFileExtension(path.extension());

			switch (assetType)
			{
				case AssetType::Mesh:
				{
					auto handle = AssetManager::getOrCreateAssetHandle(path, assetType);
					auto entity = m_context->createEntity(path.stem().string());
					entity.addComponent<ModelComponent>().handle = handle;
                    m_context->setSelectedEntity(entity);
					break;
				}
				default: break;
			}
		}

		ImGui::EndDragDropTarget(); 
	} 

    ImGui::End();
    processPendingDeletions();
}

void SceneHierarchyPanel::drawEntityNode(Entity entity, const char *query)
{ 
    auto flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_FramePadding |
        ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_DefaultOpen;

    if (entity.getComponent<RelationshipComponent>().firstChild == NULL_UUID)
    {
        flags |= ImGuiTreeNodeFlags_Leaf;
    }

    const auto tag = entity.getComponent<TagComponent>();

    ImGui::TableNextRow();

	ImGui::TableNextColumn();
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, {0, 0, 0, 0});
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, {0, 0, 0, 0});
    bool open = ImGui::TreeNodeEx((void *)(uint64_t)(uint32_t)entity, flags, "%s", tag.c_str());
    ImGui::PopStyleColor(2);

    if (ImGui::BeginDragDropSource())
    {
        UUID id = entity.getComponent<IDComponent>();
        ImGui::SetDragDropPayload("SCENE_ENTITY", &id, sizeof(UUID));
        ImGui::Text("Move %s", tag.c_str());
        ImGui::EndDragDropSource();
    }

    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("SCENE_ENTITY"))
        {
            UUID droppedUUID = *(const UUID*)payload->Data;
            Entity droppedEntity = m_context->getEntityFromUUID(droppedUUID);
            auto sceneGraph = m_context->getSceneGraph();
            if (!sceneGraph->isDescendantOf(droppedEntity, entity))
                sceneGraph->parentEntity(entity, droppedEntity);
        }
        ImGui::EndDragDropTarget();
    }

    // Highlight the row if the entity is selected
    if (ImGui::IsItemHovered() || m_context->getSelectedEntity() == entity)
    {
        ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, ImGui::GetColorU32(ImGuiCol_Header));
    }

	if (ImGui::IsItemClicked())
    {
        m_context->setSelectedEntity(entity);
    }

    if (ImGui::BeginPopupContextItem())
    {
        if (ImGui::BeginMenu("Add child"))
        {
            auto child = createEntityPopup();
            if (child)
            {
                auto sceneGraph = m_context->getSceneGraph();
                sceneGraph->parentEntity(entity, child);
            }
            ImGui::EndPopup();
        }
        if (tag != "Root")
        {
            if (ImGui::MenuItem("Delete entity"))
                m_entitiesToDelete.push_back(entity);
        }
        ImGui::EndPopup();
    }

	ImGui::TableNextColumn();
    ImGui::PushStyleColor(ImGuiCol_Text, {1, 1, 1, 0.6});
    ImGui::Text(getEntityType(entity).c_str());
    ImGui::PopStyleColor();

	ImGui::TableNextColumn();
    float cellWidth = ImGui::GetContentRegionAvail().x;
    float iconWidth = ImGui::CalcTextSize(ICON_FA_EYE).x;
    float horizontalPadding = (cellWidth - iconWidth) * 0.5f;
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + horizontalPadding);

    ImGui::PushStyleColor(ImGuiCol_Button, {0, 0, 0, 0});
    auto &visibility = entity.getComponent<VisibilityComponent>();
    if (ImGui::Button(visibility ? ICON_FA_EYE : ICON_FA_EYE_SLASH))
    {
        visibility = !visibility;
    }
    ImGui::PopStyleColor();

    if (open)
    {
        auto& rel = entity.getComponent<RelationshipComponent>();
        auto childUUID = rel.firstChild;

        while (childUUID != NULL_UUID)
        {
            Entity child = m_context->getEntityFromUUID(childUUID);
            drawEntityNode(child); // Recursive draw
            childUUID = child.getComponent<RelationshipComponent>().nextSibling;
        }

        ImGui::TreePop();
    }
}

Entity SceneHierarchyPanel::createEntityPopup() 
{
    auto entity = Entity{entt::null, m_context.get()};
    auto renderer = Application::getRenderer();

    if (ImGui::BeginMenu(ICON_FA_CUBE "  Mesh"))
    {
        if (ImGui::MenuItem(ICON_FA_CUBE "  Cube"))
        {
            entity = m_context->createEntity("Default Cube");
            auto &model = entity.addComponent<ModelComponent>();
            model.type = ModelType::Cube;
        }
        if (ImGui::MenuItem(ICON_FA_CUBE "  Plane"))
        {
            entity = m_context->createEntity("Default Plane");
            auto &model = entity.addComponent<ModelComponent>();
            model.type = ModelType::Plane;
        }
        if (ImGui::MenuItem(ICON_FA_CIRCLE "  Sphere"))
        {
            entity = m_context->createEntity("Default Sphere");
            auto &model = entity.addComponent<ModelComponent>();
            model.type = ModelType::Sphere;
        }
        if (ImGui::MenuItem(ICON_FA_CIRCLE "  Cylinder"))
        {
            entity = m_context->createEntity("Default Cylinder");
            auto &model = entity.addComponent<ModelComponent>();
            model.type = ModelType::Cylinder;
        }
        if (ImGui::MenuItem(ICON_FA_CIRCLE "  Taurus"))
        {
            entity = m_context->createEntity("Default Taurus");
            auto &model = entity.addComponent<ModelComponent>();
            model.type = ModelType::Taurus;
        }
        if (ImGui::MenuItem(ICON_FA_CIRCLE "  Cone"))
        {
            entity = m_context->createEntity("Default Cone");
            auto &model = entity.addComponent<ModelComponent>();
            model.type = ModelType::Cone;
        }
        ImGui::Separator();
        if (ImGui::MenuItem(ICON_FA_CUBE "  Empty Mesh"))
        {
        }
        ImGui::EndPopup();
    }
    if (ImGui::BeginMenu(ICON_FA_LIGHTBULB "  Light"))
    {
        if (!m_context->hasDirectionalLight() && ImGui::MenuItem(ICON_FA_LIGHTBULB "  Directional Light")) 
        {
            entity = m_context->createEntity("Directional Light");
            auto &dl = entity.addComponent<DirectionalLightComponent>().light;
            dl.type = LightType::Directional;
            dl.intensity = 2.f;
            dl.color = LinearColor::white();
            dl.id = m_context->addLightToCache(dl, entity.getComponent<TransformComponent>().transform);
        }
        if (ImGui::MenuItem(ICON_FA_LIGHTBULB "  Point Light")) 
        {
			entity = m_context->createEntity("Point Light");
            auto &pl = entity.addComponent<PointLightComponent>().light;
            pl.type = LightType::Point;
            pl.id = m_context->addLightToCache(pl, entity.getComponent<TransformComponent>().transform);
        }
        if (ImGui::MenuItem(ICON_FA_LIGHTBULB "  Spot Light")) 
        {
			entity = m_context->createEntity("Spot Light");
            auto &sl = entity.addComponent<SpotLightComponent>().light;
            sl.type = LightType::Spot;
            sl.id = m_context->addLightToCache(sl, entity.getComponent<TransformComponent>().transform);
        }
        ImGui::EndPopup();
    }
    if (ImGui::MenuItem("Camera"))
    {
        entity = m_context->createEntity("Camera");
        entity.addComponent<CameraComponent>(ProjectionType::Perspective);
        m_context->getCameraSystem()->findAndSetPrimaryCamera();
    }
    ImGui::Separator();
    if (ImGui::MenuItem("Sprite Renderer"))
    {
        entity = m_context->createEntity("Empty entity");
        auto spriteRenderer = entity.addComponent<SpriteRendererComponent>();
    }
    ImGui::Separator();
    if (ImGui::MenuItem("Create empty entity"))
    {
        entity = m_context->createEntity("Empty entity");
    }
    m_context->setSelectedEntity(entity);
    return entity;
}

void SceneHierarchyPanel::processPendingDeletions()
{
    for (Entity entity : m_entitiesToDelete)
    {
        m_context->getSceneGraph()->deleteEntity(entity);
    }
    m_entitiesToDelete.clear();
}
} // namespace sky