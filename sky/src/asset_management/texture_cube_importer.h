#pragma once

#include <skypch.h>

#include "asset_importer.h"
#include "renderer/texture.h"

namespace sky
{
class TextureCubeImporter
{
  public:
    static Ref<TextureCube> importAsset(AssetHandle handle, AssetMetadata &metadata);
    static Ref<TextureCube> loadTexture(const fs::path &texturePath);
};
}