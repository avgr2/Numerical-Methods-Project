#version 330 core

// ──────────────────────────────────────────────────────────────────────────
//  point.vert  –  observation point shader
//
//  Each observed (x, y) is rendered as a large GL_POINT so the fragment
//  shader can discard pixels outside a circle and add a soft glow ring.
// ──────────────────────────────────────────────────────────────────────────

layout (location = 0) in vec2 aPos;

uniform mat4 uProjection;

void main()
{
    gl_Position  = uProjection * vec4(aPos, 0.0, 1.0);
    gl_PointSize = 10.0;   // pixels – adjusted to taste
}
