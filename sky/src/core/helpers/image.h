#pragma once

#include <skypch.h>
#include "core/filesystem.h"
#include "graphics/vulkan/vk_types.h"
#include "renderer/texture.h"

namespace sky
{
namespace helper
{
ImageID loadImageFromFile(const fs::path& path, float scaleFactor = 1.f);
ImageID loadImageFromData(const void *buffer, uint64_t length);
ImageID loadImageFromTexture(Ref<Texture2D> tex, VkFormat format, 
	VkImageUsageFlags usage = VK_IMAGE_USAGE_SAMPLED_BIT, bool mipMap = true);
ImageID loadImageFromTexture(Ref<TextureCube> tex, VkFormat format, 
	VkImageUsageFlags usage = VK_IMAGE_USAGE_SAMPLED_BIT, bool mipMap = true);
}
}