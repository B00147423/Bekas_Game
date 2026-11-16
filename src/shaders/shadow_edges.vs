#version 330
layout(location=0) in vec2 edgeA;
layout(location=1) in vec2 edgeB;
layout(location=2) in vec2 corner;
layout(location=3) in vec4 instanceTransform;
uniform mat4 mvp;
uniform vec2 lightPosition;
uniform float shadowLength;
void main() {
    vec2 firstWorldEndpoint=instanceTransform.xy+edgeA*instanceTransform.zw;
    vec2 secondWorldEndpoint=instanceTransform.xy+edgeB*instanceTransform.zw;
    vec2 edgeDirection=secondWorldEndpoint-firstWorldEndpoint;
    vec2 firstLightDirection=firstWorldEndpoint-lightPosition, secondLightDirection=secondWorldEndpoint-lightPosition;
    float firstDistanceToLight=length(firstLightDirection), secondDistanceToLight=length(secondLightDirection);
    if(dot(vec2(edgeDirection.y,-edgeDirection.x),lightPosition-firstWorldEndpoint)<=0.0 || min(firstDistanceToLight,secondDistanceToLight)<0.001) {
        gl_Position=mvp*vec4(firstWorldEndpoint,0,1); // Degenerate non-facing edge.
        return;
    }
    vec2 edgePoint=mix(firstWorldEndpoint,secondWorldEndpoint,corner.x);
    vec2 shadowDirection=mix(firstLightDirection/firstDistanceToLight,secondLightDirection/secondDistanceToLight,corner.x);
    gl_Position=mvp*vec4(edgePoint+shadowDirection*shadowLength*corner.y,0,1);
}
