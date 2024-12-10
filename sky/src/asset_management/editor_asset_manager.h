#pragma once

#include <skypch.h>

#include "asset.h"
#include "asset_manager_base.h"

namespace sky
{
using AssetRegistry = std::unordered_map<AssetHandle, AssetMetadata>;

AssetType getAssetTypeFromFileExtension(const fs::path &extension);

class EditorAssetManager : public AssetManagerBase
{
  public:

    Ref<Asset> getAsset(AssetHandle handle) override;
    AssetHandle getOrCreateAssetHandle(fs::path path, AssetType assetType) override;
    AssetMetadata &getMetadata(AssetHandle handle) override;

    bool isAssetHandleValid(AssetHandle handle) const override;
    bool isAssetLoaded(AssetHandle handle) override;
    AssetType getAssetType(AssetHandle handle) const override;
    bool deserializeAssetRegistry() override;
    void serializeAssetRegistry() override;

    void importAsset(const fs::path &filepath);

    const fs::path &getFilePath(AssetHandle handle);

    const AssetRegistry &getAssetRegistry() const { return m_assetRegistry; }

  private:

  private:
    AssetRegistry m_assetRegistry;
    AssetMap m_loadedAssets;

    // TODO: memory-only assets
};
}