#version 330 core

// grid.vert – axis-aligned grid lines in world space
// Each vertex is just a 2D world position.

layout (location = 0) in vec2 aPos;

uniform mat4 uProjection;

// Pass a "type" flag: 0 = minor grid, 1 = major axis
// We encode it as a float attribute at location 1.
layout (location = 1) in float aType;

out float vType;

void main()
{
    gl_Position = uProjection * vec4(aPos, 0.0, 1.0);
    vType = aType;
}
