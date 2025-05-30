#version 450

#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_nonuniform_qualifier : enable

#include "bindless.glsl"
#include "vertex.glsl"

layout(push_constant) uniform PushConstants {
    mat4 view;
    mat4 proj;
    VertexBuffer vertexBuffer;
    float roughness;
    uint numSamples;
    uint envMapId;
} pc;

layout(location = 0) in vec3 localPos;
layout(location = 0) out vec4 fragColor;

const float PI = 3.14159265359;
const float MAX_LOD = 4.0; // Adjust if your env map has more/less mip levels

// === Hammersley Sequence ===
float radicalInverse_VdC(uint bits) {
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10; // 1 / 0x100000000
}

vec2 hammersley(uint i, uint N) {
    return vec2(float(i)/float(N), radicalInverse_VdC(i));
}

// === Importance Sampling ===
vec3 importanceSampleGGX(vec2 Xi, vec3 N, float roughness) {
    float a = roughness * roughness;

    float phi = 2.0 * PI * Xi.x;
    float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a * a - 1.0) * Xi.y));
    float sinTheta = sqrt(1.0 - cosTheta * cosTheta);

    vec3 H = vec3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);

    vec3 up = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
    vec3 tangent = normalize(cross(up, N));
    vec3 bitangent = cross(N, tangent);

    return normalize(tangent * H.x + bitangent * H.y + N * H.z);
}

void main() {
    vec3 N = normalize(localPos) * 0.99; // Clamp to prevent edge seams
    vec3 R = N;
    vec3 V = R;

    const uint SAMPLE_COUNT = pc.numSamples;
    vec3 prefilteredColor = vec3(0.0);
    float totalWeight = 0.0;

    for (uint i = 0u; i < SAMPLE_COUNT; ++i) {
        vec2 Xi = hammersley(i, SAMPLE_COUNT);
        vec3 H = importanceSampleGGX(Xi, N, pc.roughness);
        vec3 L = normalize(2.0 * dot(V, H) * H - V);
        L *= 0.99; // Prevent cross-face seam artifacts

        float NdotL = max(dot(N, L), 0.0);
        if (NdotL > 0.0) {
            vec3 s = sampleCubeLod(pc.envMapId, L, pc.roughness * MAX_LOD).rgb;
            prefilteredColor += s * NdotL;
            totalWeight += NdotL;
        }
    }

    prefilteredColor /= max(totalWeight, 0.001);
    fragColor = vec4(prefilteredColor, 1.0);
}
