#include "game/World.hpp"

#include <cmath>

#include "core/MathUtil.hpp"

namespace ew {

WorldLayout generateWorld(const ScatterParams& p) {
    WorldLayout w;

    // Grass: jittered grid, density falls off toward the tree line.
    const int n = static_cast<int>(p.grassRadius * 2.0f / p.grassSpacing);
    for (int ix = 0; ix < n; ++ix) {
        for (int iz = 0; iz < n; ++iz) {
            const float jx = hash01(ix, iz, 1) - 0.5f;
            const float jz = hash01(ix, iz, 2) - 0.5f;
            const float x = p.center.x - p.grassRadius + (static_cast<float>(ix) + 0.5f + jx * 0.9f) * p.grassSpacing;
            const float z = p.center.z - p.grassRadius + (static_cast<float>(iz) + 0.5f + jz * 0.9f) * p.grassSpacing;
            const float r = std::sqrt((x - p.center.x) * (x - p.center.x) + (z - p.center.z) * (z - p.center.z));
            if (r > p.grassRadius || r < p.clearingRadius) continue;
            // Thin out patches with low-frequency "noise" so the field isn't uniform.
            const float patch = 0.5f + 0.5f * std::sin(x * 0.11f + 1.3f) * std::cos(z * 0.09f - 0.4f);
            if (hash01(ix, iz, 3) > 0.35f + 0.6f * patch) continue;
            const float yaw = hash01(ix, iz, 4) * 2.0f * kPi;
            const bool tall = hash01(ix, iz, 5) < 0.55f;
            const float scale = tall ? 1.6f + 1.4f * hash01(ix, iz, 6) : 2.2f + 1.6f * hash01(ix, iz, 6);
            (tall ? w.tallGrass : w.shortGrass).emplace_back(x, z, yaw, scale);
        }
    }

    // Trees: a ring of forest around the clearing (nothing inside it, so the
    // play area and camera paths stay unobstructed).
    for (int i = 0; i < p.treeCount; ++i) {
        const float a = hash01(i, 11) * 2.0f * kPi;
        const float t = hash01(i, 12);
        const float r = p.treeRingInner + (p.treeRingOuter - p.treeRingInner) * t * t;  // denser near the edge
        const float scale = 6.0f + 5.0f * hash01(i, 13) + 3.0f * t;
        w.trees.emplace_back(p.center.x + r * std::cos(a), p.center.z + r * std::sin(a), hash01(i, 14) * 2.0f * kPi, scale);
    }
    return w;
}

std::vector<Firefly> fireflies(float time, int count) {
    // Warm palette: amber, gold, pale lime, soft orange.
    static const glm::vec3 kPalette[] = {
        {1.00f, 0.72f, 0.30f}, {1.00f, 0.85f, 0.45f}, {0.75f, 1.00f, 0.45f}, {1.00f, 0.60f, 0.25f}, {0.95f, 0.90f, 0.60f},
    };
    std::vector<Firefly> out;
    out.reserve(count);
    for (int i = 0; i < count; ++i) {
        const float fi = static_cast<float>(i);
        const float phase = hash01(i, 31) * 2.0f * kPi;
        const float baseRadius = 14.0f + 24.0f * hash01(i, 32);
        const float speed = 0.05f + 0.06f * hash01(i, 33);
        const float a = time * speed + phase;
        // Orbit with a breathing radius: wanders in and out but never leaves the clearing.
        const float radius = baseRadius + 4.0f * std::sin(time * 0.31f + fi * 1.7f);
        glm::vec3 pos;
        pos.x = 5.0f + radius * std::cos(a);
        pos.z = 5.0f + radius * std::sin(a);
        pos.y = 2.5f + 3.5f * (0.5f + 0.5f * std::sin(time * 0.5f + phase * 3.0f)) + 0.4f * std::sin(time * 3.1f + fi);
        // Fireflies pulse: brightness breathes on a slow cycle.
        const float pulse = 0.55f + 0.45f * std::pow(0.5f + 0.5f * std::sin(time * (1.2f + 0.3f * fi) + phase), 2.0f);
        out.push_back({pos, kPalette[i % 5] * (5.0f * pulse)});
    }
    return out;
}

// Low moon (~30 degrees above the horizon) for long, dramatic shadows across the clearing.
glm::vec3 moonDirection() { return glm::normalize(glm::vec3(0.80f, -0.55f, 0.50f)); }

}  // namespace ew
