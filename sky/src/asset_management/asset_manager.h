#pragma once

#include <skypch.h>

#include "asset.h"
#include "core/project_management/project_manager.h"

namespace sky
{
class AssetManager
{
  public:
    template <typename T> static std::shared_ptr<T> getAsset(AssetHandle handle)
    {
        Ref<Asset> asset = ProjectManager::getEditorAssetManager()->getAsset(handle);
        return std::static_pointer_cast<T>(asset);
    }

    static AssetHandle getOrCreateAssetHandle(fs::path path)
    {
        return ProjectManager::getEditorAssetManager()->getOrCreateAssetHandle(path);
    }

    static AssetMetadata& getMetadata(AssetHandle handle)
    {
        return ProjectManager::getEditorAssetManager()->getMetadata(handle);
    }
};
}