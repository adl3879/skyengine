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
    virtual SKY_INLINE const glm::vec3 &getPosition() const = 0;
    virtual SKY_INLINE const glm::mat4 &getViewProjection() = 0;
    virtual SKY_INLINE const glm::vec3 &getCameraDir() const = 0;
    virtual SKY_INLINE const float &getNear() const = 0;
    virtual SKY_INLINE const float &getFar() const = 0;
    virtual SKY_INLINE const float &getAspect() const = 0;
};
}