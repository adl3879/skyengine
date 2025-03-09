#include "scene_hierarchy_panel.h"

#include <imgui.h>
#include <tracy/Tracy.hpp>
#include <IconsFontAwesome5.h>
#include "scene/components.h"
#include "asset_management/asset_manager.h"

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

    // Check children recursively
    for (const auto &child : entity.getComponent<HierarchyComponent>().children)
    {
        auto c = m_context->getEntityFromUUID(child);
        if (matchesSearchRecursively(c, query))
        {
            return true;
        }
    }

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
			createEntityPopup();
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

			for (auto entId : m_context->getRegistry().view<entt::entity>())
			{
				auto entity = Entity{entId, m_context.get()};
				if (entity.getComponent<HierarchyComponent>().parent == NULL_UUID)
					drawEntityNode(entity, searchQuery);
			}
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
}

void SceneHierarchyPanel::drawEntityNode(Entity entity, const char *query)
{
    // Skip entities that don’t match the search query and have no matching children
    if (strlen(query) > 0 && !matchesSearchRecursively(entity, query))
    {
        return;
    }

    auto flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_FramePadding |
                 ImGuiTreeNodeFlags_SpanFullWidth |
                 ImGuiTreeNodeFlags_DefaultOpen;

    if (entity.getComponent<HierarchyComponent>().children.size() <= 0)
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

    // Highlight the row if the entity is selected
    if (ImGui::IsItemHovered() || m_context->getSelectedEntity() == entity)
    {
        ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, ImGui::GetColorU32(ImGuiCol_Header));
    }

	if (ImGui::IsItemClicked())
    {
        m_context->setSelectedEntity(entity);
    }

    bool isEntityDeleted = false;
    if (ImGui::BeginPopupContextItem())
    {
        if (ImGui::BeginMenu("Add child"))
        {
            auto child = createEntityPopup();
            if (child) entity.addChild(child);
            ImGui::EndPopup();
        }
        if (ImGui::MenuItem("Delete entity"))
        {
            isEntityDeleted = true;
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
        const auto parent = entity.getComponent<HierarchyComponent>();
		for (const auto child : parent.children)
		{
			auto entity = m_context->getEntityFromUUID(child);
			drawEntityNode(entity, query);
		}
        ImGui::TreePop();
    }

    if (isEntityDeleted)
    {
        m_context->destroyEntity(entity);
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
       /* if (ImGui::MenuItem(ICON_FA_CIRCLE "  Capsule"))
        {
            entity = m_context->createEntity("Default Capsule");
            auto &model = entity.addComponent<ModelComponent>();
            model.type = ModelType::Capsule;
        }*/
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
            dl.id = m_context->addLightToCache(dl, entity.getComponent<TransformComponent>());
        }
        if (ImGui::MenuItem(ICON_FA_LIGHTBULB "  Point Light")) 
        {
			entity = m_context->createEntity("Point Light");
            auto &pl = entity.addComponent<PointLightComponent>().light;
            pl.type = LightType::Point;
            pl.id = m_context->addLightToCache(pl, entity.getComponent<TransformComponent>());
        }
        if (ImGui::MenuItem(ICON_FA_LIGHTBULB "  Spot Light")) 
        {
			entity = m_context->createEntity("Spot Light");
            auto &sl = entity.addComponent<SpotLightComponent>().light;
            sl.type = LightType::Spot;
            sl.id = m_context->addLightToCache(sl, entity.getComponent<TransformComponent>());
        }
        ImGui::EndPopup();
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
} // namespace sky