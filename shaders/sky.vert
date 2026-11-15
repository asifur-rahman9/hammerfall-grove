#version 330 core
layout(location = 0) in vec3 aPos;

uniform mat4 uViewRotProj;  // projection * view without translation

out vec3 vDir;

void main() {
    vDir = aPos;
    vec4 p = uViewRotProj * vec4(aPos, 1.0);
    gl_Position = p.xyww;  // depth = 1: the sky is always behind everything
}
