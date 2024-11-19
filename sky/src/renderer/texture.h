#pragma once

#include <skypch.h>

#include "asset_management/asset.h"
#include "core/buffer.h"
#include "graphics/vulkan/vk_types.h"

namespace sky
{
enum class ImageFormat
{
    None = 0,
    R8,
    RGB8,
    RGBA8,
    RGBA32F
};

struct TextureSpecification
{
    uint32_t width = 1;
    uint32_t height = 1;
    ImageFormat format = ImageFormat::RGBA8;
    bool generateMips = true;
};

class Texture2D : public Asset
{
  public:
    Texture2D() = default;
    Texture2D(Buffer data, TextureSpecification specs);

    ImageID imageID;
    auto getData() const { return m_data; }
    auto getSpecs() const { return m_specs; }

    AssetType getType() const override { return AssetType::Texture2D; }

  private:
    Buffer m_data;
    TextureSpecification m_specs;
};
}