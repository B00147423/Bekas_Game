#version 330

in vec2 fragTexCoord;

uniform sampler2D texture0;
uniform vec2 resolution;
uniform vec2 direction;

out vec4 finalColor;

void main()
{
    vec2 texturePixelSize = 1.0 / resolution;

    vec2 sampleOffset = direction * texturePixelSize;

    float blurredVisibility = 0.0;

    blurredVisibility += texture(texture0, fragTexCoord - sampleOffset * 4.0).r * 0.05;
    blurredVisibility += texture(texture0, fragTexCoord - sampleOffset * 3.0).r * 0.09;
    blurredVisibility += texture(texture0, fragTexCoord - sampleOffset * 2.0).r * 0.12;
    blurredVisibility += texture(texture0, fragTexCoord - sampleOffset).r       * 0.15;

    blurredVisibility += texture(texture0, fragTexCoord).r                * 0.18;

    blurredVisibility += texture(texture0, fragTexCoord + sampleOffset).r       * 0.15;
    blurredVisibility += texture(texture0, fragTexCoord + sampleOffset * 2.0).r * 0.12;
    blurredVisibility += texture(texture0, fragTexCoord + sampleOffset * 3.0).r * 0.09;
    blurredVisibility += texture(texture0, fragTexCoord + sampleOffset * 4.0).r * 0.05;

    finalColor = vec4(blurredVisibility, blurredVisibility, blurredVisibility, 1.0);
}