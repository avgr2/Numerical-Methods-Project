#version 330 core

// comet.vert – animated projectile trail
//
// Each vertex carries:
//   aPos   (x, y)  world position
//   aAlpha  float  fade factor  [0=tail end … 1=head]

layout (location = 0) in vec2  aPos;
layout (location = 1) in float aAlpha;

uniform mat4 uProjection;

out float vAlpha;

void main()
{
    gl_Position  = uProjection * vec4(aPos, 0.0, 1.0);
    gl_PointSize = mix(2.0, 14.0, aAlpha);   // head is large, tail is tiny
    vAlpha = aAlpha;
}
