#pragma once
// The Warden's arm as a chain of parts: base (yaw) -> bicep (pitch) -> forearm (pitch) -> hammer.
// Each part's transform is built on top of the previous one.

#include <glm/glm.hpp>

namespace ew {

struct ArmConfig {
    glm::vec3 base{5.0f, 0.0f, 5.0f};  // shoulder pivot in world space
    float bicepLength = 20.0f;
    float forearmLength = 10.0f;
};

// Angles in degrees. Pitch angles rotate around the arm's local X axis and are
// measured from straight up (+Y) toward the arm's forward direction (+Z).
struct ArmPose {
    float yaw = 0.0f;      // rotation of the whole arm around world +Y
    float bicep = 0.0f;    // relative to vertical
    float forearm = 0.0f;  // relative to the bicep
};

struct ArmTransforms {
    glm::mat4 base{1.0f};     // at the pivot, rotated by yaw
    glm::mat4 shoulder{1.0f}; // base * pitch(bicep); bicep extends along local +Y
    glm::mat4 elbow{1.0f};    // at the end of the bicep, * pitch(forearm)
    glm::mat4 hammer{1.0f};   // at the end of the forearm
    glm::vec3 elbowPosition{0.0f};
    glm::vec3 hammerPosition{0.0f};
};

ArmTransforms forwardKinematics(const ArmConfig& cfg, const ArmPose& pose);

inline glm::vec3 hammerPosition(const ArmConfig& cfg, const ArmPose& pose) {
    return forwardKinematics(cfg, pose).hammerPosition;
}

// Yaw (degrees) that points the arm's forward axis toward `target`.
float yawToward(const ArmConfig& cfg, const glm::vec3& target);

}  // namespace ew
