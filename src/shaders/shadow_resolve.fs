
#version 330

in vec2 fragTexCoord;

uniform sampler2D texture0;
uniform sampler2D samples1;
uniform sampler2D samples2;
uniform sampler2D samples3;

uniform int sampleCount;
uniform float opacity;
uniform float smoothingRadius;

out vec4 finalColor;

// Read and average the packed RGBA shadow samples.
float GetSampleVisibility(vec2 textureCoordinates)
{
    vec4 firstPackedSamples = texture(texture0, textureCoordinates);

    float sampleVisibility = firstPackedSamples.r;

    if (sampleCount >= 4)
        sampleVisibility = dot(firstPackedSamples, vec4(1.0));

    if (sampleCount >= 8)
        sampleVisibility += dot(texture(samples1, textureCoordinates), vec4(1.0));

    if (sampleCount >= 16)
    {
        sampleVisibility += dot(texture(samples2, textureCoordinates), vec4(1.0));
        sampleVisibility += dot(texture(samples3, textureCoordinates), vec4(1.0));
    }

    return sampleVisibility / float(max(sampleCount, 1));
}

void main()
{
    vec2 texturePixelSize = 1.0 / vec2(textureSize(texture0, 0));

    // Spatial smoothing radius in shadow-buffer pixels.
    if (smoothingRadius <= 0.0) {
        float sampleVisibility = 1.0 - opacity * (1.0 - GetSampleVisibility(fragTexCoord));
        finalColor = vec4(vec3(sampleVisibility), 1.0);
        return;
    }
    float filterRadius = smoothingRadius;

    vec2 filterSampleOffset = texturePixelSize * filterRadius;

    // 3x3 Gaussian filter.
    float blurredVisibility = 0.0;

    blurredVisibility += GetSampleVisibility(fragTexCoord + filterSampleOffset * vec2(-1.0, -1.0)) * 1.0;
    blurredVisibility += GetSampleVisibility(fragTexCoord + filterSampleOffset * vec2( 0.0, -1.0)) * 2.0;
    blurredVisibility += GetSampleVisibility(fragTexCoord + filterSampleOffset * vec2( 1.0, -1.0)) * 1.0;

    blurredVisibility += GetSampleVisibility(fragTexCoord + filterSampleOffset * vec2(-1.0,  0.0)) * 2.0;
    blurredVisibility += GetSampleVisibility(fragTexCoord)                              * 4.0;
    blurredVisibility += GetSampleVisibility(fragTexCoord + filterSampleOffset * vec2( 1.0,  0.0)) * 2.0;

    blurredVisibility += GetSampleVisibility(fragTexCoord + filterSampleOffset * vec2(-1.0,  1.0)) * 1.0;
    blurredVisibility += GetSampleVisibility(fragTexCoord + filterSampleOffset * vec2( 0.0,  1.0)) * 2.0;
    blurredVisibility += GetSampleVisibility(fragTexCoord + filterSampleOffset * vec2( 1.0,  1.0)) * 1.0;

    blurredVisibility /= 16.0;

    float resolvedVisibility =
        1.0 - opacity * (1.0 - blurredVisibility);

    finalColor = vec4(vec3(resolvedVisibility), 1.0);
}
