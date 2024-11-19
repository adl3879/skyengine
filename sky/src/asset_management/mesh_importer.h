#pragma once

#include <skypch.h>
#include "asset_importer.h"
#include "renderer/mesh.h"

namespace sky
{
class MeshImporter
{
  public:
    static Ref<Model> importAsset(AssetHandle handle, AssetMetadata &metadata);

    static Ref<Model> loadModel(AssetHandle handle, const fs::path &path);
};
}