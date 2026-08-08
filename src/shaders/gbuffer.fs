#version 330

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;
uniform vec4 colDiffuse;

layout(location = 0) out vec4 outAlbedo;
layout(location = 1) out vec4 outNormal;

void main()
{
    vec4 color = texture(texture0, fragTexCoord) * colDiffuse * fragColor;

    outAlbedo = color;

    // Flat 2D surface pointing toward the camera
    outNormal = vec4(0.5, 0.5, 1.0, 1.0);
}
