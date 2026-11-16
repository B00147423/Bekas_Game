#version 330

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;

out vec4 finalColor;

// Outside the image counts as transparent.
float SampleSpriteAlpha(vec2 textureCoordinates)
{
    if (any(lessThan(textureCoordinates, vec2(0.0))) || any(greaterThanEqual(textureCoordinates, vec2(1.0))))
        return 0.0;

    return texture(texture0, textureCoordinates).a;
}

void main()
{
    vec4 spriteColor = texture(texture0, fragTexCoord) * fragColor;

    if (spriteColor.a < 0.1)
    {
        finalColor = vec4(0.0);
        return;
    }

    vec2 texturePixelSize = 1.0 / vec2(textureSize(texture0, 0));

    float surroundingAlpha = 1.0;

    for (int offsetY = -1; offsetY <= 1; ++offsetY)
    {
        for (int offsetX = -1; offsetX <= 1; ++offsetX)
        {
            surroundingAlpha = min(surroundingAlpha, SampleSpriteAlpha(fragTexCoord + vec2(offsetX, offsetY) * texturePixelSize));
        }
    }

    // Slightly brighten the original colours.
    vec3 highlightedColor = mix(spriteColor.rgb, vec3(1.0), 0.15);

    // Warm outline along the inside edge of the sprite.
    vec3 outlineColor = vec3(1.0, 0.88, 0.45);
    float edgeAmount = 1.0 - step(0.1, surroundingAlpha);

    finalColor = vec4(mix(highlightedColor, outlineColor, edgeAmount), spriteColor.a);
}