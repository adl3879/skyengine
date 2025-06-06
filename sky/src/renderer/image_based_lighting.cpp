#include "image_based_lighting.h"
#include "asset_management/texture_cube_importer.h"
#include "core/helpers/image.h"
#include "graphics/vulkan/vk_types.h"

namespace sky 
{
void ImageBasedLighting::init(gfx::Device &device)
{
    // auto path = "C:\\Users\\Toyosi Adekanmbi\\Desktop\\TYGame\\assets\\textures\\skybox\\skybox.hdr";
    // auto hdrTex = TextureCubeImporter::loadTexture(path);
    // m_hdrImageId = helper::loadImageFromTexture(hdrTex, VK_FORMAT_R32G32B32A32_SFLOAT);
    
    m_equirectangularToCubemapPass.init(device, VK_FORMAT_R16G16B16A16_SFLOAT, {512, 512});
    m_irradiancePass.init(device, VK_FORMAT_R16G16B16A16_SFLOAT, 32);
    m_prefilterEnvmapPass.init(device, VK_FORMAT_R16G16B16A16_SFLOAT, 1024, 5);
    m_brdfLutPass.init(device, VK_FORMAT_R16G16B16A16_SFLOAT, 512);
    m_skyboxPass.init(device, VK_FORMAT_R16G16B16A16_SFLOAT);
}

void ImageBasedLighting::draw(gfx::Device &device, gfx::CommandBuffer cmd, const gfx::AllocatedBuffer &sceneDataBuffer) {
    if (m_hdrImageId == NULL_IMAGE_ID)  return;
    if (m_dirty) 
    {
        m_dirty = false;
        m_equirectangularToCubemapPass.draw(device, cmd, m_hdrImageId, {512, 512});
        m_irradiancePass.draw(device, cmd, m_equirectangularToCubemapPass.getCubemapId());
        m_prefilterEnvmapPass.draw(device, cmd, m_equirectangularToCubemapPass.getCubemapId());
        m_brdfLutPass.draw(device, cmd);
    }
}

void ImageBasedLighting::drawSky(gfx::Device &device, 
    gfx::CommandBuffer cmd, 
    VkExtent2D extent,
    const gfx::AllocatedBuffer &sceneDataBuffer)
{
    if (m_hdrImageId == NULL_IMAGE_ID)  return;
    m_skyboxPass.draw(device, 
        cmd, 
        extent,
        m_equirectangularToCubemapPass.getCubemapId(), 
        sceneDataBuffer);
}

void ImageBasedLighting::cleanup(gfx::Device &device)
{
    m_equirectangularToCubemapPass.cleanup(device);
    m_irradiancePass.cleanup(device);
    m_prefilterEnvmapPass.cleanup(device);
    m_skyboxPass.cleanup(device);
}
}