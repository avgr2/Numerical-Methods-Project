#version 330 core

// ──────────────────────────────────────────────────────────────────────────
//  line.vert  –  trajectory line shader
//
//  Attributes:
//    aPos      : (x, y) world-space position
//    aSpeed    : scalar speed at this vertex  (used for colour mapping)
//
//  Uniforms:
//    uProjection : orthographic projection matrix
//    uSpeedMin   : minimum speed in the dataset  (mapped to blue)
//    uSpeedMax   : maximum speed in the dataset  (mapped to orange)
// ──────────────────────────────────────────────────────────────────────────

layout (location = 0) in vec2  aPos;
layout (location = 1) in float aSpeed;

uniform mat4  uProjection;
uniform float uSpeedMin;
uniform float uSpeedMax;

// Passed to fragment shader
out float vT;   // normalised speed  [0, 1]

void main()
{
    gl_Position = uProjection * vec4(aPos, 0.0, 1.0);

    // Clamp and normalise speed
    float range = uSpeedMax - uSpeedMin;
    vT = (range > 0.0)
         ? clamp((aSpeed - uSpeedMin) / range, 0.0, 1.0)
         : 0.5;
}
