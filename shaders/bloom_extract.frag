#version 330 core

// bloom_extract.frag
//
// Pass 1 of bloom:
//   Samples the scene texture and EXTRACTS only bright pixels
//   (those above a luminance threshold).
//   Output is fed into the Gaussian blur passes.
//
// Bright pixels → kept at full intensity
// Dark  pixels  → clamped to black
//
// This prevents dark parts of the scene from blooming.

in  vec2 vUV;
out vec4 FragColor;

uniform sampler2D uScene;
uniform float     uThreshold;   // luminance threshold  (default 0.35)

void main()
{
    vec3 col = texture(uScene, vUV).rgb;

    // Perceived luminance (BT.601)
    float lum = dot(col, vec3(0.299, 0.587, 0.114));

    // Smooth threshold so there's no hard cutoff artefact
    float factor = smoothstep(uThreshold, uThreshold + 0.15, lum);

    FragColor = vec4(col * factor, 1.0);
}
