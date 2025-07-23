#include "scene_graph.h"

#include "scene/entity.h"
#include "scene/components.h"

namespace sky
{
SceneGraph::SceneGraph(Scene *context)
    : m_context(context)
{}

void SceneGraph::deleteEntity(Entity entity)
{
    if (!valid(entity)) return;
    
    auto &rel = entity.getComponent<RelationshipComponent>();

    // Recursively destroy children
    auto &child = rel.firstChild;
    while (child != NULL_UUID)
    {
        auto childEntity = getEntityFromUUID(child);
        auto &next = childEntity.getComponent<RelationshipComponent>().nextSibling;
        deleteEntity(childEntity);
        child = next;
    }

    // Unlink the entity from its parent and siblings
    unlink(entity);

    m_context->destroyEntity(entity);
}

void SceneGraph::parentEntity(Entity parent, Entity child)
{
    if (parent == child || !valid(parent) || !valid(child)) return;
    unlink(child);

    auto &parentRel = parent.getComponent<RelationshipComponent>();
    auto &childRel = child.getComponent<RelationshipComponent>();

    childRel.parent = parent.getComponent<IDComponent>();
    childRel.nextSibling = NULL_UUID;
    childRel.previousSibling = NULL_UUID;

    if (parentRel.firstChild == NULL_UUID)
    {
        parentRel.firstChild = child.getComponent<IDComponent>();
    }
    else
    {
        // Find the last child and link it to the new child
        auto lastChild = parentRel.firstChild;
        while (lastChild != NULL_UUID)
        {
            auto lastChildEntity = getEntityFromUUID(lastChild);
            auto &lastChildRel = lastChildEntity.getComponent<RelationshipComponent>();
            if (lastChildRel.nextSibling == NULL_UUID)
            {
                lastChildRel.nextSibling = child.getComponent<IDComponent>();
                childRel.previousSibling = lastChild;
                break;
            }
            lastChild = lastChildRel.nextSibling;
        }
    }
}

void SceneGraph::unparentEntity(Entity entity)
{
    unlink(entity);
}

void SceneGraph::traverse(Entity entity, std::function<void(Entity, int)> visit, int depth) 
{
    visit(entity, depth);

    auto &rel = entity.getComponent<RelationshipComponent>();
    auto child = rel.firstChild;
    while (child != NULL_UUID)
    {
        auto childEntity = getEntityFromUUID(child);
        traverse(childEntity, visit, depth + 1);
        child = childEntity.getComponent<RelationshipComponent>().nextSibling;
    }
}

void SceneGraph::unlink(Entity entity)
{
    if (!valid(entity)) return;
    auto tag = entity.getComponent<TagComponent>();

    UUID entityUUID = entity.getComponent<IDComponent>();
    auto &rel = entity.getComponent<RelationshipComponent>();

    if (rel.parent != NULL_UUID)
    {
        Entity parent = getEntityFromUUID(rel.parent);
        auto &parentRel = parent.getComponent<RelationshipComponent>();
        if (parentRel.firstChild == entityUUID)
        {
            parentRel.firstChild = rel.nextSibling;
        }
    }

    if (rel.previousSibling != NULL_UUID)
    {
        Entity prev = getEntityFromUUID(rel.previousSibling);
        prev.getComponent<RelationshipComponent>().nextSibling = rel.nextSibling;
    }

    if (rel.nextSibling != NULL_UUID)
    {
        Entity next = getEntityFromUUID(rel.nextSibling);
        next.getComponent<RelationshipComponent>().previousSibling = rel.previousSibling;
    }

    rel.parent = NULL_UUID;
    rel.previousSibling = NULL_UUID;
    rel.nextSibling = NULL_UUID;
}

bool SceneGraph::valid(Entity entity) const
{
    return m_context->getRegistry().valid(entity) 
        && m_context->getRegistry().all_of<RelationshipComponent>(entity);
}

bool SceneGraph::isDescendantOf(Entity child, Entity parent)
{
    if (!valid(child) || !valid(parent)) return false;

    auto &childRel = child.getComponent<RelationshipComponent>();
    while (childRel.parent != NULL_UUID)
    {
        if (childRel.parent == parent.getComponent<IDComponent>())
        {
            return true;
        }
        child = getEntityFromUUID(childRel.parent);
        childRel = child.getComponent<RelationshipComponent>();
    }
    return false;
}

Entity SceneGraph::getEntityFromUUID(UUID uuid)
{
    return m_context->getEntityFromUUID(uuid);
}
};