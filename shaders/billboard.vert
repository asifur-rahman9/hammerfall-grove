#version 330 core
// Camera-facing quads, one instance per particle / glow.
layout(location = 0) in vec2 aCorner;    // [-1, 1]^2
layout(location = 1) in vec4 aPosSize;   // world position, half-size
layout(location = 2) in vec4 aColor;     // rgb = color, alpha = intensity

uniform mat4 uViewProj;
uniform vec3 uCamRight;
uniform vec3 uCamUp;

out vec2 vCorner;
out vec4 vColor;

void main() {
    vec3 wp = aPosSize.xyz + (uCamRight * aCorner.x + uCamUp * aCorner.y) * aPosSize.w;
    vCorner = aCorner;
    vColor = aColor;
    gl_Position = uViewProj * vec4(wp, 1.0);
}
