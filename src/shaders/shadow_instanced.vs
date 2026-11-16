
#version 330

layout(location = 0) in vec4 edge;
layout(location = 1) in float corner;
layout(location = 2) in vec4 destination;

uniform mat4 mvp;
uniform vec2 lightPosition;
uniform float shadowLength;
uniform vec2 sourceSize;
// 0: PNG occlusion, 1: instanced footprint, 2: CPU-submitted footprint quad.
uniform float propMode;
uniform vec4 propSettings; // max projection, near fade distance, projection/contact opacity
uniform vec4 referenceDest;
out vec2 propLocal;
flat out vec2 propShift;
flat out float propFade;

void main()
{
    propLocal = vec2(0.0);
    propShift = vec2(0.0);
    propFade = 0.0;
    if (propMode > 0.5) {
        vec4 footprintBounds = propMode > 1.5 ? referenceDest : destination;
        vec2 footprintRadii = max(footprintBounds.zw * 0.5, vec2(0.001));
        vec2 footprintCenter = footprintBounds.xy + footprintRadii;
        vec2 directionFromLight = footprintCenter - lightPosition;
        float distanceToCenter = length(directionFromLight);
        float distanceBeyondFootprint = max(0.0, distanceToCenter - max(footprintRadii.x, footprintRadii.y));
        propFade = propSettings.x > 0.0 ? smoothstep(0.0, propSettings.y, distanceBeyondFootprint) : 0.0;
        vec2 projectionOffset = directionFromLight / max(distanceToCenter, 0.001) * propSettings.x * propFade;
        propShift = projectionOffset / footprintRadii;
        vec2 projectedBoundsMin = footprintBounds.xy + min(projectionOffset, vec2(0.0));
        vec2 projectedBoundsMax = footprintBounds.xy + footprintBounds.zw + max(projectionOffset, vec2(0.0));
        vec2 quadCoordinates = corner < 0.5 ? vec2(0,0) : corner < 1.5 ? vec2(0,1) :
                  corner < 2.5 ? vec2(1,1) : corner < 3.5 ? vec2(0,0) :
                  corner < 4.5 ? vec2(1,1) : vec2(1,0);
        vec2 vertexWorldPosition = propMode > 1.5 ? edge.xy : mix(projectedBoundsMin, projectedBoundsMax, quadCoordinates);
        propLocal = (vertexWorldPosition - footprintCenter) / footprintRadii;
        gl_Position = mvp * vec4(vertexWorldPosition, 0.0, 1.0);
        return;
    }
    vec2 firstEdgePoint = destination.xy + edge.xy * (destination.zw / sourceSize);
    vec2 secondEdgePoint = destination.xy + edge.zw * (destination.zw / sourceSize);

    vec2 edgeDirection = secondEdgePoint - firstEdgePoint;
    vec2 outwardNormal = vec2(edgeDirection.y, -edgeDirection.x);

    float edgeLength = length(edgeDirection);
    float distanceToEdge = dot(outwardNormal, lightPosition - firstEdgePoint) / max(edgeLength, 0.0001);
    if (distanceToEdge <= 0.0 || edgeLength <= 0.0001) {
        gl_Position = mvp * vec4(firstEdgePoint, 0.0, 1.0); // Degenerate, not a distant sentinel.
        return;
    }

    // Both far endpoints use the same projective scale. Normalizing them
    // independently collapses the cap when the light is close to the edge.
    // Homogeneous coordinates avoid huge values as distanceToEdge approaches 0.
    float projectionWeight = distanceToEdge / (distanceToEdge + shadowLength);
    vec4 nearFirstPoint = vec4(firstEdgePoint, 0.0, 1.0);
    vec4 nearSecondPoint = vec4(secondEdgePoint, 0.0, 1.0);
    vec4 farFirstPoint = vec4(firstEdgePoint - lightPosition * (1.0 - projectionWeight), 0.0, projectionWeight);
    vec4 farSecondPoint = vec4(secondEdgePoint - lightPosition * (1.0 - projectionWeight), 0.0, projectionWeight);
    vec4 shadowVertexPosition = corner < 0.5 ? nearFirstPoint :
               corner < 1.5 ? farSecondPoint :
               corner < 2.5 ? nearSecondPoint :
               corner < 3.5 ? nearFirstPoint :
               corner < 4.5 ? farFirstPoint : farSecondPoint;
    gl_Position = mvp * shadowVertexPosition;
}
