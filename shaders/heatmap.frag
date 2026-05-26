#version 330 core

// heatmap.frag
//
// Samples a 1-channel texture (chi2 values on a parameter grid)
// and maps them to a colour ramp matching the reference image:
//   black (min) → purple → red → orange → yellow (max)
// This is the "plasma" / "hot" colormap style.

in  vec2 vNDC;
out vec4 FragColor;

uniform sampler2D uHeatmap;   // R32F texture, chi2 values [0..1] normalised
uniform float     uChi2Min;   // actual chi2 min (for legend)
uniform float     uChi2Max;   // actual chi2 max (for legend)

// Plasma-style colour ramp (matches the reference image)
vec3 plasma(float t)
{
    // 5-stop ramp: black → deep purple → magenta → red → orange → yellow
    vec3 c0 = vec3(0.00, 0.00, 0.00);   // black
    vec3 c1 = vec3(0.30, 0.00, 0.50);   // deep purple
    vec3 c2 = vec3(0.70, 0.00, 0.40);   // magenta-red
    vec3 c3 = vec3(0.95, 0.20, 0.00);   // orange-red
    vec3 c4 = vec3(1.00, 0.85, 0.00);   // yellow

    if      (t < 0.25) return mix(c0, c1, t / 0.25);
    else if (t < 0.50) return mix(c1, c2, (t-0.25)/0.25);
    else if (t < 0.75) return mix(c2, c3, (t-0.50)/0.25);
    else               return mix(c3, c4, (t-0.75)/0.25);
}

void main()
{
    // Map NDC [-1,+1] → UV [0,1]
    vec2 uv = vNDC * 0.5 + 0.5;

    float val = texture(uHeatmap, uv).r;   // already normalised [0,1]

    // Apply sqrt compression so low values are visible (like the image)
    float compressed = sqrt(val);

    vec3 col = plasma(compressed);
    FragColor = vec4(col, 1.0);
}
