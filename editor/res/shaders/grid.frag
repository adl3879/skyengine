#version 460

#extension GL_GOOGLE_include_directive : require

#include "scene_data.glsl"

layout (push_constant) uniform constants
{
    SceneDataBuffer sceneData;
} pcs;

layout (location = 0) out vec4 outFragColor;

layout (location = 0) in vec3 worldPos;

void main() {
    const float thicknessNorm = 0.60f; // grid line size (in pixels)
    const float thicknessBold = 2.0 * thicknessNorm;
    const float frequencyBold = 10.0;
    const float maxCameraDist = 500.0; // Increased from 100.0 to allow viewing from further away
    const vec3 colorDefault   = vec3(0.10, 0.10, 0.10);
    const vec3 colorBold      = vec3(0.85, 0.85, 0.85);
    const vec3 colorAxisX     = vec3(0.85, 0.40, 0.30);
    const vec3 colorAxisZ     = vec3(0.40, 0.50, 0.85);
    
    // Improved anti-aliasing for grid lines
    // Get the rate of change of the world position for better AA
    vec2 derivative = fwidth(worldPos.xz);
    // Increase the AA factor for smoother lines
    vec2 gridAA     = derivative * 2.5; // Increased from 1.5 for smoother edges
    vec2 gridUV     = 1.0 - abs(fract(worldPos.xz) * 2.0 - 1.0);
    vec2 lineWidth  = thicknessNorm * derivative;
    vec2 drawWidth  = clamp(lineWidth, derivative, vec2(0.5));
    // Use a wider smoothstep range for better anti-aliasing
    vec2 gridTest   = 1.0 - smoothstep(drawWidth - gridAA, drawWidth + gridAA, gridUV);
    gridTest       *= clamp(lineWidth / drawWidth, 0.0, 1.0);
    
    float gridNorm = mix(gridTest.x, 1.0, gridTest.y);
    float alphaGridNorm = clamp(gridNorm, 0.0, 0.8);
    
    // Improved anti-aliasing for bold grid lines
    derivative = fwidth(worldPos.xz / frequencyBold);
    gridAA     = derivative * 2.5; // Increased for smoother edges
    gridUV     = 1.0 - abs(fract(worldPos.xz / frequencyBold) * 2.0 - 1.0);
    lineWidth  = thicknessBold * derivative;
    drawWidth  = clamp(lineWidth, derivative, vec2(0.5));
    gridTest   = 1.0 - smoothstep(drawWidth - gridAA, drawWidth + gridAA, gridUV);
    gridTest  *= clamp(lineWidth / drawWidth, 0.0, 1.0);
    
    float gridBold = mix(gridTest.x, 1.0, gridTest.y);
    float alphaGridBold = clamp(gridBold, 0.0, 0.9);
    
    // Final grid alpha
    float alphaGrid = max(alphaGridNorm, alphaGridBold);
    
    // Color Test
    vec3 colorOutput = mix(colorDefault, colorBold, gridBold);
    
    // Improved axis highlighting with better anti-aliasing
    float axisWidth = 0.02; // Width of the axis lines
    float axisAA = fwidth(worldPos.z) * 3.0; // Increased AA factor for axes
    float alignAxisX = 1.0 - smoothstep(axisWidth - axisAA, axisWidth + axisAA, abs(worldPos.z));
    
    axisAA = fwidth(worldPos.x) * 3.0;
    float alignAxisZ = 1.0 - smoothstep(axisWidth - axisAA, axisWidth + axisAA, abs(worldPos.x));
    
    float sum = clamp(alignAxisX + alignAxisZ, 0.0, 1.0);
    
    colorOutput = mix(colorOutput, colorAxisZ, alignAxisZ);
    colorOutput = mix(colorOutput, colorAxisX, alignAxisX);
    
    // Improved camera distance test with smoother falloff
    float cameraDist = length(pcs.sceneData.cameraPos.xz - worldPos.xz);
    // Use a quadratic falloff for more natural distance fading
    float alphaDist = 1.0 - smoothstep(0.0, maxCameraDist, cameraDist * cameraDist / maxCameraDist);
    
    // Apply distance-based line thickness scaling to keep lines visible at distance
    float distanceScale = mix(1.0, 0.5, smoothstep(0.0, maxCameraDist * 0.5, cameraDist));
    alphaGrid *= distanceScale;
    
    float alpha = min(alphaDist, alphaGrid);
    
    outFragColor = vec4(colorOutput, alpha); 
}