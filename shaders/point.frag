#version 330 core

// ──────────────────────────────────────────────────────────────────────────
//  point.frag  –  circular observation marker with soft glow
//
//  gl_PointCoord gives (u,v) in [0,1]² centred at (0.5, 0.5).
//  We compute distance to the centre and:
//    r < 0.30   → solid white core
//    0.30–0.50  → orange halo fading to transparent
// ──────────────────────────────────────────────────────────────────────────

out vec4 FragColor;

void main()
{
    // Distance from pixel to point centre  (in [0, 0.5])
    vec2  uv   = gl_PointCoord - vec2(0.5);
    float dist = length(uv);

    // Discard corners of the point sprite square
    if (dist > 0.5) discard;

    // Colours
    vec3 coreCol = vec3(1.0, 1.0, 1.0);           // white core
    vec3 haloCol = vec3(1.0, 0.55, 0.05);          // orange halo

    float alpha;
    vec3  col;

    if (dist < 0.25)
    {
        // Solid white core with slight anti-aliased edge
        float edge = smoothstep(0.22, 0.25, dist);
        col   = mix(coreCol, haloCol, edge);
        alpha = 1.0;
    }
    else
    {
        // Halo fades out from dist=0.25 to 0.50
        col   = haloCol;
        alpha = 1.0 - smoothstep(0.25, 0.50, dist);
    }

    FragColor = vec4(col, alpha);
}
