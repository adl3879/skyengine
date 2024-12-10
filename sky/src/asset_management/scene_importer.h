#pragma once

#include <skypch.h>

#include "asset_importer.h"
#include "scene/scene.h"

namespace sky
{
class SceneImporter
{
  public:
    static Ref<Scene> importAsset(AssetHandle handle, AssetMetadata &metadata);
};
}