#pragma once

#include <skypch.h>
#include <glm/vec2.hpp>

#include <vulkan/vulkan.h>
#include <imgui.h>

#include "vk_NBuffer.h"
#include "vk_types.h"

namespace sky::gfx
{
class Device;

/*
 * This is a custom Dear ImGui rendering backend which does the following:
 * 1. Implements sRGB framebuffer support via various hacks outlined in vert.frag
 * 2. Doesn't use descriptor sets- the bindless texture IDs can be passed to ImGui::Image
 * 3. Properly displays _SRGB and other linear space textures
 *
 * Caveat: maximum idx/vtx count of 1 million - I'm too lazy to implement dynamic
 * idx/vtx buffer resizing.
 */
class ImGuiBackend
{
  public:
    void init(Device &gfxDevice, VkFormat swapchainFormat);
    void draw(VkCommandBuffer cmd, Device &gfxDevice, VkImageView swapchainImageView, VkExtent2D swapchainExtent);
    void cleanup(Device &gfxDevice);

    inline static std::map<std::string, ImFont*> s_fonts;
  private:
    void copyBuffers(VkCommandBuffer cmd, Device &gfxDevice) const;
	void setDarkThemeColors();

    NBuffer idxBuffer;
    NBuffer vtxBuffer;

    ImageID fontTextureId{NULL_IMAGE_ID};

    struct PushConstants
    {
        VkDeviceAddress vertexBuffer;
        std::uint32_t textureId;
        std::uint32_t textureIsSRGB;
        glm::vec2 translate;
        glm::vec2 scale;
    };

    VkPipelineLayout pipelineLayout;
    VkPipeline pipeline;
};

}