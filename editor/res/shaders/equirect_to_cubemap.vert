#version 450

layout(location = 0) out vec3 vWorldDir;

layout(push_constant) uniform PushConstants {
    mat4 view;
    mat4 proj;
    uint hdrImageId;
} pc;

// Hardcoded fullscreen triangle positions in clip space
vec2 positions[3] = vec2[](
    vec2(-1.0, -1.0),
    vec2(3.0, -1.0),
    vec2(-1.0, 3.0)
);

// Corresponding world directions for cubemap sampling
vec3 directions[3] = vec3[](
    vec3(-1.0, -1.0, 1.0),
    vec3( 3.0, -1.0, 1.0),
    vec3(-1.0,  3.0, 1.0)
);

void main() {
    gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
    // Transform the direction into world space using view matrix
    vWorldDir = (pc.view * vec4(directions[gl_VertexIndex], 0.0)).xyz;
}
