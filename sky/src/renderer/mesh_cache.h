#pragma once

#include <skypch.h>

#include "graphics/vulkan/vk_types.h"
#include "graphics/vulkan/vk_device.h"
#include "renderer/mesh.h"
#include "core/uuid.h"

namespace sky
{
struct Mesh;

class MeshCache
{
  public:
    void cleanup(gfx::Device &gfxDevice);

    MeshID addMesh(gfx::Device &gfxDevice, const Mesh &mesh);
    const gfx::GPUMeshBuffers &getMesh(MeshID id) const;

  private:
    void uploadMesh(gfx::Device &gfxDevice, const Mesh &mesh, gfx::GPUMeshBuffers &gpuMesh) const;

    std::unordered_map<MeshID, gfx::GPUMeshBuffers> m_meshes;
};
} // namespace sky