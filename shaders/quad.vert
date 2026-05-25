#version 330 core

// quad.vert
// Shared vertex shader for ALL fullscreen post-process passes:
//   background, bloom_extract, bloom_blur, bloom_composite.
//
// A single hardcoded fullscreen triangle (NDC) avoids uploading
// any vertex data – just call glDrawArrays(GL_TRIANGLES, 0, 3).

void main()
{
    // Emit a triangle that covers the entire NDC cube
    vec2 pos[3];
    pos[0] = vec2(-1.0, -1.0);
    pos[1] = vec2( 3.0, -1.0);
    pos[2] = vec2(-1.0,  3.0);

    gl_Position = vec4(pos[gl_VertexID], 0.0, 1.0);
}
