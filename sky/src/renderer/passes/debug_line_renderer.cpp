#include "debug_line_renderer.h"

#include "graphics/vulkan/vk_pipelines.h"
#include "renderer/frustum_culling.h"

namespace sky
{
void DebugLineRenderer::init(gfx::Device &device, VkFormat format, VkSampleCountFlagBits samples)
{
    VkDeviceSize bufferSize = sizeof(LineVertex) * m_maxLines * 6; // 2 vertices per line
    m_vertexBuffer = device.createBuffer(
        bufferSize,
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VMA_MEMORY_USAGE_GPU_ONLY);

    m_stagingBuffer = device.createBuffer(
        bufferSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VMA_MEMORY_USAGE_CPU_ONLY);

    const auto vertexShader = gfx::vkutil::loadShaderModule("shaders/debug_line.vert.spv", device.getDevice());
    const auto fragShader = gfx::vkutil::loadShaderModule("shaders/debug_line.frag.spv", device.getDevice());

    const auto bufferRange = VkPushConstantRange{
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
        .offset = 0,
        .size = sizeof(PushConstants),
    };

    const auto pushConstantRanges = std::array{bufferRange};
    const auto layouts = std::array<VkDescriptorSetLayout, 0>{};

    m_pInfo.pipelineLayout = gfx::vkutil::createPipelineLayout(device.getDevice(), layouts, pushConstantRanges);

    m_pInfo.pipeline = gfx::PipelineBuilder{m_pInfo.pipelineLayout}
        .setShaders(vertexShader, fragShader)
        .setInputTopology(VK_PRIMITIVE_TOPOLOGY_LINE_LIST)
        .setVertexInputState(getVertexInputState())
        .setPolygonMode(VK_POLYGON_MODE_FILL)
        .setCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE)
        .setMultisampling(samples)
        .enableBlending()
        .enableDepthTest(true, VK_COMPARE_OP_LESS_OR_EQUAL)
        .setColorAttachmentFormat(format)
        .build(device.getDevice());
}

void DebugLineRenderer::addLine(const glm::vec3& start, const glm::vec3& end, const glm::vec3& color)
{
    if (m_vertices.size() >= m_maxLines * 6) return; // 6 vertices per line (2 triangles)
    
    // Calculate line direction and perpendicular vector
    glm::vec3 direction = glm::normalize(end - start);
    glm::vec3 perpendicular = glm::vec3(-direction.y, direction.x, 0.0f);
    
    // If line is mostly vertical, use different perpendicular
    if (glm::length(perpendicular) < 0.1f) {
        perpendicular = glm::vec3(0.0f, 0.0f, 1.0f);
    }
    float thickness = 0.01f; // Adjust thickness as needed
    perpendicular = glm::normalize(perpendicular) * (thickness * 0.5f);
    
    // Create quad vertices
    glm::vec3 v1 = start - perpendicular;
    glm::vec3 v2 = start + perpendicular;
    glm::vec3 v3 = end + perpendicular;
    glm::vec3 v4 = end - perpendicular;
    
    // Add two triangles to form a quad with proper UV coordinates
    // Triangle 1: v1, v2, v3
    m_vertices.push_back({v1, color, {0.0f, 0.0f}});
    m_vertices.push_back({v2, color, {0.0f, 1.0f}});
    m_vertices.push_back({v3, color, {1.0f, 1.0f}});
    
    // Triangle 2: v1, v3, v4
    m_vertices.push_back({v1, color, {0.0f, 0.0f}});
    m_vertices.push_back({v3, color, {1.0f, 1.0f}});
    m_vertices.push_back({v4, color, {1.0f, 0.0f}});
    
    m_needsBufferUpdate = true;
}

void DebugLineRenderer::draw(gfx::Device &device, 
    gfx::CommandBuffer cmd, 
    const gfx::AllocatedBuffer &sceneDataBuffer)
{
    if (m_vertices.empty()) return;

    uploadVertexBuffer(device);

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pInfo.pipeline);

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

    vkCmdDraw(cmd, static_cast<uint32_t>(m_vertices.size()), 1, 0, 0);

    clear();
}

VkPipelineVertexInputStateCreateInfo DebugLineRenderer::getVertexInputState() const
{
    static VkVertexInputBindingDescription bindingDescription{
        .binding = 0,
        .stride = sizeof(LineVertex),
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
    };

    static std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{{
        {
            .location = 0,
            .binding = 0,
            .format = VK_FORMAT_R32G32B32_SFLOAT, // position
            .offset = offsetof(LineVertex, position)
        },
        {
            .location = 1,
            .binding = 0,
            .format = VK_FORMAT_R32G32B32_SFLOAT, // color
            .offset = offsetof(LineVertex, color)
        },
        {
            .location = 2,
            .binding = 0,
            .format = VK_FORMAT_R32G32_SFLOAT, // uv
            .offset = offsetof(LineVertex, uv)
        }
    }}; 

    return VkPipelineVertexInputStateCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = 1,
        .pVertexBindingDescriptions = &bindingDescription,
        .vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size()),
        .pVertexAttributeDescriptions = attributeDescriptions.data()
    };
}


void DebugLineRenderer::uploadVertexBuffer(gfx::Device &device)
{
    if (!m_needsBufferUpdate || m_vertices.empty()) return;
    
    // Only copy the actual vertex data, not the full buffer
    auto actualDataSize = sizeof(LineVertex) * m_vertices.size();
    
    void *mappedMemory = m_stagingBuffer.info.pMappedData;
    memcpy(mappedMemory, m_vertices.data(), actualDataSize);

    device.immediateSubmit([&](VkCommandBuffer cmd) {
        VkBufferCopy vertexCopy{
            .srcOffset = 0,
            .dstOffset = 0,
            .size = actualDataSize  // Use actual data size, not max buffer size
        };
        vkCmdCopyBuffer(cmd, m_stagingBuffer.buffer, m_vertexBuffer.buffer, 1, &vertexCopy);
    });
    
    m_needsBufferUpdate = false;
}


void DebugLineRenderer::clear()
{
    if (!m_vertices.empty()) {
        m_vertices.clear();
        m_needsBufferUpdate = false; // No need to update if we're empty
    }
}

void DebugLineRenderer::cleanup(gfx::Device &device)
{
    vkDestroyPipeline(device.getDevice(), m_pInfo.pipeline, nullptr);
    vkDestroyPipelineLayout(device.getDevice(), m_pInfo.pipelineLayout, nullptr);
}

void DebugLineRenderer::addCameraFrustum(Camera &camera, const glm::vec3 &color)
{
    auto corners = sky::edge::calculateFrustumCornersWorldSpace(camera);
    
    // Get camera position to calculate distances
    glm::vec3 cameraPos = camera.getPosition();
    
    // Scale far plane corners to limit distance
    float maxDistance = 50.0f; // Example max distance, adjust as needed
    for (int i = 4; i < 8; ++i) { // Far plane corners are indices 4-7
        glm::vec3 direction = corners[i] - cameraPos;
        float currentDistance = glm::length(direction);
        
        if (currentDistance > maxDistance) {
            // Clamp to max distance
            corners[i] = cameraPos + glm::normalize(direction) * maxDistance;
        }
    }
    
    // Draw near plane (first 4 corners)
    addLine(corners[0], corners[1], color);
    addLine(corners[1], corners[2], color); 
    addLine(corners[2], corners[3], color);
    addLine(corners[3], corners[0], color);
    
    // Draw far plane (last 4 corners)
    addLine(corners[4], corners[5], color);
    addLine(corners[5], corners[6], color);
    addLine(corners[6], corners[7], color);
    addLine(corners[7], corners[4], color);
    
    // Connect near to far
    addLine(corners[0], corners[4], color);
    addLine(corners[1], corners[5], color);
    addLine(corners[2], corners[6], color);
    addLine(corners[3], corners[7], color);
}
}