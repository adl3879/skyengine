#pragma once

#include <skypch.h>
#include <glm/glm.hpp>

namespace sky
{
class Camera
{
  public:
    virtual SKY_INLINE const glm::mat4 &getView() const = 0;
    virtual SKY_INLINE const glm::mat4 &getProjection() const = 0;
    virtual SKY_INLINE const glm::vec4 &getPosition() const = 0;
    virtual SKY_INLINE const glm::mat4 &getViewProjection() = 0;
};
}