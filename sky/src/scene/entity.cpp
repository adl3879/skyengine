#include "entity.h"

namespace sky
{
Entity::Entity(entt::entity handle, Scene *scene): 
	m_entityHandle(handle), 
	m_scene(scene)
{}

void Entity::addChild(Entity &child) {}
} // namespace sky