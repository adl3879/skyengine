#include "entity.h"

#include "scene/components.h"

namespace sky
{
Entity::Entity(entt::entity handle, Scene *scene): 
	m_entityHandle(handle), 
	m_scene(scene)
{}

void Entity::addChild(Entity &child) 
{
	auto &parentHierarchy = getComponent<HierarchyComponent>();
    auto &childHierarchy = child.getComponent<HierarchyComponent>();

	parentHierarchy.children.push_back(child.getComponent<IDComponent>());
    childHierarchy.parent = getComponent<IDComponent>();
}

void Entity::removeChild(Entity& child)
{
    auto &parentHierarchy = getComponent<HierarchyComponent>();
    auto &childHierarchy = child.getComponent<HierarchyComponent>();

	// remove child from the parent's list of children
    auto &children = parentHierarchy.children;
    children.erase(std::remove(children.begin(), children.end(), child.getComponent<IDComponent>()), children.end());

    // reset child's parent to indicate it has no parent
    childHierarchy.parent = NULL_UUID;
}

void Entity::reparentChild(Entity& child)
{
    auto &childHierarchy = child.getComponent<HierarchyComponent>();

    // remove from the current parent if it exists
    if (childHierarchy.parent != NULL_UUID)
    {
        removeChild(child);
    }

    addChild(child);
}
} // namespace sky