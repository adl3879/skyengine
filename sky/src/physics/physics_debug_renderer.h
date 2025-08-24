#pragma once

#include <skypch.h>

#include <Jolt/Jolt.h>
#include <Jolt/Renderer/DebugRenderer.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace sky 
{
class DebugLineRenderer;

namespace physics
{
class PhysicsDebugRenderer : public JPH::DebugRenderer 
{
  public:
    void DrawLine(JPH::RVec3Arg inFrom, JPH::RVec3Arg inTo, JPH::ColorArg inColor) override;
    void DrawTriangle(JPH::RVec3Arg inV1, JPH::RVec3Arg inV2, JPH::RVec3Arg inV3,
        JPH::ColorArg inColor, ECastShadow /*inCastShadow*/) override;
    void DrawGeometry(JPH::RMat44Arg inModelMatrix, const JPH::AABox& inWorldSpaceBounds, 
        float inLODScaleSq, JPH::ColorArg inModelColor, const GeometryRef& inGeometry, 
        ECullMode inCullMode = ECullMode::CullBackFace, 
        ECastShadow inCastShadow = ECastShadow::On, 
        EDrawMode inDrawMode = EDrawMode::Solid) override;
    void DrawText3D(JPH::RVec3Arg inPosition, const std::string_view& inString, 
        JPH::ColorArg inColor = JPH::Color::sWhite, float inHeight = 0.5f) override;

    Batch CreateTriangleBatch(const Triangle* inTriangles, int inTriangleCount) override;
    Batch CreateTriangleBatch(const Vertex* inVertices, int inVertexCount, const JPH::uint32* inIndices, int inIndexCount) override;
};
}
}