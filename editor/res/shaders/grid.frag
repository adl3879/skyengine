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
    const float maxCameraDist = 100.0;
    const vec3 colorDefault   = vec3(0.10, 0.10, 0.10);
    const vec3 colorBold      = vec3(0.85, 0.85, 0.85);
    const vec3 colorAxisX     = vec3(0.85, 0.40, 0.30);
    const vec3 colorAxisZ     = vec3(0.40, 0.50, 0.85);
    
    // Grid Test Normal
    vec2 derivative = fwidth(worldPos.xz);
    vec2 gridAA     = derivative * 1.5;
    vec2 gridUV     = 1.0 - abs(fract(worldPos.xz) * 2.0 - 1.0);
    vec2 lineWidth  = thicknessNorm * derivative;
    vec2 drawWidth  = clamp(lineWidth, derivative, vec2(0.5));
    vec2 gridTest   = 1.0 - smoothstep(drawWidth - gridAA, drawWidth + gridAA, gridUV);
    gridTest       *= clamp(lineWidth / drawWidth, 0.0, 1.0);
    
    float gridNorm = mix(gridTest.x, 1.0, gridTest.y);
    float alphaGridNorm = clamp(gridNorm, 0.0, 0.8);
    
    // Grid Test Bold
    derivative = fwidth(worldPos.xz / frequencyBold);
    gridAA     = derivative * 1.5;
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
    
    float alignAxisX = step(abs(worldPos.z), gridTest.y);
    float alignAxisZ = step(abs(worldPos.x), gridTest.x);
    float sum = clamp(alignAxisX + alignAxisZ, 0.0, 1.0);
    
    colorOutput = mix(colorOutput, colorAxisZ, alignAxisZ);
    colorOutput = mix(colorOutput, colorAxisX, alignAxisX);
    
    // Camera distance test
    float cameraDist = length(pcs.sceneData.cameraPos.xz - worldPos.xz);
    float alphaDist = 1.0 - smoothstep(0.0, maxCameraDist, cameraDist);
    
    float alpha = min(alphaDist, alphaGrid);
    
    outFragColor = vec4(colorOutput, alpha); 
}