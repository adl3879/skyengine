#pragma once

#include <skypch.h>

#include <vulkan/vulkan.h>
#include "vk_types.h"

namespace sky::gfx
{
class Device;

class NBuffer
{
  public:
    void init(Device &gfxDevice, VkBufferUsageFlags usage, std::size_t dataSize, std::size_t numFramesInFlight,
              const char *label);
    void cleanup(Device &gfxDevice);
    void uploadNewData(VkCommandBuffer cmd, std::size_t frameIndex, void *newData, std::size_t dataSize,
                       std::size_t offset = 0, bool sync = true) const;
    const AllocatedBuffer &getBuffer() const { return gpuBuffer; }

  private:
    std::size_t framesInFlight{0};
    std::size_t gpuBufferSize{0};
    std::vector<AllocatedBuffer> stagingBuffers;
    AllocatedBuffer gpuBuffer;
    bool initialized{false};
};
}