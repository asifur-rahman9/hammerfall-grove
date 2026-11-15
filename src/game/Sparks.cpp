#include "game/Sparks.hpp"

#include <algorithm>
#include <cmath>

#include "core/MathUtil.hpp"

namespace ew {

SparkSystem::SparkSystem(std::size_t capacity, std::uint32_t seed) : capacity_(capacity), rng_(seed) {
    sparks_.reserve(capacity_);
}

float SparkSystem::random01() {
    return std::uniform_real_distribution<float>(0.0f, 1.0f)(rng_);
}

void SparkSystem::emit(const Spark& s) {
    if (sparks_.size() >= capacity_) return;  // pool exhausted: drop the newest
    sparks_.push_back(s);
}

void SparkSystem::burst(const BurstDesc& d) {
    for (int i = 0; i < d.count; ++i) {
        // Uniform direction on the unit sphere, then bent upward.
        const float z = random01() * 2.0f - 1.0f;
        const float a = random01() * 2.0f * kPi;
        const float r = std::sqrt(std::max(0.0f, 1.0f - z * z));
        glm::vec3 dir(r * std::cos(a), z, r * std::sin(a));
        dir.y = glm::mix(dir.y, std::fabs(dir.y) + 0.3f, d.upwardBias);
        dir = glm::normalize(dir);

        Spark s;
        s.position = d.origin;
        s.velocity = dir * glm::mix(d.minSpeed, d.maxSpeed, random01());
        s.maxLife = s.life = glm::mix(d.minLife, d.maxLife, random01());
        // Slight hue/brightness variation so a burst doesn't look uniform.
        s.color = d.color * glm::mix(0.6f, 1.3f, random01());
        s.size = d.size * glm::mix(0.6f, 1.4f, random01());
        s.gravityScale = d.gravityScale;
        s.drag = d.drag;
        emit(s);
    }
}

void SparkSystem::update(float dt, float gravity) {
    for (std::size_t i = 0; i < sparks_.size();) {
        Spark& s = sparks_[i];
        s.life -= dt;
        if (s.life <= 0.0f) {
            // Swap-and-pop: O(1) removal, order doesn't matter for additive blending.
            sparks_[i] = sparks_.back();
            sparks_.pop_back();
            continue;
        }
        s.velocity.y += gravity * s.gravityScale * dt;
        s.velocity *= std::exp(-s.drag * dt);
        s.position += s.velocity * dt;
        if (s.position.y < 0.05f) {  // bounce off the ground, losing energy
            s.position.y = 0.05f;
            s.velocity.y = -s.velocity.y * 0.35f;
            s.velocity.x *= 0.6f;
            s.velocity.z *= 0.6f;
        }
        ++i;
    }
}

void SparkSystem::clear() { sparks_.clear(); }

}  // namespace ew
