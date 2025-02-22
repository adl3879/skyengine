#version 460

#extension GL_GOOGLE_include_directive : require

#include "bindless.glsl"
#include "scene_data.glsl"

layout (location = 0) in vec2 inUV;
layout (location = 1) in vec4 inColor;
layout (location = 2) flat in uint inTextureIndex;

layout (location = 0) out vec4 outFragColor;

void main()
{
    // outFragColor = sampleTexture2DLinear(inTextureIndex, inUV) * inColor;
    outFragColor = vec4(1, 0, 0, 1);
}