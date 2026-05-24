#version 330 core

// grid.frag – major axes brighter, minor grid lines dim

in  float vType;
out vec4  FragColor;

void main()
{
    if (vType > 0.5)
    {
        // Major axis (x=0 or y=0) : bright white
        FragColor = vec4(0.85, 0.85, 0.85, 1.0);
    }
    else
    {
        // Minor grid lines : dim blue-grey
        FragColor = vec4(0.20, 0.22, 0.35, 1.0);
    }
}
