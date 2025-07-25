#include "vk_bindless_set.h"

namespace
{
static const std::uint32_t maxBindlessResources = 16536;
static const std::uint32_t maxSamplers = 32;

static const std::uint32_t texturesBinding = 0;
static const std::uint32_t samplersBinding = 1;
} // namespace

namespace sky::gfx
{
void BindlessSetManager::init(VkDevice device, float maxAnisotropy)
{
    { // create pool
        const auto poolSizesBindless = std::array<VkDescriptorPoolSize, 2>{{
            {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, maxBindlessResources},
            {VK_DESCRIPTOR_TYPE_SAMPLER, maxSamplers},
        }};

        const auto poolInfo = VkDescriptorPoolCreateInfo{
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
            .flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT_EXT,
            .maxSets = maxBindlessResources * poolSizesBindless.size(),
            .poolSizeCount = (std::uint32_t)poolSizesBindless.size(),
            .pPoolSizes = poolSizesBindless.data(),
        };

        VK_CHECK(vkCreateDescriptorPool(device, &poolInfo, nullptr, &descPool));
    }

    { // build desc set layout
        const auto bindings = std::array<VkDescriptorSetLayoutBinding, 2>{{
            {
                .binding = texturesBinding,
                .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
                .descriptorCount = maxBindlessResources,
                .stageFlags = VK_SHADER_STAGE_ALL,
            },
            {
                .binding = samplersBinding,
                .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER,
                .descriptorCount = maxSamplers,
                .stageFlags = VK_SHADER_STAGE_ALL,
            },
        }};

        const VkDescriptorBindingFlags bindlessFlags = 
            VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT;
        const auto bindingFlags = std::array{bindlessFlags, bindlessFlags};

        const auto flagInfo = VkDescriptorSetLayoutBindingFlagsCreateInfo{
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO,
            .bindingCount = (std::uint32_t)bindingFlags.size(),
            .pBindingFlags = bindingFlags.data(),
        };

        const auto info = VkDescriptorSetLayoutCreateInfo{
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .pNext = &flagInfo,
            .flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT_EXT,
            .bindingCount = (std::uint32_t)bindings.size(),
            .pBindings = bindings.data(),
        };

        VK_CHECK(vkCreateDescriptorSetLayout(device, &info, nullptr, &descSetLayout));
    }

    { // alloc desc set
        const auto allocInfo = VkDescriptorSetAllocateInfo{
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .descriptorPool = descPool,
            .descriptorSetCount = 1,
            .pSetLayouts = &descSetLayout,
        };

        std::uint32_t maxBinding = maxBindlessResources - 1;
        const auto countInfo = VkDescriptorSetVariableDescriptorCountAllocateInfo{
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO,
            .descriptorSetCount = 1,
            .pDescriptorCounts = &maxBinding,
        };

        VK_CHECK(vkAllocateDescriptorSets(device, &allocInfo, &descSet));
    }

    initDefaultSamplers(device, maxAnisotropy);
}

void BindlessSetManager::initDefaultSamplers(VkDevice device, float maxAnisotropy)
{
    // Keep in sync with bindless.glsl
    static const std::uint32_t nearestSamplerId = 0;
    static const std::uint32_t linearSamplerId = 1;
    static const std::uint32_t shadowSamplerId = 2;

    { // init nearest sampler
        const auto samplerCreateInfo = VkSamplerCreateInfo{
            .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
            .magFilter = VK_FILTER_NEAREST,
            .minFilter = VK_FILTER_NEAREST,
        };
        VK_CHECK(vkCreateSampler(device, &samplerCreateInfo, nullptr, &nearestSampler));
        //vkutil::addDebugLabel(device, nearestSampler, "nearest");
        addSampler(device, nearestSamplerId, nearestSampler);
    }

    { // init linear sampler
        const auto samplerCreateInfo = VkSamplerCreateInfo{
            .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
            .magFilter = VK_FILTER_LINEAR,
            .minFilter = VK_FILTER_LINEAR,
            .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
            // TODO: remove these and create separate sampler for clamp-to-edge
            .addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
            .addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
            .addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
            // TODO: make possible to disable anisotropy or set other values?
            .anisotropyEnable = VK_TRUE,
            .maxAnisotropy = maxAnisotropy,
        };

        VK_CHECK(vkCreateSampler(device, &samplerCreateInfo, nullptr, &linearSampler));
        //vkutil::addDebugLabel(device, linearSampler, "linear");
        addSampler(device, linearSamplerId, linearSampler);
    }

    { // init shadow map sampler
        const auto samplerCreateInfo = VkSamplerCreateInfo{
            .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
            .magFilter = VK_FILTER_LINEAR,
            .minFilter = VK_FILTER_LINEAR,
            .compareEnable = VK_TRUE,
            .compareOp = VK_COMPARE_OP_GREATER_OR_EQUAL,
        };
        VK_CHECK(vkCreateSampler(device, &samplerCreateInfo, nullptr, &shadowMapSampler));
        //vkutil::addDebugLabel(device, shadowMapSampler, "shadow");
        addSampler(device, shadowSamplerId, shadowMapSampler);
    }
}

void BindlessSetManager::cleanup(VkDevice device)
{
    vkDestroySampler(device, nearestSampler, nullptr);
    vkDestroySampler(device, linearSampler, nullptr);
    vkDestroySampler(device, shadowMapSampler, nullptr);

    vkDestroyDescriptorSetLayout(device, descSetLayout, nullptr);
    vkDestroyDescriptorPool(device, descPool, nullptr);
}

void BindlessSetManager::addImage(const VkDevice device, std::uint32_t id, const VkImageView imageView)
{
    const auto imageInfo =
        VkDescriptorImageInfo{.imageView = imageView, .imageLayout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL};
    const auto writeSet = VkWriteDescriptorSet{
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = descSet,
        .dstBinding = texturesBinding,
        .dstArrayElement = id,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
        .pImageInfo = &imageInfo,
    };
    vkUpdateDescriptorSets(device, 1, &writeSet, 0, nullptr);
}

void BindlessSetManager::addSampler(const VkDevice device, std::uint32_t id, VkSampler sampler)
{
    const auto imageInfo = VkDescriptorImageInfo{.sampler = sampler, .imageLayout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL};
    const auto writeSet = VkWriteDescriptorSet{
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = descSet,
        .dstBinding = samplersBinding,
        .dstArrayElement = id,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER,
        .pImageInfo = &imageInfo,
    };
    vkUpdateDescriptorSets(device, 1, &writeSet, 0, nullptr);
}
}