#include "vk_utils.h"

namespace sky::gfx::vkutil
{
RenderInfo createRenderingInfo(const RenderingInfoParams &params)
{
    assert(params.renderExtent.width != 0.f && params.renderExtent.height != 0.f && "renderExtent not specified");

    RenderInfo ri;
    if (params.colorImageView)
    {
        ri.colorAttachment = VkRenderingAttachmentInfo{
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .imageView = params.colorImageView,
            .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .loadOp = params.colorImageClearValue ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        };
        if (params.colorImageClearValue)
        {
            const auto col = params.colorImageClearValue.value();
            ri.colorAttachment.clearValue.color = {col[0], col[1], col[2], col[3]};
        }
    }

    if (params.depthImageView)
    {
        ri.depthAttachment = VkRenderingAttachmentInfo{
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .imageView = params.depthImageView,
            .imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
            .loadOp = params.depthImageClearValue ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        };
        if (params.depthImageClearValue)
        {
            ri.depthAttachment.clearValue.depthStencil.depth = params.depthImageClearValue.value();
        }
    }

    if (params.resolveImageView)
    {
        ri.colorAttachment.resolveImageView = params.resolveImageView;
        ri.colorAttachment.resolveImageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        ri.colorAttachment.resolveMode = VK_RESOLVE_MODE_AVERAGE_BIT;
    }

    ri.renderingInfo = VkRenderingInfo{
        .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
        .renderArea =
            VkRect2D{
                .offset = {},
                .extent = params.renderExtent,
            },
        .layerCount = 1,
        .colorAttachmentCount = params.colorImageView ? 1u : 0u,
        .pColorAttachments = params.colorImageView ? &ri.colorAttachment : nullptr,
        .pDepthAttachment = params.depthImageView ? &ri.depthAttachment : nullptr,
    };

    return ri;
}

void clearColorImage(VkCommandBuffer cmd, VkExtent2D colorImageExtent, VkImageView colorImageView,
                     const glm::vec4 &clearColor)
{
    auto ri = createRenderingInfo(
        {.renderExtent = colorImageExtent, .colorImageView = colorImageView, .colorImageClearValue = clearColor});
    vkCmdBeginRendering(cmd, &ri.renderingInfo);
    vkCmdEndRendering(cmd);
}

int sampleCountToInt(VkSampleCountFlagBits count)
{
    switch (count)
    {
        case VK_SAMPLE_COUNT_1_BIT: return 1;
        case VK_SAMPLE_COUNT_2_BIT: return 2;
        case VK_SAMPLE_COUNT_4_BIT: return 4;
        case VK_SAMPLE_COUNT_8_BIT: return 8;
        case VK_SAMPLE_COUNT_16_BIT: return 16;
        case VK_SAMPLE_COUNT_32_BIT: return 32;
        case VK_SAMPLE_COUNT_64_BIT: return 64;
        default: return 0;
    }
}

const char *sampleCountToString(VkSampleCountFlagBits count)
{
    switch (count)
    {
        case VK_SAMPLE_COUNT_1_BIT: return "Off";
        case VK_SAMPLE_COUNT_2_BIT: return "2x";
        case VK_SAMPLE_COUNT_4_BIT: return "4x";
        case VK_SAMPLE_COUNT_8_BIT: return "8x";
        case VK_SAMPLE_COUNT_16_BIT: return "16x";
        case VK_SAMPLE_COUNT_32_BIT: return "32x";
        case VK_SAMPLE_COUNT_64_BIT: return "64x";
        default: return "Unknown";
    }
}
} // namespace sky::vkutils