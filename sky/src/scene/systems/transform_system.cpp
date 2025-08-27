#include "transform_system.h"

#include "scene/scene.h"
#include "scene/entity.h"

namespace sky 
{
void TransformSystem::update()
{
    auto rootEntity = m_scene->getRootEntity();
    updateTransformRecursive(rootEntity, glm::mat4(1.0f));
}

void TransformSystem::updateTransformRecursive(Entity entity, const glm::mat4 &parentMatrix)
{
    auto &transform = entity.getComponent<TransformComponent>().transform;

    // Compute local model matrix
    glm::mat4 localMatrix = transform.getModelMatrix();

    // Compute world transform
    glm::mat4 world = parentMatrix * localMatrix;
    transform.setWorldFromMatrix(world);

    // Process children
    auto &rel = entity.getComponent<RelationshipComponent>();
    UUID child = rel.firstChild;
    while (child != NULL_UUID)
    {
        auto childEntity = m_scene->getEntityFromUUID(child);
        updateTransformRecursive(childEntity, world);
        auto &childRel = childEntity.getComponent<RelationshipComponent>();
        child = childRel.nextSibling;
    }
}
}