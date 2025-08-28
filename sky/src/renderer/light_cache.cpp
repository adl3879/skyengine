#include "light_cache.h"

namespace sky
{
void LightCache::init(gfx::Device &gfxDevice) 
{
    m_lightDataBuffer.init(
        gfxDevice,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        sizeof(GPULightData) * MAX_LIGHTS,
        gfx::FRAME_OVERLAP,
        "light data");
    m_lightDataCPU.reserve(MAX_LIGHTS);
}

void LightCache::updateAndUpload(gfx::Device &gfxDevice, gfx::CommandBuffer cmd, 
    const std::vector<std::pair<Light, Transform>>& lights) 
{
    m_lightDataCPU.clear();
    m_sunlightIndex = UINT32_MAX;
    
    // Add all lights
    for (const auto& [light, transform] : lights) {
        if (m_lightDataCPU.size() >= MAX_LIGHTS) break;
        
        if (light.type == LightType::Directional) {
            m_sunlightIndex = static_cast<std::uint32_t>(m_lightDataCPU.size());
        }
        
        GPULightData ld{};
        ld.position = transform.getPosition();
        ld.type = light.getShaderType();
        ld.direction = transform.getForwardDirection();
        ld.range = light.range;
        ld.color = LinearColorNoAlpha{light.color};
        ld.intensity = light.intensity;
        ld.scaleOffset = light.scaleOffset;
        
        m_lightDataCPU.push_back(ld);
    }
    
    // Upload to GPU
    if (!m_lightDataCPU.empty()) 
    {
        m_lightDataBuffer.uploadNewData(
            cmd,
            gfxDevice.getCurrentFrameIndex(),
            (void*)m_lightDataCPU.data(),
            sizeof(GPULightData) * m_lightDataCPU.size());
    }
}
} // namespace sky