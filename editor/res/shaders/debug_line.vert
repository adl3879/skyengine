#version 450

#include "scene_data.glsl"

layout (push_constant) uniform constants
{
    SceneDataBuffer sceneData;
} pcs;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inUV;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragUV;

void main() {
    gl_Position = pcs.sceneData.viewProj * vec4(inPosition, 1.0);
    fragColor = inColor;
    fragUV = inUV;
}