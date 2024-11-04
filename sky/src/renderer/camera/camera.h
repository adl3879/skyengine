#pragma once

#include <skypch.h>
#include <glm/glm.hpp>

namespace sky
{
class Camera
{
  public:
    struct RenderData
    {
        glm::mat4 viewProjection;
        glm::mat4 view;
        glm::mat4 projection;
        alignas(16) glm::vec3 position;
        alignas(16) glm::vec3 forward;
        float near_;
        float far_;
    };

  public:
    virtual SKY_INLINE glm::mat4 getViewProjection() const = 0;
    virtual SKY_INLINE RenderData getRenderData() const = 0;
};
}