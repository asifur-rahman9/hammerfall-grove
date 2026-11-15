#include "game/Kinematics.hpp"

#include <cmath>
#include <glm/gtc/matrix_transform.hpp>

namespace ew {

ArmTransforms forwardKinematics(const ArmConfig& cfg, const ArmPose& pose) {
    const glm::vec3 kUp(0.0f, 1.0f, 0.0f);
    const glm::vec3 kPitchAxis(1.0f, 0.0f, 0.0f);

    ArmTransforms t;
    t.base = glm::translate(glm::mat4(1.0f), cfg.base) * glm::rotate(glm::mat4(1.0f), glm::radians(pose.yaw), kUp);
    t.shoulder = t.base * glm::rotate(glm::mat4(1.0f), glm::radians(pose.bicep), kPitchAxis);
    t.elbow = t.shoulder * glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, cfg.bicepLength, 0.0f)) *
              glm::rotate(glm::mat4(1.0f), glm::radians(pose.forearm), kPitchAxis);
    t.hammer = t.elbow * glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, cfg.forearmLength, 0.0f));
    t.elbowPosition = glm::vec3(t.elbow[3]);
    t.hammerPosition = glm::vec3(t.hammer[3]);
    return t;
}

float yawToward(const ArmConfig& cfg, const glm::vec3& target) {
    const glm::vec3 d = target - cfg.base;
    // Local +Z maps to (sin(yaw), 0, cos(yaw)) after rotating around +Y.
    return glm::degrees(std::atan2(d.x, d.z));
}

}  // namespace ew
