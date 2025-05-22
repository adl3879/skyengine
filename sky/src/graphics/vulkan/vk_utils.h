#pragma once

#include <skypch.h>

#include "vk_initializers.h"

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

struct CreateImageInfo
{
    VkFormat format;
    VkImageUsageFlags usage;
    VkImageCreateFlags flags;
    VkExtent3D extent{};
    std::uint32_t numLayers{1};
    VkSampleCountFlagBits samples{VK_SAMPLE_COUNT_1_BIT};
    bool mipMap{false};
    bool isCubemap{false};
};

RenderInfo createRenderingInfo(const RenderingInfoParams &params);

void clearColorImage(VkCommandBuffer cmd, VkExtent2D colorImageExtent, VkImageView colorImageView,
    const glm::vec4 &clearColor);

int sampleCountToInt(VkSampleCountFlagBits count);

const char *sampleCountToString(VkSampleCountFlagBits count);

/**
    * Transitions a color image and depth image to shader readable layout
    * @param cmd Command buffer to record the transition into
    * @param colorImage The color image to transition
    * @param depthImage The depth image to transition
    */
inline void transitionImagesToShaderReadable(
    VkCommandBuffer cmd,
    VkImage colorImage,
    VkImage depthImage)
{
    const auto imageBarrier = VkImageMemoryBarrier2{
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
        .srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
        .dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        .image = colorImage,
        .subresourceRange = gfx::vkinit::imageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT),
    };
    const auto depthBarrier = VkImageMemoryBarrier2{
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .srcStageMask = VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
        .srcAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
        .dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
        .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        .image = depthImage,
        .subresourceRange = gfx::vkinit::imageSubresourceRange(VK_IMAGE_ASPECT_DEPTH_BIT),
    };
    const auto barriers = std::array{imageBarrier, depthBarrier};
    const auto dependencyInfo = VkDependencyInfo{
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .imageMemoryBarrierCount = barriers.size(),
        .pImageMemoryBarriers = barriers.data(),
    };
    vkCmdPipelineBarrier2(cmd, &dependencyInfo);
}
}