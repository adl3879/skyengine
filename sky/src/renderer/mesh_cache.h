#pragma once

#include <skypch.h>

#include "graphics/vulkan/vk_types.h"
#include "graphics/vulkan/vk_device.h"
#include "renderer/mesh.h"

namespace sky
{
struct Mesh;

using MeshId = uint32_t;

class MeshCache
{
  public:
    void cleanup(gfx::Device &gfxDevice);

    MeshId addMesh(gfx::Device &gfxDevice, const Mesh &mesh);
    const gfx::GPUMeshBuffers &getMesh(MeshId id) const;

  private:
    void uploadMesh(gfx::Device &gfxDevice, const Mesh &mesh, gfx::GPUMeshBuffers &gpuMesh) const;

    std::vector<gfx::GPUMeshBuffers> meshes;
};
} // namespace sky