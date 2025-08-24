#include "physics_manager.h"

#include "scene/scene.h"

namespace sky
{
namespace physics
{
void PhysicsManager::registerBody(Ref<RigidBody> rb) { m_world->addRigidBody(rb); }

void PhysicsManager::step(float dt)
{
    if (m_isRunning || m_stepCount-- > 0) m_world->stepSimulation(dt);
}

void PhysicsManager::stepFrame(int steps)
{
    m_stepCount = steps;
}

void PhysicsManager::reset() { m_world->clear(); }

void PhysicsManager::drawDebug()
{
    m_world->drawDebug(&m_joltDebugRenderer);
}

void PhysicsManager::init(Scene *scene)
{
    m_world = new PhysicsWorld(scene);
    m_isRunning = false;
}

void PhysicsManager::setScene(Scene *scene)
{
    m_world->setScene(scene);
}
} 
}