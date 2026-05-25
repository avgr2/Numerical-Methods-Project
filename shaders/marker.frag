#version 330 core

// marker.frag
//
// Draws scientific annotation markers entirely in GLSL using gl_PointCoord.
//
// Shape 0 (y_max)   : ✚  bright green cross / plus sign
// Shape 1 (impact)  : ▼  red downward-pointing triangle
//
// Both shapes have:
//   - A white 1px outline for legibility on any background
//   - A filled interior in the shape colour
//   - Soft anti-aliased edges via smoothstep

in  float vShape;
out vec4  FragColor;

// ── Signed distance functions in point-sprite space ────────────────────
// uv is in [-0.5, +0.5]

// SDF for a cross / plus (arm half-width = hw)
float sdCross(vec2 uv, float hw)
{
    vec2 d = abs(uv);
    return min(max(d.x - hw, d.y - 0.40),
               max(d.y - hw, d.x - 0.40));
}

// SDF for a downward equilateral triangle (inscribed in radius r)
float sdTriangleDown(vec2 uv, float r)
{
    uv.y = -uv.y;   // flip so triangle points down
    const float k = sqrt(3.0);
    uv.x  = abs(uv.x) - r;
    uv.y  = uv.y + r / k;
    if (uv.x + k * uv.y > 0.0)
        uv = vec2(uv.x - k * uv.y, -k * uv.x - uv.y) / 2.0;
    uv.x -= clamp(uv.x, -2.0 * r, 0.0);
    return -length(uv) * sign(uv.y);
}

void main()
{
    // gl_PointCoord in [0,1] → remap to [-0.5, +0.5]
    vec2 uv = gl_PointCoord - vec2(0.5);

    // Discard outside the point square (circular clip with radius 0.5)
    if (length(uv) > 0.50) discard;

    float sdf;
    vec3  fillCol;

    if (vShape < 0.5)
    {
        // ── y_max marker : bright green cross ──────────────────────
        sdf     = sdCross(uv, 0.10);
        fillCol = vec3(0.20, 1.00, 0.40);   // vivid green
    }
    else
    {
        // ── x_impact marker : red downward triangle ─────────────────
        sdf     = sdTriangleDown(uv * 1.15, 0.38);
        fillCol = vec3(1.00, 0.25, 0.20);   // vivid red
    }

    // White outline ring  (sdf slightly positive = outline band)
    float outline = smoothstep( 0.02, -0.01, sdf - 0.04);
    float fill    = smoothstep( 0.02, -0.01, sdf);

    vec3  col   = mix(vec3(1.0), fillCol, fill);
    float alpha = outline;

    if (alpha < 0.01) discard;

    FragColor = vec4(col, alpha);
}
