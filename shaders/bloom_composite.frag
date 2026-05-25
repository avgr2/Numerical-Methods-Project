#version 330 core

// bloom_composite.frag
//
// Pass 4 (final):
//   Combines the original scene texture with the blurred bloom texture.
//   Uses additive blending in the shader so we don't need GL blend state.
//
//   Final colour = scene + bloom * uIntensity
//
//   Tone-map with a simple Reinhard to keep highlights from blowing out.

in  vec2 vUV;
out vec4 FragColor;

uniform sampler2D uScene;       // original render
uniform sampler2D uBloom;       // blurred bright layer
uniform float     uIntensity;   // bloom strength  (default 1.2)

// Reinhard tone mapping (operates per-channel)
vec3 reinhard(vec3 c) { return c / (c + vec3(1.0)); }

void main()
{
    vec3 scene = texture(uScene, vUV).rgb;
    vec3 bloom = texture(uBloom, vUV).rgb;

    vec3 combined = scene + bloom * uIntensity;

    // Tone map so we don't get ugly white clipping
    combined = reinhard(combined);

    // Slight gamma correction  (2.2 display)
    combined = pow(combined, vec3(1.0 / 2.2));

    FragColor = vec4(combined, 1.0);
}
