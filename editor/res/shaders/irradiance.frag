#version 450
#extension GL_EXT_scalar_block_layout : require

#include "bindless.glsl"
#include "vertex.glsl"

layout(location = 0) in vec3 worldPos;
layout(location = 0) out vec4 fragColor;

layout(push_constant) uniform PushConstants {
    mat4 view;
    mat4 proj;
    VertexBuffer vertexBuffer;
    uint envMapId;
} pc;

const float PI = 3.14159265359;

vec3 sampleSphericalMap(vec3 v) {
    vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
    uv *= vec2(0.1591, 0.3183); // inv(2*PI), inv(PI)
    uv += 0.5;
    return sampleTexture2DLinear(pc.envMapId, uv).rgb;
}

void main() {
    // The world position represents the direction from center of cube
    vec3 N = normalize(worldPos);
    
    vec3 irradiance = vec3(0.0);
    
    // Tangent space calculation
    vec3 up = vec3(0.0, 1.0, 0.0);
    vec3 right = normalize(cross(up, N));
    up = normalize(cross(N, right));
    
    float sampleDelta = 0.025;
    float nrSamples = 0.0;
    
    // Convolve environment map over hemisphere
    for(float phi = 0.0; phi < 2.0 * PI; phi += sampleDelta) {
        for(float theta = 0.0; theta < 0.5 * PI; theta += sampleDelta) {
            // Spherical to cartesian (in tangent space)
            vec3 tangentSample = vec3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));
            
            // Tangent space to world
            vec3 sampleVec = tangentSample.x * right + tangentSample.y * up + tangentSample.z * N;
            
            // Sample environment map
            irradiance += sampleSphericalMap(sampleVec) * cos(theta) * sin(theta);
            nrSamples++;
        }
    }
    
    irradiance = PI * irradiance * (1.0 / float(nrSamples));
    
    fragColor = vec4(irradiance, 1.0);
}