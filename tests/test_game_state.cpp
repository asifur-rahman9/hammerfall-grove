#include <cstdio>
#include <filesystem>

#include "TestUtil.hpp"
#include "game/GameState.hpp"

using namespace ew;

namespace {

const glm::vec3 kFarAway(500.0f, 4.0f, 500.0f);
const glm::vec3 kForward(1.0f, 0.0f, 0.0f);

// Moves a stationary player onto the ember for one step.
const std::vector<GameEvent>& catchEmber(GameState& g) {
    return g.update(0.01f, g.ember().position(), kForward);
}

bool has(const std::vector<GameEvent>& events, GameEventType type) {
    for (const auto& e : events)
        if (e.type == type) return true;
    return false;
}

}  // namespace

TEST_CASE("a run starts with full lives and an active ember inside the arena") {
    GameState g(1);
    CHECK(g.phase() == Phase::Title);
    g.startRun();
    CHECK(g.phase() == Phase::Playing);
    CHECK(g.lives() == g.rules().startingLives);
    CHECK(g.score() == 0);
    REQUIRE(g.ember().active);
    const glm::vec3 a = g.ember().anchor - g.rules().arenaCenter;
    const float r = std::sqrt(a.x * a.x + a.z * a.z);
    CHECK(r >= g.rules().spawnRadiusMin - 1e-3f);
    CHECK(r <= g.rules().spawnRadiusMax + 1e-3f);
    CHECK(a.y >= g.rules().spawnHeightMin);
    CHECK(a.y <= g.rules().spawnHeightMax);
}

TEST_CASE("catching embers scores with a growing chain") {
    GameState g(2);
    g.startRun();
    CHECK(has(catchEmber(g), GameEventType::EmberCaught));
    CHECK(g.score() == 1);
    CHECK(g.combo() == 2);
    CHECK_FALSE(g.ember().active);

    g.update(g.rules().respawnDelay + 0.01f, kFarAway, kForward);  // respawn
    REQUIRE(g.ember().active);
    catchEmber(g);
    CHECK(g.score() == 1 + 2);
    CHECK(g.combo() == 3);
}

TEST_CASE("embers expire, which breaks the chain but costs no lives") {
    GameState g(5);
    g.startRun();
    catchEmber(g);
    g.update(g.rules().respawnDelay + 0.01f, kFarAway, kForward);
    REQUIRE(g.combo() == 2);
    const auto& events = g.update(g.ember().lifetime + 0.1f, kFarAway, kForward);
    CHECK(has(events, GameEventType::EmberExpired));
    CHECK(g.combo() == 1);
    CHECK(g.lives() == g.rules().startingLives);
}

TEST_CASE("hits cost a lantern, grant brief invulnerability and end the run at zero") {
    GameState g(6);
    g.startRun();
    CHECK(g.registerHit({0, 0, 0}));
    CHECK(g.lives() == 2);
    CHECK_FALSE(g.registerHit({0, 0, 0}));  // invulnerable
    g.update(g.rules().invulnerableTime + 0.01f, kFarAway, kForward);
    CHECK(g.registerHit({0, 0, 0}));
    g.update(g.rules().invulnerableTime + 0.01f, kFarAway, kForward);
    CHECK(g.registerHit({0, 0, 0}));
    CHECK(g.lives() == 0);
    CHECK(g.phase() == Phase::GameOver);
    CHECK_FALSE(g.registerHit({0, 0, 0}));  // nothing happens after game over

    g.startRun();
    CHECK(g.phase() == Phase::Playing);
    CHECK(g.lives() == g.rules().startingLives);
    CHECK(g.score() == 0);
}
