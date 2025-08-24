#include "physics_debug_renderer.h"

#include "core/application.h"
#include "renderer/passes/debug_line_renderer.h"

namespace sky
{
namespace physics
{
void PhysicsDebugRenderer::DrawLine(JPH::RVec3Arg inFrom, JPH::RVec3Arg inTo, JPH::ColorArg inColor)
{
    glm::vec3 start = { (float)inFrom.GetX(), (float)inFrom.GetY(), (float)inFrom.GetZ() };
    glm::vec3 end   = { (float)inTo.GetX(), (float)inTo.GetY(), (float)inTo.GetZ() };
    glm::vec3 color = {
        inColor.r / 255.0f,
        inColor.g / 255.0f,
        inColor.b / 255.0f
    };

    auto lineRenderer = Application::getRenderer()->getDebugLineRenderer();
    lineRenderer.addLine(start, end, color);
}

void PhysicsDebugRenderer::DrawTriangle(JPH::RVec3Arg inV1, JPH::RVec3Arg inV2, JPH::RVec3Arg inV3,
    JPH::ColorArg inColor, ECastShadow inCastShadow)
{
    glm::vec3 c = {
        inColor.r / 255.0f,
        inColor.g / 255.0f,
        inColor.b / 255.0f
    };

    glm::vec3 v1 = { (float)inV1.GetX(), (float)inV1.GetY(), (float)inV1.GetZ() };
    glm::vec3 v2 = { (float)inV2.GetX(), (float)inV2.GetY(), (float)inV2.GetZ() };
    glm::vec3 v3 = { (float)inV3.GetX(), (float)inV3.GetY(), (float)inV3.GetZ() };

    // draw edges of the triangle as lines
    auto lineRenderer = Application::getRenderer()->getDebugLineRenderer();
    lineRenderer.addLine(v1, v2, c);
    lineRenderer.addLine(v2, v3, c);
    lineRenderer.addLine(v3, v1, c);
}

void PhysicsDebugRenderer::DrawGeometry(JPH::RMat44Arg inModelMatrix, const JPH::AABox& inWorldSpaceBounds, 
    float inLODScaleSq, JPH::ColorArg inModelColor, const GeometryRef& inGeometry, 
    ECullMode inCullMode, 
    ECastShadow inCastShadow, 
    EDrawMode inDrawMode)
{
    // Not implemented
}

void PhysicsDebugRenderer::DrawText3D(JPH::RVec3Arg inPosition, const std::string_view& inString, 
    JPH::ColorArg inColor, float inHeight)
{
    // Not implemented
}

JPH::DebugRenderer::Batch PhysicsDebugRenderer::CreateTriangleBatch(const Triangle* inTriangles, int inTriangleCount)
{
   return {}; 
}

JPH::DebugRenderer::Batch PhysicsDebugRenderer::CreateTriangleBatch(const Vertex* inVertices, int inVertexCount, const JPH::uint32* inIndices, int inIndexCount)
{
    return {};
} 
}
}