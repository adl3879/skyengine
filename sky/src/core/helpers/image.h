#pragma once

#include <skypch.h>
#include "core/filesystem.h"
#include "graphics/vulkan/vk_types.h"
#include "renderer/texture.h"

namespace sky
{
namespace helper
{
ImageID loadImageFromFile(const fs::path& path);
ImageID loadImageFromData(const void *buffer, uint64_t length);
ImageID loadImageFromTexture(Ref<Texture2D> tex, VkFormat format, VkImageUsageFlags usage, bool mipMap);
}
}