#include "sprite_renderer.h"

#include "graphics/vulkan/vk_pipelines.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>

namespace sky
{
const size_t vertexBufferSize = MAX_VERTICES * sizeof(QuadVertex);
const size_t indexBufferSize = MAX_INDICES * sizeof(uint32_t);

void SpriteBatchRenderer::init(gfx::Device &device, VkFormat format) 
{
    m_currentVertexCount = 0;

    m_vertexBuffer = device.createBuffer(
        vertexBufferSize,
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VMA_MEMORY_USAGE_GPU_ONLY);

    m_indexBuffer = device.createBuffer(
        indexBufferSize,
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VMA_MEMORY_USAGE_GPU_ONLY);

    m_stagingBuffer = device.createBuffer(
        vertexBufferSize + indexBufferSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VMA_MEMORY_USAGE_CPU_ONLY);

    m_indices.reserve(MAX_INDICES);
    m_vertices.reserve(MAX_VERTICES);

    for (uint32_t i = 0; i < MAX_SPRITES; ++i)
    {
        uint32_t baseIndex = i * VERTICES_PER_QUAD;
        m_indices.push_back(baseIndex + 0);
        m_indices.push_back(baseIndex + 1);
        m_indices.push_back(baseIndex + 2);
        m_indices.push_back(baseIndex + 2);
        m_indices.push_back(baseIndex + 3);
        m_indices.push_back(baseIndex + 0);
    }

    // Vertex input bindings and attributes
    VkVertexInputBindingDescription bindingDescription{
        .binding = 0, .stride = sizeof(QuadVertex), .inputRate = VK_VERTEX_INPUT_RATE_VERTEX};

    std::vector<VkVertexInputAttributeDescription> vertexAttributes = {
        {0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(QuadVertex, position)},    // position
        {1, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(QuadVertex, texCoord)},    // uv
        {2, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(QuadVertex, color)}, // color
        {3, 0, VK_FORMAT_R32_UINT, offsetof(QuadVertex, textureId)},        // textureId
        {4, 0, VK_FORMAT_R32_UINT, offsetof(QuadVertex, uniqueId)}          // uniqueId
    };

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexAttributes.size());
    vertexInputInfo.pVertexAttributeDescriptions = vertexAttributes.data();

    const auto vertexShader = gfx::vkutil::loadShaderModule("shaders/sprite.vert.spv", device.getDevice());
    const auto fragShader = gfx::vkutil::loadShaderModule("shaders/sprite.frag.spv", device.getDevice());

    const auto bufferRange = VkPushConstantRange{
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
        .offset = 0,
        .size = sizeof(PushConstants),
    };

    const auto pushConstantRanges = std::array{bufferRange};
    const auto layouts = std::array<VkDescriptorSetLayout, 2>{
        device.getBindlessDescSetLayout(),
        device.getStorageBufferLayout()
    };

    m_pInfo.pipelineLayout = gfx::vkutil::createPipelineLayout(device.getDevice(), layouts, pushConstantRanges);

    m_pInfo.pipeline = gfx::PipelineBuilder{m_pInfo.pipelineLayout}
        .setShaders(vertexShader, fragShader)
        .setInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
        .setVertexInputState(vertexInputInfo)
        .setPolygonMode(VK_POLYGON_MODE_FILL)
        .setCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE)
        .setMultisamplingNone()
        .enableBlending()
        .enableDepthTest(true, VK_COMPARE_OP_ALWAYS)
        .setColorAttachmentFormat(format)
        .setDepthFormat(VK_FORMAT_D32_SFLOAT)
        .build(device.getDevice());
}

void SpriteBatchRenderer::cleanup(gfx::Device &device) 
{
    device.destroyBuffer(m_vertexBuffer);
    device.destroyBuffer(m_indexBuffer);
    device.destroyBuffer(m_stagingBuffer);
}

void SpriteBatchRenderer::drawSprite(gfx::Device &device, const Sprite &sprite) 
{
   ImageID textureId;
   textureId = sprite.textureId == NULL_IMAGE_ID ? device.getWhiteTextureID() 
       : sprite.textureId;

   auto transformedVertices = calculateTransformedVertices(sprite);

    m_vertices.push_back({transformedVertices[0], sprite.texCoord, sprite.color, textureId, sprite.uniqueId});
    m_vertices.push_back({transformedVertices[1], sprite.texCoord + glm::vec2(1.0f, 0.0f), sprite.color, textureId, sprite.uniqueId});
    m_vertices.push_back({transformedVertices[2], sprite.texCoord + glm::vec2(1.0f, 1.0f), sprite.color, textureId, sprite.uniqueId});
    m_vertices.push_back({transformedVertices[3], sprite.texCoord + glm::vec2(0.0f, 1.0f), sprite.color, textureId, sprite.uniqueId});

    m_currentVertexCount += VERTICES_PER_QUAD;
}

void SpriteBatchRenderer::flush(gfx::Device &device, 
    gfx::CommandBuffer cmd,
    VkExtent2D extent,
	const gfx::AllocatedBuffer &sceneDataBuffer) 
{
    if (m_currentVertexCount == 0) return;

    uploadBuffers(device);

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pInfo.pipeline);
    VkDescriptorSet descriptorSets[] = {
        device.getBindlessDescSet(),
        device.getStorageBufferDescSet(),
    };
    device.bindDescriptorSets(cmd, m_pInfo.pipelineLayout, descriptorSets);

    const auto viewport = VkViewport{
        .x = 0.f,
		.y = 0.f,
		.width = (float)extent.width,
		.height = (float)extent.height,
		.minDepth = 0.f,
		.maxDepth = 1.f,
    };
    vkCmdSetViewport(cmd, 0, 1, &viewport);

    const auto scissor = VkRect2D{
        .offset = {0, 0},
        .extent = extent,
    };
    vkCmdSetScissor(cmd, 0, 1, &scissor);

    const auto pushConstants = PushConstants{
        .sceneDataBuffer = sceneDataBuffer.address,
    };

    vkCmdPushConstants(cmd, 
        m_pInfo.pipelineLayout, 
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 
        0, 
        sizeof(PushConstants), 
        &pushConstants);
    
    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(cmd, 0, 1, &m_vertexBuffer.buffer, &offset);
    vkCmdBindIndexBuffer(cmd, m_indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);

    vkCmdDrawIndexed(cmd, m_indices.size(), 1, 0, 0, 0);

    m_vertices.clear();
    m_currentVertexCount = 0;
}

void SpriteBatchRenderer::uploadBuffers(gfx::Device &device) 
{ 
    void *mappedMemory = m_stagingBuffer.info.pMappedData;
    memcpy(mappedMemory, m_vertices.data(), vertexBufferSize);
    memcpy(static_cast<char *>(mappedMemory) + vertexBufferSize, m_indices.data(), indexBufferSize);

    device.immediateSubmit(
        [&](VkCommandBuffer cmd)
        {
            VkBufferCopy vertexCopy{0};
            vertexCopy.dstOffset = 0;
            vertexCopy.srcOffset = 0;
            vertexCopy.size = vertexBufferSize;

            vkCmdCopyBuffer(cmd, m_stagingBuffer.buffer, m_vertexBuffer.buffer, 1, &vertexCopy);
            
            VkBufferCopy indexCopy{0};
            indexCopy.dstOffset = 0;
            indexCopy.srcOffset = vertexBufferSize;
            indexCopy.size = indexBufferSize;

            vkCmdCopyBuffer(cmd, m_stagingBuffer.buffer, m_indexBuffer.buffer, 1, &indexCopy);
        });
}

std::array<glm::vec2, 4> SpriteBatchRenderer::calculateTransformedVertices(const Sprite &sprite)
{
    // Extract position and size
    glm::vec2 position = sprite.position;
    glm::vec2 size = sprite.size;

    // Calculate the four corners of the quad
    glm::vec2 bottomLeft = position;
    glm::vec2 bottomRight = position + glm::vec2(size.x, 0.0f);
    glm::vec2 topRight = position + size;
    glm::vec2 topLeft = position + glm::vec2(0.0f, size.y);

    // If the sprite has a rotation or origin (pivot), apply transformations
    if (sprite.rotation != 0.0f || sprite.origin != glm::vec2(0.0f))
    {
        // Calculate the pivot point (origin)
        glm::vec2 pivot = position + sprite.origin;

        // Create a rotation matrix
        float cosAngle = std::cos(sprite.rotation);
        float sinAngle = std::sin(sprite.rotation);

        // Rotate each corner around the pivot
        auto rotate = [&](const glm::vec2 &point) -> glm::vec2
        {
            glm::vec2 translated = point - pivot;
            return glm::vec2(translated.x * cosAngle - translated.y * sinAngle + pivot.x,
                translated.x * sinAngle + translated.y * cosAngle + pivot.y);
        };

        bottomLeft = rotate(bottomLeft);
        bottomRight = rotate(bottomRight);
        topRight = rotate(topRight);
        topLeft = rotate(topLeft);
    }

    return {bottomLeft, bottomRight, topRight, topLeft};
}
} // namespace sky