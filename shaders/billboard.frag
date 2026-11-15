#version 330 core
in vec2 vCorner;
in vec4 vColor;
out vec4 FragColor;

void main() {
    float r = length(vCorner);
    if (r > 1.0) discard;
    // Hot core + soft halo; additive blending makes overlapping sparks glow.
    float core = exp(-r * r * 18.0);
    float halo = pow(1.0 - r, 3.0) * 0.35;
    FragColor = vec4(vColor.rgb * (core + halo) * vColor.a, 1.0);
}
