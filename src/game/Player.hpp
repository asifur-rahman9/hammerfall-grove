#pragma once
// First-person player controller: mouse look, WASD movement, jumping, knockback.
// Input arrives as plain data (PlayerInput), so the physics is GL/GLFW-free and
// can be driven by the keyboard, the demo autopilot, or a unit test.

#include <glm/glm.hpp>

namespace ew {

struct PlayerInput {
    bool forward = false;
    bool back = false;
    bool left = false;
    bool right = false;
    bool jump = false;
    bool sprint = false;
    float lookDeltaX = 0.0f;  // mouse delta in pixels (or degrees * 1/sensitivity)
    float lookDeltaY = 0.0f;
};

struct PlayerParams {
    float eyeHeight = 4.0f;
    float walkSpeed = 15.0f;
    float sprintMultiplier = 1.75f;
    float jumpSpeed = 12.0f;
    float gravity = -24.0f;
    float lookSensitivity = 0.1f;  // degrees per pixel
    float airControl = 0.35f;      // fraction of walk acceleration available mid-air
    float knockbackSpeed = 30.0f;
    float knockbackLift = 7.0f;
    float knockbackStun = 0.6f;    // seconds without movement control
    glm::vec3 arenaCenter{5.0f, 0.0f, 5.0f};
    float arenaRadius = 47.0f;     // the clearing ends at the tree line
    float obstacleRadius = 7.5f;   // keep out of the Warden's stone plinth
};

class Player {
public:
    explicit Player(PlayerParams params = {});

    void reset(const glm::vec3& eyePosition, float yawDegrees, float pitchDegrees);
    void update(const PlayerInput& input, float dt);
    void knockBack(const glm::vec3& from);

    // Orientation
    glm::vec3 forward() const;          // full look direction
    glm::vec3 flatForward() const;      // look direction projected on the ground
    glm::vec3 right() const;
    glm::mat4 viewMatrix() const;

    const glm::vec3& eye() const { return eye_; }
    const glm::vec3& velocity() const { return velocity_; }
    float yaw() const { return yaw_; }
    float pitch() const { return pitch_; }
    bool onGround() const { return onGround_; }
    bool stunned() const { return stun_ > 0.0f; }
    const PlayerParams& params() const { return params_; }
    PlayerParams& params() { return params_; }

    void setLook(float yawDegrees, float pitchDegrees);

private:
    PlayerParams params_;
    glm::vec3 eye_{0.0f};
    glm::vec3 velocity_{0.0f};
    float yaw_ = 0.0f;
    float pitch_ = 0.0f;
    float stun_ = 0.0f;
    bool onGround_ = true;
};

}  // namespace ew
