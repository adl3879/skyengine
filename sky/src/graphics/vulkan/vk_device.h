#pragma once

#include "vk_types.h"
#include "vk_initializers.h"
#include "vk_descriptors.h"
#include "vk_swapchain.h"
#include "vk_image_cache.h"
#include "vk_utils.h"
#include "vk_imgui_backend.h"
#include "vk_images.h"

#include "renderer/model_loader.h"
#include "renderer/camera/camera.h"

#include <VkBootstrap.h>

namespace sky
{
class Window;
}

namespace sky::gfx
{
static constexpr bool bUseValidationLayers = true;
static constexpr unsigned int DEPTH_ARRAY_SCALE = 512;

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

struct DescriptorSetInfo
{
    VkDescriptorSet descriptorSet;
    VkDescriptorSetLayout descriptorSetLayout;
};

class Device
{
  public:
    Device(Window &window);
    ~Device();

	FrameData &getCurrentFrame() { return m_frames[m_frameNumber % gfx::FRAME_OVERLAP]; }
    uint32_t getCurrentFrameIndex() { return m_frameNumber % gfx::FRAME_OVERLAP; }
	VkDevice getDevice() const { return m_device; }
	VmaAllocator getAllocator() { return m_allocator; }
    ImageID getWhiteTextureID() const { return m_whiteImageId; }
    ImageID getCheckerboardTextureID() const { return m_checkerboardImageId; }
	VkDescriptorSetLayout getBindlessDescSetLayout() const;
	VkDescriptorSetLayout getStrorageBufferLayout() const { return m_storageBufferLayout; }
	VkDescriptorSet getBindlessDescSet() const { return m_imageCache.bindlessSetManager.getDescSet(); }
	VkDescriptorSet getStorageBufferDescSet() const { return m_storageBufferDescriptorSet; }
	AllocatedBuffer getStorageBuffer() const { return m_storageBuffer; }
    float getMaxAnisotropy() const { return m_maxSamplerAnisotropy; }
	void resetSwapchainFences();

    auto getQueue() const { return m_graphicsQueue; }
	auto getCommandPool() const { return m_frames[m_frameNumber % gfx::FRAME_OVERLAP].commandPool; }

	CommandBuffer beginFrame();
    void endFrame(CommandBuffer cmd, const AllocatedImage &drawImage);
    void cleanup();

	AllocatedBuffer createBuffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage);
	void destroyBuffer(const AllocatedBuffer &buffer);
	void immediateSubmit(std::function<void(VkCommandBuffer cmd)> &&function);
	uint32_t getMemoryType(uint32_t typeBits, VkMemoryPropertyFlags properties, VkBool32 *memTypeFound) const;

	bool isInitialized() const { return m_isInitialized; }
	bool needsSwapchainRecreate() const { return m_swapchain.needsRecreation(); }
	void recreateSwapchain(CommandBuffer cmd, int width, int height);

	template <size_t N>
	void bindDescriptorSets(VkCommandBuffer cmd, VkPipelineLayout layout, const VkDescriptorSet (&descriptorSets)[N])
	{
		vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, N, descriptorSets, 0, nullptr);
	}
	
  public:
    ImageID createImage(const vkutil::CreateImageInfo &createInfo, void *pixelData, ImageID imageId = NULL_IMAGE_ID);
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

	void createStorageBufferDescriptor();

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
	VkPhysicalDeviceMemoryProperties m_memoryProperties;

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
    ImageID m_checkerboardImageId{NULL_IMAGE_ID};

	// mouse picker storage buffer
    AllocatedBuffer m_storageBuffer;
    VkDescriptorSetLayout m_storageBufferLayout;
    VkDescriptorSet m_storageBufferDescriptorSet;

	// temp
    VkFence m_immFence;
    VkCommandBuffer m_immCommandBuffer;
    VkCommandPool m_immCommandPool;
};
} // namespace sky