#version 330

in vec2 fragTexCoord;

uniform sampler2D texture0;
uniform vec2 resolution;
uniform vec2 direction;

out vec4 finalColor;

void main()
{
    vec2 texelSize = 1.0 / resolution;

    vec2 offset = direction * texelSize;

    float result = 0.0;

    result += texture(texture0, fragTexCoord - offset * 4.0).r * 0.05;
    result += texture(texture0, fragTexCoord - offset * 3.0).r * 0.09;
    result += texture(texture0, fragTexCoord - offset * 2.0).r * 0.12;
    result += texture(texture0, fragTexCoord - offset).r       * 0.15;

    result += texture(texture0, fragTexCoord).r                * 0.18;

    result += texture(texture0, fragTexCoord + offset).r       * 0.15;
    result += texture(texture0, fragTexCoord + offset * 2.0).r * 0.12;
    result += texture(texture0, fragTexCoord + offset * 3.0).r * 0.09;
    result += texture(texture0, fragTexCoord + offset * 4.0).r * 0.05;

    finalColor = vec4(result, result, result, 1.0);
}