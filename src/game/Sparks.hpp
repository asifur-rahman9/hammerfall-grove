#pragma once
// CPU particle system for ember sparks, hammer-impact dust and firefly trails.
// Fixed-capacity pool (no per-particle heap allocations), simulated on the CPU
// and drawn by the renderer as instanced camera-facing billboards.

#include <cstdint>
#include <glm/glm.hpp>
#include <random>
#include <vector>

namespace ew {

struct Spark {
    glm::vec3 position{0.0f};
    glm::vec3 velocity{0.0f};
    glm::vec3 color{1.0f};
    float size = 0.2f;
    float life = 0.0f;      // seconds remaining
    float maxLife = 1.0f;
    float gravityScale = 1.0f;
    float drag = 1.0f;
};

struct BurstDesc {
    glm::vec3 origin{0.0f};
    int count = 120;
    glm::vec3 color{6.0f, 2.4f, 0.6f};
    float minSpeed = 4.0f;
    float maxSpeed = 14.0f;
    float upwardBias = 0.6f;  // 0 = isotropic sphere, 1 = mostly upward
    float minLife = 0.6f;
    float maxLife = 1.6f;
    float size = 0.22f;
    float gravityScale = 1.0f;
    float drag = 1.2f;
};

class SparkSystem {
public:
    explicit SparkSystem(std::size_t capacity = 4096, std::uint32_t seed = 1337);

    void burst(const BurstDesc& desc);
    void emit(const Spark& s);
    void update(float dt, float gravity = -9.8f);
    void clear();

    const std::vector<Spark>& sparks() const { return sparks_; }
    std::size_t capacity() const { return capacity_; }

private:
    float random01();

    std::vector<Spark> sparks_;
    std::size_t capacity_;
    std::mt19937 rng_;
};

}  // namespace ew
