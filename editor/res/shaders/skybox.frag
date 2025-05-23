#version 450

#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_nonuniform_qualifier : enable

#include "scene_data.glsl"
#include "bindless.glsl"

layout (push_constant, scalar) uniform constants 
{
    SceneDataBuffer sceneData;
    uint cubemapImage;
} pcs;

layout(location = 0) in vec3 inTexCoord;
layout(location = 0) out vec4 outColor;

void main() {
    // Sample the cubemap texture using the helper function from bindless.glsl
    outColor = sampleTextureCubeLinear(pcs.cubemapImage, inTexCoord);
}
