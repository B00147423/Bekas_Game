#version 330

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;
uniform sampler2D normalTexture;
uniform sampler2D shadowTexture;

uniform vec2 lightPos;
uniform vec2 resolution;
uniform float lightRadius;
uniform vec3 lightColor;
uniform float lightIntensity;
uniform float ambient;
uniform int useShadows;
uniform int useScreenPosition;

out vec4 finalColor;

void main()
{
    vec4 albedo = texture(texture0, fragTexCoord) * fragColor;

    vec2 fragPos;

    if (useScreenPosition == 1)
    {
        // Direct sprite drawing: trees
        fragPos = vec2(gl_FragCoord.x, resolution.y - gl_FragCoord.y);
    }
    else
    {
        // Fullscreen G-buffer
        fragPos = vec2(fragTexCoord.x * resolution.x, (1.0 - fragTexCoord.y) * resolution.y);
    }
    float dist = distance(fragPos, lightPos);

    float attenuation = clamp(1.0 - (dist / lightRadius), 0.0, 1.0);

    attenuation *= attenuation;

    // Already calculated by shadow.fs and then blurred.

    if (useShadows == 1)
    {
        float visibility = texture(shadowTexture, fragTexCoord).r;
        attenuation *= visibility;
    }

    vec3 lighting =lightColor * lightIntensity * attenuation + vec3(ambient);

    finalColor = vec4(albedo.rgb * lighting, albedo.a);
}