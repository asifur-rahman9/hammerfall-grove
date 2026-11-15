#include "game/GameState.hpp"

#include <algorithm>
#include <cmath>
#include <fstream>

#include "core/MathUtil.hpp"

namespace ew {

glm::vec3 Ember::position() const {
    // Gentle hover: bob up and down and drift in a small circle.
    return anchor + glm::vec3(0.4f * std::sin(age * 0.9f), 0.5f * std::sin(age * 1.7f), 0.4f * std::cos(age * 0.7f));
}

GameState::GameState(std::uint32_t seed, GameRules rules) : rules_(rules), rng_(seed) {}

float GameState::currentEmberLifetime() const {
    return std::max(rules_.minEmberLifetime, rules_.emberLifetime - rules_.lifetimeDropPerLevel * static_cast<float>(level()));
}

void GameState::push(GameEventType type, const glm::vec3& pos, int points) {
    events_.push_back(GameEvent{type, pos, points});
}

void GameState::startRun() {
    phase_ = Phase::Playing;
    score_ = 0;
    lives_ = rules_.startingLives;
    combo_ = 1;
    caught_ = 0;
    runTime_ = 0.0f;
    invulnerable_ = 0.0f;
    newBest_ = false;
    events_.clear();
    spawnEmber();
}

void GameState::returnToTitle() {
    phase_ = Phase::Title;
    ember_.active = false;
}

void GameState::spawnEmberAt(const glm::vec3& anchor) {
    ember_.anchor = anchor;
    ember_.age = 0.0f;
    ember_.lifetime = currentEmberLifetime();
    ember_.active = true;
    respawnTimer_ = 0.0f;
    push(GameEventType::EmberSpawned, anchor);
}

void GameState::spawnEmber() {
    std::uniform_real_distribution<float> unit(0.0f, 1.0f);
    // sqrt() gives a uniform distribution over the ring's area.
    const float rMin2 = rules_.spawnRadiusMin * rules_.spawnRadiusMin;
    const float rMax2 = rules_.spawnRadiusMax * rules_.spawnRadiusMax;
    const float radius = std::sqrt(rMin2 + (rMax2 - rMin2) * unit(rng_));
    const float angle = unit(rng_) * 2.0f * kPi;
    const float height = glm::mix(rules_.spawnHeightMin, rules_.spawnHeightMax, unit(rng_));
    spawnEmberAt(rules_.arenaCenter + glm::vec3(radius * std::cos(angle), height, radius * std::sin(angle)));
}

const std::vector<GameEvent>& GameState::update(float dt, const glm::vec3& playerEye, const glm::vec3& playerForward) {
    events_.clear();
    if (phase_ != Phase::Playing) return events_;

    runTime_ += dt;
    invulnerable_ = std::max(0.0f, invulnerable_ - dt);

    if (!ember_.active) {
        respawnTimer_ -= dt;
        if (respawnTimer_ <= 0.0f) spawnEmber();
        return events_;
    }

    ember_.age += dt;
    const glm::vec3 reach = playerEye + playerForward * rules_.reachDistance;
    const glm::vec3 emberPos = ember_.position();

    if (glm::length(reach - emberPos) < rules_.catchRadius || glm::length(playerEye - emberPos) < rules_.catchRadius) {
        const int points = combo_;
        score_ += points;
        ++caught_;
        combo_ = std::min(combo_ + 1, rules_.maxCombo);
        if (score_ > best_) {
            best_ = score_;
            newBest_ = true;
        }
        ember_.active = false;
        respawnTimer_ = rules_.respawnDelay;
        push(GameEventType::EmberCaught, emberPos, points);
    } else if (ember_.age >= ember_.lifetime) {
        combo_ = 1;  // letting an ember die breaks the chain
        ember_.active = false;
        respawnTimer_ = rules_.respawnDelay;
        push(GameEventType::EmberExpired, emberPos);
    }
    return events_;
}

bool GameState::registerHit(const glm::vec3& at) {
    if (phase_ != Phase::Playing || invulnerable_ > 0.0f) return false;
    --lives_;
    combo_ = 1;
    invulnerable_ = rules_.invulnerableTime;
    push(GameEventType::PlayerHit, at);
    if (lives_ <= 0) {
        lives_ = 0;
        phase_ = Phase::GameOver;
        ember_.active = false;
        if (newBest_) saveHighScore();
        push(GameEventType::GameOver, at);
    }
    return true;
}

void GameState::setHighScorePath(std::string path) { highScorePath_ = std::move(path); }

bool GameState::loadHighScore() {
    if (highScorePath_.empty()) return false;
    std::ifstream in(highScorePath_);
    int value = 0;
    if (!(in >> value) || value < 0) return false;
    best_ = value;
    return true;
}

bool GameState::saveHighScore() const {
    if (highScorePath_.empty()) return false;
    std::ofstream out(highScorePath_, std::ios::trunc);
    if (!out) return false;
    out << best_ << '\n';
    return static_cast<bool>(out);
}

}  // namespace ew
