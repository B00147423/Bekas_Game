#version 330

in vec2 fragTexCoord;

uniform sampler2D occlusionTexture;

uniform vec2 lightPos;
uniform vec2 resolution;
uniform float lightRadius;

out vec4 finalColor;

void main()
{
    vec2 fragPos = vec2(
        fragTexCoord.x * resolution.x,
        (1.0 - fragTexCoord.y) * resolution.y
    );

    float distanceToLight = distance(fragPos, lightPos);

    // Outside the light radius = shadow
    if (distanceToLight > lightRadius)
    {
        finalColor = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }

    vec2 ray = fragPos - lightPos;

    const int STEPS = 64;

    float visibility = 1.0;

    for (int i = 1; i < STEPS; ++i)
    {
        float t = float(i) / float(STEPS);

        vec2 samplePos = lightPos + ray * t;

        vec2 uv = samplePos / resolution;

        // RenderTexture is vertically flipped
        uv.y = 1.0 - uv.y;

        float blocker = texture(occlusionTexture, uv).r;

        if (blocker > 0.5)
        {
            visibility = 0.0;
            break;
        }
    }

    finalColor = vec4(
        visibility,
        visibility,
        visibility,
        1.0
    );
}