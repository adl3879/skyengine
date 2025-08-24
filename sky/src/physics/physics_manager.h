#pragma once

#include <skypch.h>

#include "physics_world.h"

namespace sky 
{
class Scene;

namespace physics
{
class PhysicsManager
{
  public:
    static PhysicsManager &get()
    {
        static PhysicsManager instance;
        return instance;
    }

    [[nodiscard]] PhysicsWorld *getWorld() const { return m_world; }

    void setDrawDebug(bool value) { m_drawDebug = value; }
    [[nodiscard]] bool getDrawDebug() const { return m_drawDebug; }

    void init(Scene *scene);
    void step(float dt);
    void stepFrame(int steps);
    void reset();
    void drawDebug();
    [[nodiscard]] bool IsRunning() const { return m_isRunning; }
    void registerBody(Ref<RigidBody> rb);
    void setScene(Scene *scene);

    void stop() { m_isRunning = false; }
    void start() { m_isRunning = true; }

  private:
    PhysicsManager() = default;

  private:
    PhysicsWorld *m_world = nullptr;
    PhysicsDebugRenderer m_joltDebugRenderer;
    bool m_isRunning = false;
    int m_stepCount = 0;
    bool m_drawDebug = true;
};
}
}