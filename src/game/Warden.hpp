#pragma once
// The Warden: the arm guarding the clearing. A small state machine drives it:
// it sleeps, notices the player, winds up, and slams the hammer down toward
// where the player stood when the wind-up finished.

#include "game/Kinematics.hpp"

namespace ew {

enum class WardenState { Dormant, Tracking, WindUp, Strike, Recover };

const char* wardenStateName(WardenState s);

struct WardenParams {
    float detectionRadius = 40.0f;  // horizontal distance at which it wakes up
    float trackTime = 1.2f;         // time spent turning to face the player before winding up
    float windUpTime = 1.1f;        // telegraph: the hammer rises behind the Warden
    float strikeTime = 0.38f;       // slam duration
    float recoverTime = 0.9f;       // hammer rests on the ground, then returns to guard
    float turnRate = 4.0f;          // yaw smoothing rate (1/s)
};

// Scales the parameters with the player's progress so the game gets harder.
WardenParams wardenParamsForLevel(int level);

// Bicep and forearm angles for slamming the hammer at a given horizontal distance
// from the base. Picked from a small table of hand-tuned poses and blended in between.
ArmPose strikePoseForDistance(float distance, float yaw);

class Warden {
public:
    explicit Warden(ArmConfig config = {});

    void reset();

    // `playerEye` is the player's eye position; `playerEyeHeight` lets the
    // Warden aim at the player's feet.
    void update(float dt, const glm::vec3& playerEye, float playerEyeHeight, const WardenParams& params);

    const ArmConfig& config() const { return config_; }
    const ArmPose& pose() const { return pose_; }
    WardenState state() const { return state_; }
    float stateTime() const { return stateTime_; }
    float time() const { return time_; }

    // 0 = asleep, 1 = about to strike. Drives the glow of the Warden's core.
    float alertness() const { return alertness_; }

    // True while the hammer is moving fast enough to hurt.
    bool isStriking() const { return state_ == WardenState::Strike; }

    // Returns true exactly once per strike, on the frame the hammer lands.
    bool consumeImpact(glm::vec3& impactPoint);

    glm::vec3 hammerPosition() const { return ew::hammerPosition(config_, pose_); }
    glm::vec3 strikeTarget() const { return strikeTarget_; }

private:
    void enter(WardenState s);

    ArmConfig config_;
    ArmPose pose_;
    ArmPose fromPose_;    // pose at the start of the current transition
    ArmPose strikePose_;  // pose the hammer slams down into
    WardenState state_ = WardenState::Dormant;
    float stateTime_ = 0.0f;
    float time_ = 0.0f;
    float alertness_ = 0.0f;
    glm::vec3 strikeTarget_{0.0f};
    bool impactPending_ = false;
    glm::vec3 impactPoint_{0.0f};
};

}  // namespace ew
