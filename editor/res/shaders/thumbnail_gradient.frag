#version 460

layout (location = 0) in vec2 inUV;

layout (location = 0) out vec4 outFragColor;

void main()
{
    // Interpolate between colorBottom and colorTop based on the y-coordinate
    vec3 colorTopLeft = vec3(0.4, 1.0, 1.0);
    vec3 colorBottomRight = vec3(0.9, 1.0, 1.0);

    // Calculate interpolation factor
    float mixFactor = dot(inUV, vec2(0.5));
    vec3 gradientColor = mix(colorTopLeft, colorBottomRight, mixFactor);
    outFragColor = vec4(gradientColor, 1.0);
    // make it darker
    outFragColor = vec4(gradientColor * 0.3, 1.0);
}