#version 330 core

// text.frag – sample the 1-channel glyph atlas
//
// White text on transparent background.
// uColor lets the caller tint individual lines.

in  vec2 vUV;
out vec4 FragColor;

uniform sampler2D uAtlas;
uniform vec3      uColor;   // text colour  (default white)

void main()
{
    float mask = texture(uAtlas, vUV).r;
    if (mask < 0.1) discard;
    FragColor = vec4(uColor, mask);
}
