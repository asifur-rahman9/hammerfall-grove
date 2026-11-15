#version 330 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aUV;
layout(location = 3) in vec4 aInstance;

uniform mat4 uModel;
uniform mat3 uNormalMatrix;   // computed on the CPU (was inverse() per vertex)
uniform mat4 uViewProj;
uniform mat4 uLightSpace;
uniform int uInstanceMode;
uniform float uTime;
uniform float uWind;

// Instance = (x, z, yaw radians, uniform scale) -> model matrix (Y-rotation, scale, translation).
mat4 instanceMatrix(vec4 inst) {
    float c = cos(inst.z);
    float s = sin(inst.z);
    float k = inst.w;
    return mat4(vec4(c * k, 0.0, -s * k, 0.0),
                vec4(0.0,   k,    0.0,    0.0),
                vec4(s * k, 0.0,  c * k,  0.0),
                vec4(inst.x, 0.0, inst.y, 1.0));
}

// Procedural wind: a travelling wave modulated by slow gusts. `bend` is how far
// this vertex is allowed to move (0 at the root, grows toward the tip).
vec3 windOffset(vec3 worldPos, float bend, float strength, float time) {
    float phase = time * 1.8 + worldPos.x * 0.23 + worldPos.z * 0.19;
    float gust = 0.55 + 0.45 * sin(time * 0.43 + worldPos.x * 0.035 + worldPos.z * 0.02);
    float sway = (sin(phase) + 0.4 * sin(phase * 2.7 + 1.3)) * gust;
    return vec3(0.8, 0.0, 0.5) * sway * bend * strength;
}

// mode 0: regular mesh (uModel / uNormalMatrix)
// mode 1: instanced grass (strong wind at the tips)
// mode 2: instanced trees (gentle canopy sway)
void transformVertex(vec3 p, vec3 n, vec4 inst, int mode, mat4 model, mat3 normalMatrix,
                     float time, float wind, out vec3 worldPos, out vec3 worldNormal) {
    if (mode == 0) {
        worldPos = (model * vec4(p, 1.0)).xyz;
        worldNormal = normalMatrix * n;
        return;
    }
    mat4 m = instanceMatrix(inst);
    worldPos = (m * vec4(p, 1.0)).xyz;
    worldNormal = mat3(m) * n / inst.w;  // rotation + uniform scale: no inverse-transpose needed
    if (mode == 1) {
        worldPos += windOffset(worldPos, p.y * p.y * 0.45, wind, time);
    } else if (mode == 2) {
        float h = max(p.y - 0.9, 0.0);
        worldPos += windOffset(worldPos, h * h * 0.6, wind * 0.5, time * 0.7);
    }
}

float instanceVariation(vec4 inst) {
    return fract(sin(dot(inst.xy, vec2(12.9898, 78.233))) * 43758.5453);
}

out vec3 vWorldPos;
out vec3 vNormal;
out vec2 vUV;
out vec4 vLightSpacePos;
out float vLocalHeight;
out float vVariation;

void main() {
    vec3 worldPos;
    vec3 worldNormal;
    transformVertex(aPos, aNormal, aInstance, uInstanceMode, uModel, uNormalMatrix, uTime, uWind, worldPos, worldNormal);

    vWorldPos = worldPos;
    vNormal = worldNormal;
    vUV = aUV;
    vLocalHeight = aPos.y;
    vVariation = uInstanceMode != 0 ? instanceVariation(aInstance) : 0.5;
    // Normal-offset shadow lookup: push the sample point off the surface to kill acne.
    vLightSpacePos = uLightSpace * vec4(worldPos + normalize(worldNormal) * 0.12, 1.0);
    gl_Position = uViewProj * vec4(worldPos, 1.0);
}
