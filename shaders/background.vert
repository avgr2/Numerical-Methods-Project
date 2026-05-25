#version 330 core

// background.vert
// Fullscreen quad in NDC [-1,+1]. No projection needed.

layout (location = 0) in vec2 aPos;

out vec2 vUV;   // [0,1] for gradient computation

void main()
{
    gl_Position = vec4(aPos, 0.0, 1.0);
    vUV = aPos * 0.5 + 0.5;   // remap [-1,1] → [0,1]
}
