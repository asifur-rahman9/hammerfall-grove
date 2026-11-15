#version 330 core
// Depth-only pass from the moon's point of view.

layout(location = 0) in vec3 aPos;
layout(location = 3) in vec4 aInstance;

uniform mat4 uModel;
uniform mat4 uLightSpace;
uniform int uInstanceMode;   // 0 = regular mesh, otherwise instanced (grass / trees)

// Instance = (x, z, yaw radians, scale) -> model matrix
mat4 instanceMatrix(vec4 inst) {
    float c = cos(inst.z);
    float s = sin(inst.z);
    float k = inst.w;
    return mat4(vec4(c * k, 0.0, -s * k, 0.0),
                vec4(0.0,   k,    0.0,    0.0),
                vec4(s * k, 0.0,  c * k,  0.0),
                vec4(inst.x, 0.0, inst.y, 1.0));
}

void main() {
    mat4 model = uInstanceMode == 0 ? uModel : instanceMatrix(aInstance);
    gl_Position = uLightSpace * model * vec4(aPos, 1.0);
}
