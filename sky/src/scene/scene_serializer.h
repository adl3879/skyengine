#pragma once

#include <skypch.h>
#include <yaml-cpp/yaml.h>

#include "scene.h"
#include "entity.h"
#include "core/filesystem.h"
#include "asset_management/asset.h"

namespace sky
{
class SceneSerializer
{
  public:
    explicit SceneSerializer(const Ref<Scene> &scene) : m_scene(scene) {}

    void serialize(const fs::path &filepath, AssetHandle handle = NULL_UUID);
    bool deserialize(const fs::path &filepath);

  public:
    void serializeEntity(YAML::Emitter &out, Entity entity, AssetHandle handle);
    void deserializeEntity(YAML::detail::iterator_value entity, Entity deserializedEntity);

  private:
    std::shared_ptr<Scene> m_scene;
};
}