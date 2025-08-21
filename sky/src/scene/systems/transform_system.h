#pragma once

#include <skypch.h>

#include <glm/glm.hpp>

namespace sky 
{
class Scene;
class Entity;

class TransformSystem
{
  public:
    TransformSystem(Scene *scene) : m_scene(scene) {}

    void update();

  private:
    void updateTransformRecursive(Entity entity, const glm::mat4 &parentMatrix);

  private:
    Scene* m_scene = nullptr;
};
}