#pragma once
// Game rules for Hammerfall Grove: phases, embers, scoring, lives and difficulty.
// Pure logic (no rendering, no input), so the whole ruleset is unit tested.

#include <cstdint>
#include <glm/glm.hpp>
#include <random>
#include <string>
#include <vector>

namespace ew {

enum class Phase { Title, Playing, GameOver };

enum class GameEventType { EmberSpawned, EmberCaught, EmberExpired, PlayerHit, GameOver };

struct GameEvent {
    GameEventType type;
    glm::vec3 position{0.0f};
    int points = 0;  // for EmberCaught
};

struct Ember {
    glm::vec3 anchor{0.0f};  // spawn point; the ember bobs around it
    float age = 0.0f;
    float lifetime = 10.0f;
    bool active = false;

    glm::vec3 position() const;               // anchor + bobbing
    float remaining() const { return lifetime - age; }
    float remainingFraction() const { return lifetime > 0.0f ? glm::clamp(remaining() / lifetime, 0.0f, 1.0f) : 0.0f; }
};

struct GameRules {
    int startingLives = 3;
    float emberLifetime = 10.0f;     // seconds before an ember fades (level 0)
    float minEmberLifetime = 5.5f;
    float lifetimeDropPerLevel = 0.6f;
    int catchesPerLevel = 4;
    int maxCombo = 5;
    float catchRadius = 3.2f;        // distance from the player's reach point
    float reachDistance = 1.5f;      // reach point = eye + forward * reachDistance
    float invulnerableTime = 1.6f;   // after being hit
    float spawnRadiusMin = 12.0f;
    float spawnRadiusMax = 34.0f;
    float spawnHeightMin = 4.5f;
    float spawnHeightMax = 8.0f;
    float respawnDelay = 0.6f;       // pause between embers
    glm::vec3 arenaCenter{5.0f, 0.0f, 5.0f};
};

class GameState {
public:
    explicit GameState(std::uint32_t seed = 7, GameRules rules = {});

    void startRun();           // Title/GameOver -> Playing
    void returnToTitle();

    // Advance timers; catches are tested against the player's eye/look.
    // Returns the events produced during this step.
    const std::vector<GameEvent>& update(float dt, const glm::vec3& playerEye, const glm::vec3& playerForward);

    // Called by the game when the Warden's hammer connects. Returns false if the
    // player was invulnerable (recently hit) or not playing.
    bool registerHit(const glm::vec3& at);

    // Accessors
    Phase phase() const { return phase_; }
    int score() const { return score_; }
    int bestScore() const { return best_; }
    int lives() const { return lives_; }
    int combo() const { return combo_; }
    int level() const { return caught_ / rules_.catchesPerLevel; }
    int embersCaught() const { return caught_; }
    float runTime() const { return runTime_; }
    float invulnerableFor() const { return invulnerable_; }
    bool newBest() const { return newBest_; }
    const Ember& ember() const { return ember_; }
    const GameRules& rules() const { return rules_; }
    float currentEmberLifetime() const;

    // High score persistence (plain text file with one integer).
    void setHighScorePath(std::string path);
    bool loadHighScore();
    bool saveHighScore() const;

    // Testing / demo helpers.
    void spawnEmberAt(const glm::vec3& anchor);

private:
    void spawnEmber();
    void push(GameEventType type, const glm::vec3& pos = glm::vec3(0.0f), int points = 0);

    GameRules rules_;
    std::mt19937 rng_;
    Phase phase_ = Phase::Title;
    Ember ember_;
    std::vector<GameEvent> events_;
    std::string highScorePath_;

    int score_ = 0;
    int best_ = 0;
    int lives_ = 0;
    int combo_ = 1;
    int caught_ = 0;
    float runTime_ = 0.0f;
    float invulnerable_ = 0.0f;
    float respawnTimer_ = 0.0f;
    bool newBest_ = false;
};

}  // namespace ew
