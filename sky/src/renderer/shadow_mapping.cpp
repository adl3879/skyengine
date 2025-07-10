#include "shadow_mapping.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>

namespace sky 
{
glm::mat4 calculateCSMCamera(const std::array<glm::vec3, 8>& frustumCorners,
    const glm::vec3& lightDir,
    float shadowMapSize)
{
    // Create a coordinate system with the light direction as the forward axis
    glm::vec3 lightDirNorm = glm::normalize(lightDir);
    
    // Find a suitable up vector (avoid using world up if light direction is nearly parallel to it)
    glm::vec3 upVector = glm::abs(glm::dot(lightDirNorm, glm::vec3(0.0f, 1.0f, 0.0f))) > 0.99f ? 
        glm::vec3(0.0f, 0.0f, 1.0f) : glm::vec3(0.0f, 1.0f, 0.0f);
    
    // Create the light's coordinate system
    glm::vec3 lightRight = glm::normalize(glm::cross(upVector, lightDirNorm));
    glm::vec3 lightUp = glm::normalize(glm::cross(lightDirNorm, lightRight));
    
    // Find the bounding box of the frustum in light space
    auto minX = std::numeric_limits<float>::max();
    auto maxX = std::numeric_limits<float>::lowest();
    auto minY = std::numeric_limits<float>::max();
    auto maxY = std::numeric_limits<float>::lowest();
    auto minZ = std::numeric_limits<float>::max();
    auto maxZ = std::numeric_limits<float>::lowest();
    
    // Project each frustum corner into light space and find the bounds
    for (const auto& corner : frustumCorners)
    {
        float dotX = glm::dot(corner, lightRight);
        float dotY = glm::dot(corner, lightUp);
        float dotZ = glm::dot(corner, lightDirNorm);
        
        minX = std::min(minX, dotX);
        maxX = std::max(maxX, dotX);
        minY = std::min(minY, dotY);
        maxY = std::max(maxY, dotY);
        minZ = std::min(minZ, dotZ);
        maxZ = std::max(maxZ, dotZ);
    }
    
    // Add a small bias to avoid precision issues
    const float bias = 10.0f;
    minX -= bias;
    maxX += bias;
    minY -= bias;
    maxY += bias;
    minZ -= bias; // Move the near plane back
    maxZ += 1000.0f; // Extend the far plane to catch more shadows
    
    // Calculate the center of the light view
    glm::vec3 lightViewCenter = lightRight * (minX + maxX) * 0.5f + 
        lightUp * (minY + maxY) * 0.5f + 
        lightDirNorm * (minZ + maxZ) * 0.5f;
    
    // Calculate the light view matrix (look from light toward center)
    glm::mat4 lightView = glm::lookAt(
        lightViewCenter - lightDirNorm * maxZ,  // Position the light far enough
        lightViewCenter,                    // Look at the center
        lightUp                                // Use our calculated up vector
    );
    
    // Calculate the orthographic projection matrix
    float width = maxX - minX;
    float height = maxY - minY;
    float depth = maxZ - minZ;
    
    // Adjust to maintain texel-to-world ratio for shadow map resolution
    float texelsPerUnit = shadowMapSize / width;
    float adjustedWidth = width;
    float adjustedHeight = height;
    
    // Ensure square aspect ratio for the shadow map
    if (width > height) {
        adjustedHeight = width;
    } else {
        adjustedWidth = height;
    }
    
    // Create the orthographic projection matrix
    glm::mat4 lightProjection = glm::ortho(
        -adjustedWidth * 0.5f, adjustedWidth * 0.5f,
        -adjustedHeight * 0.5f, adjustedHeight * 0.5f,
        0.0f, depth
    );
    
    // Return the combined light view-projection matrix
    return lightProjection * lightView;
}
}