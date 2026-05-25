#version 330 core

// bloom_blur.frag
//
// Pass 2 & 3 of bloom:  separable 9-tap Gaussian blur.
//
//   Pass 2 (horizontal): uDirection = (1/screenW, 0)
//   Pass 3 (vertical)  : uDirection = (0, 1/screenH)
//
// Running horizontal then vertical gives a true 2-D Gaussian kernel
// at the cost of only 2 × 9 = 18 texture samples instead of 81.

in  vec2 vUV;
out vec4 FragColor;

uniform sampler2D uBlurInput;
uniform vec2      uDirection;   // (1/W, 0) or (0, 1/H)

// 9-tap Gaussian weights (sigma ≈ 2.0, normalised)
const float weight[5] = float[](0.227027, 0.194595, 0.121622,
                                 0.054054, 0.016216);

void main()
{
    vec3 result = texture(uBlurInput, vUV).rgb * weight[0];

    for (int i = 1; i < 5; ++i)
    {
        vec2 offset = float(i) * uDirection;
        result += texture(uBlurInput, vUV + offset).rgb * weight[i];
        result += texture(uBlurInput, vUV - offset).rgb * weight[i];
    }

    FragColor = vec4(result, 1.0);
}
