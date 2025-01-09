#pragma once

#include <skypch.h>

#include "graphics/vulkan/vk_device.h"

namespace sky
{
class SkyAtmosphere
{
  public:
    void init(gfx::Device &device);
    void initSky(gfx::Device &device, VkFormat format);

    void draw(
        gfx::Device &device, 
        gfx::CommandBuffer cmd, 
        VkExtent2D extent,
        Camera &camera);
    void drawSky(
        gfx::Device &device, 
        gfx::CommandBuffer cmd, 
        VkExtent2D extent);
    void cleanup(const gfx::Device &device);

  private:

    void initTransmittanceLUT(gfx::Device &device);
    void updateTransmittanceLUT(gfx::Device &device, gfx::CommandBuffer cmd);
    
    void initMultiScatteringLUT(gfx::Device &device);
    void updateMultiScatteringLUT(gfx::Device &device, gfx::CommandBuffer cmd);

    void initSkyLUT(gfx::Device &device);
    void updateSkyLUT(gfx::Device &device, gfx::CommandBuffer cmd);

    VkSampler createSampler(gfx::Device &device);

  private:
    // sky
    gfx::PipelineInfo m_skyPipelineInfo;
    gfx::DescriptorSetInfo m_skyDescInfo;

    // common uniform buffer
    gfx::AllocatedBuffer m_viewParamsBuffer;

    // transmittance lut
    gfx::PipelineInfo m_transmittanceLUTPipelineInfo;
    gfx::AllocatedBuffer m_transmittanceLUTBuffer;
    gfx::DescriptorSetInfo m_transmittanceLUTDescInfo; 
    gfx::AllocatedImage m_transmittanceLUTImage;
    glm::vec2 m_transmittanceLUTRes = {256, 64};

    // multi scattering lut
    gfx::PipelineInfo m_multiScatteringLUTPipelineInfo;
    gfx::AllocatedBuffer m_multiScatteringLUTBuffer;
    gfx::DescriptorSetInfo m_multiScatteringLUTDescInfo;
    gfx::AllocatedImage m_multiScatteringLUTImage;
    glm::vec2 m_multiScatteringLUTRes = {32, 32};

    // sky lut
    gfx::PipelineInfo m_skyLUTPipelineInfo;
    gfx::AllocatedBuffer m_skyLUTBuffer;
    gfx::DescriptorSetInfo m_skyLUTDescInfo;
    gfx::AllocatedImage m_skyLUTImage;
    glm::vec2 m_skyLUTRes = {200.f, 200.f};

    struct ViewParams
    {
        glm::vec3   viewPos;
        float       time;
        glm::vec2   resolution;
        glm::vec2   tLUTRes;
        glm::vec2   msLUTRes;
        glm::vec2   skyLUTRes;
        glm::vec4   cameraDir;
    };

    struct PushConstants
    {
        VkDeviceAddress sceneDataBuffer;
    };
};
} // namespace sky