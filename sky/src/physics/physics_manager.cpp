#include "physics_manager.h"

#include "scene/scene.h"

namespace sky
{
namespace physics
{
void PhysicsManager::registerBody(RigidBody *rb, UUID entityID) { m_world->addRigidBody(rb, entityID); }

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

void PhysicsManager::init()
{
    m_world = new PhysicsWorld();
    m_isRunning = false;
}

void PhysicsManager::setScene(Scene *scene)
{
    m_world->setScene(scene);
}
} 
}