#version 330 core
// Forward Blinn-Phong: one shadowed directional light (the moon), hemispheric
// ambient, up to MAX_POINT_LIGHTS unshadowed point lights (fireflies, the ember,
// the player's lantern, the Warden's core), exponential fog.

#define MAX_POINT_LIGHTS 16

in vec3 vWorldPos;
in vec3 vNormal;
in vec2 vUV;
in vec4 vLightSpacePos;
in float vLocalHeight;
in float vVariation;

out vec4 FragColor;

// Material
uniform int uMaterial;          // 0 textured, 1 grass, 2 tree, 3 flat color
uniform sampler2D uAlbedo;
uniform vec2 uUVScale;
uniform vec3 uTint;
uniform float uSpecular;
uniform float uShininess;
uniform vec3 uEmissive;
uniform float uHeightRef;       // grass: model height used for the root->tip gradient

// Lights
uniform vec3 uMoonDir;          // direction the moonlight travels
uniform vec3 uMoonColor;
uniform vec3 uSkyAmbient;
uniform vec3 uGroundAmbient;
uniform int uPointCount;
uniform vec3 uPointPos[MAX_POINT_LIGHTS];
uniform vec3 uPointColor[MAX_POINT_LIGHTS];
uniform float uPointRange[MAX_POINT_LIGHTS];

// Shadows
uniform sampler2D uShadowMap;
uniform int uShadows;
uniform int uPCFRadius;         // 0 = hard shadows, 2 = 5x5 percentage-closer filtering
uniform vec2 uShadowTexel;

// Atmosphere
uniform vec3 uCamPos;
uniform vec3 uFogColor;
uniform float uFogDensity;

float shadowFactor(vec3 N, vec3 L) {
    if (uShadows == 0) return 1.0;
    vec3 p = vLightSpacePos.xyz / vLightSpacePos.w * 0.5 + 0.5;
    if (p.z > 1.0) return 1.0;
    // Slope-scaled bias: surfaces at grazing angles need more.
    float bias = max(0.0012 * (1.0 - dot(N, L)), 0.0002);
    float lit = 0.0;
    float taps = 0.0;
    for (int x = -uPCFRadius; x <= uPCFRadius; ++x) {
        for (int y = -uPCFRadius; y <= uPCFRadius; ++y) {
            float closest = texture(uShadowMap, p.xy + vec2(x, y) * uShadowTexel).r;
            lit += (p.z - bias > closest) ? 0.0 : 1.0;
            taps += 1.0;
        }
    }
    return lit / taps;
}

vec3 materialAlbedo() {
    if (uMaterial == 0) {
        return texture(uAlbedo, vUV * uUVScale).rgb * uTint;
    }
    if (uMaterial == 1) {
        float h = clamp(vLocalHeight / uHeightRef, 0.0, 1.0);
        vec3 root = vec3(0.020, 0.045, 0.015);
        vec3 tip = mix(vec3(0.16, 0.30, 0.07), vec3(0.30, 0.34, 0.10), vVariation);
        return mix(root, tip, h * h);
    }
    if (uMaterial == 2) {
        vec3 bark = vec3(0.10, 0.065, 0.040);
        vec3 leaves = mix(vec3(0.035, 0.11, 0.035), vec3(0.09, 0.15, 0.04), vVariation);
        return mix(bark, leaves, smoothstep(0.85, 1.15, vLocalHeight));
    }
    return uTint;
}

void main() {
    vec3 N = normalize(vNormal);
    if (!gl_FrontFacing) N = -N;  // grass blades are double-sided
    vec3 V = normalize(uCamPos - vWorldPos);
    vec3 albedo = materialAlbedo();
    bool foliage = uMaterial == 1 || uMaterial == 2;

    // Hemispheric ambient: sky light from above, bounce from the ground below.
    vec3 color = albedo * mix(uGroundAmbient, uSkyAmbient, N.y * 0.5 + 0.5);

    // Moonlight (shadowed). Foliage uses wrap lighting to fake translucency.
    vec3 L = -uMoonDir;
    float ndl = dot(N, L);
    float diffuse = foliage ? max((ndl + 0.5) / 1.5, 0.0) : max(ndl, 0.0);
    vec3 H = normalize(L + V);
    float spec = pow(max(dot(N, H), 0.0), uShininess) * uSpecular * step(0.0, ndl);
    color += (albedo * diffuse + spec) * uMoonColor * shadowFactor(N, L);

    // Point lights with a smooth range window so they fade to exactly zero.
    for (int i = 0; i < uPointCount; ++i) {
        vec3 toLight = uPointPos[i] - vWorldPos;
        float d = length(toLight);
        if (d > uPointRange[i]) continue;
        vec3 Lp = toLight / d;
        float window = clamp(1.0 - pow(d / uPointRange[i], 4.0), 0.0, 1.0);
        float atten = window * window / (1.0 + 0.25 * d * d);
        float nl = dot(N, Lp);
        float pd = foliage ? max((nl + 0.6) / 1.6, 0.0) : max(nl, 0.0);
        vec3 Hp = normalize(Lp + V);
        float ps = pow(max(dot(N, Hp), 0.0), uShininess) * uSpecular * step(0.0, nl);
        color += (albedo * pd + ps) * uPointColor[i] * atten;
    }

    color += uEmissive;

    // Exponential-squared distance fog blends the scene into the night sky.
    float dist = length(uCamPos - vWorldPos);
    float fog = 1.0 - exp(-pow(dist * uFogDensity, 2.0));
    color = mix(color, uFogColor, fog);

    FragColor = vec4(color, 1.0);
}
