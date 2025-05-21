#version 460

#extension GL_EXT_scalar_block_layout: require

layout (location = 0) in vec2 inUV;
layout (location = 0) out vec4 outColor;

#include "bindless.glsl"

layout (push_constant, scalar) uniform constants
{
    uint hdrImageId;
} pcs;

void main() {
    vec4 hdrColor = sampleTexture2DLinear(pcs.hdrImageId, inUV);
    
    // Simple tone mapping (exposure adjustment)
    float exposure = 1.0;
    hdrColor.rgb = 1.0 - exp(-hdrColor.rgb * exposure);
    
    outColor = clamp(hdrColor, 0.0, 1.0);
}