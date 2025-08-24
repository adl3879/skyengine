#pragma once

#include <skypch.h>

#include "physics_debug_renderer.h"
#include "physics_shapes.h"
#include "rigid_body.h"

namespace JPH
{
class PhysicsSystem;
class JobSystemThreadPool;
class ContactListener;
class BodyInterface;
class Shape;
class Character;

template <class T> class Ref;
} // namespace JPH

namespace sky 
{
class BPLayerInterfaceImpl;
class MyContactListener;
class Scene;

namespace physics
{
class PhysicsWorld
{
  public:
    PhysicsWorld(Scene *scene);
    virtual ~PhysicsWorld() = default;

    void setScene(Scene *scene) { m_scene = scene; }

  public:
    void setGravity(const glm::vec3 &gravity);
    void addRigidBody(Ref<RigidBody> rb);
    void addForceToRigidBody(Entity entity, const glm::vec3 &force);
    void stepSimulation(float dt);
    void clear();
    void drawDebug(PhysicsDebugRenderer *debugRenderer);

  private:
    JPH::Ref<JPH::Shape> getJoltShape(const Ref<PhysicShape> shape);
    void syncEntitiesTransforms();
    void syncCharactersTransforms();

  private:
    Ref<JPH::PhysicsSystem> m_joltPhysicsSystem;
    JPH::JobSystemThreadPool *m_joltJobSystem;
    JPH::BodyInterface *m_joltBodyInterface;
    BPLayerInterfaceImpl *m_joltBroadphaseLayerInterface;

    std::vector<uint32_t> m_registeredBodies;
    std::map<uint32_t, JPH::Character *> m_registeredCharacters;

  private:
    Scene *m_scene;
};
}
}