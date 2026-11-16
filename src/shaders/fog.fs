#version 330

uniform sampler2D explorationTexture;
uniform vec2 resolution;
uniform vec2 worldTopLeft;
uniform vec2 worldBottomRight;
uniform vec2 worldSize;
uniform vec2 playerPos;
uniform float revealRadius;
uniform float exploredDarkness;
uniform float feather;
out vec4 finalColor;

float GetExplorationVisibility(vec2 worldPosition)
{
    if (any(lessThan(worldPosition, vec2(0))) || any(greaterThanEqual(worldPosition, worldSize)))
        return 0.0;
    ivec2 explorationSize = textureSize(explorationTexture, 0);
    ivec2 explorationPixel = ivec2(worldPosition / worldSize * vec2(explorationSize));
    if (texelFetch(explorationTexture, explorationPixel, 0).r == 0.0) return 0.0;

    vec2 worldUnitsPerPixel = worldSize / vec2(explorationSize);
    float distanceToUnknown = feather;
    ivec2 searchRadius = ivec2(ceil(feather / worldUnitsPerPixel)) + 1;
    for (int neighborOffsetY = -searchRadius.y; neighborOffsetY <= searchRadius.y; ++neighborOffsetY) {
        for (int neighborOffsetX = -searchRadius.x; neighborOffsetX <= searchRadius.x; ++neighborOffsetX) {
            ivec2 neighborPixel = explorationPixel + ivec2(neighborOffsetX, neighborOffsetY);
            bool isOutsideWorld = any(lessThan(neighborPixel, ivec2(0))) || any(greaterThanEqual(neighborPixel, explorationSize));
            if (!isOutsideWorld && texelFetch(explorationTexture, neighborPixel, 0).r > 0.0) continue;
            vec2 neighborWorldMin = vec2(neighborPixel) * worldUnitsPerPixel;
            float distanceToPixel = length(worldPosition - clamp(worldPosition, neighborWorldMin, neighborWorldMin + worldUnitsPerPixel));
            distanceToUnknown = min(distanceToUnknown, distanceToPixel);
        }
    }
    return feather > 0.0 ? smoothstep(0.0, feather, distanceToUnknown) : 1.0;
}

void main()
{
    vec2 screenCoordinates = vec2(gl_FragCoord.x, resolution.y - gl_FragCoord.y) / resolution;
    vec2 worldPosition = mix(worldTopLeft, worldBottomRight, screenCoordinates);
    float exploredAmount = GetExplorationVisibility(worldPosition);
    float playerVisibility = revealRadius > 0.0
        ? 1.0 - smoothstep(max(0.0, revealRadius - 32.0), revealRadius, distance(worldPosition, playerPos))
        : 0.0;
    float fogTransmission = exploredAmount * mix(1.0 - exploredDarkness, 1.0, playerVisibility);
    finalColor = vec4(0, 0, 0, 1.0 - fogTransmission);
}
