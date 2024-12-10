#pragma once

#include <skypch.h>

#include "renderer/camera/camera.h"
#include "renderer/passes/pass.h"
#include "renderer/mesh_cache.h"

namespace sky
{
class ForwardRendererPass : public Pass
{
  public:
    ForwardRendererPass() = default;
    ~ForwardRendererPass() = default;

    void init(const gfx::Device &device);
    void draw(
        gfx::Device &device, 
        gfx::CommandBuffer cmd, 
        VkExtent2D extent,
        const Camera &camera,
        const gfx::AllocatedBuffer &sceneDataBuffer,
        const MeshCache &meshCache,
        const std::vector<MeshDrawCommand> &drawCommands);
    void cleanup(const gfx::Device &device);

  private:
    struct PushConstants
	{
        glm::mat4 transform;
        VkDeviceAddress sceneDataBuffer;
        VkDeviceAddress vertexBuffer;
        MaterialID materialId;
        std::uint32_t padding;
	};
};
} // namespace sky
