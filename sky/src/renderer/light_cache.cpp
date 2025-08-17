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

void LightCache::upload(gfx::Device &gfxDevice, gfx::CommandBuffer cmd) 
{
	m_lightDataBuffer.uploadNewData(
		cmd,
		gfxDevice.getCurrentFrameIndex(),
		(void*)m_lightDataCPU.data(),
		sizeof(GPULightData) * m_lightDataCPU.size());
}

LightID LightCache::addLight(const Light &light, const Transform &transform) 
{
	if (light.type == LightType::Directional)
    {
        //assert(m_sunlightIndex == -1 && "directional light was already added before in the frame");
        m_sunlightIndex = (std::uint32_t)m_lightDataCPU.size();
    }

    const auto id = getFreeLightID();

    GPULightData ld{};
    ld.position = transform.getPosition();
    ld.type = light.getShaderType();
    ld.direction = transform.getForwardDirection();
    ld.range = light.range;
    if (light.range == 0)
    {
        if (light.type == LightType::Point) ld.range = m_pointLightMaxRange;
        else if (light.type == LightType::Spot) ld.range = m_spotLightMaxRange;
    }

    ld.color = LinearColorNoAlpha{light.color};
    ld.intensity = light.intensity;
    if (light.type == LightType::Directional)
    {
        //ld.intensity = 1.0; // don't have intensity for directional light yet
    }

    ld.scaleOffset.x = light.scaleOffset.x;
    ld.scaleOffset.y = light.scaleOffset.y;

    m_lightDataCPU.push_back(ld);

    return id;
}

void LightCache::updateLight(LightID id, const Light &light, const Transform &transform) 
{
    if (m_lightDataCPU.size() <= id) return;

    auto &ld = m_lightDataCPU.at(id);
	ld.position = transform.getPosition();
    ld.type = light.getShaderType();
    ld.direction = transform.getForwardDirection();
    ld.range = light.range;

	ld.color = LinearColorNoAlpha{light.color};
    ld.intensity = light.intensity;

    ld.scaleOffset = light.scaleOffset;
}

GPULightData LightCache::getLight(LightID id) 
{ 
    return GPULightData{}; 
}

LightID LightCache::getFreeLightID() 
{
    return m_lightDataCPU.size();
}
} // namespace sky