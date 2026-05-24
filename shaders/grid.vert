#version 330 core

// grid.vert
// Each vertex: vec2 position + float type (0=minor, 1=major axis)
// Segments are uploaded as GL_LINES pairs, so no zigzag.

layout (location = 0) in vec2  aPos;
layout (location = 1) in float aType;

uniform mat4 uProjection;

out float vType;

void main()
{
    gl_Position = uProjection * vec4(aPos, 0.0, 1.0);
    vType = aType;
}
