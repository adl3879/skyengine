#pragma once

#include <skypch.h>

#include "vk_types.h"

namespace sky::gfx::vkutil
{
struct RenderingInfoParams
{
    VkExtent2D renderExtent;
    VkImageView colorImageView{VK_NULL_HANDLE};
    std::optional<glm::vec4> colorImageClearValue;
    VkImageView depthImageView{VK_NULL_HANDLE};
    std::optional<float> depthImageClearValue;
    VkImageView resolveImageView{VK_NULL_HANDLE};
};

struct RenderInfo
{
    VkRenderingAttachmentInfo colorAttachment;
    VkRenderingAttachmentInfo depthAttachment;
    VkRenderingInfo renderingInfo;
};

RenderInfo createRenderingInfo(const RenderingInfoParams &params);

void clearColorImage(VkCommandBuffer cmd, VkExtent2D colorImageExtent, VkImageView colorImageView,
                     const glm::vec4 &clearColor);

int sampleCountToInt(VkSampleCountFlagBits count);
const char *sampleCountToString(VkSampleCountFlagBits count);
} // namespace sky::vkutils