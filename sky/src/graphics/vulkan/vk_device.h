#pragma once

#include "vk_types.h"
#include "vk_initializers.h"
#include "vk_descriptors.h"
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
constexpr unsigned int MAX_FRAMES_IN_FLIGHT = 2;

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

	FrameData &getCurrentFrame() { return m_frames[m_frameNumber % MAX_FRAMES_IN_FLIGHT]; }

	VkDevice getDevice() const const { return m_device; }
	VkPhysicalDevice getPhysicalDevice() const const  { return m_chosenGPU; }
	VkInstance getInstance() const { return m_instance; }
	VkSurfaceKHR getSurface() const { return m_surface; }
	VkQueue getGraphicsQueue() const { return m_graphicsQueue; }
	uint32_t getGraphicsQueueFamily() const { return m_graphicsQueueFamily; }
	VkExtent2D getSwapchainExtent() const { return m_swapchainExtent; }
	VkFormat getSwapchainImageFormat() const { return m_swapchainImageFormat; }
	bool getResizeRequested() const { return resizeRequested; }
    Camera &getCamera() { return m_camera; }
    VkExtent2D getDrawImageExtent() const { return m_drawExtent; } 

    VkDescriptorSetLayout getGpuSceneDescriptorLayout() const { return m_gpuSceneDescriptorLayout; }

	CommandBuffer beginFrame();
	void draw();
	void drawImgui(VkCommandBuffer cmd, VkImageView target);

	bool isInitialized() const { return m_isInitialized; }
    void resizeSwapchain();

	AllocatedImage createImage(VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped = false);
    AllocatedImage createImage(void *data, VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped = false);
    void destroyImage(const AllocatedImage &img);
	AllocatedBuffer createBuffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage);
	void destroyBuffer(const AllocatedBuffer &buffer);
	void immediateSubmit(std::function<void(VkCommandBuffer cmd)> &&function);

  private:
    void init();
    void initVulkan();
	void initSwapchain();
	void initCommands();
	void initSyncStructures();
	void initDescriptors();
	void initImgui();
    void cleanup();

	void createSwapchain(uint32_t width, uint32_t height);
	void destroySwapchain();

  private:
	FrameData m_frames[MAX_FRAMES_IN_FLIGHT];
	uint32_t m_frameNumber = 0;	
	VkQueue m_graphicsQueue;
	uint32_t m_graphicsQueueFamily;

	// draw resources
    AllocatedImage m_drawImage;
	AllocatedImage m_depthImage;
	float renderScale = 1.0f;
    VkExtent2D m_drawExtent;

	DescriptorAllocator globalDescriptorAllocator;

	GPUSceneData m_sceneData;
	VkDescriptorSetLayout m_gpuSceneDescriptorLayout;

  private:
    VkInstance m_instance;// Vulkan library handle
	VkDebugUtilsMessengerEXT m_debugMessenger;// Vulkan debug output handle
	VkPhysicalDevice m_chosenGPU;// GPU chosen as the default device
	VkDevice m_device; // Vulkan device for commands
	VkSurfaceKHR m_surface;// Vulkan window surface

    VkSwapchainKHR m_swapchain;
	VkFormat m_swapchainImageFormat;

	std::vector<VkImage> m_swapchainImages;
	std::vector<VkImageView> m_swapchainImageViews;
	VkExtent2D m_swapchainExtent;

    bool m_isInitialized = false;
	bool resizeRequested = false;
    Window &m_window;

	DeletionQueue m_mainDeletionQueue;
	VmaAllocator m_allocator;

	// imgui
    VkFence m_immFence;
    VkCommandBuffer m_immCommandBuffer;
    VkCommandPool m_immCommandPool;

	Camera m_camera;
};
} // namespace sky