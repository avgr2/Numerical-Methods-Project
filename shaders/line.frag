#version 330 core

// ──────────────────────────────────────────────────────────────────────────
//  line.frag  –  trajectory colour  (speed → colour ramp)
//
//  Colour ramp (scientific style, inspired by matplotlib "plasma"):
//    t = 0  (slow)   →  deep blue     #1a1aff
//    t = 0.4         →  cyan          #00e5ff
//    t = 0.7         →  yellow-green  #aaff00
//    t = 1  (fast)   →  orange-red    #ff6600
// ──────────────────────────────────────────────────────────────────────────

in  float vT;
out vec4  FragColor;

// Simple 4-stop colour ramp
vec3 speedColor(float t)
{
    // Stops
    vec3 c0 = vec3(0.102, 0.102, 1.000);  // blue
    vec3 c1 = vec3(0.000, 0.898, 1.000);  // cyan
    vec3 c2 = vec3(0.667, 1.000, 0.000);  // yellow-green
    vec3 c3 = vec3(1.000, 0.400, 0.000);  // orange

    if (t < 0.333)
        return mix(c0, c1, t / 0.333);
    else if (t < 0.667)
        return mix(c1, c2, (t - 0.333) / 0.334);
    else
        return mix(c2, c3, (t - 0.667) / 0.333);
}

void main()
{
    vec3 col = speedColor(vT);

    // Slight brightness boost so the line pops on the dark background
    col = pow(col, vec3(0.85));

    FragColor = vec4(col, 1.0);
}
