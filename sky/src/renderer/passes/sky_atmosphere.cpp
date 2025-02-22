#include "sky_atmosphere.h"

#include "graphics/vulkan/vk_pipelines.h"
#include "graphics/vulkan/vk_device.h"

namespace sky
{
void SkyAtmospherePass::init(gfx::Device &device) 
{
    initTransmittanceLUT(device);
    initMultiScatteringLUT(device);
    initSkyLUT(device);
    initSky(device, VK_FORMAT_R16G16B16A16_SFLOAT);
}

void SkyAtmospherePass::initSky(gfx::Device &device, VkFormat format) 
{
    VkDescriptorPool descPool;
    { // create pool
        const auto poolSizesBindless = std::array<VkDescriptorPoolSize, 3>{{
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1},  // view params 
            {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,  1},  // transmittance lut
            {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,  1},  // multi scattering lut
        }};

        const auto poolInfo = VkDescriptorPoolCreateInfo{
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
            .flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT_EXT,
            .maxSets = 3,
            .poolSizeCount = (std::uint32_t)poolSizesBindless.size(),
            .pPoolSizes = poolSizesBindless.data(),
        };

        VK_CHECK(vkCreateDescriptorPool(device.getDevice(), &poolInfo, nullptr, &descPool));
    }
    { // build desc set layout
        const auto bindings = std::array<VkDescriptorSetLayoutBinding, 3>{{
            {
                .binding = 0,
                .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .descriptorCount = 1,
                .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
            },
            {
                .binding = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .descriptorCount = 1,
                .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
            },
            {
                .binding = 2,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .descriptorCount = 1,
                .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
            },
        }};
        const auto info = VkDescriptorSetLayoutCreateInfo{
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT_EXT,
            .bindingCount = (std::uint32_t)bindings.size(),
            .pBindings = bindings.data(),
        };
        VK_CHECK(vkCreateDescriptorSetLayout(device.getDevice(), &info, nullptr,
			 &m_skyDescInfo.descriptorSetLayout));
    }

    const auto vertexShader = gfx::vkutil::loadShaderModule("shaders/sky.vert.spv", device.getDevice());
    const auto fragShader = gfx::vkutil::loadShaderModule("shaders/sky.frag.spv", device.getDevice());

    const auto bufferRange = VkPushConstantRange{
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
        .offset = 0,
        .size = sizeof(PushConstants),
    };
    
    const auto pushConstantRanges = std::array<VkPushConstantRange, 0>{};
    const auto layouts = std::array<VkDescriptorSetLayout, 1>{
        m_skyDescInfo.descriptorSetLayout,
    };
    m_skyPipelineInfo.pipelineLayout = gfx::vkutil::createPipelineLayout(device.getDevice(), layouts, pushConstantRanges);

    m_skyPipelineInfo.pipeline = gfx::PipelineBuilder{m_skyPipelineInfo.pipelineLayout}
                           .setShaders(vertexShader, fragShader)
                           .setInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP)
                           .setPolygonMode(VK_POLYGON_MODE_FILL)
                           .setCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE)
                           .setMultisamplingNone()
                           .enableBlending()
                           .disableDepthTest()
                           .setColorAttachmentFormat(format)
                           .build(device.getDevice());

    {
	    const auto allocInfo = VkDescriptorSetAllocateInfo{
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .descriptorPool = descPool,
            .descriptorSetCount = 1,
            .pSetLayouts = &m_skyDescInfo.descriptorSetLayout,
        };

        vkAllocateDescriptorSets(device.getDevice(), &allocInfo, &m_skyDescInfo.descriptorSet);

        VkDescriptorBufferInfo bufferInfo = {};
        bufferInfo.buffer = m_viewParamsBuffer.buffer;
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(SkyAtmospherePass::ViewParams);

        VkSampler transmittanceLUTSampler = createSampler(device); 
        VkDescriptorImageInfo transmittanceLUTImageInfo = {};
        transmittanceLUTImageInfo.imageView = m_transmittanceLUTImage.imageView;
        transmittanceLUTImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        transmittanceLUTImageInfo.sampler = transmittanceLUTSampler;

        auto skyLUTSampler = createSampler(device);
        VkDescriptorImageInfo skyLUTImageInfo = {};
        skyLUTImageInfo.imageView = m_skyLUTImage.imageView;
        skyLUTImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        skyLUTImageInfo.sampler = skyLUTSampler;

        const auto descriptorWrite =
            std::array<VkWriteDescriptorSet, 3>{{
		        {
				    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
					.dstSet = m_skyDescInfo.descriptorSet,
					.dstBinding = 0,
					.descriptorCount = 1,
					.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
					.pBufferInfo = &bufferInfo,
				},
				{
					.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
					.dstSet = m_skyDescInfo.descriptorSet,
					.dstBinding = 1,
					.descriptorCount = 1,
					.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
					.pImageInfo = &transmittanceLUTImageInfo,
				},
				{
					.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
					.dstSet = m_skyDescInfo.descriptorSet,
					.dstBinding = 2,
					.descriptorCount = 1,
					.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
					.pImageInfo = &skyLUTImageInfo,
				}
            }};

		vkUpdateDescriptorSets(device.getDevice(), static_cast<uint32_t>(descriptorWrite.size()),
		   descriptorWrite.data(), 0, nullptr);
    }
}

void SkyAtmospherePass::drawSky(
        gfx::Device &device, 
        gfx::CommandBuffer cmd, 
        VkExtent2D extent)
{
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_skyPipelineInfo.pipeline);

    VkDescriptorSet descriptorSets[] = {
        m_skyDescInfo.descriptorSet,
    };
    device.bindDescriptorSets(cmd, m_skyPipelineInfo.pipelineLayout, descriptorSets);

    // set dynamic viewport and scissor
    const auto viewport = VkViewport{
        .x = 0.f,
        .y = 0.f,
        .width = (float)extent.width,
        .height = (float)extent.height,
        .minDepth = 0.f,
        .maxDepth = 1.f,
    };
    vkCmdSetViewport(cmd, 0, 1, &viewport);
    const auto scissor = VkRect2D{
        .offset = {0, 0},
        .extent = extent,
    };
    vkCmdSetScissor(cmd, 0, 1, &scissor);

    vkCmdDraw(cmd, 6, 1, 0, 0);
}

void SkyAtmospherePass::draw(gfx::Device &device, 
	gfx::CommandBuffer cmd, 
	VkExtent2D extent,
    Camera &camera,
    const SkyAtmosphere &sky) 
{
    static const float EARTH_RADIUS = 6.360f;
    const glm::vec3 viewPos = glm::vec3(0, EARTH_RADIUS + (0.0002), 0);
    auto cm = camera.getCameraDir();

    const auto viewParams = ViewParams{
        .viewPos = viewPos,
        .time = sky.time,
        .resolution = glm::vec2{extent.width, extent.height},
        .tLUTRes = m_transmittanceLUTRes,
        .msLUTRes = m_multiScatteringLUTRes,
        .skyLUTRes = m_multiScatteringLUTRes,
        .cameraDir = {cm.x, cm.y, -glm::abs(cm.z) , 0.f},
        .groundAlbedo = sky.groundAlbedo,
        .mieScatteringBase = sky.mieScatteringBase,
        .rayleighScatteringBase = sky.rayleighScatteringBase,
        .mieAbsorptionBase = sky.mieAbsorptionBase,
	    .ozoneAbsorptionBase = sky.ozoneAbsorptionBase,
        .exposure = sky.exposure,
        .rayleighAbsorptionBase = sky.rayleighAbsorptionBase,
    };

    // Map memory
    void *data;
    vmaMapMemory(device.getAllocator(), m_viewParamsBuffer.allocation, &data);
    std::memcpy(data, &viewParams, sizeof(ViewParams));
    vmaUnmapMemory(device.getAllocator(), m_viewParamsBuffer.allocation);

    gfx::vkutil::transitionImage(cmd, m_transmittanceLUTImage.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

    updateTransmittanceLUT(device, cmd);

    gfx::vkutil::transitionImage(cmd, m_transmittanceLUTImage.image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    gfx::vkutil::transitionImage(cmd, m_multiScatteringLUTImage.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

    updateMultiScatteringLUT(device, cmd);

    gfx::vkutil::transitionImage(cmd, m_multiScatteringLUTImage.image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    gfx::vkutil::transitionImage(cmd, m_skyLUTImage.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

    updateSkyLUT(device, cmd);

    gfx::vkutil::transitionImage(cmd, m_skyLUTImage.image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}

void SkyAtmospherePass::cleanup(const gfx::Device &device) 
{
}

void SkyAtmospherePass::initTransmittanceLUT(gfx::Device &device) 
{
    VkDescriptorPool descPool;
    { // create pool
        const auto poolSizesBindless = std::array<VkDescriptorPoolSize, 2>{{
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1}, 
            {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1},
        }};

        const auto poolInfo = VkDescriptorPoolCreateInfo{
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
            .flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT_EXT,
            .maxSets = 2,
            .poolSizeCount = (std::uint32_t)poolSizesBindless.size(),
            .pPoolSizes = poolSizesBindless.data(),
        };

        VK_CHECK(vkCreateDescriptorPool(device.getDevice(), &poolInfo, nullptr, &descPool));
    }

    { // build desc set layout
        const auto bindings = std::array<VkDescriptorSetLayoutBinding, 2>{{
            {
                .binding = 0,
                .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .descriptorCount = 1,
                .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
            },
            {
                .binding = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                .descriptorCount = 1,
                .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
            },
        }};

        const auto info = VkDescriptorSetLayoutCreateInfo{
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT_EXT,
            .bindingCount = (std::uint32_t)bindings.size(),
            .pBindings = bindings.data(),
        };

        VK_CHECK(vkCreateDescriptorSetLayout(device.getDevice(), &info, nullptr, 
            &m_transmittanceLUTDescInfo.descriptorSetLayout));
    }

    { // create pipeline
        VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 1;
        pipelineLayoutInfo.pSetLayouts = &m_transmittanceLUTDescInfo.descriptorSetLayout;

        vkCreatePipelineLayout(device.getDevice(), &pipelineLayoutInfo, nullptr,
                               &m_transmittanceLUTPipelineInfo.pipelineLayout);

        auto computeShaderModule =
            gfx::vkutil::loadShaderModule("shaders/transmittance_lut.comp.spv", device.getDevice());

        VkPipelineShaderStageCreateInfo shaderStageInfo = {};
        shaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
        shaderStageInfo.module = computeShaderModule;
        shaderStageInfo.pName = "main"; // Entry point of the shader

        VkComputePipelineCreateInfo pipelineInfo = {};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
        pipelineInfo.stage = shaderStageInfo;
        pipelineInfo.layout = m_transmittanceLUTPipelineInfo.pipelineLayout;

        vkCreateComputePipelines(device.getDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr,
                                 &m_transmittanceLUTPipelineInfo.pipeline);
    }

    {
        unsigned int width = m_transmittanceLUTRes.x;
        unsigned int height = m_transmittanceLUTRes.y;

        m_viewParamsBuffer = device.createBuffer(sizeof(SkyAtmospherePass::ViewParams), 
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

        m_transmittanceLUTImage = device.createImageRaw({
            .format = VK_FORMAT_R32G32B32A32_SFLOAT,
            .usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            .extent = VkExtent3D{width, height, 1},
        });
       
        VkDescriptorSetAllocateInfo allocInfo2 = {};
        allocInfo2.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo2.descriptorPool = descPool; // Ensure a descriptor pool exists
        allocInfo2.descriptorSetCount = 1;
        allocInfo2.pSetLayouts = &m_transmittanceLUTDescInfo.descriptorSetLayout;

        vkAllocateDescriptorSets(device.getDevice(), &allocInfo2, &m_transmittanceLUTDescInfo.descriptorSet);

        VkDescriptorBufferInfo bufferInfo = {};
        bufferInfo.buffer = m_viewParamsBuffer.buffer;
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(SkyAtmospherePass::ViewParams);

        VkDescriptorImageInfo imageInfo = {};
        imageInfo.imageView = m_transmittanceLUTImage.imageView;
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

        const auto descriptorWrite = std::array<VkWriteDescriptorSet, 2>{{
            {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = m_transmittanceLUTDescInfo.descriptorSet,
                .dstBinding = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .pBufferInfo = &bufferInfo,
            },
            {
		        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.dstSet = m_transmittanceLUTDescInfo.descriptorSet,
				.dstBinding = 1,
				.descriptorCount = 1,
				.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
				.pImageInfo = &imageInfo,
            }
        }};

        // Correct the number of descriptor writes to 2
        vkUpdateDescriptorSets(device.getDevice(), static_cast<uint32_t>(descriptorWrite.size()),
		   descriptorWrite.data(), 0, nullptr);
    }
}

void SkyAtmospherePass::updateTransmittanceLUT(gfx::Device &device, gfx::CommandBuffer cmd)
{
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_transmittanceLUTPipelineInfo.pipeline);
    
    vkCmdBindDescriptorSets(cmd, 
        VK_PIPELINE_BIND_POINT_COMPUTE, 
        m_transmittanceLUTPipelineInfo.pipelineLayout, 
        0, 
        1,
        &m_transmittanceLUTDescInfo.descriptorSet, 
        0, 
        nullptr);

    vkCmdDispatch(cmd, m_transmittanceLUTRes.x, m_transmittanceLUTRes.y, 1);
}

void SkyAtmospherePass::initMultiScatteringLUT(gfx::Device &device) 
{
	VkDescriptorPool descPool;
    { // create pool
        const auto poolSizesBindless = std::array<VkDescriptorPoolSize, 3>{{
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1}, // view params 
            {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1},  // transmittance lut
            {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1},  // multi scattering lut out
        }};

        const auto poolInfo = VkDescriptorPoolCreateInfo{
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
            .flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT_EXT,
            .maxSets = 3,
            .poolSizeCount = (std::uint32_t)poolSizesBindless.size(),
            .pPoolSizes = poolSizesBindless.data(),
        };

        VK_CHECK(vkCreateDescriptorPool(device.getDevice(), &poolInfo, nullptr, &descPool));
    }

    { // build desc set layout
        const auto bindings = std::array<VkDescriptorSetLayoutBinding, 3>{{
            {
                .binding = 0,
                .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .descriptorCount = 1,
                .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
            },
            {
                .binding = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .descriptorCount = 1,
                .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
            },
            {
                .binding = 2,
                .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                .descriptorCount = 1,
                .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
            },
        }};
        const auto info = VkDescriptorSetLayoutCreateInfo{
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT_EXT,
            .bindingCount = (std::uint32_t)bindings.size(),
            .pBindings = bindings.data(),
        };
        VK_CHECK(vkCreateDescriptorSetLayout(device.getDevice(), &info, nullptr,
			 &m_multiScatteringLUTDescInfo.descriptorSetLayout));
    }
    { // create pipeline
        VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 1;
        pipelineLayoutInfo.pSetLayouts = &m_multiScatteringLUTDescInfo.descriptorSetLayout;
        vkCreatePipelineLayout(device.getDevice(), &pipelineLayoutInfo, nullptr,
		   &m_multiScatteringLUTPipelineInfo.pipelineLayout);

        auto computeShaderModule =
            gfx::vkutil::loadShaderModule("shaders/multiscatter_lut.comp.spv", device.getDevice());
        
        VkPipelineShaderStageCreateInfo shaderStageInfo = {};
        shaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
        shaderStageInfo.module = computeShaderModule;
        shaderStageInfo.pName = "main"; // Entry point of the shader

        VkComputePipelineCreateInfo pipelineInfo = {};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
        pipelineInfo.stage = shaderStageInfo;
        pipelineInfo.layout = m_multiScatteringLUTPipelineInfo.pipelineLayout;

        vkCreateComputePipelines(device.getDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr,
			&m_multiScatteringLUTPipelineInfo.pipeline);
    }
    {
        unsigned int width = m_multiScatteringLUTRes.x;
        unsigned int height = m_multiScatteringLUTRes.y;

        m_multiScatteringLUTImage = device.createImageRaw({
            .format = VK_FORMAT_R32G32B32A32_SFLOAT,
            .usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            .extent = VkExtent3D{width, height, 1},
        });

        const auto allocInfo = VkDescriptorSetAllocateInfo{
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .descriptorPool = descPool,
            .descriptorSetCount = 1,
            .pSetLayouts = &m_multiScatteringLUTDescInfo.descriptorSetLayout,
        };

        vkAllocateDescriptorSets(device.getDevice(), &allocInfo, &m_multiScatteringLUTDescInfo.descriptorSet);

        VkDescriptorBufferInfo bufferInfo = {};
        bufferInfo.buffer = m_viewParamsBuffer.buffer;
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(SkyAtmospherePass::ViewParams);

        VkSampler transmittanceLUTSampler = createSampler(device); 
        VkDescriptorImageInfo transmittanceLUTImageInfo = {};
        transmittanceLUTImageInfo.imageView = m_transmittanceLUTImage.imageView;
        transmittanceLUTImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        transmittanceLUTImageInfo.sampler = transmittanceLUTSampler;

        VkDescriptorImageInfo multiScatteringLUTImageInfo = {};
        multiScatteringLUTImageInfo.imageView = m_multiScatteringLUTImage.imageView;
        multiScatteringLUTImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

        const auto descriptorWrite =
            std::array<VkWriteDescriptorSet, 3>{{
		        {
				    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
					.dstSet = m_multiScatteringLUTDescInfo.descriptorSet,
					.dstBinding = 0,
					.descriptorCount = 1,
					.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
					.pBufferInfo = &bufferInfo,
				},
				{
					.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
					.dstSet = m_multiScatteringLUTDescInfo.descriptorSet,
					.dstBinding = 1,
					.descriptorCount = 1,
					.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
					.pImageInfo = &transmittanceLUTImageInfo,
				},
				{
					.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
					.dstSet = m_multiScatteringLUTDescInfo.descriptorSet,
					.dstBinding = 2,
					.descriptorCount = 1,
					.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
					.pImageInfo = &multiScatteringLUTImageInfo,
				}
            }};

			vkUpdateDescriptorSets(device.getDevice(), static_cast<uint32_t>(descriptorWrite.size()),
			   descriptorWrite.data(), 0, nullptr);
    }
}

void SkyAtmospherePass::updateMultiScatteringLUT(gfx::Device &device, gfx::CommandBuffer cmd) 
{
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_multiScatteringLUTPipelineInfo.pipeline);
    
    vkCmdBindDescriptorSets(cmd, 
        VK_PIPELINE_BIND_POINT_COMPUTE, 
        m_multiScatteringLUTPipelineInfo.pipelineLayout, 
        0, 
        1,
        &m_multiScatteringLUTDescInfo.descriptorSet, 
        0, 
        nullptr);

    vkCmdDispatch(cmd, m_multiScatteringLUTRes.x, m_multiScatteringLUTRes.y, 1);
}

void SkyAtmospherePass::initSkyLUT(gfx::Device &device) 
{
	VkDescriptorPool descPool;
    { // create pool
        const auto poolSizesBindless = std::array<VkDescriptorPoolSize, 4>{{
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1}, // view params 
            {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,  1},  // transmittance lut
            {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,  1},  // multi scattering lut
            {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,  1},  // sky lut out
        }};

        const auto poolInfo = VkDescriptorPoolCreateInfo{
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
            .flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT_EXT,
            .maxSets = 4,
            .poolSizeCount = (std::uint32_t)poolSizesBindless.size(),
            .pPoolSizes = poolSizesBindless.data(),
        };

        VK_CHECK(vkCreateDescriptorPool(device.getDevice(), &poolInfo, nullptr, &descPool));
    }

    { // build desc set layout
        const auto bindings = std::array<VkDescriptorSetLayoutBinding, 4>{{
            {
                .binding = 0,
                .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .descriptorCount = 1,
                .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
            },
            {
                .binding = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .descriptorCount = 1,
                .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
            },
            {
                .binding = 2,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .descriptorCount = 1,
                .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
            },
		    {
                .binding = 3,
                .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                .descriptorCount = 1,
                .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
            },
        }};
        const auto info = VkDescriptorSetLayoutCreateInfo{
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT_EXT,
            .bindingCount = (std::uint32_t)bindings.size(),
            .pBindings = bindings.data(),
        };
        VK_CHECK(vkCreateDescriptorSetLayout(device.getDevice(), &info, nullptr,
			 &m_skyLUTDescInfo.descriptorSetLayout));
    }
    { // create pipeline
        VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 1;
        pipelineLayoutInfo.pSetLayouts = &m_skyLUTDescInfo.descriptorSetLayout;
        vkCreatePipelineLayout(device.getDevice(), &pipelineLayoutInfo, nullptr,
		   &m_skyLUTPipelineInfo.pipelineLayout);

        auto computeShaderModule =
            gfx::vkutil::loadShaderModule("shaders/sky_lut.comp.spv", device.getDevice());
        
        VkPipelineShaderStageCreateInfo shaderStageInfo = {};
        shaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
        shaderStageInfo.module = computeShaderModule;
        shaderStageInfo.pName = "main"; // Entry point of the shader

        VkComputePipelineCreateInfo pipelineInfo = {};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
        pipelineInfo.stage = shaderStageInfo;
        pipelineInfo.layout = m_skyLUTPipelineInfo.pipelineLayout;

        vkCreateComputePipelines(device.getDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr,
			&m_skyLUTPipelineInfo.pipeline);
    }
    {
        unsigned int width = m_skyLUTRes.x;
        unsigned int height = m_skyLUTRes.y;

        m_skyLUTImage = device.createImageRaw({
            .format = VK_FORMAT_R32G32B32A32_SFLOAT,
            .usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            .extent = VkExtent3D{width, height, 1},
        });

        const auto allocInfo = VkDescriptorSetAllocateInfo{
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .descriptorPool = descPool,
            .descriptorSetCount = 1,
            .pSetLayouts = &m_skyLUTDescInfo.descriptorSetLayout,
        };

        vkAllocateDescriptorSets(device.getDevice(), &allocInfo, &m_skyLUTDescInfo.descriptorSet);

        VkDescriptorBufferInfo bufferInfo = {};
        bufferInfo.buffer = m_viewParamsBuffer.buffer;
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(SkyAtmospherePass::ViewParams);

        VkSampler transmittanceLUTSampler = createSampler(device); 
        VkDescriptorImageInfo transmittanceLUTImageInfo = {};
        transmittanceLUTImageInfo.imageView = m_transmittanceLUTImage.imageView;
        transmittanceLUTImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        transmittanceLUTImageInfo.sampler = transmittanceLUTSampler;

        VkSampler multiScatteringLUTSampler = createSampler(device);
        VkDescriptorImageInfo multiScatteringLUTImageInfo = {};
        multiScatteringLUTImageInfo.imageView = m_multiScatteringLUTImage.imageView;
        multiScatteringLUTImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        multiScatteringLUTImageInfo.sampler = multiScatteringLUTSampler;

        VkDescriptorImageInfo skyLUTImageInfo = {};
        skyLUTImageInfo.imageView = m_skyLUTImage.imageView;
        skyLUTImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

        const auto descriptorWrite =
            std::array<VkWriteDescriptorSet, 4>{{
		        {
				    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
					.dstSet = m_skyLUTDescInfo.descriptorSet,
					.dstBinding = 0,
					.descriptorCount = 1,
					.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
					.pBufferInfo = &bufferInfo,
				},
				{
					.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
					.dstSet = m_skyLUTDescInfo.descriptorSet,
					.dstBinding = 1,
					.descriptorCount = 1,
					.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
					.pImageInfo = &transmittanceLUTImageInfo,
				},
				{
					.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
					.dstSet = m_skyLUTDescInfo.descriptorSet,
					.dstBinding = 2,
					.descriptorCount = 1,
					.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
					.pImageInfo = &multiScatteringLUTImageInfo,
				},
				{
					.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
					.dstSet = m_skyLUTDescInfo.descriptorSet,
					.dstBinding = 3,
					.descriptorCount = 1,
					.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
					.pImageInfo = &skyLUTImageInfo,
				}
            }};

			vkUpdateDescriptorSets(device.getDevice(), static_cast<uint32_t>(descriptorWrite.size()),
			   descriptorWrite.data(), 0, nullptr);
    }
}

void SkyAtmospherePass::updateSkyLUT(gfx::Device &device, gfx::CommandBuffer cmd) 
{
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_skyLUTPipelineInfo.pipeline);

    vkCmdBindDescriptorSets(cmd, 
        VK_PIPELINE_BIND_POINT_COMPUTE, 
        m_skyLUTPipelineInfo.pipelineLayout, 
        0, 
        1,
        &m_skyLUTDescInfo.descriptorSet, 
        0, 
        nullptr);

    vkCmdDispatch(cmd, m_skyLUTRes.x, m_skyLUTRes.y, 1);
}

VkSampler SkyAtmospherePass::createSampler(gfx::Device &device) 
{
	VkSamplerCreateInfo samplerCreateInfo{};
    samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerCreateInfo.magFilter = VK_FILTER_LINEAR;                         // Magnification filter
	samplerCreateInfo.minFilter = VK_FILTER_LINEAR;                         // Minification filter
	samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;           // Mipmap mode
	samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE; // U coordinate addressing
	samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE; // V coordinate addressing
	samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;     // W coordinate addressing (3D textures only, ignored for 2D)
	samplerCreateInfo.mipLodBias = 0.0f;           // Optional LOD bias
	samplerCreateInfo.anisotropyEnable = VK_FALSE; // Disable anisotropic filtering
	samplerCreateInfo.maxAnisotropy = 1.0f;        // Not used if anisotropy is disabled
	samplerCreateInfo.compareEnable = VK_FALSE;    // No comparison for shadow mapping
	samplerCreateInfo.compareOp = VK_COMPARE_OP_ALWAYS;        // Comparison operation (irrelevant if compareEnable is VK_FALSE)
	samplerCreateInfo.minLod = 0.0f; // Minimum LOD
	samplerCreateInfo.maxLod = 1.0f; // Maximum LOD (adjust based on your texture mipmap levels)
	samplerCreateInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK; // Border color for clamp-to-border mode
	samplerCreateInfo.unnormalizedCoordinates = VK_FALSE;             // Use normalized texture coordinates

	VkSampler transmittanceLUTSampler;
	vkCreateSampler(device.getDevice(), &samplerCreateInfo, nullptr, &transmittanceLUTSampler);
	return transmittanceLUTSampler;
}
}// namespace sky