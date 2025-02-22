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

	m_vertices = {
        {{-0.5f, -0.5f}, {0.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, 0}, // Bottom-left
        {{0.5f, -0.5f}, {1.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, 0}, // Bottom-right
        {{0.5f, 0.5f}, {1.0f, 1.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, 0}, // Top-right
        {{-0.5f, 0.5f}, {0.0f, 1.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, 0}  // Top-left
    };
    m_indices = {
        0, 1, 2,
        2, 3, 0
    };
	const size_t vertexBufferSize = m_vertices.size() * sizeof(QuadVertex);
	const size_t indexBufferSize = m_indices.size() * sizeof(uint32_t);

    m_vertexBuffer = device.createBuffer(
        vertexBufferSize,
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VMA_MEMORY_USAGE_GPU_ONLY);

    m_indexBuffer = device.createBuffer(
        indexBufferSize,
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VMA_MEMORY_USAGE_GPU_ONLY);

    //m_stagingBuffer = device.createBuffer(
    //    vertexBufferSize + indexBufferSize,
    //    VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
    //    VMA_MEMORY_USAGE_CPU_ONLY);

    //m_indices.reserve(MAX_INDICES);
    //m_vertices.reserve(MAX_VERTICES);

    //for (uint32_t i = 0; i < MAX_SPRITES; ++i)
    //{
    //    uint32_t baseIndex = i * VERTICES_PER_QUAD;
    //    m_indices.push_back(baseIndex + 0);
    //    m_indices.push_back(baseIndex + 1);
    //    m_indices.push_back(baseIndex + 2);
    //    m_indices.push_back(baseIndex + 2);
    //    m_indices.push_back(baseIndex + 3);
    //    m_indices.push_back(baseIndex + 0);
    //}

    //uploadBuffers(device);

     // Vertex input bindings and attributes
    VkVertexInputBindingDescription bindingDescription{
        .binding = 0, .stride = sizeof(QuadVertex), .inputRate = VK_VERTEX_INPUT_RATE_VERTEX};

    std::vector<VkVertexInputAttributeDescription> vertexAttributes = {
        {0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(QuadVertex, position)},    // position
        {1, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(QuadVertex, texCoord)},    // uv
        {2, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(QuadVertex, color)}, // color
        {3, 0, VK_FORMAT_R32_UINT, offsetof(QuadVertex, textureId)}         // textureId
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
    const auto layouts = std::array<VkDescriptorSetLayout, 1>{
        device.getBindlessDescSetLayout()};

    m_pInfo.pipelineLayout = gfx::vkutil::createPipelineLayout(device.getDevice(), layouts, pushConstantRanges);

    m_pInfo.pipeline = gfx::PipelineBuilder{m_pInfo.pipelineLayout}
                           .setShaders(vertexShader, fragShader)
                           .setInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
                           .setVertexInputState(vertexInputInfo)
                           .setPolygonMode(VK_POLYGON_MODE_FILL)
                           .setCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE)
                           .setMultisamplingNone()
                           .disableBlending()
                           .enableDepthTest(true, VK_COMPARE_OP_ALWAYS)
                           .setColorAttachmentFormat(format)
                           .setDepthFormat(VK_FORMAT_D32_SFLOAT)
                           .build(device.getDevice());
}

void SpriteBatchRenderer::shutdown(gfx::Device &device) {}

void SpriteBatchRenderer::addSprite(gfx::Device &device, const Sprite &sprite) 
{
   auto transformedVertices = calculateTransformedVertices(sprite);

    /*m_vertices.push_back({transformedVertices[0], sprite.texCoord, sprite.color, sprite.textureId});
    m_vertices.push_back({transformedVertices[1], sprite.texCoord + glm::vec2(1.0f, 0.0f), sprite.color, sprite.textureId});
    m_vertices.push_back({transformedVertices[2], sprite.texCoord + glm::vec2(1.0f, 1.0f), sprite.color, sprite.textureId});
    m_vertices.push_back({transformedVertices[3], sprite.texCoord + glm::vec2(0.0f, 1.0f), sprite.color, sprite.textureId});*/

   // Calculate the four vertices of the quad
   float x = sprite.position.x;
   float y = sprite.position.y;
   float width = sprite.size.x;
   float height = sprite.size.y;

   /* m_vertices.push_back({{x, y}, {0.0f, 0.0f}, sprite.color, sprite.textureId});
    m_vertices.push_back({{x + width, y}, {1.0f, 0.0f}, sprite.color, sprite.textureId});
    m_vertices.push_back({{x + width, y + height}, {1.0f, 1.0f}, sprite.color, sprite.textureId});
    m_vertices.push_back({{x, y + height}, {0.0f, 1.0f}, sprite.color, sprite.textureId});*/

   m_vertices = {
       {{0.0f, 0.0f}, {0.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, 0}, // Bottom-left
       {{1.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, 0}, // Bottom-right
       {{1.0f, 1.0f}, {1.0f, 1.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, 0}, // Top-right
       {{0.0f, 1.0f}, {0.0f, 1.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, 0}  // Top-left
   };

    /*m_indices.push_back(m_currentVertexCount + 0);
    m_indices.push_back(m_currentVertexCount + 1);
    m_indices.push_back(m_currentVertexCount + 2);
    m_indices.push_back(m_currentVertexCount + 2);
    m_indices.push_back(m_currentVertexCount + 3);
    m_indices.push_back(m_currentVertexCount + 0);*/

   //m_indices = {
   //     0, 1, 2, 2, 3, 0
   //};

    m_currentVertexCount += VERTICES_PER_QUAD;
}

void SpriteBatchRenderer::flush(gfx::Device &device, 
    gfx::CommandBuffer cmd,
    VkExtent2D extent,
	const gfx::AllocatedBuffer &sceneDataBuffer) 
{
    //if (m_currentVertexCount == 0) return;

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pInfo.pipeline);
    device.bindDescriptorSets(cmd, m_pInfo.pipelineLayout, {device.getBindlessDescSet()});

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
        .vertexBuffer = m_vertexBuffer.address,
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

    //m_vertices.clear();
    //m_indices.clear();
    //m_currentVertexCount = 0;
}

void SpriteBatchRenderer::uploadBuffers(gfx::Device &device) 
{ 
	const size_t vBufferSize = m_vertices.size() * sizeof(QuadVertex);
    const size_t iBufferSize = m_indices.size() * sizeof(uint32_t);

     auto stagingBuffer = device.createBuffer(
        vBufferSize + iBufferSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VMA_MEMORY_USAGE_CPU_ONLY);

    void *mappedMemory = stagingBuffer.info.pMappedData;
    memcpy(mappedMemory, m_vertices.data(), vBufferSize);
    memcpy(static_cast<char *>(mappedMemory) + vBufferSize, m_indices.data(), iBufferSize);

    device.immediateSubmit(
        [&](VkCommandBuffer cmd)
        {
            VkBufferCopy vertexCopy{0};
            vertexCopy.dstOffset = 0;
            vertexCopy.srcOffset = 0;
            vertexCopy.size = vBufferSize;

            vkCmdCopyBuffer(cmd, stagingBuffer.buffer, m_vertexBuffer.buffer, 1, &vertexCopy);
            
            VkBufferCopy indexCopy{0};
            indexCopy.dstOffset = 0;
            indexCopy.srcOffset = vBufferSize;
            indexCopy.size = iBufferSize;

            vkCmdCopyBuffer(cmd, stagingBuffer.buffer, m_indexBuffer.buffer, 1, &indexCopy);
        });

	vkQueueWaitIdle(device.getQueue());
    device.destroyBuffer(stagingBuffer);
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

    // Return the transformed vertices
    return {bottomLeft, bottomRight, topRight, topLeft};
}
} // namespace sky