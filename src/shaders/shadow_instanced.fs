#version 330

uniform vec4 maskColor;
uniform float propMode;
uniform vec4 propSettings;
in vec2 propLocal;
flat in vec2 propShift;
flat in float propFade;

out vec4 finalColor;

void main()
{
    if (propMode > 0.5) {
        // Analytic soft ellipse swept along a bounded segment. This is an
        // artistic ground shadow, not an infinitely extruded light blocker.
        float contactShadow = (1.0 - smoothstep(0.3, 1.0, length(propLocal))) * propSettings.w;
        float projectionFraction = clamp(dot(propLocal, propShift) / max(dot(propShift, propShift), 0.0001), 0.0, 1.0);
        float projectedShadow = 1.0 - smoothstep(0.45, 1.0, length(propLocal - projectionFraction * propShift));
        projectedShadow *= propSettings.z * propFade * mix(1.0, 0.35, projectionFraction);
        finalColor = mix(vec4(1.0), maskColor, max(contactShadow, projectedShadow));
        return;
    }
    finalColor = maskColor;
}
