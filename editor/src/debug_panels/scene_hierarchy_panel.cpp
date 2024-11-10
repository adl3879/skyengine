#include "scene_hierarchy_panel.h"

#include <imgui.h>
#include "scene/components.h"

namespace sky
{
void SceneHierarchyPanel::render() 
{
	ImGui::Begin("Scene Hierarchy");

    if (ImGui::BeginPopupContextWindow())
    {
        createEntityPopup();
        ImGui::EndPopup();
    }

	if (m_context)
	{
        for (auto entId : m_context->getRegistry().view<entt::entity>())
        {
            auto entity = Entity{entId, m_context.get()};
            if (entity.getComponent<HierarchyComponent>().parent == NULL_UUID)
                drawEntityNode(entity);
        }
	}
    ImGui::End();
}

void SceneHierarchyPanel::drawEntityNode(Entity entity)
{
    auto flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_FramePadding |
                 ImGuiTreeNodeFlags_SpanFullWidth |
                 ImGuiTreeNodeFlags_DefaultOpen;

    if (entity.getComponent<HierarchyComponent>().children.size() <= 0)
    {
        flags |= ImGuiTreeNodeFlags_Leaf;
    }

    const auto tag = entity.getComponent<TagComponent>();
    bool open = ImGui::TreeNodeEx((void *)(uint64_t)(uint32_t)entity, flags, "%s", tag.c_str());

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

    if (open)
    {
        const auto parent = entity.getComponent<HierarchyComponent>();
        for (const auto child : parent.children)
        {
            auto entity = m_context->getEntityFromUUID(child);
            drawEntityNode(entity);
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
    const auto entity = Entity{entt::null, m_context.get()};
    if (ImGui::MenuItem("Create empty entity"))
    {
        return m_context->createEntity("Empty entity");
    }
    return entity;
}
} // namespace sky