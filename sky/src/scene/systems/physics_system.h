#pragma once

#include <skypch.h>

namespace sky
{
class Scene;

class PhysicsSystem 
{
  public:
    PhysicsSystem(Scene *scene) : m_scene(scene) {}

    void init();
    void fixedUpdate(float dt);
    void draw();

  private:
    void initShapes();
    void initRigidBodies();
    void applyForces();

  private:
    Scene *m_scene;
};
}