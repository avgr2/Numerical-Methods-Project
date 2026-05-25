#version 330 core

// marker.vert
//
// Special annotation points drawn as large GL_POINTS.
// Each point carries a "shape type" flag:
//   aShape = 0.0 → cross / plus  (y_max marker)
//   aShape = 1.0 → downward triangle (x_impact / ground marker)
//
// Point size is fixed at 22px so the shape has enough pixels to be legible.

layout (location = 0) in vec2  aPos;
layout (location = 1) in float aShape;

uniform mat4 uProjection;

out float vShape;

void main()
{
    gl_Position  = uProjection * vec4(aPos, 0.0, 1.0);
    gl_PointSize = 22.0;
    vShape       = aShape;
}
