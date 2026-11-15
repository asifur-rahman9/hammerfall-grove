#include "game/Warden.hpp"

#include <algorithm>
#include <cmath>

#include "core/MathUtil.hpp"

namespace ew {
namespace {

// Key poses (yaw is filled in at runtime). Positive pitch leans toward the player.
constexpr float kGuardBicep = -12.0f;
constexpr float kGuardForearm = 40.0f;
constexpr float kWindUpBicep = -28.0f;
constexpr float kWindUpForearm = -35.0f;
constexpr float kImpactHold = 0.35f;

// Strike poses at a few distances from the base. Tuned so the hammer head
// ends up about 3 units above the ground at that distance.
struct StrikeKey {
    float distance;
    float bicep;
    float forearm;
};
constexpr StrikeKey kStrikeTable[] = {
    {12.0f, 52.3f, 150.2f},
    {16.0f, 49.6f, 126.0f},
    {20.0f, 52.7f, 103.1f},
    {24.0f, 59.0f, 77.7f},
    {28.0f, 69.9f, 42.9f},
};

ArmPose lerpPose(const ArmPose& a, const ArmPose& b, float t) {
    ArmPose p;
    p.yaw = a.yaw + wrapDegrees(b.yaw - a.yaw) * t;
    p.bicep = a.bicep + (b.bicep - a.bicep) * t;
    p.forearm = a.forearm + (b.forearm - a.forearm) * t;
    return p;
}

}  // namespace

const char* wardenStateName(WardenState s) {
    switch (s) {
        case WardenState::Dormant: return "Dormant";
        case WardenState::Tracking: return "Tracking";
        case WardenState::WindUp: return "Wind-up";
        case WardenState::Strike: return "Strike";
        case WardenState::Recover: return "Recover";
    }
    return "?";
}

WardenParams wardenParamsForLevel(int level) {
    WardenParams p;
    const float l = static_cast<float>(std::max(level, 0));
    p.detectionRadius = std::min(56.0f, 38.0f + 2.5f * l);
    p.trackTime = std::max(0.45f, 1.3f - 0.1f * l);
    p.windUpTime = std::max(0.6f, 1.15f - 0.07f * l);
    p.strikeTime = std::max(0.24f, 0.40f - 0.02f * l);
    p.recoverTime = std::max(0.5f, 0.95f - 0.05f * l);
    p.turnRate = std::min(9.0f, 3.5f + 0.6f * l);
    return p;
}

ArmPose strikePoseForDistance(float distance, float yaw) {
    constexpr int n = sizeof(kStrikeTable) / sizeof(kStrikeTable[0]);
    ArmPose pose;
    pose.yaw = yaw;
    if (distance <= kStrikeTable[0].distance) {
        pose.bicep = kStrikeTable[0].bicep;
        pose.forearm = kStrikeTable[0].forearm;
        return pose;
    }
    for (int i = 0; i < n - 1; ++i) {
        const StrikeKey& a = kStrikeTable[i];
        const StrikeKey& b = kStrikeTable[i + 1];
        if (distance <= b.distance) {
            const float t = (distance - a.distance) / (b.distance - a.distance);
            pose.bicep = a.bicep + (b.bicep - a.bicep) * t;
            pose.forearm = a.forearm + (b.forearm - a.forearm) * t;
            return pose;
        }
    }
    pose.bicep = kStrikeTable[n - 1].bicep;  // too far: swing at max reach
    pose.forearm = kStrikeTable[n - 1].forearm;
    return pose;
}

Warden::Warden(ArmConfig config) : config_(config) { reset(); }

void Warden::reset() {
    pose_ = ArmPose{};
    fromPose_ = pose_;
    strikePose_ = pose_;
    state_ = WardenState::Dormant;
    stateTime_ = 0.0f;
    time_ = 0.0f;
    alertness_ = 0.0f;
    impactPending_ = false;
}

void Warden::enter(WardenState s) {
    state_ = s;
    stateTime_ = 0.0f;
    fromPose_ = pose_;
}

bool Warden::consumeImpact(glm::vec3& impactPoint) {
    if (!impactPending_) return false;
    impactPending_ = false;
    impactPoint = impactPoint_;
    return true;
}

void Warden::update(float dt, const glm::vec3& playerEye, float playerEyeHeight, const WardenParams& params) {
    time_ += dt;
    stateTime_ += dt;

    const glm::vec3 playerFeet = playerEye - glm::vec3(0.0f, playerEyeHeight, 0.0f);
    const float distance = horizontalDistance(playerFeet, config_.base);
    const float targetYaw = yawToward(config_, playerFeet);
    const bool inRange = distance < params.detectionRadius;

    float targetAlert = 0.0f;

    switch (state_) {
        case WardenState::Dormant: {
            // Idle "breathing": the arm sways slowly around upright.
            ArmPose idle;
            idle.yaw = wrapDegrees(pose_.yaw + 6.0f * dt);  // slow patrol turn
            idle.bicep = 4.0f * std::sin(time_ * 0.6f);
            idle.forearm = 10.0f * std::sin(time_ * 0.45f + 1.0f);
            pose_.yaw = idle.yaw;
            pose_.bicep = damp(pose_.bicep, idle.bicep, 2.0f, dt);
            pose_.forearm = damp(pose_.forearm, idle.forearm, 2.0f, dt);
            targetAlert = 0.0f;
            if (inRange) enter(WardenState::Tracking);
            break;
        }
        case WardenState::Tracking: {
            pose_.yaw = dampAngleDegrees(pose_.yaw, targetYaw, params.turnRate, dt);
            pose_.bicep = damp(pose_.bicep, kGuardBicep + 3.0f * std::sin(time_ * 2.0f), 4.0f, dt);
            pose_.forearm = damp(pose_.forearm, kGuardForearm, 4.0f, dt);
            targetAlert = 0.35f;
            if (distance > params.detectionRadius * 1.15f) {
                enter(WardenState::Dormant);
            } else if (stateTime_ >= params.trackTime && std::fabs(wrapDegrees(targetYaw - pose_.yaw)) < 25.0f) {
                enter(WardenState::WindUp);
            }
            break;
        }
        case WardenState::WindUp: {
            const float t = smoothstep01(stateTime_ / params.windUpTime);
            ArmPose windUp{pose_.yaw, kWindUpBicep, kWindUpForearm};
            const ArmPose blended = lerpPose(fromPose_, windUp, t);
            pose_.bicep = blended.bicep;
            pose_.forearm = blended.forearm;
            // Keeps turning toward the player while raising the hammer, but slower.
            pose_.yaw = dampAngleDegrees(pose_.yaw, targetYaw, params.turnRate * 0.5f, dt);
            targetAlert = 0.35f + 0.65f * t;
            if (stateTime_ >= params.windUpTime) {
                // Lock the target: the player can still dodge during the slam.
                strikeTarget_ = playerFeet;
                strikePose_ = strikePoseForDistance(distance, yawToward(config_, playerFeet));
                enter(WardenState::Strike);
            }
            break;
        }
        case WardenState::Strike: {
            // Ease-in (accelerating) slam.
            float t = std::min(stateTime_ / params.strikeTime, 1.0f);
            t = t * t;
            pose_ = lerpPose(fromPose_, strikePose_, t);
            targetAlert = 1.0f;
            if (stateTime_ >= params.strikeTime) {
                pose_ = strikePose_;
                impactPending_ = true;
                impactPoint_ = hammerPosition();
                enter(WardenState::Recover);
            }
            break;
        }
        case WardenState::Recover: {
            targetAlert = 0.6f;
            if (stateTime_ > kImpactHold) {
                const float t = smoothstep01((stateTime_ - kImpactHold) / params.recoverTime);
                ArmPose guard{fromPose_.yaw, kGuardBicep, kGuardForearm};
                pose_ = lerpPose(fromPose_, guard, t);
            }
            if (stateTime_ >= kImpactHold + params.recoverTime) {
                enter(inRange ? WardenState::Tracking : WardenState::Dormant);
            }
            break;
        }
    }

    alertness_ = damp(alertness_, targetAlert, 6.0f, dt);
}

}  // namespace ew
