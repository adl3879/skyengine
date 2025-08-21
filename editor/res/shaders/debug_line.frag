#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragUV;

layout(location = 0) out vec4 outColor;

void main() {
    // Simple, reliable line rendering
    float distToLine = abs(fragUV.y - 0.5);
    
    // Since we're scaling the geometry to maintain pixel thickness,
    // we can use a fixed UV threshold
    float lineHalfWidth = 0.4;
    float smoothness = 0.05; // Small smoothing for anti-aliasing
    
    float alpha = 1.0 - smoothstep(lineHalfWidth - smoothness, lineHalfWidth + smoothness, distToLine);
    
    outColor = vec4(fragColor, 1.f);
}