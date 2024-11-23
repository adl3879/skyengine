#pragma once

#include <skypch.h>

#include <entt/entt.hpp>
#include "scene.h"

namespace sky
{
class Entity
{
  public:
    Entity() = default;
    Entity(entt::entity handle, Scene *scene);
    Entity(const Entity &other) = default;

    bool isNull() { return m_entityHandle == entt::null; }
    void setHandle(entt::entity handle) { m_entityHandle = handle; }

    void addChild(Entity &child);
    void removeChild(Entity &child);
    void reparentChild(Entity &child);

    template <typename T, typename... Args> T &addComponent(Args &&...args)
    {
        return m_scene->m_registry.emplace<T>(m_entityHandle, std::forward<Args>(args)...);
    }

    template <typename T, typename... Args> T &addOrReplaceComponent(Args &&...args)
    {
        T &component = m_scene->m_registry.emplace_or_replace<T>(m_entityHandle, std::forward<Args>(args)...);
        return component;
    }

    template <typename T> T &getComponent()
    {
        return m_scene->m_registry.get<T>(m_entityHandle);
    }

    template <typename T> bool hasComponent() { return m_scene->m_registry.any_of<T>(m_entityHandle); }

    template <typename T> void removeComponent()
    {
        m_scene->m_registry.remove<T>(m_entityHandle);
    }

    operator bool() const { return m_entityHandle != entt::null; }
    operator uint32_t() const { return (uint32_t)m_entityHandle; }
    operator entt::entity() const { return m_entityHandle; }

    bool operator==(const Entity &other) const
    {
        return m_entityHandle == other.m_entityHandle && m_scene == other.m_scene;
    }

    const auto &getEntityID() { return m_entityHandle; }

  private:
    entt::entity m_entityHandle{entt::null};
    Scene *m_scene = nullptr;
};
}