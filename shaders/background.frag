#version 330 core

// background.frag
// Draws a scientific dark background:
//   - Deep navy blue centre (like a real oscilloscope / plotting tool)
//   - Dark vignette toward edges
//   - Subtle horizontal scanline texture for depth

in  vec2 vUV;
out vec4 FragColor;

void main()
{
    // ── Radial gradient ─────────────────────────────────────────────
    // Distance from centre of screen
    vec2  centre = vec2(0.5, 0.5);
    float dist   = length(vUV - centre);

    // Inner colour : deep navy  #060D1F
    vec3 cInner = vec3(0.024, 0.051, 0.122);
    // Outer colour : near-black #020408
    vec3 cOuter = vec3(0.008, 0.016, 0.031);

    // Smooth radial blend – pow() controls falloff sharpness
    float t   = smoothstep(0.0, 0.75, dist);
    vec3  col = mix(cInner, cOuter, t);

    // ── Vignette ────────────────────────────────────────────────────
    // Extra darkening near corners
    float vign = 1.0 - smoothstep(0.55, 1.0, dist * 1.35);
    col *= vign;

    // ── Subtle scanline / grid micro-texture ────────────────────────
    // Very faint horizontal lines every 3 pixels to give depth
    // Uses screen-space derivative to stay pixel-accurate
    float line = mod(gl_FragCoord.y, 3.0);
    float scan = (line < 1.0) ? 0.93 : 1.0;
    col *= scan;

    FragColor = vec4(col, 1.0);
}
