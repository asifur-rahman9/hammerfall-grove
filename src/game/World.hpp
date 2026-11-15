#pragma once
// Static layout of the clearing (procedural, deterministic) and the fireflies
// that light it. GL-free: the renderer uploads the instance data once.

#include <glm/glm.hpp>
#include <vector>

namespace ew {

// Per-instance data for instanced draws: x, z, yaw (radians), uniform scale.
using InstanceData = glm::vec4;

struct WorldLayout {
    std::vector<InstanceData> tallGrass;
    std::vector<InstanceData> shortGrass;
    std::vector<InstanceData> trees;
};

struct ScatterParams {
    glm::vec3 center{5.0f, 0.0f, 5.0f};
    float grassRadius = 64.0f;       // beyond this the tree line and fog hide the ground
    float grassSpacing = 0.9f;
    float clearingRadius = 8.0f;     // no grass on the Warden's base
    float treeRingInner = 50.0f;     // the forest edge that encloses the clearing
    float treeRingOuter = 125.0f;
    int treeCount = 220;
};

WorldLayout generateWorld(const ScatterParams& params = {});

struct Firefly {
    glm::vec3 position;
    glm::vec3 color;  // brightness already applied
};

// Fireflies wander on smooth Lissajous paths. Deterministic in `time`.
std::vector<Firefly> fireflies(float time, int count);

// Moonlight: the single shadow-casting directional light.
glm::vec3 moonDirection();  // direction the light travels (pointing down)

}  // namespace ew
