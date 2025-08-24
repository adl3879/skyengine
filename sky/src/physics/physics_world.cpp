#include "physics_world.h"

#include <Jolt/Jolt.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/CylinderShape.h>
#include <Jolt/Physics/Collision/Shape/MeshShape.h>
#include <Jolt/Physics/Collision/Shape/ConvexHullShape.h>

#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyActivationListener.h>
#include <Jolt/Physics/Character/Character.h>
#include <Jolt/Physics/Collision/RayCast.h>
#include <Jolt/Physics/Collision/CastResult.h>

#include <glm/ext/quaternion_common.hpp>
#include <glm/gtx/matrix_decompose.hpp>

#include "Jolt/Physics/Body/BodyManager.h"
#include "physics_debug_renderer.h"
#include "scene/scene.h"
#include "scene/components.h"
#include "scene/entity.h"

namespace sky
{
// Callback for traces, connect this to your own trace function if you have one
static void TraceImpl(const char *inFMT, ...)
{
    // Format the message
    va_list list;
    __va_start(&list, inFMT);
    char buffer[1024];
    vsnprintf(buffer, sizeof(buffer), inFMT, list);

    // Print to the TTY
    std::cout << buffer << std::endl;
}

#ifdef JPH_ENABLE_ASSERTS

// Callback for asserts, connect this to your own assert handler if you have one
static bool AssertFailedImpl(const char *inExpression, const char *inMessage, const char *inFile, uint32_t inLine)
{
    // Print to the TTY
    SKY_CORE_INFO("{}:{}: ({}) {}", inFile, inLine, inExpression, (inMessage != nullptr ? inMessage : ""));

    // Breakpoint
    return true;
};

#endif // JPH_ENABLE_ASSERTS

// Layer that objects can be in, determines which other objects it can collide with
// Typically you at least want to have 1 layer for moving bodies and 1 layer for static bodies, but you can have more
// layers if you want. E.g. you could have a layer for high detail collision (which is not used by the physics
// simulation but only if you do collision testing).
namespace Layers
{
static constexpr uint8_t NON_MOVING = 0;
static constexpr uint8_t MOVING = 1;
static constexpr uint8_t NUM_LAYERS = 2;
}; // namespace Layers

// Function that determines if two object layers can collide
static bool MyObjectCanCollide(JPH::ObjectLayer inObject1, JPH::ObjectLayer inObject2)
{
    switch (inObject1)
    {
        case Layers::NON_MOVING: return inObject2 == Layers::MOVING; // Non moving only collides with moving
        case Layers::MOVING: return true;                            // Moving collides with everything
        default:
            JPH_ASSERT(false);
            return false;
    }
};

// Each broadphase layer results in a separate bounding volume tree in the broad phase. You at least want to have
// a layer for non-moving and moving objects to avoid having to update a tree full of static objects every frame.
// You can have a 1-on-1 mapping between object layers and broadphase layers (like in this case) but if you have
// many object layers you'll be creating many broad phase trees, which is not efficient. If you want to fine tune
// your broadphase layers define JPH_TRACK_BROADPHASE_STATS and look at the stats reported on the TTY.
namespace BroadPhaseLayers
{
static constexpr JPH::BroadPhaseLayer NON_MOVING(0);
static constexpr JPH::BroadPhaseLayer MOVING(1);
static constexpr uint32_t NUM_LAYERS(2);
}; // namespace BroadPhaseLayers

// BroadPhaseLayerInterface implementation
// This defines a mapping between object and broadphase layers.
class BPLayerInterfaceImpl final : public JPH::BroadPhaseLayerInterface
{
public:
    BPLayerInterfaceImpl()
    {
        // Create a mapping table from object to broad phase layer
        mObjectToBroadPhase[Layers::NON_MOVING] = BroadPhaseLayers::NON_MOVING;
        mObjectToBroadPhase[Layers::MOVING] = BroadPhaseLayers::MOVING;
    }

    [[nodiscard]] JPH::uint GetNumBroadPhaseLayers() const override { return BroadPhaseLayers::NUM_LAYERS; }

    [[nodiscard]] JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer inLayer) const override
    {
        using namespace JPH;
        JPH_ASSERT(inLayer < Layers::NUM_LAYERS);
        return mObjectToBroadPhase[inLayer];
    }

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
    [[nodiscard]] const char *GetBroadPhaseLayerName(JPH::BroadPhaseLayer inLayer) const override
    {
        switch ((JPH::BroadPhaseLayer::Type)inLayer)
        {
            case (JPH::BroadPhaseLayer::Type)BroadPhaseLayers::NON_MOVING: return "NON_MOVING";
            case (JPH::BroadPhaseLayer::Type)BroadPhaseLayers::MOVING: return "MOVING";
            default: JPH_ASSERT(false);
                return "INVALID";
        }
    }
#endif // JPH_EXTERNAL_PROFILE || JPH_PROFILE_ENABLED

private:
    JPH::BroadPhaseLayer mObjectToBroadPhase[Layers::NUM_LAYERS];
};

// Function that determines if two broadphase layers can collide
static bool MyBroadPhaseCanCollide(JPH::ObjectLayer inLayer1, JPH::BroadPhaseLayer inLayer2)
{
    using namespace JPH;
    switch (inLayer1)
    {
        case Layers::NON_MOVING: return inLayer2 == BroadPhaseLayers::MOVING;
        case Layers::MOVING: return true;
        default: JPH_ASSERT(false);
            return false;
    }
}

class ObjectVsBroadPhaseLayerFilterImpl : public JPH::ObjectVsBroadPhaseLayerFilter
{
public:
    [[nodiscard]] bool ShouldCollide(JPH::ObjectLayer inLayer1, JPH::BroadPhaseLayer inLayer2) const override
    {
        switch (inLayer1)
        {
            case Layers::NON_MOVING: return inLayer2 == BroadPhaseLayers::MOVING;
            case Layers::MOVING: return true;
            default: return false;
        }
    }
};

class ObjectLayerPairFilterImpl : public JPH::ObjectLayerPairFilter
{
public:
    [[nodiscard]] bool ShouldCollide(JPH::ObjectLayer inObject1, JPH::ObjectLayer inObject2) const override
    {
        switch (inObject1)
        {
            case Layers::NON_MOVING: return inObject2 == Layers::MOVING; // Non moving only collides with moving
            case Layers::MOVING: return true;                            // Moving collides with everything
            default: return false;
        }
    }
};

BPLayerInterfaceImpl JoltBroadphaseLayerInterface;
ObjectVsBroadPhaseLayerFilterImpl JoltObjectVSBroadphaseLayerFilter;
ObjectLayerPairFilterImpl JoltObjectVSObjectLayerFilter;

namespace physics
{
PhysicsWorld::PhysicsWorld(Scene *scene)
    : m_scene(scene)
{
    // Register allocation hook
    JPH::RegisterDefaultAllocator();

    // Install callbacks
    JPH::Trace = TraceImpl;
    JPH_IF_ENABLE_ASSERTS(JPH::AssertFailed = AssertFailedImpl;)

    // Create a factory
    JPH::Factory::sInstance = new JPH::Factory();

    // Register all Jolt physics types
    JPH::RegisterTypes();

    m_registeredCharacters = std::map<uint32_t, JPH::Character *>();

    // initialize jolt physics
    constexpr uint32_t MaxBodies = 2048;
    constexpr uint32_t NumBodyMutexes = 0;
    constexpr uint32_t MaxBodyPairs = 1024;
    constexpr uint32_t MaxContactConstraints = 1024;

    m_joltPhysicsSystem = CreateRef<JPH::PhysicsSystem>();
    m_joltPhysicsSystem->Init(MaxBodies, NumBodyMutexes, MaxBodyPairs, MaxContactConstraints,
        JoltBroadphaseLayerInterface, JoltObjectVSBroadphaseLayerFilter,
        JoltObjectVSObjectLayerFilter);

    // m_ContactListener = std::make_unique<MyContactListener>();
    // m_joltPhysicsSystem->SetContactListener(m_ContactListener.get());

    // The main way to interact with the bodies in the physics system is through the body interface.There is a locking
    // and a non - locking variant of this. We're going to use the locking version (even though we're not planning to
    // access bodies from multiple threads)
    m_joltBodyInterface = &m_joltPhysicsSystem->GetBodyInterface();

    // Optional step: Before starting the physics simulation you can optimize the broad phase. This improves collision
    // detection performance (it's pointless here because we only have 2 bodies). You should definitely not call this
    // every frame or when e.g. streaming in a new level section as it is an expensive operation. Instead insert all new
    // objects in batches instead of 1 at a time to keep the broad phase efficient.
    m_joltPhysicsSystem->OptimizeBroadPhase();
    const uint32_t availableThreads = std::thread::hardware_concurrency() - 1;
    m_joltJobSystem = new JPH::JobSystemThreadPool(JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers, availableThreads);
}

void PhysicsWorld::drawDebug(PhysicsDebugRenderer *debugRenderer)
{
    // m_joltPhysicsSystem->DrawBodies(JPH::BodyManager::DrawSettings(), debugRenderer);
}

void PhysicsWorld::setGravity(const glm::vec3 &gravity)
{
    m_joltPhysicsSystem->SetGravity(JPH::Vec3(gravity.x, gravity.y, gravity.z));
}

void PhysicsWorld::addRigidBody(Ref<RigidBody> rb)
{
    JPH::BodyInterface &bodyInterface = m_joltPhysicsSystem->GetBodyInterface();

    const float mass = rb->Mass;
    JPH::EMotionType motionType = JPH::EMotionType::Static;

    // According to jolt documentation, Mesh shapes should only be static.
    const bool isMeshShape = rb->getShape()->getType() == RigidBodyShapes::MESH;
    if (mass > 0.0f && !isMeshShape)
    {
        if (rb->IsKinematic) motionType = JPH::EMotionType::Kinematic;
        else motionType = JPH::EMotionType::Dynamic;
    }
    if (rb->MotionType == MotionType::Static) motionType = JPH::EMotionType::Static;

    const auto &startPos = rb->getPosition();
    const auto &bodyRotation = rb->getRotation();
    const auto &joltRotation = JPH::Quat(bodyRotation.x, bodyRotation.y, bodyRotation.z, bodyRotation.w);
    const auto &joltPos = JPH::Vec3(startPos.x, startPos.y, startPos.z);
    auto joltShape = getJoltShape(rb->getShape());

    JPH::BodyCreationSettings bodySettings(joltShape, joltPos, joltRotation, motionType, Layers::MOVING);
    bodySettings.mLinearDamping = rb->LinearDamping;
    bodySettings.mAngularDamping = rb->AngularDamping;
    // bodySettings.mGravityFactor = 0.05;
    if (mass > 0.0f)
    {
        bodySettings.mOverrideMassProperties = JPH::EOverrideMassProperties::CalculateInertia;
        bodySettings.mMassPropertiesOverride.mMass = mass;
    }

    Entity e = {rb->getEntity(), m_scene};
    bodySettings.mUserData = static_cast<uint64_t>(e.getEntityID());
    // Create the actual rigid body
    JPH::BodyID body = m_joltBodyInterface->CreateAndAddBody(bodySettings, JPH::EActivation::Activate);
    m_registeredBodies.push_back((uint32_t)body.GetIndexAndSequenceNumber());
}

JPH::Ref<JPH::Shape> PhysicsWorld::getJoltShape(const Ref<PhysicShape> shape)
{
    JPH::ShapeSettings::ShapeResult result;

    switch (shape->getType())
    {
        case RigidBodyShapes::BOX:
        {
            auto *box = reinterpret_cast<Box *>(shape.get());
            const glm::vec3 &boxSize = box->getSize();
            JPH::BoxShapeSettings shapeSettings(JPH::Vec3(boxSize.x, boxSize.y, boxSize.z));
            result = shapeSettings.Create();
        }
        break;
        case RigidBodyShapes::SPHERE:
        {
            auto *sphere = reinterpret_cast<Sphere *>(shape.get());
            const float sphereRadius = sphere->getRadius();
            JPH::SphereShapeSettings shapeSettings(sphereRadius);
            result = shapeSettings.Create();
        }
        break;
        case RigidBodyShapes::CAPSULE:
        {
            auto *capsule = reinterpret_cast<Capsule *>(shape.get());
            const float radius = capsule->getRadius();
            const float height = capsule->getHeight();
            JPH::CapsuleShapeSettings shapeSettings(height / 2.0f, radius);
            result = shapeSettings.Create();
        }
        break;
        case RigidBodyShapes::CYLINDER:
        {
            auto *cylinder = reinterpret_cast<Cylinder *>(shape.get());
            const float radius = cylinder->getRadius();
            const float height = cylinder->getHeight();
            JPH::CylinderShapeSettings shapeSettings(height / 2.0f, radius);
            result = shapeSettings.Create();
        }
        break;
        case RigidBodyShapes::MESH:
        {
            //
        }
        break;
        case RigidBodyShapes::CONVEX_HULL:
        {
            auto *convexHullShape = reinterpret_cast<ConvexHullShape *>(shape.get());
            const auto &hullPoints = convexHullShape->getPoints();
            JPH::Array<JPH::Vec3> points;
            points.reserve(std::size(hullPoints));
            for (const auto &p : hullPoints)
            {
                points.push_back(JPH::Vec3(p.x, p.y, p.z));
            }

            JPH::ConvexHullShapeSettings shapeSettings(points);
            result = shapeSettings.Create();
        }
        break;
    }

    return result.Get();
}

void PhysicsWorld::syncEntitiesTransforms()
{
    auto &bodyInterface = m_joltPhysicsSystem->GetBodyInterface();
    for (const auto &body : m_registeredBodies)
    {
        auto bodyId = static_cast<JPH::BodyID>(body);
        JPH::Vec3 position = bodyInterface.GetCenterOfMassPosition(bodyId);
        JPH::Vec3 velocity = bodyInterface.GetLinearVelocity(bodyId);
        JPH::Mat44 joltTransform = bodyInterface.GetWorldTransform(bodyId);
        const auto bodyRotation = bodyInterface.GetRotation(bodyId);

        auto transform =
            glm::mat4(joltTransform(0, 0), joltTransform(1, 0), joltTransform(2, 0), joltTransform(3, 0),
                joltTransform(0, 1), joltTransform(1, 1), joltTransform(2, 1), joltTransform(3, 1),
                joltTransform(0, 2), joltTransform(1, 2), joltTransform(2, 2), joltTransform(3, 2),
                joltTransform(0, 3), joltTransform(1, 3), joltTransform(2, 3), joltTransform(3, 3));

        auto entId = static_cast<entt::entity>(bodyInterface.GetUserData(bodyId));
        Entity entity = Entity{entId, m_scene};
        auto &transformComponent = entity.getComponent<TransformComponent>();
        transformComponent.setScale(glm::vec3{0.5});
        entity.getComponent<VisibilityComponent>() = false;

        // transformComponent.setWorldFromMatrix(transform);
    }
}

void PhysicsWorld::syncCharactersTransforms()
{
}

void PhysicsWorld::stepSimulation(float dt)
{
    // If you take larger steps than 1 / 90th of a second you need to do multiple collision steps in order to keep the
    // simulation stable. Do 1 collision step per 1 / 60th of a second (round up).
    int collisionSteps = 1;
    constexpr float minStepDuration = 1.0f / 90.0f; // ? make this configurable
    constexpr int maxStepCount = 32;

    if (dt > minStepDuration)
    {
        SKY_CORE_WARN("Large step detected: {}", dt);
        collisionSteps = static_cast<float>(dt) / minStepDuration;
    }

    if (collisionSteps >= maxStepCount)
    {
        SKY_CORE_WARN("Very large step detected: {}", dt);
    }

    // Prevents having too many steps and running out of jobs
    collisionSteps = std::min(collisionSteps, maxStepCount);

    // step the world
    try
    {
        auto joltTempAllocator = std::make_shared<JPH::TempAllocatorMalloc>();
        m_joltPhysicsSystem->Update(dt, collisionSteps, joltTempAllocator.get(), m_joltJobSystem);
    }
    catch (...)
    {
        SKY_CORE_CRITICAL("Failed to run simulation step!");
    }

    syncEntitiesTransforms();
}

void PhysicsWorld::clear()
{
    if (!m_registeredBodies.empty())
    {
        m_joltBodyInterface->RemoveBodies(reinterpret_cast<JPH::BodyID *>(m_registeredBodies.data()),
            m_registeredBodies.size());
        m_registeredBodies.clear();
    }

    if (!m_registeredCharacters.empty())
    {
        for (auto &character : m_registeredCharacters)
        {
            // character.second->RemoveFromPhysicsSystem();
        }

        m_registeredCharacters.clear();
    }
}

void PhysicsWorld::addForceToRigidBody(Entity entity, const glm::vec3 &force)
{
    auto &bodyInterface = m_joltPhysicsSystem->GetBodyInterface();
    for (const auto &body : m_registeredBodies)
    {
        auto bodyId = static_cast<JPH::BodyID>(body);
        auto entityId = static_cast<uint32_t>(bodyInterface.GetUserData(bodyId));
        if (entityId == static_cast<uint32_t>(entity.getEntityID()))
        {
            bodyInterface.AddForce(bodyId, JPH::Vec3(force.x, force.y, force.z));
            return;
        }
    }
}
}
}