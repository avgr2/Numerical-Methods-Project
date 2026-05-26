#version 330 core

// heatmap.vert
// Fullscreen quad. Passes NDC position to fragment shader
// so it can map back to parameter space.

out vec2 vNDC;

void main()
{
    vec2 pos[3];
    pos[0] = vec2(-1.0, -1.0);
    pos[1] = vec2( 3.0, -1.0);
    pos[2] = vec2(-1.0,  3.0);
    gl_Position = vec4(pos[gl_VertexID], 0.0, 1.0);
    vNDC = pos[gl_VertexID];
}
