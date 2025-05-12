#version 460

#extension GL_EXT_scalar_block_layout: require

layout (location = 0) in vec2 inUV;
layout (location = 0) out vec4 outColor;

#include "bindless.glsl"
#include "scene_data.glsl"

layout (push_constant, scalar) uniform constants
{
    SceneDataBuffer sceneDataBuffer;
    uint drawImageId;
    uint depthImageId;
} pcs;

void main() {
    vec4 color = sampleTexture2DLinear(pcs.drawImageId, inUV);
    float depth = sampleTexture2DLinear(pcs.depthImageId, inUV).r;
    
    // Simple tone mapping (exposure adjustment)
    float exposure = 1.0;
    color.rgb = 1.0 - exp(-color.rgb * exposure);
    
    outColor = color;
    // outColor = vec4(1.0, 0.0, 0.0, 1.0);
}