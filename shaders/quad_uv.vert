#version 330 core

// quad_uv.vert
// Same fullscreen triangle trick as quad.vert but also outputs vUV
// for shaders that need to sample a texture (bloom passes, background).

out vec2 vUV;

void main()
{
    vec2 pos[3];
    pos[0] = vec2(-1.0, -1.0);
    pos[1] = vec2( 3.0, -1.0);
    pos[2] = vec2(-1.0,  3.0);

    gl_Position = vec4(pos[gl_VertexID], 0.0, 1.0);

    // UV matches NDC remapped to [0,1]
    vUV = pos[gl_VertexID] * 0.5 + 0.5;
}
