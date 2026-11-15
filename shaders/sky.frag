#version 330 core
// Night sky: the original daytime cubemap, desaturated and tinted to night,
// plus procedural twinkling stars, a moon disc with a soft halo,
// and a horizon fade into the fog color.

in vec3 vDir;
out vec4 FragColor;

uniform samplerCube uSky;
uniform vec3 uSkyTint;
uniform vec3 uMoonDir;   // direction the moonlight travels
uniform vec3 uFogColor;
uniform float uTime;

float hash13(vec3 p) {
    p = fract(p * 0.1031);
    p += dot(p, p.zyx + 31.32);
    return fract((p.x + p.y) * p.z);
}

void main() {
    vec3 d = normalize(vDir);
    vec3 tex = texture(uSky, d).rgb;
    float lum = dot(tex, vec3(0.2126, 0.7152, 0.0722));
    vec3 col = mix(vec3(lum), tex, 0.3) * uSkyTint;

    // Stars: one candidate per grid cell on a sphere shell, round falloff, twinkle.
    vec3 sp = d * 260.0;
    vec3 cell = floor(sp);
    float h = hash13(cell);
    if (h > 0.9975) {
        float r = length(fract(sp) - 0.5);
        float twinkle = 0.6 + 0.4 * sin(uTime * (2.0 + 6.0 * hash13(cell + 7.0)) + h * 50.0);
        col += vec3(0.9, 0.95, 1.0) * smoothstep(0.22, 0.0, r) * twinkle * 2.5 * smoothstep(0.05, 0.3, d.y);
    }

    // Moon
    vec3 toMoon = normalize(-uMoonDir);
    float c = max(dot(d, toMoon), 0.0);
    col += vec3(0.85, 0.9, 1.0) * (smoothstep(0.99935, 0.99965, c) * 9.0 + pow(c, 180.0) * 0.6 + pow(c, 12.0) * 0.06);

    // Horizon fades into fog so the ground plane edge is never visible.
    col = mix(uFogColor, col, smoothstep(-0.02, 0.32, d.y));
    FragColor = vec4(col, 1.0);
}
