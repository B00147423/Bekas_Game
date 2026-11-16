
#version 330

in vec3 vertexPosition;

uniform mat4 mvp;
uniform float time;
uniform vec2 tileOrigin;

out vec3 fragNormal;
out vec2 fragWorld;

void main()
{
    vec3 displacedPosition = vertexPosition;

    fragWorld = displacedPosition.xz + tileOrigin;

    float horizontalWavePhase = fragWorld.x * 0.12 + time * 0.9;

    float verticalWavePhase =fragWorld.y * 0.10 - time * 0.7;

    // Animate the water surface height.
    displacedPosition.y += sin(horizontalWavePhase) * 0.18;
    displacedPosition.y += cos(verticalWavePhase) * 0.12;

    // Calculate how the water slopes along each axis.
    float heightSlopeX = cos(horizontalWavePhase) * 0.12 * 0.18;

    float heightSlopeZ =-sin(verticalWavePhase) * 0.10 * 0.12;

    // Calculate the surface normal for lighting.
    fragNormal = normalize(vec3(
        -heightSlopeX,
        1.0,
        -heightSlopeZ
    ));

    gl_Position = mvp * vec4(displacedPosition, 1.0);
}
