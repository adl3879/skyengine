#pragma once

#include "vk_types.h"
#include "vk_initializers.h"
#include "vk_descriptors.h"
#include "vk_swapchain.h"
#include "vk_image_cache.h"
#include "vk_utils.h"
#include "vk_imgui_backend.h"

#include "renderer/model_loader.h"
#include "renderer/camera/camera.h"

#include <VkBootstrap.h>

namespace sky
{
class Window;
}

namespace sky::gfx
{
static constexpr bool bUseValidationLayers = false;

struct DeletionQueue
{
    std::deque<std::function<void()>> deletors;

    void push_function(std::function<void()> &&function) { deletors.push_back(function); }

    void flush()
    {
        // reverse iterate the deletion queue to execute all the functions
        for (auto it = deletors.rbegin(); it != deletors.rend(); it++)
        {
            (*it)(); // call functors
        }
        deletors.clear();
    }
};

struct FrameData
{
	VkCommandPool commandPool;
	VkCommandBuffer mainCommandBuffer;
    VkSemaphore swapchainSemaphore, renderSemaphore;
    VkFence renderFence;

	DeletionQueue deletionQueue;
	DescriptorAllocatorGrowable frameDescriptors;
};

struct CommandBuffer
{
	VkCommandBuffer handle;

	operator VkCommandBuffer() const { return handle; }
};

struct PipelineInfo
{
	VkPipeline pipeline;
	VkPipelineLayout pipelineLayout;
};

class Device
{
  public:
    Device(Window &window);
    ~Device();

	FrameData &getCurrentFrame() { return m_frames[m_frameNumber % gfx::FRAME_OVERLAP]; }
    uint32_t getCurrentFrameIndex() { return m_frameNumber % gfx::FRAME_OVERLAP; }
	VkDevice getDevice() const const { return m_device; }
    ImageID getWhiteTextureID() const { return m_whiteImageId; }
	VkDescriptorSetLayout getBindlessDescSetLayout() const;
    float getMaxAnisotropy() const { return m_maxSamplerAnisotropy; }

	CommandBuffer beginFrame();
    void endFrame(CommandBuffer cmd, const AllocatedImage &drawImage);
    void cleanup();

	AllocatedBuffer createBuffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage);
	void destroyBuffer(const AllocatedBuffer &buffer);
	void immediateSubmit(std::function<void(VkCommandBuffer cmd)> &&function);

	bool isInitialized() const { return m_isInitialized; }
	bool needsSwapchainRecreate() const { return m_swapchain.needsRecreation(); }
	void recreateSwapchain(CommandBuffer cmd, int width, int height);
    void bindBindlessDescSet(VkCommandBuffer cmd, VkPipelineLayout layout);
	
  public:
    ImageID createImage(const vkutil::CreateImageInfo &createInfo,
        void *pixelData, 
		ImageID imageId = NULL_IMAGE_ID);
    void uploadImageData(const AllocatedImage &image, void *pixelData, std::uint32_t layer = 0);
	AllocatedImage createImageRaw(const vkutil::CreateImageInfo& createInfo) const;
	ImageID createImage(const vkutil::CreateImageInfo& createInfo);
	ImageID createDrawImage(VkFormat format, glm::ivec2 size);
	AllocatedImage getImage(ImageID id);
	void destroyImage(const AllocatedImage &image) const;

  private:
    void init();
    void initVulkan();
	void initCommands();
    void checkDeviceCapabilities();

  private:
	FrameData m_frames[gfx::FRAME_OVERLAP];
	uint32_t m_frameNumber = 0;	
	VkQueue m_graphicsQueue;
	uint32_t m_graphicsQueueFamily;

  private:
    vkb::Instance m_instance;
	vkb::PhysicalDevice m_physicalDevice;
	vkb::Device m_device;
	VmaAllocator m_allocator;

	VkDebugUtilsMessengerEXT m_debugMessenger;// Vulkan debug output handle
	VkSurfaceKHR m_surface;// Vulkan window surface

    gfx::Swapchain m_swapchain;
	VkFormat m_swapchainFormat;

	VkSampleCountFlagBits m_supportedSampleCounts;
    VkSampleCountFlagBits m_highestSupportedSamples{VK_SAMPLE_COUNT_1_BIT};
    float m_maxSamplerAnisotropy{1.f};

    bool m_isInitialized = false;
    Window &m_window;

	ImGuiBackend m_imguiBackend;

	gfx::ImageCache m_imageCache;
    ImageID m_whiteImageId{NULL_IMAGE_ID};

	// temp
    VkFence m_immFence;
    VkCommandBuffer m_immCommandBuffer;
    VkCommandPool m_immCommandPool;
};
} // namespace sky