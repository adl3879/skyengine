#include "vk_swapchain.h"

#include "vk_types.h"
#include "vk_initializers.h"

namespace
{
static constexpr auto NO_TIMEOUT = std::numeric_limits<std::uint64_t>::max();
}

namespace sky::gfx
{
void Swapchain::initSyncStructures(VkDevice device)
{
    const auto fenceCreateInfo = VkFenceCreateInfo{
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT,
    };
    const auto semaphoreCreateInfo = VkSemaphoreCreateInfo{
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
    };
    for (std::uint32_t i = 0; i < FRAME_OVERLAP; ++i)
    {
        VK_CHECK(vkCreateFence(device, &fenceCreateInfo, nullptr, &m_frames[i].renderFence));
        VK_CHECK(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &m_frames[i].swapchainSemaphore));
        VK_CHECK(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &m_frames[i].renderSemaphore));
    }
}

void Swapchain::create(const vkb::Device &device, VkFormat swapchainFormat, std::uint32_t width, std::uint32_t height,
                       bool vSync)
{
    assert(swapchainFormat == VK_FORMAT_B8G8R8A8_SRGB && "TODO: test other formats");
    // vSync = false;

    auto res = vkb::SwapchainBuilder{device}
                   .set_desired_format(VkSurfaceFormatKHR{
                       .format = swapchainFormat,
                       .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
                   })
                   .add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
                   .set_desired_present_mode(vSync ? VK_PRESENT_MODE_FIFO_KHR : VK_PRESENT_MODE_IMMEDIATE_KHR)
                   .set_desired_extent(width, height)
                   .build();
    if (!res.has_value())
    {
        SKY_CORE_ERROR("failed to create swapchain: error = {}, vk result = {}", res.full_error().type.message(),
                       string_VkResult(res.full_error().vk_result));
    }
    m_swapchain = res.value();
    m_extent = VkExtent2D{.width = width, .height = height};

    m_images = m_swapchain.get_images().value();
    m_imageViews = m_swapchain.get_image_views().value();

    // TODO: if re-creation of swapchain is supported, don't forget to call
    // vkutil::initSwapchainViews here.
}

void Swapchain::recreate(const vkb::Device &device, VkFormat swapchainFormat, std::uint32_t width, std::uint32_t height,
                         bool vSync)
{
    vkDeviceWaitIdle(device);

    vkb::destroy_swapchain(m_swapchain);
    create(device, swapchainFormat, width, height, vSync);
    m_dirty = false;
}

void Swapchain::cleanup(VkDevice device)
{
    for (auto &frame : m_frames)
    {
        vkDestroyFence(device, frame.renderFence, nullptr);
        vkDestroySemaphore(device, frame.swapchainSemaphore, nullptr);
        vkDestroySemaphore(device, frame.renderSemaphore, nullptr);
    }

    { // destroy swapchain and its views
        for (auto imageView : m_imageViews)
        {
            vkDestroyImageView(device, imageView, nullptr);
        }
        m_imageViews.clear();

        vkb::destroy_swapchain(m_swapchain);
    }
}

void Swapchain::beginFrame(VkDevice device, std::size_t frameIndex) const
{
    auto &frame = m_frames[frameIndex];
    VK_CHECK(vkWaitForFences(device, 1, &frame.renderFence, true, NO_TIMEOUT));
}

void Swapchain::resetFences(VkDevice device, std::size_t frameIndex) const
{
    auto &frame = m_frames[frameIndex];
    VK_CHECK(vkResetFences(device, 1, &frame.renderFence));
}

std::pair<VkImage, std::uint32_t> Swapchain::acquireImage(VkDevice device, std::size_t frameIndex)
{
    std::uint32_t swapchainImageIndex{};
    const auto result = vkAcquireNextImageKHR(device, m_swapchain, NO_TIMEOUT, m_frames[frameIndex].swapchainSemaphore,
                                              VK_NULL_HANDLE, &swapchainImageIndex);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
    {
        m_dirty = true;
        return {VK_NULL_HANDLE, 0};
    }
    else if (result != VK_SUCCESS)
    {
        SKY_CORE_ERROR("failed to acquire swap chain image!");
    }

    return {m_images[swapchainImageIndex], swapchainImageIndex};
}

// Add this new method to submit a command buffer without presenting
void Swapchain::submit(VkCommandBuffer cmd, VkQueue graphicsQueue, std::size_t frameIndex, 
    VkSemaphore waitSemaphore, VkSemaphore signalSemaphore)
{
    // Create submit info
    const auto submitInfo = VkCommandBufferSubmitInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
        .commandBuffer = cmd,
    };
    
    // Use the provided wait semaphore
    const auto waitInfo =
        vkinit::semaphoreSubmitInfo(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR, waitSemaphore);
    
    // Use the provided signal semaphore
    const auto signalInfo =
        vkinit::semaphoreSubmitInfo(VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, signalSemaphore);

    const auto submit = vkinit::submitInfo(&submitInfo, &waitInfo, &signalInfo);
    VK_CHECK(vkQueueSubmit2(graphicsQueue, 1, &submit, m_frames[frameIndex].renderFence));
}

// Add this new method to present a swapchain image
void Swapchain::present(VkQueue graphicsQueue, std::size_t frameIndex, std::uint32_t swapchainImageIndex,
    VkSemaphore waitSemaphore)
{
    // Create present info
    const auto presentInfo = VkPresentInfoKHR{
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &waitSemaphore,
        .swapchainCount = 1,
        .pSwapchains = &m_swapchain.swapchain,
        .pImageIndices = &swapchainImageIndex,
    };

    auto res = vkQueuePresentKHR(graphicsQueue, &presentInfo);
    if (res != VK_SUCCESS)
    {
        SKY_CORE_ERROR("failed to present: {}", string_VkResult(res));
        m_dirty = true;
    }
}

// Modify the existing submitAndPresent method to use the new methods
void Swapchain::submitAndPresent(VkCommandBuffer cmd, VkQueue graphicsQueue, std::size_t frameIndex,
                                 std::uint32_t swapchainImageIndex)
{
    const auto &frame = m_frames[frameIndex];

    // Submit the command buffer
    submit(cmd, graphicsQueue, frameIndex, frame.swapchainSemaphore, frame.renderSemaphore);
    
    // Present the swapchain image
    present(graphicsQueue, frameIndex, swapchainImageIndex, frame.renderSemaphore);
}
} // namespace sky::gfx