
#pragma once

#include <vulkan/vulkan.h>

namespace sky::gfx::vkutil
{
void transitionImage(VkCommandBuffer cmd, VkImage image, VkImageLayout currentLayout, VkImageLayout newLayout);
void copyImageToImage(VkCommandBuffer cmd, VkImage source, VkImage destination, VkExtent2D srcSize, VkExtent2D dstSize);
void copyImageToBuffer(VkCommandBuffer cmd, VkImage image, VkBuffer buffer, VkExtent2D extent);
} // namespace vkutil