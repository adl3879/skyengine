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
        Camera &camera,
        const gfx::AllocatedBuffer &sceneDataBuffer,
        const MeshCache &meshCache,
        const std::vector<MeshDrawCommand> &drawCommands);
	void draw2(
        gfx::Device &device, 
        gfx::CommandBuffer cmd, 
        VkExtent2D extent,
        const gfx::AllocatedBuffer &sceneDataBuffer,
        const MeshCache &meshCache,
        std::vector<MeshID> meshId,
        MaterialID materialId,
        bool useDefaultMaterial = false);

    void cleanup(const gfx::Device &device);

  private:
    struct PushConstants
	{
        glm::mat4 transform;
        uint32_t uniqueId;
        VkDeviceAddress sceneDataBuffer;
        VkDeviceAddress vertexBuffer;
        MaterialID materialId;
	};
};
} // namespace sky
