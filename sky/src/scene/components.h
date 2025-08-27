#pragma once

#include <skypch.h>

#include "core/transform/transform.h"
#include "renderer/mesh.h"
#include "renderer/light.h"
#include "core/uuid.h"
#include "renderer/camera/game_camera.h"
#include "physics/rigid_body.h"

namespace sky
{
using IDComponent = UUID;
using TagComponent = std::string;
using VisibilityComponent = bool;

struct TransformComponent
{
    Transform transform;

    TransformComponent() = default;
    TransformComponent(const Transform &t) : transform(t) {}
};

// Lights
struct DirectionalLightComponent { Light light; };
struct PointLightComponent { Light light; };
struct SpotLightComponent { Light light; };

struct ModelComponent
{
    ModelType type = ModelType::Custom;
    AssetHandle builtinMaterial = NULL_UUID;
    AssetHandle handle = NULL_UUID;
    std::map<uint32_t, AssetHandle> customMaterialOverrides;
};

struct RelationshipComponent
{
    UUID parent{NULL_UUID};
    UUID firstChild{NULL_UUID};
    UUID nextSibling{NULL_UUID};
    UUID previousSibling{NULL_UUID};   
};

struct SpriteRendererComponent
{
    AssetHandle textureHandle = NULL_UUID;
    glm::vec4 tint{1.f, 1.f, 1.f, 1.f};
};

struct CameraComponent
{
    GameCamera camera;

    bool isPrimary = false;
    bool isActive = true;
    int renderOrder = 0;

    CameraComponent() = default;
    
    CameraComponent(ProjectionType projType, float fov = 60.0f, float orthoSize = 5.0f)
    {
        camera.setProjectionType(projType);
        if (projType == ProjectionType::Perspective) camera.setFieldOfView(fov);
        else camera.setOrthographicSize(orthoSize);
    }
    
    void makePerspective(float fov = 60.0f, float aspect = 16.0f/9.0f, float nearPlane = 0.1f, float farPlane = 1000.0f)
    {
        camera.setPerspective(fov, aspect, nearPlane, farPlane);
    }
    
    void makeOrthographic(float size = 5.0f, float aspect = 16.0f/9.0f, float nearPlane = 0.1f, float farPlane = 1000.0f)
    {
        camera.setOrthographic(size, aspect, nearPlane, farPlane);
    }
};


struct RigidBodyComponent
{
    std::optional<physics::RigidBody> RigidBody;
    physics::MotionType MotionType = physics::MotionType::Static;
    float Mass = 1.0f;
    glm::vec3 QueuedForce{};
    float LinearDamping = 0.05f;
    float AngularDamping = 0.05f;
    bool IsKinematic = false;
    bool UseGravity = true;

    auto getRigidBody() -> physics::RigidBody* { return RigidBody ? &(*RigidBody) : nullptr; }
};

struct CapsuleColliderComponent
{
    physics::PhysicShape Capsule;
    float Radius = 1.0f;
    float Height = 1.0f;
    bool IsTrigger = false;
};

struct SphereColliderComponent
{
    physics::PhysicShape Sphere;
    float Radius = 1.0f;
    bool IsTrigger = false;
};

struct BoxColliderComponent
{
    physics::PhysicShape Box;
    glm::vec3 Size = glm::vec3(0.5f, 0.5f, 0.5f);
    bool IsTrigger = false;
};
}