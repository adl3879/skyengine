#pragma once

#include <skypch.h>

#include "asset_management/asset.h"
#include "graphics/vulkan/vk_types.h"

namespace sky
{
struct Texture2D : public Asset
{
    Texture2D() = default;
    ~Texture2D() = default;

    // move only
    Texture2D(Texture2D &&o) = default;
    Texture2D &operator=(Texture2D &&o) = default;

    // no copies
    Texture2D(const Texture2D &o) = delete;
    Texture2D &operator=(const Texture2D &o) = delete;

    // data
    unsigned char *pixels{nullptr};
    int width{0};
    int height{0};
    int channels{0};

	bool shouldSTBFree{false};

    // for vulkan
    ImageID vkImageID = NULL_IMAGE_ID;

	AssetType getType() const override { return AssetType::Texture2D; }
};

struct TextureCube : public Asset
{
    TextureCube() = default;
    ~TextureCube() = default;

    // move only
    TextureCube(TextureCube &&o) = default;
    TextureCube &operator=(TextureCube &&o) = default;

    // no copies
    TextureCube(const TextureCube &o) = delete;
    TextureCube &operator=(const TextureCube &o) = delete;

    // data
    float *pixels{nullptr};
    int width{0};
    int height{0};
    int channels{0};

	bool shouldSTBFree{false};

    // for vulkan
    ImageID vkImageID = NULL_IMAGE_ID;

	AssetType getType() const override { return AssetType::TextureCube; }
};
}