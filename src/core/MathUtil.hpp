#pragma once
// Small, dependency-free math helpers shared by gameplay and rendering code.

#include <cmath>
#include <cstdint>
#include <glm/glm.hpp>

namespace ew {

constexpr float kPi = 3.14159265358979323846f;

// Wrap an angle in degrees into the range [-180, 180).
inline float wrapDegrees(float degrees) {
    float a = std::fmod(degrees + 180.0f, 360.0f);
    if (a < 0.0f) a += 360.0f;
    return a - 180.0f;
}

// Frame-rate independent exponential smoothing: moves `current` toward `target`
// so that the remaining distance shrinks by e^(-rate * dt) every step.
inline float damp(float current, float target, float rate, float dt) {
    return current + (target - current) * (1.0f - std::exp(-rate * dt));
}

inline glm::vec3 damp(const glm::vec3& current, const glm::vec3& target, float rate, float dt) {
    return current + (target - current) * (1.0f - std::exp(-rate * dt));
}

// Same as damp() but takes the shortest path around the circle (degrees).
inline float dampAngleDegrees(float current, float target, float rate, float dt) {
    const float diff = wrapDegrees(target - current);
    return current + diff * (1.0f - std::exp(-rate * dt));
}

inline float smoothstep01(float t) {
    t = glm::clamp(t, 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

// Cheap deterministic hash -> [0, 1). Used for procedural scattering so the
// world layout is identical on every run and every machine.
inline float hash01(std::uint32_t x) {
    x ^= x >> 16;
    x *= 0x7feb352dU;
    x ^= x >> 15;
    x *= 0x846ca68bU;
    x ^= x >> 16;
    return static_cast<float>(x & 0x00FFFFFFU) / static_cast<float>(0x01000000U);
}

inline float hash01(int a, int b, std::uint32_t salt = 0) {
    return hash01(static_cast<std::uint32_t>(a) * 73856093U ^ static_cast<std::uint32_t>(b) * 19349663U ^ salt * 83492791U);
}

inline float horizontalDistance(const glm::vec3& a, const glm::vec3& b) {
    const glm::vec2 d(a.x - b.x, a.z - b.z);
    return std::sqrt(d.x * d.x + d.y * d.y);
}

}  // namespace ew
