#pragma once

#include <skypch.h>

#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>
#include <entt/entt.hpp>

#include "physics_shapes.h"

namespace sky
{
class Entity;

namespace physics
{
enum class MotionType
{
    Static,
    Dynamic,
};

class RigidBody
{
  public:
    RigidBody() = default;
    RigidBody(glm::vec3 position, Entity handle);
    RigidBody(float mass, glm::vec3 position, glm::quat rotation, glm::mat4 transform, Ref<PhysicShape> shape,
        Entity entity, glm::vec3 initialValue = glm::vec3(0.0f)); 
    RigidBody(float mass, glm::mat4 transform, Ref<PhysicShape> shape, Entity entity);

    void setEntityID(Entity ent);

    [[nodiscard]] bool hasShape() const { return m_collisionShape != nullptr; }
    void setShape(Ref<PhysicShape> shape) { m_collisionShape = shape; }
    void addForce(const glm::vec3 &force);

    [[nodiscard]] Ref<PhysicShape> getShape() const { return m_collisionShape; }
    [[nodiscard]] auto getEntity() const { return m_entity; }
    [[nodiscard]] glm::vec3 getPosition() const { return m_position; }
    [[nodiscard]] glm::quat getRotation() const { return m_rotation; }
    [[nodiscard]] glm::vec3 getScale() const { return m_scale; }

  public:
    float Mass;
    glm::mat4 Transform;
    float LinearDamping;
    float AngularDamping;
    bool IsKinematic;
    bool UseGravity;
    MotionType MotionType;

  private:
    Ref<PhysicShape> m_collisionShape;
    glm::vec3 m_position = glm::vec3(0.0f);
    glm::quat m_rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    glm::vec3 m_scale = glm::vec3(1.0f);
    entt::entity m_entity;
};

static std::string motionTypeToString(MotionType type)
{
    switch (type)
    {
        case MotionType::Static: return "Static";
        case MotionType::Dynamic: return "Dynamic";
        default: return "Unknown";
    }
}

static MotionType motionTypeFromString(std::string type)
{
    if (type == "Static") return MotionType::Static;
    if (type == "Dynamic") return MotionType::Dynamic;
    return MotionType::Static;
}
} // namespace physics
} // namespace sky