#include "vk_device.h"

#include "core/window.h"
#include "vk_images.h"
#include "vk_pipelines.h"

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

#include <imgui.h>
#include <imgui_impl_vulkan.h>
#include <imgui_impl_glfw.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>

namespace sky::gfx
{
Device::Device(Window &window) : m_window(window), m_imageCache(*this)
{
    init();
}

Device::~Device() 
{
    cleanup();
}

void Device::init() 
{
    initVulkan();

    m_swapchain.initSyncStructures(m_device);

    VkFenceCreateInfo fenceCreateInfo = vkinit::fenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);
    VK_CHECK(vkCreateFence(m_device, &fenceCreateInfo, nullptr, &m_immFence));

    m_swapchainFormat = VK_FORMAT_B8G8R8A8_SRGB;
    const auto extent = m_window.getExtent();
    m_swapchain.create(m_device, m_swapchainFormat, extent.width, extent.height, false);

    m_imageCache.bindlessSetManager.init(m_device, getMaxAnisotropy());

    initCommands();
    //initImgui();

    { // create white texture
        std::uint32_t pixel = 0xFFFFFFFF;
        m_whiteImageId = createImage(
            {
                .format = VK_FORMAT_R8G8B8A8_UNORM,
                .usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                .extent = VkExtent3D{1, 1, 1},
            }, &pixel);
    }

    ImGui::CreateContext();
    auto &io = ImGui::GetIO();
    io.ConfigWindowsMoveFromTitleBarOnly = true;
    io.Fonts->AddFontFromFileTTF("res/fonts/IBM_Plex_Sans/IBMPlexSans-Regular.ttf", 22.0f);
    m_imguiBackend.init(*this, m_swapchainFormat);
    ImGui_ImplGlfw_InitForVulkan(m_window.getGLFWwindow(), true);

    m_isInitialized = true;
}

void Device::initVulkan() 
{
    vkb::InstanceBuilder builder;

	//make the vulkan instance, with basic debug features
    m_instance = builder.set_app_name("Sky Engine")
                     .request_validation_layers(bUseValidationLayers)
                     .use_default_debug_messenger()
                     .require_api_version(1, 3, 0)
                     .build()
                     .value();

	m_debugMessenger = m_instance.debug_messenger;

    m_window.createWindowSurface(m_instance, &m_surface);
        
    const auto deviceFeatures = VkPhysicalDeviceFeatures{
        //.geometryShader = VK_TRUE, // for im3d
        .depthClamp = VK_TRUE,
        .samplerAnisotropy = VK_TRUE,
    };
    const auto features12 = VkPhysicalDeviceVulkan12Features{
        .descriptorIndexing = true,
        .descriptorBindingSampledImageUpdateAfterBind = true,
        .descriptorBindingStorageImageUpdateAfterBind = true,
        .descriptorBindingPartiallyBound = true,
        .descriptorBindingVariableDescriptorCount = true,
        .runtimeDescriptorArray = true,
        .scalarBlockLayout = true,
        .bufferDeviceAddress = true,
    };
    const auto features13 = VkPhysicalDeviceVulkan13Features{
        .synchronization2 = true,
        .dynamicRendering = true,
    };

    //use vkbootstrap to select a gpu. 
	//We want a gpu that can write to the SDL surface and supports vulkan 1.3 with the correct features
	vkb::PhysicalDeviceSelector selector{ m_instance };
    m_physicalDevice = selector.set_minimum_version(1, 3)
                               .set_required_features(deviceFeatures)
                               .set_required_features_13(features13)
                               .set_required_features_12(features12)
                               .set_surface(m_surface)
                               .select()
                               .value();

    checkDeviceCapabilities();

	m_device = vkb::DeviceBuilder{m_physicalDevice}.build().value();

	m_graphicsQueue = m_device.get_queue(vkb::QueueType::graphics).value();
	m_graphicsQueueFamily = m_device.get_queue_index(vkb::QueueType::graphics).value();

    // create a memory allocator
	VmaAllocatorCreateInfo allocatorInfo = {};
	allocatorInfo.physicalDevice = m_physicalDevice;
	allocatorInfo.device = m_device;
	allocatorInfo.instance = m_instance;
    allocatorInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT; 
	vmaCreateAllocator(&allocatorInfo, &m_allocator);
}

void Device::initCommands() 
{
    VkCommandPoolCreateInfo commandPoolInfo =
        vkinit::commandPoolCreateInfo(m_graphicsQueueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

    for (int i = 0; i < gfx::FRAME_OVERLAP; i++)
    {
        VK_CHECK(vkCreateCommandPool(m_device, &commandPoolInfo, nullptr, &m_frames[i].commandPool));

        // allocate the default command buffer that we will use for rendering
        VkCommandBufferAllocateInfo cmdAllocInfo = vkinit::commandBufferAllocateInfo(m_frames[i].commandPool, 1);
        VK_CHECK(vkAllocateCommandBuffers(m_device, &cmdAllocInfo, &m_frames[i].mainCommandBuffer));
    }

     // create a command pool for immediate commands
    VK_CHECK(vkCreateCommandPool(m_device, &commandPoolInfo, nullptr, &m_immCommandPool));

    // allocate the command buffer for immediate submits
    VkCommandBufferAllocateInfo cmdAllocInfo = vkinit::commandBufferAllocateInfo(m_immCommandPool, 1);
    VK_CHECK(vkAllocateCommandBuffers(m_device, &cmdAllocInfo, &m_immCommandBuffer));
}

void Device::recreateSwapchain(CommandBuffer cmd, int width, int height) 
{
    vkResetCommandBuffer(cmd, 0);
    m_swapchain.recreate(m_device, m_swapchainFormat, width, height, false);
}

void Device::checkDeviceCapabilities()
{
    // check limits
    VkPhysicalDeviceProperties props{};
    vkGetPhysicalDeviceProperties(m_physicalDevice, &props);

    m_maxSamplerAnisotropy = props.limits.maxSamplerAnisotropy;

    { // store which sampling counts HW supports
        const auto counts = std::array{
            VK_SAMPLE_COUNT_1_BIT,  VK_SAMPLE_COUNT_2_BIT,  VK_SAMPLE_COUNT_4_BIT,  VK_SAMPLE_COUNT_8_BIT,
            VK_SAMPLE_COUNT_16_BIT, VK_SAMPLE_COUNT_32_BIT, VK_SAMPLE_COUNT_64_BIT,
        };

        const auto supportedByDepthAndColor =
            props.limits.framebufferColorSampleCounts & props.limits.framebufferDepthSampleCounts;
        m_supportedSampleCounts = {};
        for (const auto &count : counts)
        {
            if (supportedByDepthAndColor & count)
            {
                m_supportedSampleCounts = (VkSampleCountFlagBits)(m_supportedSampleCounts | count);
                m_highestSupportedSamples = count;
            }
        }
    }
}

AllocatedBuffer Device::createBuffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage)
{
    // allocate buffer
    VkBufferCreateInfo bufferInfo = {.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    bufferInfo.pNext = nullptr;
    bufferInfo.size = allocSize;

    bufferInfo.usage = usage;

    VmaAllocationCreateInfo vmaallocInfo = {};
    vmaallocInfo.usage = memoryUsage;
    vmaallocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
   
    AllocatedBuffer newBuffer;
    // allocate the buffer
    VK_CHECK(vmaCreateBuffer(m_allocator, &bufferInfo, &vmaallocInfo, &newBuffer.buffer, &newBuffer.allocation, &newBuffer.info));
    if ((usage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT) != 0)
    {
        const auto deviceAdressInfo = VkBufferDeviceAddressInfo{
            .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
            .buffer = newBuffer.buffer,
        };
        newBuffer.address = vkGetBufferDeviceAddress(m_device, &deviceAdressInfo);
    }

    return newBuffer;
}

void Device::destroyBuffer(const AllocatedBuffer &buffer) 
{
    vmaDestroyBuffer(m_allocator, buffer.buffer, buffer.allocation);
}

void Device::immediateSubmit(std::function<void(VkCommandBuffer cmd)> &&function) 
{
    VK_CHECK(vkResetFences(m_device, 1, &m_immFence));
    VK_CHECK(vkResetCommandBuffer(m_immCommandBuffer, 0));

    VkCommandBuffer cmd = m_immCommandBuffer;

    VkCommandBufferBeginInfo cmdBeginInfo =
        vkinit::commandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

    VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

    function(cmd);

    VK_CHECK(vkEndCommandBuffer(cmd));

    VkCommandBufferSubmitInfo cmdinfo = vkinit::commandBufferSubmitInfo(cmd);
    VkSubmitInfo2 submit = vkinit::submitInfo(&cmdinfo, nullptr, nullptr);

    // submit command buffer to the queue and execute it.
    //  _renderFence will now block until the graphic commands finish execution
    VK_CHECK(vkQueueSubmit2(m_graphicsQueue, 1, &submit, m_immFence));

    VK_CHECK(vkWaitForFences(m_device, 1, &m_immFence, true, 9999999999));
}

ImageID Device::createImage(const vkutil::CreateImageInfo& createInfo, void* pixelData, ImageID imageId)
{
    auto image = createImageRaw(createInfo);
    if (pixelData)
    {
        uploadImageData(image, pixelData);
    }
    if (imageId != NULL_IMAGE_ID)
    {
        return m_imageCache.addImage(imageId, std::move(image));
    }
    return m_imageCache.addImage(std::move(image));
}

AllocatedImage Device::createImageRaw(const vkutil::CreateImageInfo &createInfo) const 
{ 
    std::uint32_t mipLevels = 1;
    if (createInfo.mipMap)
    {
        const auto maxExtent = std::max(createInfo.extent.width, createInfo.extent.height);
        mipLevels = (std::uint32_t)std::floor(std::log2(maxExtent)) + 1;
    }

    if (createInfo.isCubemap)
    {
        assert(createInfo.numLayers == 6);
        assert(!createInfo.mipMap);
        assert((createInfo.flags & VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT) != 0);
    }

    const auto imgInfo = VkImageCreateInfo{
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .flags = createInfo.flags,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = createInfo.format,
        .extent = createInfo.extent,
        .mipLevels = mipLevels,
        .arrayLayers = createInfo.numLayers,
        .samples = createInfo.samples,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = createInfo.usage,
    };
    const auto allocInfo = VmaAllocationCreateInfo{
        .usage = VMA_MEMORY_USAGE_AUTO,
        .requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
    };

    AllocatedImage image{};
    image.imageFormat = createInfo.format;
    image.imageExtent = createInfo.extent;
    /*image.usage = createInfo.usage;
    image.mipLevels = mipLevels;
    image.numLayers = createInfo.numLayers;
    image.isCubemap = createInfo.isCubemap;*/

    VK_CHECK(vmaCreateImage(m_allocator, &imgInfo, &allocInfo, &image.image, &image.allocation, nullptr));

    // create view
    VkImageAspectFlags aspectFlag = VK_IMAGE_ASPECT_COLOR_BIT;
    if (createInfo.format == VK_FORMAT_D32_SFLOAT)
    { // TODO: support other depth formats
        aspectFlag = VK_IMAGE_ASPECT_DEPTH_BIT;
    }

    auto viewType = createInfo.numLayers == 1 ? VK_IMAGE_VIEW_TYPE_2D : VK_IMAGE_VIEW_TYPE_2D_ARRAY;
    if (createInfo.isCubemap)
    {
        viewType = VK_IMAGE_VIEW_TYPE_CUBE;
    }

    const auto viewCreateInfo = VkImageViewCreateInfo{
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = image.image,
        .viewType = viewType,
        .format = createInfo.format,
        .subresourceRange =
            VkImageSubresourceRange{
                .aspectMask = aspectFlag,
                .baseMipLevel = 0,
                .levelCount = mipLevels,
                .baseArrayLayer = 0,
                .layerCount = createInfo.numLayers,
            },
    };

    VK_CHECK(vkCreateImageView(m_device, &viewCreateInfo, nullptr, &image.imageView));

    return image;
}

ImageID Device::createImage(const vkutil::CreateImageInfo &createInfo) 
{
    auto image = createImageRaw(createInfo);
    return m_imageCache.addImage(std::move(image));
}

ImageID Device::createDrawImage(VkFormat format, glm::ivec2 size) 
{
    const auto extent = VkExtent3D{
        .width = (std::uint32_t)size.x,
        .height = (std::uint32_t)size.y,
        .depth = 1,
    };

    VkImageUsageFlags usages{};
    usages |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    usages |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    usages |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    usages |= VK_IMAGE_USAGE_SAMPLED_BIT;

    const auto createImageInfo = vkutil::CreateImageInfo{
        .format = format,
        .usage = usages,
        .extent = extent,
    };

    return createImage(createImageInfo);
}

void Device::uploadImageData(const AllocatedImage &image, void *pixelData, std::uint32_t layer)
{
    int numChannels = 4;
    if (image.imageFormat == VK_FORMAT_R8_UNORM)
    {
        // FIXME: support more types
        numChannels = 1;
    }
    const auto dataSize = image.imageExtent.depth * image.imageExtent.width * image.imageExtent.height * numChannels;

    const auto uploadBuffer = createBuffer(dataSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_AUTO);
    memcpy(uploadBuffer.info.pMappedData, pixelData, dataSize);

    immediateSubmit(
        [&](VkCommandBuffer cmd)
        {
            /*assert((image.usage & VK_IMAGE_USAGE_TRANSFER_DST_BIT) != 0 &&
                   "Image needs to have VK_IMAGE_USAGE_TRANSFER_DST_BIT to upload data to it");*/
            vkutil::transitionImage(cmd, image.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

            const auto copyRegion = VkBufferImageCopy{
                .bufferOffset = 0,
                .bufferRowLength = 0,
                .bufferImageHeight = 0,
                .imageSubresource =
                    {
                        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                        .mipLevel = 0,
                        .baseArrayLayer = layer,
                        .layerCount = 1,
                    },
                .imageExtent = image.imageExtent,
            };

            vkCmdCopyBufferToImage(cmd, uploadBuffer.buffer, image.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
                                   &copyRegion);

            /*if (image.mipLevels > 1)
            {
                assert((image.usage & VK_IMAGE_USAGE_TRANSFER_DST_BIT) != 0 &&
                       (image.usage & VK_IMAGE_USAGE_TRANSFER_SRC_BIT) != 0 &&
                       "Image needs to have VK_IMAGE_USAGE_TRANSFER_{DST,SRC}_BIT to generate mip maps");
                graphics::generateMipmaps(cmd, image.image, VkExtent2D{image.extent.width, image.extent.height},
                                          image.mipLevels);
            }
            else*/
            {
                vkutil::transitionImage(cmd, image.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
            }
        });

    destroyBuffer(uploadBuffer);
}

AllocatedImage Device::getImage(ImageID id) 
{
    return m_imageCache.getImage(id);
}

void Device::destroyImage(const AllocatedImage &image) const 
{
    vkDestroyImageView(m_device, image.imageView, nullptr);
	vmaDestroyImage(m_allocator, image.image, image.allocation);
}

void Device::cleanup()
{
	if (m_isInitialized) 
	{
        vkDeviceWaitIdle(m_device);

        m_imageCache.bindlessSetManager.cleanup(m_device);
        m_imageCache.destroyImages();
        
        for (auto &frame : m_frames)
        {
            vkDestroyCommandPool(m_device, frame.commandPool, 0);
        }
        m_imguiBackend.cleanup(*this);
        m_swapchain.cleanup(m_device);

        vkDestroyFence(m_device, m_immFence, nullptr);
        vkDestroyCommandPool(m_device, m_immCommandPool, nullptr);

		vkb::destroy_surface(m_instance, m_surface);
        //vmaDestroyAllocator(m_allocator);
        vkb::destroy_device(m_device);
        vkb::destroy_instance(m_instance);
	}
}

CommandBuffer Device::beginFrame() 
{
    m_swapchain.beginFrame(m_device, getCurrentFrameIndex());
    
    const auto &frame = getCurrentFrame();
    const auto &cmd = frame.mainCommandBuffer;
    const auto cmdBeginInfo = VkCommandBufferBeginInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };
    VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

    return CommandBuffer{cmd};
}

void Device::endFrame(CommandBuffer cmd, const AllocatedImage &drawImage) 
{
    // get swapchain image
    const auto [swapchainImage, swapchainImageIndex] = m_swapchain.acquireImage(m_device, getCurrentFrameIndex());
    if (swapchainImage == VK_NULL_HANDLE)
    {
        return;
    }

    // Fences are reset here to prevent the deadlock in case swapchain becomes dirty
    m_swapchain.resetFences(m_device, getCurrentFrameIndex());

    auto swapchainLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    {
        // clear swapchain image
        VkImageSubresourceRange clearRange = vkinit::imageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT);
        vkutil::transitionImage(cmd, swapchainImage, swapchainLayout, VK_IMAGE_LAYOUT_GENERAL);
        swapchainLayout = VK_IMAGE_LAYOUT_GENERAL;

        const auto clearValue = VkClearColorValue{{1.f, 1.f, 1.f, 1.f}};
        vkCmdClearColorImage(cmd, swapchainImage, VK_IMAGE_LAYOUT_GENERAL, &clearValue, 1, &clearRange);
    }

    {
        // copy from draw image into swapchain
        vkutil::transitionImage(cmd, drawImage.image, VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
        vkutil::transitionImage(cmd, swapchainImage, swapchainLayout, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        swapchainLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

        // will stretch image to swapchain
        auto imageExtent = VkExtent2D{drawImage.imageExtent.width, drawImage.imageExtent.height};
        vkutil::copyImageToImage(cmd, drawImage.image, swapchainImage, imageExtent, m_swapchain.getExtent());
    }

    {
        vkutil::transitionImage(cmd, swapchainImage, swapchainLayout, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        swapchainLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        m_imguiBackend.draw(cmd, *this, m_swapchain.getImageView(swapchainImageIndex), m_swapchain.getExtent());
    }

    // prepare for present
    vkutil::transitionImage(cmd, swapchainImage, swapchainLayout, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
    swapchainLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    {
        // TODO: don't collect every frame?
        auto &frame = getCurrentFrame();
    }

    VK_CHECK(vkEndCommandBuffer(cmd));

    m_swapchain.submitAndPresent(cmd, m_graphicsQueue, getCurrentFrameIndex(), swapchainImageIndex);

    m_frameNumber++;
}

VkDescriptorSetLayout Device::getBindlessDescSetLayout() const
{
    return m_imageCache.bindlessSetManager.getDescSetLayout();
}

void Device::bindBindlessDescSet(VkCommandBuffer cmd, VkPipelineLayout layout)
{
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, 1,
                            &m_imageCache.bindlessSetManager.getDescSet(), 0, nullptr);
}
} // namespace sky