#pragma once

#include <skypch.h>

#include <vulkan/vulkan.h>
#include <VkBootstrap.h>
#include <vulkan/vulkan_core.h>

namespace sky::gfx
{
constexpr unsigned int FRAME_OVERLAP = 2;

class Swapchain
{
  public:
    void initSyncStructures(VkDevice device);
    void create(const vkb::Device &device, VkFormat format, std::uint32_t width, std::uint32_t height, bool vSync);
    void recreate(const vkb::Device &device, VkFormat format, std::uint32_t width, std::uint32_t height, bool vSync);
    void cleanup(VkDevice device);

    VkExtent2D getExtent() const { return m_extent; }

    const std::vector<VkImage> &getImages() { return m_images; };

    void beginFrame(VkDevice device, std::size_t frameIndex) const;
    void resetFences(VkDevice device, std::size_t frameIndex) const;

    // returns the image and its index
    std::pair<VkImage, std::uint32_t> acquireImage(VkDevice device, std::size_t frameIndex);
    void submit(VkCommandBuffer cmd, VkQueue graphicsQueue, std::size_t frameIndex, 
        VkSemaphore waitSemaphore = VK_NULL_HANDLE, VkSemaphore signalSemaphore = VK_NULL_HANDLE);
    void present(VkQueue graphicsQueue, std::size_t frameIndex, std::uint32_t swapchainImageIndex,
        VkSemaphore waitSemaphore = VK_NULL_HANDLE);
    void submitAndPresent(VkCommandBuffer cmd, VkQueue graphicsQueue, std::size_t frameIndex, std::uint32_t swapchainImageIndex);
    VkImageView getImageView(std::size_t swapchainImageIndex) { return m_imageViews[swapchainImageIndex]; }

    bool needsRecreation() const { return m_dirty; }
  
  private:
    struct FrameData
    {
        VkSemaphore swapchainSemaphore;
        VkSemaphore renderSemaphore;
        VkFence renderFence;
    };

    std::array<FrameData, FRAME_OVERLAP> m_frames;
    vkb::Swapchain m_swapchain;
    VkExtent2D m_extent;
    std::vector<VkImage> m_images;
    std::vector<VkImageView> m_imageViews;
    bool m_dirty{false};
};
} // namespace sky::gfx