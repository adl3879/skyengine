#version 460

#extension GL_GOOGLE_include_directive : require

#include "scene_data.glsl"
// #include "vertex.glsl"

layout (location = 0) in vec2 inPosition;
layout (location = 1) in vec2 inUV;
layout (location = 2) in vec4 inColor;
layout (location = 3) in uint inTextureId;

layout (location = 0) out vec2 outUV;
layout (location = 1) out vec4 outColor;
layout (location = 2) flat out uint outTextureIndex;

layout (push_constant) uniform constants
{
    SceneDataBuffer sceneData;
} pcs;

void main()
{
    gl_Position = pcs.sceneData.viewProj * vec4(inPosition, 0.0f, 1.0f);

    outUV = inUV;
    outColor = inColor;
    outTextureIndex = inTextureId;
}