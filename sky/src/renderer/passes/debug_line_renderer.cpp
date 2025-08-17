#include "debug_line_renderer.h"

#include "graphics/vulkan/vk_pipelines.h"
#include "renderer/frustum_culling.h"

namespace sky
{
void DebugLineRenderer::init(gfx::Device &device, VkFormat format, VkSampleCountFlagBits samples)
{
    VkDeviceSize bufferSize = sizeof(LineVertex) * m_maxLines * 2; // 2 vertices per line
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
        .enableDepthTest(true, VK_COMPARE_OP_ALWAYS)
        .setColorAttachmentFormat(format)
        .build(device.getDevice());
}

void DebugLineRenderer::addLine(const glm::vec3& start, const glm::vec3& end, const glm::vec3& color)
{
    if (m_vertices.size() >= m_maxLines * 2) return; // Buffer full
    
    m_vertices.push_back({start, color});
    m_vertices.push_back({end, color});
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

    static std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{{
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
    auto forward = camera.getForward();
    auto up = camera.getUp();
    auto position = camera.getPosition();
    auto size = camera.getFar() * 0.001f; // Adjust size based on far plane distance

    // Create a simple frustum shape
    glm::vec3 right = glm::normalize(glm::cross(forward, up));
    
    // Near plane (small rectangle)
    float nearSize = size * 0.3f;
    float nearDist = size * 0.2f;
    glm::vec3 nearCenter = position + forward * nearDist;
    
    glm::vec3 nearTL = nearCenter + up * nearSize - right * nearSize;      // top-left
    glm::vec3 nearTR = nearCenter + up * nearSize + right * nearSize;      // top-right
    glm::vec3 nearBL = nearCenter - up * nearSize - right * nearSize;      // bottom-left
    glm::vec3 nearBR = nearCenter - up * nearSize + right * nearSize;      // bottom-right
    
    // Far plane (larger rectangle)
    float farSize = size;
    float farDist = size;
    glm::vec3 farCenter = position + forward * farDist;
    
    glm::vec3 farTL = farCenter + up * farSize - right * farSize;          // top-left
    glm::vec3 farTR = farCenter + up * farSize + right * farSize;          // top-right
    glm::vec3 farBL = farCenter - up * farSize - right * farSize;          // bottom-left
    glm::vec3 farBR = farCenter - up * farSize + right * farSize;          // bottom-right
    
    // Draw near plane
    addLine(nearTL, nearTR, color);
    addLine(nearTR, nearBR, color);
    addLine(nearBR, nearBL, color);
    addLine(nearBL, nearTL, color);
    
    // Draw far plane
    addLine(farTL, farTR, color);
    addLine(farTR, farBR, color);
    addLine(farBR, farBL, color);
    addLine(farBL, farTL, color);
    
    // Connect near to far (frustum edges)
    addLine(nearTL, farTL, color);
    addLine(nearTR, farTR, color);
    addLine(nearBL, farBL, color);
    addLine(nearBR, farBR, color);
}
}