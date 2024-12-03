#pragma once

#include <skypch.h>

#include "asset_importer.h"
#include "renderer/texture.h"

namespace sky
{
class TextureImporter
{
  public:
    static Ref<Texture2D> importAsset(AssetHandle handle, AssetMetadata &metadata);
    static Ref<Texture2D> loadTexture(const fs::path &texturePath);
    static Ref<Texture2D> loadTexture(const void *buffer, uint64_t length);
};
}