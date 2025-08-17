#pragma once

#include <skypch.h>

#include "pass.h"
#include "renderer/camera/camera.h"

namespace sky
{
struct LineVertex 
{
    glm::vec3 position;
    glm::vec3 color;
};

static constexpr uint32_t DEFAULT_MAX_LINES = 10000;

class DebugLineRenderer : public Pass
{
  public:
    void init(gfx::Device &device, VkFormat format, VkSampleCountFlagBits samples);
    void cleanup(gfx::Device &device);

    void addLine(const glm::vec3& start, const glm::vec3& end, const glm::vec3& color);
    void draw(gfx::Device &device, gfx::CommandBuffer cmd, const gfx::AllocatedBuffer &sceneDataBuffer);
    void clear();

  public:
    void addCameraFrustum(Camera &camera, const glm::vec3 &color = {1.f, 1.f, 1.f});

  private:
    VkPipelineVertexInputStateCreateInfo getVertexInputState() const;
    void uploadVertexBuffer(gfx::Device &device);

  private:
    gfx::AllocatedBuffer m_vertexBuffer;
    gfx::AllocatedBuffer m_stagingBuffer;
    std::vector<LineVertex> m_vertices;

    uint32_t m_maxLines = DEFAULT_MAX_LINES;
    bool m_needsBufferUpdate;

    struct PushConstants 
    {
		VkDeviceAddress sceneDataBuffer;
    };
};
}