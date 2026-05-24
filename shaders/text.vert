#version 330 core

// text.vert – screen-space bitmap text
//
// Vertices are in NDC (Normalised Device Coordinates) [-1,+1].
// Each quad carries a UV to sample the glyph atlas texture.

layout (location = 0) in vec2 aPos;   // NDC position
layout (location = 1) in vec2 aUV;    // atlas texture coordinate

out vec2 vUV;

void main()
{
    gl_Position = vec4(aPos, 0.0, 1.0);
    vUV = aUV;
}
