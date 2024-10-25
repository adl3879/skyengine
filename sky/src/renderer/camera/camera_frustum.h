#pragma once

#include <glm/glm.hpp>

#include <skypch.h>

namespace sky
{
class CameraFrustum
{
  public:
    enum class Side
    {
        Left,
        Right,
        Top,
        Bottom,
        Near,
        Far
    };

    CameraFrustum(const glm::mat4 &viewMatrix, const glm::mat4 &projMatrix);
    CameraFrustum(const glm::mat4 &viewProjectionMatrix);
    ~CameraFrustum() = default;

    const glm::vec4 &getSide(const CameraFrustum::Side &side) const;
    const std::array<glm::vec4, 6> &getSides() const;

    bool isInFrustum(const glm::mat4 &transform, const glm::vec3 &minHalfExt, const glm::vec3 &maxHalfExt);
    // Point light test
    bool isInFrustum(const glm::vec3 &position, float radius);
    // Spot light test
    bool isInFrustum(const glm::vec3 &position, const glm::vec3 &direction, float length, float radius);

  private:
    // Camera View and Projection product
    glm::mat4 m_VP_Matrix;
    std::array<glm::vec4, 6> m_Frustum;

  private:
    void CalculateFrustum(const glm::mat4 &viewProjectionMatrix);
    void Normalize(glm::vec4 &side);
    bool AABBTest(const glm::vec3 &minHalfExt, const glm::vec3 &maxHalfExt);
    bool SSE_AABBTest(const glm::vec3 &minHalfExt, const glm::vec3 &maxHalfExt);
    bool OBBTest(const glm::mat4 &transform, const glm::vec3 &minHalfExt, const glm::vec3 &maxHalfExt);
    bool SSE_OBBTest(const glm::mat4 &transform, const glm::vec3 &minHalfExt, const glm::vec3 &maxHalfExt);
    bool Sphere_Test(const glm::vec3 &position, float radius);
};
} // namespace shade