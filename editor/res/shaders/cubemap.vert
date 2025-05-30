#version 450

#extension GL_GOOGLE_include_directive : require

#include "vertex.glsl"

layout(location = 0) out vec3 localPos;

layout(push_constant) uniform PushConstants {
    mat4 view;
    mat4 proj;
    VertexBuffer vertexBuffer;
} pcs;

void main() {
    Vertex v = pcs.vertexBuffer.vertices[gl_VertexIndex];

    localPos = v.position;
    gl_Position = vec4(v.position, 1.0);
}
