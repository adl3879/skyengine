#pragma once

#include <memory>
#include <optional>
#include <string>
#include <vector>
#include <span>
#include <array>
#include <functional>
#include <deque>

#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>

#include <vk_mem_alloc.h>

#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

#include "core/math/sphere.h"

#define VK_CHECK(x)                                                         \
    do {                                                                    \
        VkResult err = x;                                                   \
        if (err) {                                                          \
             fmt::print("Detected Vulkan error: {}", string_VkResult(err)); \
            abort();                                                        \
        }                                                                   \
    } while (0)

namespace sky
{
using ImageID = uint32_t;
static const auto NULL_IMAGE_ID = std::numeric_limits<std::uint32_t>::max();

namespace gfx
{
struct AllocatedImage
{
    VkImage image;
    VkImageView imageView;
    VmaAllocation allocation;
    VkExtent3D imageExtent;
    VkFormat imageFormat;

    VkExtent2D getExtent2D() const { return VkExtent2D{imageExtent.width, imageExtent.height}; }
};

struct AllocatedBuffer
{
    VkBuffer buffer;
    VmaAllocation allocation;
    VmaAllocationInfo info;

    // Only for buffers created with VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
    // TODO: add check that this is not 0 if requesting address?
    VkDeviceAddress address{0};
};

// holds the resources needed for a mesh
struct GPUMeshBuffers
{
    AllocatedBuffer indexBuffer;
    AllocatedBuffer vertexBuffer;

    uint32_t numIndices{0};
    uint32_t materialId{0};
    math::Sphere boundingSphere;
};

// push constants for our mesh object draws
struct GPUDrawPushConstants
{
    glm::mat4 worldMatrix;
    VkDeviceAddress vertexBuffer;
};

struct GPUSceneData
{
    glm::mat4 view;
    glm::mat4 proj;
    glm::mat4 viewproj;
    glm::vec4 ambientColor;
    glm::vec4 sunlightDirection; // w for sun power
    glm::vec4 sunlightColor;
};
} // namespace gfx
} // namespace sky