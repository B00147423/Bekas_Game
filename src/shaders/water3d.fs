#version 330

in vec3 fragNormal;
in vec2 fragWorld;

uniform sampler2D sceneTexture;
uniform sampler2D groundTexture;
uniform sampler2D waterMask;

uniform vec2 resolution;
uniform vec2 captureOrigin;
uniform vec4 waterMaskBounds;
uniform float waterTop;
uniform float time;

out vec4 finalColor;

void main() {
    // Keep the water inside the pond.
    vec2 maskUV = (fragWorld - waterMaskBounds.xy) / waterMaskBounds.zw;
    // This is an uploaded image, not a render target: keep its top-down Y.

    vec2 coverageDepth = texture(waterMask, maskUV).rg;
    float mask = coverageDepth.r;
    float edgeWidth = max(fwidth(mask), 0.001);

    float alpha = smoothstep(
        0.5 - edgeWidth,
        0.5 + edgeWidth,
        mask
    );

    if (alpha <= 0.001) discard;

    // World-aligned capture coordinates.
    vec2 uv = (fragWorld - captureOrigin) / resolution;
    uv = clamp(uv, vec2(0.001), vec2(0.999));
    uv.y = 1.0 - uv.y;

    // Underwater ground.
    vec3 groundColor = texture(groundTexture, uv).rgb;

    vec3 waterColor = mix(
        groundColor,
        vec3(0.075, 0.20, 0.22),
        mix(0.32, 0.94, smoothstep(0.0, 1.0, coverageDepth.g))
    );

    // Existing sprite reflections.
    vec4 reflection = texture(sceneTexture, uv);

    float reflectionAlpha = clamp(reflection.a, 0.0, 1.0);

    if (reflectionAlpha > 0.001) {
        vec3 reflectionColor = reflection.rgb / reflectionAlpha;

        waterColor = mix(
            waterColor,
            reflectionColor,
            reflectionAlpha
        );
    }

    finalColor = vec4(waterColor, alpha);
}
