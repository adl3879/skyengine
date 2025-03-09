#version 460

#extension GL_GOOGLE_include_directive : require

#include "bindless.glsl"
#include "scene_data.glsl"
#include "sprite_pcs.glsl"

#define DEPTH_ARRAY_SCALE 512

layout(set = 1, binding = 0) buffer writeonly s_Write_t
{
    uint data[DEPTH_ARRAY_SCALE];
} s_Write;

layout (location = 0) in vec2 inUV;
layout (location = 1) in vec4 inColor;
layout (location = 2) flat in uint inTextureIndex;
layout (location = 3) flat in uint inUniqueId;

layout (location = 0) out vec4 outFragColor;

void main()
{
    // get the depth and scale it up by
    // the total number of buckets in depth array
    uint zIndex = uint(gl_FragCoord.z * DEPTH_ARRAY_SCALE);

    if (length(pcs.sceneData.mousePos - gl_FragCoord.xy) < 1) {
        s_Write.data[0] = inUniqueId;
    }

    outFragColor = sampleTexture2DNearest(inTextureIndex, inUV) * inColor;
}