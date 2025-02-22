#ifndef VERTEX_GLSL
#define VERTEX_GLSL

#extension GL_EXT_buffer_reference : require

struct Vertex {
    vec3 position;
    float uv_x;
    vec3 normal;
    float uv_y;
    vec4 tangent;
};

layout (buffer_reference, std430) readonly buffer VertexBuffer {
	Vertex vertices[];
};

struct QuadVertex {
    vec2 position;
    vec2 uv;
    vec4 color;
    uint textureIndex;
    vec3 padding;
};

layout (buffer_reference, std430) readonly buffer QuadVertexBuffer {
    QuadVertex vertices[];
};

#endif // VERTEX_GLSL
