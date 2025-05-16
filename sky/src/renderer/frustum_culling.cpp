#include "frustum_culling.h"

#include "camera/camera.h"
#include "core/math/sphere.h"
#include "core/math/aabb.h"

namespace sky
{
namespace
{
glm::vec3 findCenter(const std::array<glm::vec3, 8> &points, const std::array<int, 4> &is)
{
    return (points[is[0]] + points[is[1]] + points[is[2]] + points[is[3]]) / 4.f;
}

glm::vec3 findNormal(const std::array<glm::vec3, 8> &points, const std::array<int, 4> &is)
{
    const auto e1 = glm::normalize(points[is[1]] - points[is[0]]);
    const auto e2 = glm::normalize(points[is[2]] - points[is[1]]);
    return glm::cross(e1, e2);
}
} // namespace

namespace edge
{
std::array<glm::vec3, 8> calculateFrustumCornersWorldSpace(Camera &camera)
{
    const auto nearDepth =  0.f;
    const auto farDepth = 1.f;
    const auto bottomY = 1.f;
    const auto topY = -1.f;

    const std::array<glm::vec3, 8> cornersNDC = {
        // near plane
        glm::vec3{-1.f, bottomY, nearDepth},
        glm::vec3{-1.f, topY, nearDepth},
        glm::vec3{1.f, topY, nearDepth},
        glm::vec3{1.f, bottomY, nearDepth},
        // far plane
        glm::vec3{-1.f, bottomY, farDepth},
        glm::vec3{-1.f, topY, farDepth},
        glm::vec3{1.f, topY, farDepth},
        glm::vec3{1.f, bottomY, farDepth},
    };

    const auto inv = glm::inverse(camera.getViewProjection());
    std::array<glm::vec3, 8> corners{};
    for (int i = 0; i < 8; ++i) {
        auto corner = inv * glm::vec4(cornersNDC[i], 1.f);
        corner /= corner.w;
        corners[i] = glm::vec3{corner};
    }
    return corners;
}
 
Frustum createFrustumFromCamera(const Camera &camera)
{
    Frustum frustum;

	const glm::vec3 camPos = { camera.getPosition().x, camera.getPosition().y, camera.getPosition().z }; 
    glm::mat4 viewMatrix = camera.getView(); // Assume you have a method to get the view matrix

	// Extract Right, Up, Front from the matrix
	glm::vec3 camRight = glm::vec3(viewMatrix[0][0], viewMatrix[1][0], viewMatrix[2][0]);  // Column 0
	glm::vec3 camUp = glm::vec3(viewMatrix[0][1], viewMatrix[1][1], viewMatrix[2][1]);     // Column 1
	glm::vec3 camFront = -glm::vec3(viewMatrix[0][2], viewMatrix[1][2], viewMatrix[2][2]); // -Column 2

	const auto zNear = camera.getNear();
	const auto zFar = camera.getFar();
    const auto fovY = glm::radians(60.f);  // ?
	const auto halfVSide = zFar * tanf(fovY * .5f);
	const auto halfHSide = halfVSide * camera.getAspect();
	const auto frontMultFar = zFar * camFront;

	frustum.nearFace = {camPos + zNear * camFront, camFront};
	frustum.farFace = {camPos + frontMultFar, -camFront};
	frustum.leftFace = {camPos, glm::cross(camUp, frontMultFar + camRight * halfHSide)};
	frustum.rightFace = {camPos, glm::cross(frontMultFar - camRight * halfHSide, camUp)};
	frustum.bottomFace = {camPos, glm::cross(frontMultFar + camUp * halfVSide, camRight)};
	frustum.topFace = {camPos, glm::cross(camRight, frontMultFar - camUp * halfVSide)};

    return frustum;
}

namespace
{
bool isOnOrForwardPlane(const Frustum::Plane &plane, const math::Sphere &sphere)
{
    return plane.getSignedDistanceToPlane(sphere.center) > -sphere.radius;
}

glm::vec3 getTransformScale(const glm::mat4 &transform)
{
    float sx = glm::length(glm::vec3{transform[0][0], transform[0][1], transform[0][2]});
    float sy = glm::length(glm::vec3{transform[1][0], transform[1][1], transform[1][2]});
    float sz = glm::length(glm::vec3{transform[2][0], transform[2][1], transform[2][2]});
    return {sx, sy, sz};
}
} // end of anonymous namespace

bool isInFrustum(const Frustum &frustum, const math::Sphere &s)
{
    return (isOnOrForwardPlane(frustum.farFace, s) && isOnOrForwardPlane(frustum.nearFace, s) &&
        isOnOrForwardPlane(frustum.leftFace, s) && isOnOrForwardPlane(frustum.rightFace, s) &&
        isOnOrForwardPlane(frustum.topFace, s) && isOnOrForwardPlane(frustum.bottomFace, s));
}

bool isInFrustum(const Frustum &frustum, const math::AABB &aabb)
{
    glm::vec3 vmin, vmax;
    bool ret = true;
    for (int i = 0; i < 6; ++i)
    {
        const auto &plane = frustum.getPlane(i);
        // X axis
        if (plane.normal.x < 0)
        {
            vmin.x = aabb.min.x;
            vmax.x = aabb.max.x;
        }
        else
        {
            vmin.x = aabb.max.x;
            vmax.x = aabb.min.x;
        }
        // Y axis
        if (plane.normal.y < 0)
        {
            vmin.y = aabb.min.y;
            vmax.y = aabb.max.y;
        }
        else
        {
            vmin.y = aabb.max.y;
            vmax.y = aabb.min.y;
        }
        // Z axis
        if (plane.normal.z < 0)
        {
            vmin.z = aabb.min.z;
            vmax.z = aabb.max.z;
        }
        else
        {
            vmin.z = aabb.max.z;
            vmax.z = aabb.min.z;
        }
        if (plane.getSignedDistanceToPlane(vmin) < 0)
        {
            return false;
        }
        if (plane.getSignedDistanceToPlane(vmax) <= 0)
        {
            ret = true;
        }
    }
    return ret;
}

math::Sphere calculateBoundingSphereWorld(const glm::mat4 &transform, const math::Sphere &s, bool hasSkeleton)
{
    const auto scale = getTransformScale(transform);
    float maxScale = std::max({scale.x, scale.y, scale.z});
    if (hasSkeleton)
    {
        maxScale = 5.f; // ignore scale for skeleton meshes (TODO: fix)
                        // setting scale to 1.f causes prolems with frustum culling
    }
    auto sphereWorld = s;
    sphereWorld.radius *= maxScale;
    sphereWorld.center = glm::vec3(transform * glm::vec4(sphereWorld.center, 1.f));
    return sphereWorld;
}
} // end of namespace edge
}