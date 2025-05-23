#version 460

#extension GL_EXT_scalar_block_layout: require

#include "bindless.glsl"

layout(location = 0) in vec3 vWorldDir;
layout(location = 0) out vec4 outColor;

layout(push_constant) uniform PushConstants {
    mat4 view;
    mat4 proj;
    uint hdrImageId;
} pc;

vec2 sampleSphericalMap(vec3 dir) {
    vec2 uv = vec2(atan(dir.z, dir.x), asin(dir.y));
    uv *= vec2(0.1591, 0.3183); // = (1 / (2π), 1 / π)
    uv += 0.5;
    return uv;
}

void main() {
    vec2 uv = sampleSphericalMap(normalize(vWorldDir));
    outColor = sampleTexture2DLinear(pc.hdrImageId, uv);
}
