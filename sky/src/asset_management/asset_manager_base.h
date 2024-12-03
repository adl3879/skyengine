#pragma once

#include <skypch.h>
#include "asset.h"

namespace sky
{
using AssetMap = std::map<AssetHandle, Ref<Asset>>;

class AssetManagerBase
{
  public:
    virtual Ref<Asset> getAsset(AssetHandle handle) = 0;
    virtual AssetHandle getOrCreateAssetHandle(fs::path path, AssetType assetType) = 0;
    virtual AssetMetadata &getMetadata(AssetHandle handle) = 0;
    virtual bool deserializeAssetRegistry() = 0;

    virtual bool isAssetHandleValid(AssetHandle handle) const = 0;
    virtual bool isAssetLoaded(AssetHandle handle) = 0;
    virtual AssetType getAssetType(AssetHandle handle) const = 0;
};
}