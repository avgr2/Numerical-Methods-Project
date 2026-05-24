#version 330 core

// comet.frag – glowing head + fading trail
//
// Head (aAlpha ≈ 1) : bright white core + cyan halo
// Tail (aAlpha ≈ 0) : small dim cyan dots

in  float vAlpha;
out vec4  FragColor;

void main()
{
    vec2  uv   = gl_PointCoord - vec2(0.5);
    float dist = length(uv);

    if (dist > 0.5) discard;

    // Colour : white core blending into cyan
    vec3 coreCol = vec3(1.00, 1.00, 1.00);
    vec3 haloCol = vec3(0.20, 0.90, 1.00);   // bright cyan

    float coreEdge = 0.20 * vAlpha + 0.05;   // core shrinks toward tail
    vec3  col = mix(haloCol, coreCol, smoothstep(coreEdge + 0.05, coreEdge, dist));

    // Radial fade for the halo
    float radFade = 1.0 - smoothstep(coreEdge, 0.5, dist);

    // Combined alpha : radial fade × trail fade
    float alpha = radFade * vAlpha;
    alpha = pow(alpha, 0.7);   // slight gamma lift so tail stays visible

    FragColor = vec4(col, alpha);
}
