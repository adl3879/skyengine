#include "physics_system.h"

#include "scene/scene.h"
#include "scene/entity.h"
#include "scene/components.h"
#include "physics/physics_manager.h"

namespace sky
{
void PhysicsSystem::init()
{
    initShapes();
    initRigidBodies();
}

void PhysicsSystem::fixedUpdate(float dt)
{
    applyForces();

    physics::PhysicsManager::get().step(dt);
}

void PhysicsSystem::draw()
{
    physics::PhysicsManager::get().drawDebug();
}

void PhysicsSystem::initShapes()
{
    auto boxView = m_scene->getRegistry().view<BoxColliderComponent>();
    for (const auto entity : boxView)
    {
        Entity ent = Entity{entity, m_scene};
        auto &boxComponent = ent.getComponent<BoxColliderComponent>();
        boxComponent.Box = physics::Box{boxComponent.Size};
    }

    auto sphereView = m_scene->getRegistry().view<SphereColliderComponent>();
    for (const auto entity : sphereView)
    {
        Entity ent = Entity{entity, m_scene};
        auto &sphereComponent = ent.getComponent<SphereColliderComponent>();
        sphereComponent.Sphere = physics::Sphere{sphereComponent.Radius};
    }

    // TODO: Add support for other shapes
}

void PhysicsSystem::initRigidBodies()
{
    // this code will be called if i have rigid body component
    auto view = m_scene->getRegistry().view<TransformComponent, RigidBodyComponent>();
    for (const auto entity : view)
    {
        auto [t, rb] = view.get<TransformComponent, RigidBodyComponent>(entity);
        auto transform = t.transform;
        Entity ent = Entity{entity, m_scene};
        auto &rigidBodyComponent = ent.getComponent<RigidBodyComponent>();
        std::optional<physics::RigidBody> rigidBody;

        if (rigidBodyComponent.getRigidBody()) continue;

        // rigidBody will not be registered if it has no shape
        if (ent.hasComponent<BoxColliderComponent>())
        {
            float mass = rigidBodyComponent.Mass;
            auto &boxComponent = ent.getComponent<BoxColliderComponent>();
            auto boxShape = CreateRef<physics::Box>(boxComponent.Size * transform.getScale());
            rigidBody = physics::RigidBody{mass, transform.getWorldMatrix(), boxShape, ent};
        }

        if (ent.hasComponent<SphereColliderComponent>())
        {
            float mass = rigidBodyComponent.Mass;
            auto &sphereComponent = ent.getComponent<SphereColliderComponent>();
            auto sphereShape = CreateRef<physics::Sphere>(sphereComponent.Radius * transform.getScale().x);
            rigidBody = physics::RigidBody{mass, transform.getWorldMatrix(), sphereShape, ent};
        }

        if (ent.hasComponent<CapsuleColliderComponent>())
        {
            float mass = rigidBodyComponent.Mass;
            auto &capsuleComponent = ent.getComponent<CapsuleColliderComponent>();
            auto capsuleShape = CreateRef<physics::Capsule>(capsuleComponent.Radius, capsuleComponent.Height);
            rigidBody = physics::RigidBody{mass, transform.getWorldMatrix(), capsuleShape, ent};
        }

        if (rigidBody.has_value())
        {
            rigidBody->MotionType = rigidBodyComponent.MotionType;
            rigidBody->LinearDamping = rigidBodyComponent.LinearDamping;
            rigidBody->AngularDamping = rigidBodyComponent.AngularDamping;
            rigidBody->IsKinematic = rigidBodyComponent.IsKinematic;
            rigidBody->UseGravity = rigidBodyComponent.UseGravity;
            physics::PhysicsManager::get().registerBody(&rigidBody.value(), ent.getComponent<IDComponent>());
        }

        rigidBodyComponent.RigidBody = rigidBody;
    }

    // TODO: Add support for other shapes
}

void PhysicsSystem::applyForces()
{
    auto view = m_scene->getRegistry().view<TransformComponent, RigidBodyComponent>();
    for (const auto entity : view)
    {
        auto [transform, rb] = view.get<TransformComponent, RigidBodyComponent>(entity);
        auto ent = Entity{entity, m_scene};
        auto &rigidBodyComponent = ent.getComponent<RigidBodyComponent>();
        Ref<physics::RigidBody> rigidBody;

        // Not initialized yet.
        if (!rigidBodyComponent.getRigidBody() || rigidBodyComponent.QueuedForce == glm::vec3() ||
            rigidBodyComponent.Mass == 0.0)
            continue;

        physics::PhysicsManager::get().getWorld()->addForceToRigidBody(ent, rigidBodyComponent.QueuedForce);

        rigidBodyComponent.QueuedForce = glm::vec3();
    }
}
}