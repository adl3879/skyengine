#include "mesh_cache.h"

#include "core/math/math.h"

namespace sky
{
void MeshCache::cleanup(gfx::Device &device) 
{
    for (const auto &[key, value] : m_meshes)
	{
		device.destroyBuffer(value.vertexBuffer);
        device.destroyBuffer(value.indexBuffer);
	}
}

MeshID MeshCache::addMesh(gfx::Device &device, const Mesh &mesh) 
{
    auto n = mesh.name;
    auto gpuMesh = gfx::GPUMeshBuffers{
        .numIndices = static_cast<uint32_t>(mesh.indices.size()),
        .materialId = mesh.material,
    };

    std::vector<glm::vec3> positions(mesh.vertices.size());
    
    glm::vec3 min = glm::vec3(std::numeric_limits<float>::max());
    glm::vec3 max = glm::vec3(std::numeric_limits<float>::min());

    for (std::size_t i = 0; i < mesh.vertices.size(); ++i)
    {
        positions[i] = mesh.vertices[i].position;
        min = glm::min(min, positions[i]);
        max = glm::max(max, positions[i]);
    }
    gpuMesh.boundingSphere = math::calculateBoundingSphere(positions);
    gpuMesh.boundingBox = math::AABB{.min = min, .max = max};
    
    uploadMesh(device, mesh, gpuMesh);
    const auto id = UUID::generate();
    m_meshes[id] = gpuMesh;

    m_CPUMeshes[id] = Mesh{
        .vertices = mesh.vertices,
        .indices = mesh.indices,
        .material = mesh.material,
	    .name = mesh.name,
	    .boundingBox = gpuMesh.boundingBox,
    };

    return id;
}

const gfx::GPUMeshBuffers &MeshCache::getMesh(MeshID id) const
{
    return m_meshes.at(id);
}   

void MeshCache::uploadMesh(gfx::Device &device, const Mesh &mesh, gfx::GPUMeshBuffers &gpuMesh) const 
{
    const size_t vertexBufferSize = mesh.vertices.size() * sizeof(Vertex);
    const size_t indexBufferSize = mesh.indices.size() * sizeof(uint32_t);

    // create vertex buffer
    gpuMesh.vertexBuffer = device.createBuffer(vertexBufferSize,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT |
		VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
		VMA_MEMORY_USAGE_GPU_ONLY);

    // create index buffer
    gpuMesh.indexBuffer =
        device.createBuffer(indexBufferSize, 
            VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            VMA_MEMORY_USAGE_GPU_ONLY);

    gfx::AllocatedBuffer staging =
        device.createBuffer(vertexBufferSize + indexBufferSize, 
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
            VMA_MEMORY_USAGE_CPU_ONLY);

    // ?         staging.allocation.GetMappedData()   
    void *data = staging.info.pMappedData;
    memcpy(data, mesh.vertices.data(), vertexBufferSize);
    memcpy((char *)data + vertexBufferSize, mesh.indices.data(), indexBufferSize);

    device.immediateSubmit(
        [&](VkCommandBuffer cmd)
        {
            VkBufferCopy vertexCopy{0};
            vertexCopy.dstOffset = 0;
            vertexCopy.srcOffset = 0;
            vertexCopy.size = vertexBufferSize;

            vkCmdCopyBuffer(cmd, staging.buffer, gpuMesh.vertexBuffer.buffer, 1, &vertexCopy);

            VkBufferCopy indexCopy{0};
            indexCopy.dstOffset = 0;
            indexCopy.srcOffset = vertexBufferSize;
            indexCopy.size = indexBufferSize;

            vkCmdCopyBuffer(cmd, staging.buffer, gpuMesh.indexBuffer.buffer, 1, &indexCopy);
        });

    device.destroyBuffer(staging);
}
} // namespace sky