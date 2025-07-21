#include "entity.h"

namespace sky
{
Entity::Entity(entt::entity handle, Scene *scene): 
	m_entityHandle(handle), 
	m_scene(scene)
{}
} // namespace sky