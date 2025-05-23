#version 450

#extension GL_GOOGLE_include_directive : require

#include "vertex.glsl"

layout(location = 0) out vec3 vWorldDir;

layout(push_constant) uniform PushConstants {
    mat4 view;
    mat4 proj;
    uint hdrImageId;
    VertexBuffer vertexBuffer;
} pcs;

void main() {
    Vertex v = pcs.vertexBuffer.vertices[gl_VertexIndex];

    vWorldDir = (pcs.view * vec4(v.position, 0.0)).xyz;
    gl_Position = pcs.proj * vec4(v.position, 1.0);
}
