#pragma once

#include <skypch.h>
#include "asset.h"
#include "asset_manager_base.h"

namespace sky
{
class AssetImporter
{
  public:
    static Ref<Asset> importAsset(AssetHandle handle, AssetMetadata &metadata);
};
}