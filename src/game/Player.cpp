#include "game/Player.hpp"

#include <algorithm>
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>

#include "core/MathUtil.hpp"

namespace ew {

Player::Player(PlayerParams params) : params_(params) {
    reset(glm::vec3(-35.0f, params_.eyeHeight, -35.0f), 45.0f, -5.0f);
}

void Player::reset(const glm::vec3& eyePosition, float yawDegrees, float pitchDegrees) {
    eye_ = eyePosition;
    eye_.y = std::max(eye_.y, params_.eyeHeight);
    velocity_ = glm::vec3(0.0f);
    stun_ = 0.0f;
    onGround_ = eye_.y <= params_.eyeHeight + 1e-4f;
    setLook(yawDegrees, pitchDegrees);
}

void Player::setLook(float yawDegrees, float pitchDegrees) {
    yaw_ = wrapDegrees(yawDegrees);
    pitch_ = glm::clamp(pitchDegrees, -85.0f, 85.0f);
}

glm::vec3 Player::forward() const {
    const float y = glm::radians(yaw_);
    const float p = glm::radians(pitch_);
    return glm::normalize(glm::vec3(std::cos(p) * std::cos(y), std::sin(p), std::cos(p) * std::sin(y)));
}

glm::vec3 Player::flatForward() const {
    const float y = glm::radians(yaw_);
    return glm::vec3(std::cos(y), 0.0f, std::sin(y));
}

glm::vec3 Player::right() const {
    return glm::normalize(glm::cross(flatForward(), glm::vec3(0.0f, 1.0f, 0.0f)));
}

glm::mat4 Player::viewMatrix() const {
    return glm::lookAt(eye_, eye_ + forward(), glm::vec3(0.0f, 1.0f, 0.0f));
}

void Player::knockBack(const glm::vec3& from) {
    glm::vec3 away = eye_ - from;
    away.y = 0.0f;
    if (glm::dot(away, away) < 1e-6f) away = -flatForward();
    away = glm::normalize(away);
    velocity_ = away * params_.knockbackSpeed + glm::vec3(0.0f, params_.knockbackLift, 0.0f);
    stun_ = params_.knockbackStun;
    onGround_ = false;
}

void Player::update(const PlayerInput& input, float dt) {
    // --- Look ---
    setLook(yaw_ + input.lookDeltaX * params_.lookSensitivity, pitch_ - input.lookDeltaY * params_.lookSensitivity);

    // --- Desired horizontal velocity from WASD ---
    glm::vec3 wish(0.0f);
    if (input.forward) wish += flatForward();
    if (input.back) wish -= flatForward();
    if (input.right) wish += right();
    if (input.left) wish -= right();
    if (glm::dot(wish, wish) > 0.0f) wish = glm::normalize(wish);
    const float speed = params_.walkSpeed * (input.sprint ? params_.sprintMultiplier : 1.0f);

    glm::vec3 horizontal(velocity_.x, 0.0f, velocity_.z);
    if (stun_ > 0.0f) {
        // Knocked back: no control, momentum bleeds off.
        stun_ -= dt;
        horizontal = damp(horizontal, glm::vec3(0.0f), 2.5f, dt);
    } else if (onGround_) {
        // Snappy ground movement (high acceleration and friction).
        horizontal = damp(horizontal, wish * speed, 14.0f, dt);
    } else {
        // Keep jump momentum with limited air control.
        horizontal = damp(horizontal, wish * speed, 14.0f * params_.airControl, dt);
    }
    velocity_.x = horizontal.x;
    velocity_.z = horizontal.z;

    // --- Jump & gravity ---
    if (input.jump && onGround_ && stun_ <= 0.0f) {
        velocity_.y = params_.jumpSpeed;
        onGround_ = false;
    }
    if (!onGround_) velocity_.y += params_.gravity * dt;

    eye_ += velocity_ * dt;

    if (eye_.y <= params_.eyeHeight) {
        eye_.y = params_.eyeHeight;
        velocity_.y = 0.0f;
        onGround_ = true;
    }

    // --- World bounds: arena edge and the Warden's base ---
    glm::vec2 fromCenter(eye_.x - params_.arenaCenter.x, eye_.z - params_.arenaCenter.z);
    const float r = glm::length(fromCenter);
    if (r > params_.arenaRadius) {
        fromCenter *= params_.arenaRadius / r;
        eye_.x = params_.arenaCenter.x + fromCenter.x;
        eye_.z = params_.arenaCenter.z + fromCenter.y;
    } else if (r < params_.obstacleRadius) {
        const glm::vec2 dir = r > 1e-4f ? fromCenter / r : glm::vec2(1.0f, 0.0f);
        eye_.x = params_.arenaCenter.x + dir.x * params_.obstacleRadius;
        eye_.z = params_.arenaCenter.z + dir.y * params_.obstacleRadius;
    }
}

}  // namespace ew
