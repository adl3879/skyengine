#pragma once

#include <skypch.h>

#include "core/uuid.h"

namespace sky
{
class Scene;
class Entity;

class SceneGraph
{
  public:
    SceneGraph(Scene *context);

    void deleteEntity(Entity entity);
    void parentEntity(Entity parent, Entity child);
    void unparentEntity(Entity entity);

    void traverse(Entity entity, std::function<void(Entity, int)> visit, int depth = 0);
    void unlink(Entity entity);
    Entity getEntityFromUUID(UUID uuid);
    bool valid(Entity entity) const;
    bool isDescendantOf(Entity child, Entity parent); 

  private:

  private:
    Scene *m_context;
};
}