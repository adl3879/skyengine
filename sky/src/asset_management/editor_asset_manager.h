#pragma once

#include <skypch.h>

#include "asset.h"
#include "asset_manager_base.h"

namespace sky
{
using AssetRegistry = std::unordered_map<AssetHandle, AssetMetadata>;

class EditorAssetManager : public AssetManagerBase
{
  public:
    Ref<Asset> getAsset(AssetHandle handle) override;
    AssetHandle getOrCreateAssetHandle(fs::path path) override;
    AssetMetadata &getMetadata(AssetHandle handle) override;

    bool isAssetHandleValid(AssetHandle handle) const override;
    bool isAssetLoaded(AssetHandle handle) override;
    AssetType getAssetType(AssetHandle handle) const override;

    void importAsset(const fs::path &filepath);

    const fs::path &getFilePath(AssetHandle handle);

    const AssetRegistry &getAssetRegistry() const { return m_assetRegistry; }

  private:
    void serializeAssetRegistry();
    bool deserializeAssetRegistry();

  private:
    AssetRegistry m_assetRegistry;
    AssetMap m_loadedAssets;

    // TODO: memory-only assets
};
}