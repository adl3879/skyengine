#include "rigid_body.h"

#include "core/math/math.h"
#include "scene/entity.h"

namespace sky 
{
namespace physics
{
RigidBody::RigidBody(float mass, glm::vec3 position, glm::quat rotation, glm::mat4 transform, Ref<PhysicShape> shape,
    Entity entity, glm::vec3 initialValue) 
    : m_position(position), m_collisionShape(shape), Mass(mass), m_entity(entity), Transform(transform),
    m_rotation(rotation)
{} 


RigidBody::RigidBody(float mass, glm::mat4 transform, Ref<PhysicShape> shape, Entity entity)
{
    glm::vec3 position, scale;
    glm::quat rotation;
    math::DecomposeMatrix(transform, position, rotation, scale);

    m_position = position;
    m_collisionShape = shape;
    Mass = mass;
    m_entity = entity;
    Transform = transform;
    m_rotation = rotation;
    m_scale = scale;
}

void RigidBody::setEntityID(Entity ent) 
{ 
    m_entity = ent; 
}

void RigidBody::addForce(const glm::vec3 &force)
{
    //
}
}
}